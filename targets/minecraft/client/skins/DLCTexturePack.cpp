#include "DLCTexturePack.h"

#include <cinttypes>
#include <cstdint>
#include <cwchar>
#include <limits>
#include <vector>
#include "app/common/Audio/SoundEngine.h"
#include "app/common/DLC/DLCAudioFile.h"
#include "app/common/DLC/DLCColourTableFile.h"
#include "app/common/DLC/DLCGameRulesHeader.h"
#include "app/common/DLC/DLCLocalisationFile.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/DLC/DLCTextureFile.h"
#include "app/common/DLC/DLCUIDataFile.h"
#include "app/common/UI/All Platforms/ArchiveFile.h"
#include "app/linux/Linux_UIController.h"
#include "java/File.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/skins/AbstractTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/locale/StringTable.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "platform/fs/fs.h"
#include "platform/input/input.h"
#include "platform/storage/storage.h"
#include "util/StringHelpers.h"

#if defined(_WINDOWS64)
#endif

namespace {
bool ReadPortableBinaryFile(File& file, std::uint8_t*& data,
                            unsigned int& size) {
    const int64_t fileLength = file.length();
    if (fileLength < 0 ||
        fileLength >
            static_cast<int64_t>(std::numeric_limits<unsigned int>::max())) {
        data = nullptr;
        size = 0;
        return false;
    }

    const std::size_t capacity = static_cast<std::size_t>(fileLength);
    std::uint8_t* buffer = new std::uint8_t[capacity == 0 ? 1 : capacity];
    auto readResult =
        PlatformFilesystem.readFile(file.getPath(), buffer, capacity);
    if (readResult.status != IPlatformFilesystem::ReadStatus::Ok ||
        readResult.fileSize > std::numeric_limits<unsigned int>::max()) {
        delete[] buffer;
        data = nullptr;
        size = 0;
        return false;
    }

    data = buffer;
    size = static_cast<unsigned int>(readResult.fileSize);
    return true;
}
}  // namespace

DLCTexturePack::DLCTexturePack(std::uint32_t id, DLCPack* pack,
                               TexturePack* fallback)
    : AbstractTexturePack(id, nullptr, pack->getName(), fallback) {
    m_dlcInfoPack = pack;
    m_dlcDataPack = nullptr;
    bUILoaded = false;
    m_bLoadingData = false;
    m_bHasLoadedData = false;
    m_archiveFile = nullptr;
    if (gameServices().getLevelGenerationOptions())
        gameServices().getLevelGenerationOptions()->setLoadedData();
    m_bUsingDefaultColourTable = true;

    m_stringTable = nullptr;

    if (m_dlcInfoPack->doesPackContainFile(
            DLCManager::e_DLCType_LocalisationData, "languages.loc")) {
        DLCLocalisationFile* localisationFile =
            (DLCLocalisationFile*)m_dlcInfoPack->getFile(
                DLCManager::e_DLCType_LocalisationData, "languages.loc");
        m_stringTable = localisationFile->getStringTable();
    }

    // 4J Stu - These calls need to be in the most derived version of the class
    loadIcon();
    loadName();
    loadDescription();
    // loadDefaultHTMLColourTable();
}

void DLCTexturePack::loadIcon() {
    if (m_dlcInfoPack->doesPackContainFile(DLCManager::e_DLCType_Texture,
                                           "icon.png")) {
        DLCTextureFile* textureFile = (DLCTextureFile*)m_dlcInfoPack->getFile(
            DLCManager::e_DLCType_Texture, "icon.png");
        std::uint32_t iconSize = 0;
        m_iconData = textureFile->getData(iconSize);
        m_iconSize = iconSize;
    } else {
        AbstractTexturePack::loadIcon();
    }
}

void DLCTexturePack::loadComparison() {
    if (m_dlcInfoPack->doesPackContainFile(DLCManager::e_DLCType_Texture,
                                           "comparison.png")) {
        DLCTextureFile* textureFile = (DLCTextureFile*)m_dlcInfoPack->getFile(
            DLCManager::e_DLCType_Texture, "comparison.png");
        std::uint32_t comparisonSize = 0;
        m_comparisonData = textureFile->getData(comparisonSize);
        m_comparisonSize = comparisonSize;
    }
}

void DLCTexturePack::loadName() {
    texname = "";

    if (m_dlcInfoPack->GetPackID() & 1024) {
        if (m_stringTable != nullptr) {
            texname = m_stringTable->getString("IDS_DISPLAY_NAME");
            m_wsWorldName = m_stringTable->getString("IDS_WORLD_NAME");
        }
    } else {
        if (m_stringTable != nullptr) {
            texname = m_stringTable->getString("IDS_DISPLAY_NAME");
        }
    }
}

