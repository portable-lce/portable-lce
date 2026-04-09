#include "DLCManager.h"

#include <wchar.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "DLCFile.h"
#include "DLCPack.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "platform/fs/fs.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"
#include "simdutf.h"
#include "strings.h"
#include "util/StringHelpers.h"

// 4jcraft, this is the size of wchar_t on disk
// the DLC was created on windows, with wchar_t beeing 2 bytes and UTF-16
static const std::size_t DLC_WCHAR_BIN_SIZE = 2;

std::string dlc_read_wstring(const void* data) {
    const char16_t* chars = static_cast<const char16_t*>(data);
    const char16_t* end = chars;
    while (*end != 0) {
        ++end;
    }

    const std::size_t len16 = static_cast<std::size_t>(end - chars);
    std::string result(simdutf::utf8_length_from_utf16le(chars, len16), '\0');
    auto len = simdutf::convert_utf16le_to_utf8(chars, len16, result.data());
    result.resize(len);

    return result;
}

#define DLC_WSTRING(ptr) dlc_read_wstring(ptr)

#define DLC_PARAM_ADV(n) \
    (sizeof(IPlatformStorage::DLC_FILE_PARAM) + (n) * DLC_WCHAR_BIN_SIZE)
#define DLC_DETAIL_ADV(n) \
    (sizeof(IPlatformStorage::DLC_FILE_DETAILS) + (n) * DLC_WCHAR_BIN_SIZE)

namespace {
template <typename T>
T ReadDlcValue(const std::uint8_t* data, unsigned int offset = 0) {
    T value;
    std::memcpy(&value, data + offset, sizeof(value));
    return value;
}

template <typename T>
void ReadDlcStruct(T* out, const std::uint8_t* data, unsigned int offset = 0) {
    std::memcpy(out, data + offset, sizeof(*out));
}

std::string getMountedDlcReadPath(const std::string& path) {
    std::string readPath = path;

#if defined(_WINDOWS64)
    const std::string mountedPath =
        PlatformStorage.GetMountedPath(path.c_str());
    if (!mountedPath.empty()) {
        readPath = mountedPath;
    }
#endif

    return readPath;
}

bool readOwnedDlcFile(const std::string& path, std::uint8_t** ppData,
                      unsigned int* pBytesRead) {
    *ppData = nullptr;
    *pBytesRead = 0;

    const std::string readPath = getMountedDlcReadPath(path);
    const std::size_t fSize = PlatformFilesystem.fileSize(readPath);
    if (fSize == 0 || fSize > std::numeric_limits<unsigned int>::max()) {
        return false;
    }

    std::uint8_t* data = new std::uint8_t[fSize];
    auto result = PlatformFilesystem.readFile(readPath, data, fSize);
    if (result.status != IPlatformFilesystem::ReadStatus::Ok) {
        delete[] data;
        return false;
    }

    *ppData = data;
    *pBytesRead = static_cast<unsigned int>(result.bytesRead);
    return true;
}
}  // namespace

const char* DLCManager::wchTypeNamesA[] = {
    "DISPLAYNAME",
    "THEMENAME",
    "FREE",
    "CREDIT",
    "CAPEPATH",
    "BOX",
    "ANIM",
    "PACKID",
    "NETHERPARTICLECOLOUR",
    "ENCHANTTEXTCOLOUR",
    "ENCHANTTEXTFOCUSCOLOUR",
    "DATAPATH",
    "PACKVERSION",
};

DLCManager::DLCManager() {
    // m_bNeedsUpdated = true;
    m_bNeedsCorruptCheck = true;
}

DLCManager::~DLCManager() {
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        DLCPack* pack = *it;
        delete pack;
    }
}

DLCManager::EDLCParameterType DLCManager::getParameterType(
    const std::string& paramName) {
    EDLCParameterType type = e_DLCParamType_Invalid;

    for (unsigned int i = 0; i < e_DLCParamType_Max; ++i) {
        if (paramName.compare(wchTypeNamesA[i]) == 0) {
            type = (EDLCParameterType)i;
            break;
        }
    }

    return type;
}

unsigned int DLCManager::getPackCount(EDLCType type /*= e_DLCType_All*/) {
    unsigned int packCount = 0;
    if (type != e_DLCType_All) {
        for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
            DLCPack* pack = *it;
            if (pack->getDLCItemsCount(type) > 0) {
                ++packCount;
            }
        }
    } else {
        packCount = static_cast<unsigned int>(m_packs.size());
    }
    return packCount;
}

void DLCManager::addPack(DLCPack* pack) { m_packs.push_back(pack); }

void DLCManager::removePack(DLCPack* pack) {
    if (pack != nullptr) {
        auto it = find(m_packs.begin(), m_packs.end(), pack);
        if (it != m_packs.end()) m_packs.erase(it);
        delete pack;
    }
}

void DLCManager::removeAllPacks(void) {
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        DLCPack* pack = (DLCPack*)*it;
        delete pack;
    }

    m_packs.clear();
}

void DLCManager::LanguageChanged(void) {
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        DLCPack* pack = (DLCPack*)*it;
        // update the language
        pack->UpdateLanguage();
    }
}

DLCPack* DLCManager::getPack(const std::string& name) {
    DLCPack* pack = nullptr;
    // uint32_t currentIndex = 0;
    DLCPack* currentPack = nullptr;
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        currentPack = *it;
        std::string wsName = currentPack->getName();

        if (wsName.compare(name) == 0) {
            pack = currentPack;
            break;
        }
    }
    return pack;
}

DLCPack* DLCManager::getPack(unsigned int index,
                             EDLCType type /*= e_DLCType_All*/) {
    DLCPack* pack = nullptr;
    if (type != e_DLCType_All) {
        unsigned int currentIndex = 0;
        DLCPack* currentPack = nullptr;
        for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
            currentPack = *it;
            if (currentPack->getDLCItemsCount(type) > 0) {
                if (currentIndex == index) {
                    pack = currentPack;
                    break;
                }
                ++currentIndex;
            }
        }
    } else {
        if (index >= m_packs.size()) {
            app.DebugPrintf(
                "DLCManager: Trying to access a DLC pack beyond the range of "
                "valid packs\n");
            assert(0);
        }
        pack = m_packs[index];
    }

    return pack;
}

unsigned int DLCManager::getPackIndex(DLCPack* pack, bool& found,
                                      EDLCType type /*= e_DLCType_All*/) {
    unsigned int foundIndex = 0;
    found = false;
    if (pack == nullptr) {
        app.DebugPrintf(
            "DLCManager: Attempting to find the index for a nullptr pack\n");
        // assert(0);
        return foundIndex;
    }
    if (type != e_DLCType_All) {
        unsigned int index = 0;
        for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
            DLCPack* thisPack = *it;
            if (thisPack->getDLCItemsCount(type) > 0) {
                if (thisPack == pack) {
                    found = true;
                    foundIndex = index;
                    break;
                }
                ++index;
            }
        }
    } else {
        unsigned int index = 0;
        for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
            DLCPack* thisPack = *it;
            if (thisPack == pack) {
                found = true;
                foundIndex = index;
                break;
            }
            ++index;
        }
    }
    return foundIndex;
}

unsigned int DLCManager::getPackIndexContainingSkin(const std::string& path,
                                                    bool& found) {
    unsigned int foundIndex = 0;
    found = false;
    unsigned int index = 0;
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        DLCPack* pack = *it;
        if (pack->getDLCItemsCount(e_DLCType_Skin) > 0) {
            if (pack->doesPackContainSkin(path)) {
                foundIndex = index;
                found = true;
                break;
            }
            ++index;
        }
    }
    return foundIndex;
}

DLCPack* DLCManager::getPackContainingSkin(const std::string& path) {
    DLCPack* foundPack = nullptr;
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        DLCPack* pack = *it;
        if (pack->getDLCItemsCount(e_DLCType_Skin) > 0) {
            if (pack->doesPackContainSkin(path)) {
                foundPack = pack;
                break;
            }
        }
    }
    return foundPack;
}