void DLCTexturePack::loadDescription() {
    desc1 = "";

    if (m_stringTable != nullptr) {
        desc1 = m_stringTable->getString("IDS_TP_DESCRIPTION");
    }
}

std::string DLCTexturePack::getResource(const std::string& name) {
    // 4J Stu - We should never call this function
#if !defined(__CONTENT_PACKAGE)
    assert(0);
#endif
    return "";
}

InputStream* DLCTexturePack::getResourceImplementation(
    const std::string& name)  // throws IOException
{
    // 4J Stu - We should never call this function
#if !defined(_CONTENT_PACKAGE)
    assert(0);
    if (hasFile(name)) return nullptr;
#endif
    return nullptr;  // resource;
}

bool DLCTexturePack::hasFile(const std::string& name) {
    bool hasFile = false;
    if (m_dlcDataPack != nullptr)
        hasFile = m_dlcDataPack->doesPackContainFile(
            DLCManager::e_DLCType_Texture, name);
    return hasFile;
}

bool DLCTexturePack::isTerrainUpdateCompatible() { return true; }

std::string DLCTexturePack::getPath(bool bTitleUpdateTexture /*= false*/,
                                    const char* pchBDPatchFilename) {
    return "";
}

std::string DLCTexturePack::getAnimationString(const std::string& textureName,
                                               const std::string& path) {
    std::string result = "";

    std::string fullpath = "res/" + path + textureName + ".png";
    if (hasFile(fullpath)) {
        result = m_dlcDataPack->getFile(DLCManager::e_DLCType_Texture, fullpath)
                     ->getParameterAsString(DLCManager::e_DLCParamType_Anim);
    }

    return result;
}

BufferedImage* DLCTexturePack::getImageResource(
    const std::string& File, bool filenameHasExtension /*= false*/,
    bool bTitleUpdateTexture /*=false*/, const std::string& drive /*=""*/) {
    if (!m_dlcDataPack) {
        return fallback->getImageResource(File, filenameHasExtension,
                                          bTitleUpdateTexture, drive);
    }
    auto* image = new BufferedImage();
    const std::string filePath = "/" + File;
    for (int l = 0; l < 10; l++) {
        std::string mipMapPath =
            (l != 0) ? "MipMapLevel" + toWString<int>(l + 1) : "";
        std::string name =
            "res" + (filenameHasExtension
                         ? filePath
                         : filePath.substr(0, filePath.length() - 4) +
                               mipMapPath + ".png");

        if (!m_dlcDataPack->doesPackContainFile(DLCManager::e_DLCType_All,
                                                name)) {
            if (l == 0) gameServices().fatalLoadError();
            break;
        }

        DLCFile* dlcFile =
            m_dlcDataPack->getFile(DLCManager::e_DLCType_All, name);
        std::uint32_t dataBytes = 0;
        std::uint8_t* pbData = dlcFile->getData(dataBytes);
        if (pbData == nullptr || dataBytes == 0) {
            if (l == 0) gameServices().fatalLoadError();
            break;
        }

        if (!image->loadMipmapPng(l, pbData, dataBytes)) {
            break;
        }
    }
    return image;
}

DLCPack* DLCTexturePack::getDLCPack() { return m_dlcDataPack; }

void DLCTexturePack::loadColourTable() {
    // Load the game colours
    if (m_dlcDataPack != nullptr &&
        m_dlcDataPack->doesPackContainFile(DLCManager::e_DLCType_ColourTable,
                                           "colours.col")) {
        DLCColourTableFile* colourFile =
            (DLCColourTableFile*)m_dlcDataPack->getFile(
                DLCManager::e_DLCType_ColourTable, "colours.col");
        m_colourTable = colourFile->getColourTable();
        m_bUsingDefaultColourTable = false;
    } else {
        // 4J Stu - We can delete the default colour table, but not the one from
        // the DLCColourTableFile
        if (!m_bUsingDefaultColourTable) m_colourTable = nullptr;
        loadDefaultColourTable();
        m_bUsingDefaultColourTable = true;
    }

    // Load the text colours
    if (gameServices().hasArchiveFile("HTMLColours.col")) {
        std::vector<uint8_t> textColours =
            gameServices().getArchiveFile("HTMLColours.col");
        m_colourTable->loadColoursFromData(textColours.data(),
                                           textColours.size());
    }
}