DLCSkinFile* DLCManager::getSkinFile(const std::string& path) {
    DLCSkinFile* foundSkinfile = nullptr;
    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        DLCPack* pack = *it;
        foundSkinfile = pack->getSkinFile(path);
        if (foundSkinfile != nullptr) {
            break;
        }
    }
    return foundSkinfile;
}

unsigned int DLCManager::checkForCorruptDLCAndAlert(
    bool showMessage /*= true*/) {
    unsigned int corruptDLCCount = m_dwUnnamedCorruptDLCCount;
    DLCPack* pack = nullptr;
    DLCPack* firstCorruptPack = nullptr;

    for (auto it = m_packs.begin(); it != m_packs.end(); ++it) {
        pack = *it;
        if (pack->IsCorrupt()) {
            ++corruptDLCCount;
            if (firstCorruptPack == nullptr) firstCorruptPack = pack;
        }
    }

    // gotta fix this someday
    if (corruptDLCCount > 0 && showMessage) {
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_CONFIRM_OK;
        if (corruptDLCCount == 1 && firstCorruptPack != nullptr) {
            // pass in the pack format string
            char wchFormat[132];
            snprintf(wchFormat, 132, "%s\n\n%%s",
                     firstCorruptPack->getName().c_str());

            IPlatformStorage::EMessageResult result = ui.RequestErrorMessage(
                IDS_CORRUPT_DLC_TITLE, IDS_CORRUPT_DLC, uiIDA, 1,
                PlatformProfile.GetPrimaryPad(), nullptr, nullptr, wchFormat);

        } else {
            IPlatformStorage::EMessageResult result = ui.RequestErrorMessage(
                IDS_CORRUPT_DLC_TITLE, IDS_CORRUPT_DLC_MULTIPLE, uiIDA, 1,
                PlatformProfile.GetPrimaryPad());
        }
    }

    SetNeedsCorruptCheck(false);

    return corruptDLCCount;
}

// bool DLCManager::readDLCDataFile(unsigned int& dwFilesProcessed,
//                                  const std::string& path, DLCPack* pack,
//                                  bool fromArchive) {
//     return readDLCDataFile(dwFilesProcessed,
//                            std::filesystem::path(path).string(), pack,
//                            fromArchive);
// }

bool DLCManager::readDLCDataFile(unsigned int& dwFilesProcessed,
                                 const std::string& path, DLCPack* pack,
                                 bool fromArchive) {
    std::string wPath = path;
    if (fromArchive && app.getArchiveFileSize(wPath) >= 0) {
        std::vector<uint8_t> bytes = app.getArchiveFile(wPath);
        return processDLCDataFile(dwFilesProcessed, bytes.data(), bytes.size(),
                                  pack);
    } else if (fromArchive)
        return false;

    unsigned int bytesRead = 0;
    std::uint8_t* pbData = nullptr;
    if (!readOwnedDlcFile(path, &pbData, &bytesRead)) {
        app.DebugPrintf("Failed to open DLC data file %s\n", path.c_str());
        pack->SetIsCorrupt(true);
        SetNeedsCorruptCheck(true);
        return false;
    }
    return processDLCDataFile(dwFilesProcessed, pbData, bytesRead, pack);
}

bool DLCManager::processDLCDataFile(unsigned int& dwFilesProcessed,
                                    std::uint8_t* pbData, unsigned int dwLength,
                                    DLCPack* pack)
// a bunch of makros to reduce memcpy and offset boilerplate
#define DLC_READ_UINT(out, buf, off) \
    memcpy((out), (buf) + (off), sizeof(unsigned int))

#define DLC_READ_PARAM(out, buf, off) \
    memcpy((out), (buf) + (off), sizeof(IPlatformStorage::DLC_FILE_PARAM))

#define DLC_READ_DETAIL(out, buf, off) \
    memcpy((out), (buf) + (off), sizeof(IPlatformStorage::DLC_FILE_DETAILS))

// for details, read in the function below
#define DLC_PARAM_WSTR(buf, off) \
    DLC_WSTRING((buf) + (off) +  \
                offsetof(IPlatformStorage::DLC_FILE_PARAM, wchData))

#define DLC_DETAIL_WSTR(buf, off) \
    DLC_WSTRING((buf) + (off) +   \
                offsetof(IPlatformStorage::DLC_FILE_DETAILS, wchFile))
{
    std::unordered_map<int, DLCManager::EDLCParameterType> parameterMapping;
    unsigned int uiCurrentByte = 0;

    // File format defined in the DLC_Creator
    // File format: Version 2
    // unsigned long, version number
    // unsigned long, t = number of parameter types
    // t * DLC_FILE_PARAM structs mapping strings to id's
    // unsigned long, n = number of files
    // n * DLC_FILE_DETAILS describing each file in the pack
    // n * files of the form
    // // unsigned long, p = number of parameters
    // // p * DLC_FILE_PARAM describing each parameter for this file
    // // ulFileSize bytes of data blob of the file added

    // 4jcraft, some parts of this code changed, specifically:
    // instead of casting a goddamn raw byte pointer and dereferencing it
    // use memcpy, and access WSTRING with propper offset
    // (scince bufferoffset after advancing by variable string length is not
    // guaranteed to be properly aligned, so casting to a scalar/struct is UB)

    // those casts coult be dangerous on e.g. ARM, because it doesnt handle
    // missaligned loads, like x86/x64, so it would crash

    // WHO TF USES HUNGARIAN NOTATION

    unsigned int uiVersion;
    DLC_READ_UINT(&uiVersion, pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);

    if (uiVersion < CURRENT_DLC_VERSION_NUM) {
        if (pbData != nullptr) delete[] pbData;
        app.DebugPrintf("DLC version of %d is too old to be read\n", uiVersion);
        return false;
    }
    pack->SetDataPointer(pbData);
    // safe, offset 4, aligned
    unsigned int uiParameterCount;
    DLC_READ_UINT(&uiParameterCount, pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);

    IPlatformStorage::DLC_FILE_PARAM parBuf;
    DLC_READ_PARAM(&parBuf, pbData, uiCurrentByte);
    // uint32_t dwwchCount=0;
    for (unsigned int i = 0; i < uiParameterCount; i++) {
        // Map DLC strings to application strings, then store the DLC index
        // mapping to application index
        std::string parameterName = DLC_PARAM_WSTR(pbData, uiCurrentByte);
        DLCManager::EDLCParameterType type =
            DLCManager::getParameterType(parameterName);
        if (type != DLCManager::e_DLCParamType_Invalid) {
            parameterMapping[parBuf.dwType] = type;
        }
        uiCurrentByte += DLC_PARAM_ADV(parBuf.dwWchCount);
        DLC_READ_PARAM(&parBuf, pbData, uiCurrentByte);
    }
    // ulCurrentByte+=ulParameterCount *
    // sizeof(IPlatformStorage::DLC_FILE_PARAM);

    unsigned int uiFileCount;
    DLC_READ_UINT(&uiFileCount, pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);

    IPlatformStorage::DLC_FILE_DETAILS fileBuf;
    DLC_READ_DETAIL(&fileBuf, pbData, uiCurrentByte);

    unsigned int dwTemp = uiCurrentByte;
    for (unsigned int i = 0; i < uiFileCount; i++) {
        dwTemp += DLC_DETAIL_ADV(fileBuf.dwWchCount);
        DLC_READ_DETAIL(&fileBuf, pbData, dwTemp);
    }
    std::uint8_t* pbTemp =
        &pbData
            [dwTemp];  //+
                       //sizeof(IPlatformStorage::DLC_FILE_DETAILS)*ulFileCount;
    DLC_READ_DETAIL(&fileBuf, pbData, uiCurrentByte);

    for (unsigned int i = 0; i < uiFileCount; i++) {
        DLCManager::EDLCType type = (DLCManager::EDLCType)fileBuf.dwType;

        DLCFile* dlcFile = nullptr;
        DLCPack* dlcTexturePack = nullptr;

        if (type == e_DLCType_TexturePack) {
            dlcTexturePack =
                new DLCPack(pack->getName(), pack->getLicenseMask());
        } else if (type != e_DLCType_PackConfig) {
            dlcFile =
                pack->addFile(type, DLC_DETAIL_WSTR(pbData, uiCurrentByte));
        }

        // Params
        unsigned int uiParamCount;
        DLC_READ_UINT(&uiParamCount, pbTemp, 0);
        pbTemp += sizeof(int);

        DLC_READ_PARAM(&parBuf, pbTemp, 0);
        for (unsigned int j = 0; j < uiParamCount; j++) {
            // DLCManager::EDLCParameterType paramType =
            // DLCManager::e_DLCParamType_Invalid;

            auto it = parameterMapping.find(parBuf.dwType);

            if (it != parameterMapping.end()) {
                if (type == e_DLCType_PackConfig) {
                    pack->addParameter(it->second, DLC_PARAM_WSTR(pbTemp, 0));
                } else {
                    if (dlcFile != nullptr)
                        dlcFile->addParameter(it->second,
                                              DLC_PARAM_WSTR(pbTemp, 0));
                    else if (dlcTexturePack != nullptr)
                        dlcTexturePack->addParameter(it->second,
                                                     DLC_PARAM_WSTR(pbTemp, 0));
                }
            }
            pbTemp += DLC_PARAM_ADV(parBuf.dwWchCount);
            DLC_READ_PARAM(&parBuf, pbTemp, 0);
        }
        // pbTemp+=ulParameterCount * sizeof(IPlatformStorage::DLC_FILE_PARAM);

        if (dlcTexturePack != nullptr) {
            unsigned int texturePackFilesProcessed = 0;
            bool validPack =
                processDLCDataFile(texturePackFilesProcessed, pbTemp,
                                   fileBuf.uiFileSize, dlcTexturePack);
            pack->SetDataPointer(
                nullptr);  // If it's a child pack, it doesn't own the data
            if (!validPack || texturePackFilesProcessed == 0) {
                delete dlcTexturePack;
                dlcTexturePack = nullptr;
            } else {
                pack->addChildPack(dlcTexturePack);

                if (dlcTexturePack->getDLCItemsCount(
                        DLCManager::e_DLCType_Texture) > 0) {
                    Minecraft::GetInstance()->skins->addTexturePackFromDLC(
                        dlcTexturePack, dlcTexturePack->GetPackId());
                }
            }
            ++dwFilesProcessed;
        } else if (dlcFile != nullptr) {
            // Data
            dlcFile->addData(pbTemp, fileBuf.uiFileSize);

            // TODO - 4J Stu Remove the need for this vSkinNames vector, or
            // manage it differently
            switch (fileBuf.dwType) {
                case DLCManager::e_DLCType_Skin:
                    app.vSkinNames.push_back(
                        DLC_DETAIL_WSTR(pbData, uiCurrentByte));
                    break;
            }

            ++dwFilesProcessed;
        }

        // Move the pointer to the start of the next files data;
        pbTemp += fileBuf.uiFileSize;
        uiCurrentByte += DLC_DETAIL_ADV(fileBuf.dwWchCount);

        DLC_READ_DETAIL(&fileBuf, pbData, uiCurrentByte);
    }

    if (pack->getDLCItemsCount(DLCManager::e_DLCType_GameRules) > 0 ||
        pack->getDLCItemsCount(DLCManager::e_DLCType_GameRulesHeader) > 0) {
        app.m_gameRules.loadGameRules(pack);
    }

    if (pack->getDLCItemsCount(DLCManager::e_DLCType_Audio) > 0) {
        // app.m_Audio.loadAudioDetails(pack);
    }
    // TODO Should be able to delete this data, but we can't yet due to how it
    // is added to the Memory textures (MEM_file)

    return true;
}