void DLCTexturePack::loadData() {
    int mountIndex = m_dlcInfoPack->GetDLCMountIndex();

    if (mountIndex > -1) {
        if (PlatformStorage.MountInstalledDLC(
                PlatformInput.GetPrimaryPad(), mountIndex,
                [this](int pad, std::uint32_t err, std::uint32_t lic) {
                    return onPackMounted(pad, err, lic);
                },
                "TPACK") != 997) {
            // corrupt DLC
            m_bHasLoadedData = true;
            if (gameServices().getLevelGenerationOptions())
                gameServices().getLevelGenerationOptions()->setLoadedData();
            Log::info("Failed to mount texture pack DLC %d for pad %d\n",
                      mountIndex, PlatformInput.GetPrimaryPad());
        } else {
            m_bLoadingData = true;
            Log::info("Attempted to mount DLC data for texture pack %d\n",
                      mountIndex);
        }
    } else {
        m_bHasLoadedData = true;
        if (gameServices().getLevelGenerationOptions())
            gameServices().getLevelGenerationOptions()->setLoadedData();
        gameServices().setAction(PlatformInput.GetPrimaryPad(),
                                 eAppAction_ReloadTexturePack);
    }
}

std::string DLCTexturePack::getFilePath(std::uint32_t packId,
                                        std::string filename,
                                        bool bAddDataFolder) {
    return gameServices().getFilePath(packId, filename, bAddDataFolder);
}

int DLCTexturePack::onPackMounted(int iPad, std::uint32_t dwErr,
                                  std::uint32_t dwLicenceMask) {
    DLCTexturePack* texturePack = this;
    texturePack->m_bLoadingData = false;
    if (dwErr != 0) {
        // corrupt DLC
        Log::info("Failed to mount DLC for pad %d: %u\n", iPad, dwErr);
    } else {
        Log::info("Mounted DLC for texture pack, attempting to load data\n");
        texturePack->m_dlcDataPack =
            new DLCPack(texturePack->m_dlcInfoPack->getName(), dwLicenceMask);
        texturePack->setHasAudio(false);
        unsigned int dwFilesProcessed = 0;
        // Load the DLC textures
        std::string dataFilePath =
            texturePack->m_dlcInfoPack->getFullDataPath();
        if (!dataFilePath.empty()) {
            if (!gameServices().dlcReadDataFile(
                    dwFilesProcessed,
                    getFilePath(texturePack->m_dlcInfoPack->GetPackID(),
                                dataFilePath),
                    texturePack->m_dlcDataPack)) {
                delete texturePack->m_dlcDataPack;
                texturePack->m_dlcDataPack = nullptr;
            }

            // Load the UI data
            if (texturePack->m_dlcDataPack != nullptr) {
                File archivePath(
                    getFilePath(texturePack->m_dlcInfoPack->GetPackID(),
                                std::string("media.arc")));
                if (archivePath.exists())
                    texturePack->m_archiveFile = new ArchiveFile(archivePath);

                /**
                        4J-JEV:
                                For all the GameRuleHeader files we find
                */
                DLCPack* pack = texturePack->m_dlcInfoPack->GetParentPack();
                LevelGenerationOptions* levelGen =
                    gameServices().getLevelGenerationOptions();
                if (levelGen != nullptr && !levelGen->hasLoadedData()) {
                    int gameRulesCount = pack->getDLCItemsCount(
                        DLCManager::e_DLCType_GameRulesHeader);
                    for (int i = 0; i < gameRulesCount; ++i) {
                        DLCGameRulesHeader* dlcFile =
                            (DLCGameRulesHeader*)pack->getFile(
                                DLCManager::e_DLCType_GameRulesHeader, i);

                        if (!dlcFile->getGrfPath().empty()) {
                            File grf(getFilePath(
                                texturePack->m_dlcInfoPack->GetPackID(),
                                dlcFile->getGrfPath()));
                            if (grf.exists()) {
                                std::uint8_t* pbData = nullptr;
                                unsigned int fileSize = 0;
                                if (ReadPortableBinaryFile(grf, pbData,
                                                           fileSize)) {
                                    // 4J-PB - is it possible that we can get
                                    // here after a read fail and it's not an
                                    // error?
                                    dlcFile->setGrfData(
                                        pbData, fileSize,
                                        texturePack->m_stringTable);

                                    delete[] pbData;

                                    gameServices().setLevelGenerationOptions(
                                        dlcFile->lgo);
                                } else {
                                    gameServices().fatalLoadError();
                                }
                            }
                        }
                    }
                    if (levelGen->requiresBaseSave() &&
                        !levelGen->getBaseSavePath().empty()) {
                        File grf(
                            getFilePath(texturePack->m_dlcInfoPack->GetPackID(),
                                        levelGen->getBaseSavePath()));
                        if (grf.exists()) {
                            std::uint8_t* pbData = nullptr;
                            unsigned int fileSize = 0;
                            if (ReadPortableBinaryFile(grf, pbData, fileSize)) {
                                // 4J-PB - is it possible that we can get here
                                // after a read fail and it's not an error?
                                levelGen->setBaseSaveData(pbData, fileSize);
                            } else {
                                gameServices().fatalLoadError();
                            }
                        }
                    }
                }

                // any audio data?
                // DLCPack *pack = texturePack->m_dlcInfoPack->GetParentPack();
                if (pack->getDLCItemsCount(DLCManager::e_DLCType_Audio) > 0) {
                    DLCAudioFile* dlcFile = (DLCAudioFile*)pack->getFile(
                        DLCManager::e_DLCType_Audio, 0);
                    texturePack->setHasAudio(true);
                    // init the streaming sound ids for this texture pack
                    int iOverworldStart, iNetherStart, iEndStart;
                    int iOverworldC, iNetherC, iEndC;

                    iOverworldStart = 0;
                    iOverworldC = dlcFile->GetCountofType(
                        DLCAudioFile::e_AudioType_Overworld);
                    iNetherStart = iOverworldC;
                    iNetherC = dlcFile->GetCountofType(
                        DLCAudioFile::e_AudioType_Nether);
                    iEndStart = iOverworldC + iNetherC;
                    iEndC =
                        dlcFile->GetCountofType(DLCAudioFile::e_AudioType_End);

                    static_cast<SoundEngine*>(
                        Minecraft::GetInstance()->soundEngine)
                        ->SetStreamingSounds(
                            iOverworldStart, iOverworldStart + iOverworldC,
                            iNetherStart, iNetherStart + iNetherC, iEndStart,
                            iEndStart + iEndC,
                            iEndStart +
                                iEndC);  // push the CD start to after
                }
            }
            texturePack->loadColourTable();
        }

        // 4J-PB - we need to leave the texture pack mounted if it contained
        // streaming audio
        if (texturePack->hasAudio() == false) {
        }
    }

    texturePack->m_bHasLoadedData = true;
    if (gameServices().getLevelGenerationOptions())
        gameServices().getLevelGenerationOptions()->setLoadedData();
    gameServices().setAction(PlatformInput.GetPrimaryPad(),
                             eAppAction_ReloadTexturePack);

    return 0;
}

void DLCTexturePack::loadUI() {
    if (m_archiveFile && m_archiveFile->hasFile("skin.swf")) {
        ui.ReloadSkin();
        bUILoaded = true;
    } else {
        loadDefaultUI();
        bUILoaded = true;
    }

    AbstractTexturePack::loadUI();
    if (hasAudio() == false && !ui.IsReloadingSkin()) {
        PlatformStorage.UnmountInstalledDLC("TPACK");
    }
}

void DLCTexturePack::unloadUI() {
    // Unload skin
    if (bUILoaded) {
        setHasAudio(false);
    }
    AbstractTexturePack::unloadUI();

    gameServices().dlcRemovePack(m_dlcDataPack);
    m_dlcDataPack = nullptr;
    delete m_archiveFile;
    m_bHasLoadedData = false;

    bUILoaded = false;
}

std::string DLCTexturePack::getXuiRootPath() {
    std::string path = "";
    if (m_dlcDataPack != nullptr &&
        m_dlcDataPack->doesPackContainFile(DLCManager::e_DLCType_UIData,
                                           "TexturePack.xzp")) {
        DLCUIDataFile* dataFile = (DLCUIDataFile*)m_dlcDataPack->getFile(
            DLCManager::e_DLCType_UIData, "TexturePack.xzp");

        std::uint32_t dwSize = 0;
        std::uint8_t* pbData = dataFile->getData(dwSize);

        constexpr int LOCATOR_SIZE =
            256;  // Use this to allocate space to hold a ResourceLocator string
        char szResourceLocator[LOCATOR_SIZE];
        snprintf(szResourceLocator, LOCATOR_SIZE,
                 "memory://%08" PRIxPTR ",%04X#",
                 reinterpret_cast<std::uintptr_t>(pbData), dwSize);
        path = szResourceLocator;
    }
    return path;
}

unsigned int DLCTexturePack::getDLCParentPackId() {
    return m_dlcInfoPack->GetParentPackId();
}

unsigned char DLCTexturePack::getDLCSubPackId() {
    return (m_dlcInfoPack->GetPackId() >> 24) & 0xFF;
}

DLCPack* DLCTexturePack::getDLCInfoParentPack() {
    return m_dlcInfoPack->GetParentPack();
}

XCONTENTDEVICEID DLCTexturePack::GetDLCDeviceID() {
    return m_dlcInfoPack->GetDLCDeviceID();
}

bool DLCTexturePack::needsPurchase() {
    if (m_dlcInfoPack == nullptr) {
        return false;
    }
    DLCPack* parent = m_dlcInfoPack->GetParentPack();
    if (parent == nullptr) {
        return false;
    }
    return !parent->hasPurchasedFile(DLCManager::e_DLCType_Texture, "");
}