std::uint32_t DLCManager::retrievePackIDFromDLCDataFile(const std::string& path,
                                                        DLCPack* pack) {
    std::uint32_t packId = 0;

    unsigned int bytesRead = 0;
    std::uint8_t* pbData = nullptr;
    if (!readOwnedDlcFile(path, &pbData, &bytesRead)) {
        return 0;
    }
    packId = retrievePackID(pbData, bytesRead, pack);
    delete[] pbData;

    return packId;
}

std::uint32_t DLCManager::retrievePackID(std::uint8_t* pbData,
                                         unsigned int dwLength, DLCPack* pack) {
    std::uint32_t packId = 0;
    bool bPackIDSet = false;
    std::unordered_map<int, DLCManager::EDLCParameterType> parameterMapping;
    unsigned int uiCurrentByte = 0;

    // File format defined in the DLC_Creator
    // File format: Version 2
    // unsigned long, version number
    // unsigned long, t = number of parameter types
    // t * DLC_FILE_PARAM structs mapping strings to id's
    // unsigned long, n = number of files
    // n * DLC_FILE_DETAILS describing each file in the pack
    // n * files of the form
    // // unsigned long, p = number of parameters
    // // p * DLC_FILE_PARAM describing each parameter for this file
    // // ulFileSize bytes of data blob of the file added
    unsigned int uiVersion = ReadDlcValue<unsigned int>(pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);

    if (uiVersion < CURRENT_DLC_VERSION_NUM) {
        app.DebugPrintf("DLC version of %d is too old to be read\n", uiVersion);
        return 0;
    }
    pack->SetDataPointer(pbData);
    unsigned int uiParameterCount =
        ReadDlcValue<unsigned int>(pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);
    IPlatformStorage::DLC_FILE_PARAM paramBuf;
    ReadDlcStruct(&paramBuf, pbData, uiCurrentByte);
    for (unsigned int i = 0; i < uiParameterCount; i++) {
        // Map DLC strings to application strings, then store the DLC index
        // mapping to application index
        std::string parameterName = DLC_PARAM_WSTR(pbData, uiCurrentByte);
        DLCManager::EDLCParameterType type =
            DLCManager::getParameterType(parameterName);
        if (type != DLCManager::e_DLCParamType_Invalid) {
            parameterMapping[paramBuf.dwType] = type;
        }
        uiCurrentByte += DLC_PARAM_ADV(paramBuf.dwWchCount);
        ReadDlcStruct(&paramBuf, pbData, uiCurrentByte);
    }

    unsigned int uiFileCount =
        ReadDlcValue<unsigned int>(pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);
    IPlatformStorage::DLC_FILE_DETAILS fileBuf;
    ReadDlcStruct(&fileBuf, pbData, uiCurrentByte);

    unsigned int dwTemp = uiCurrentByte;
    for (unsigned int i = 0; i < uiFileCount; i++) {
        dwTemp += DLC_DETAIL_ADV(fileBuf.dwWchCount);
        ReadDlcStruct(&fileBuf, pbData, dwTemp);
    }
    std::uint8_t* pbTemp = &pbData[dwTemp];
    ReadDlcStruct(&fileBuf, pbData, uiCurrentByte);

    for (unsigned int i = 0; i < uiFileCount; i++) {
        DLCManager::EDLCType type = (DLCManager::EDLCType)fileBuf.dwType;

        // Params
        uiParameterCount = ReadDlcValue<unsigned int>(pbTemp);
        pbTemp += sizeof(int);
        ReadDlcStruct(&paramBuf, pbTemp);
        for (unsigned int j = 0; j < uiParameterCount; j++) {
            auto it = parameterMapping.find(paramBuf.dwType);

            if (it != parameterMapping.end()) {
                if (type == e_DLCType_PackConfig) {
                    if (it->second == e_DLCParamType_PackId) {
                        std::string wsTemp = DLC_PARAM_WSTR(pbTemp, 0);
                        std::stringstream ss;
                        // 4J Stu - numbered using decimal to make it easier for
                        // artists/people to number manually
                        ss << std::dec << wsTemp.c_str();
                        ss >> packId;
                        bPackIDSet = true;
                        break;
                    }
                }
            }
            pbTemp += DLC_PARAM_ADV(paramBuf.dwWchCount);
            ReadDlcStruct(&paramBuf, pbTemp);
        }

        if (bPackIDSet) break;
        // Move the pointer to the start of the next files data;
        pbTemp += fileBuf.uiFileSize;
        uiCurrentByte += DLC_DETAIL_ADV(fileBuf.dwWchCount);

        ReadDlcStruct(&fileBuf, pbData, uiCurrentByte);
    }

    parameterMapping.clear();
    return packId;
}
