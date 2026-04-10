#include "app/common/SkinManager.h"

#include <wchar.h>

#include <mutex>
#include <sstream>
#include <string>

#include "app/common/App_structs.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/DLC/DLCSkinFile.h"
#include "app/common/Game.h"
#include "minecraft/Minecraft_Macros.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/world/entity/player/Player.h"
#include "platform/profile/profile.h"

SkinManager::SkinManager() : m_xuidNotch(INVALID_XUID) {
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        m_dwAdditionalModelParts[i] = 0;
    }
}

void SkinManager::setPlayerSkin(int iPad, const std::string& name,
                                GAME_SETTINGS** gameSettingsA) {
    std::uint32_t skinId = getSkinIdFromPath(name);
    setPlayerSkin(iPad, skinId, gameSettingsA);
}

void SkinManager::setPlayerSkin(int iPad, std::uint32_t dwSkinId,
                                GAME_SETTINGS** gameSettingsA) {
    app.DebugPrintf("Setting skin for %d to %08X\n", iPad, dwSkinId);

    gameSettingsA[iPad]->dwSelectedSkin = dwSkinId;
    gameSettingsA[iPad]->bSettingsChanged = true;

    if (Minecraft::GetInstance()->localplayers[iPad] != nullptr)
        Minecraft::GetInstance()->localplayers[iPad]->setAndBroadcastCustomSkin(
            dwSkinId);
}

std::string SkinManager::getPlayerSkinName(int iPad,
                                           GAME_SETTINGS** gameSettingsA) {
    return getSkinPathFromId(gameSettingsA[iPad]->dwSelectedSkin);
}

std::uint32_t SkinManager::getPlayerSkinId(int iPad,
                                           GAME_SETTINGS** gameSettingsA,
                                           DLCManager& dlcManager) {
    DLCPack* Pack = nullptr;
    DLCSkinFile* skinFile = nullptr;
    std::uint32_t dwSkin = gameSettingsA[iPad]->dwSelectedSkin;
    char chars[256];

    if (GET_IS_DLC_SKIN_FROM_BITMASK(dwSkin)) {
        snprintf(chars, 256, "dlcskin%08d.png",
                 GET_DLC_SKIN_ID_FROM_BITMASK(dwSkin));

        Pack = dlcManager.getPackContainingSkin(chars);

        if (Pack) {
            skinFile = Pack->getSkinFile(chars);

            bool bSkinIsFree =
                skinFile->getParameterAsBool(DLCManager::e_DLCParamType_Free);
            bool bLicensed = Pack->hasPurchasedFile(DLCManager::e_DLCType_Skin,
                                                    skinFile->getPath());

            if (bSkinIsFree || bLicensed) {
                return dwSkin;
            } else {
                return 0;
            }
        }
    }

    return dwSkin;
}

std::uint32_t SkinManager::getAdditionalModelParts(int iPad) {
    return m_dwAdditionalModelParts[iPad];
}

void SkinManager::setPlayerCape(int iPad, const std::string& name,
                                GAME_SETTINGS** gameSettingsA) {
    std::uint32_t capeId = Player::getCapeIdFromPath(name);
    setPlayerCape(iPad, capeId, gameSettingsA);
}

void SkinManager::setPlayerCape(int iPad, std::uint32_t dwCapeId,
                                GAME_SETTINGS** gameSettingsA) {
    app.DebugPrintf("Setting cape for %d to %08X\n", iPad, dwCapeId);

    gameSettingsA[iPad]->dwSelectedCape = dwCapeId;
    gameSettingsA[iPad]->bSettingsChanged = true;

    if (Minecraft::GetInstance()->localplayers[iPad] != nullptr)
        Minecraft::GetInstance()->localplayers[iPad]->setAndBroadcastCustomCape(
            dwCapeId);
}

std::string SkinManager::getPlayerCapeName(int iPad,
                                           GAME_SETTINGS** gameSettingsA) {
    return Player::getCapePathFromId(gameSettingsA[iPad]->dwSelectedCape);
}

std::uint32_t SkinManager::getPlayerCapeId(int iPad,
                                           GAME_SETTINGS** gameSettingsA) {
    return gameSettingsA[iPad]->dwSelectedCape;
}

void SkinManager::setPlayerFavoriteSkin(int iPad, int iIndex,
                                        unsigned int uiSkinID,
                                        GAME_SETTINGS** gameSettingsA) {
    app.DebugPrintf("Setting favorite skin for %d to %08X\n", iPad, uiSkinID);

    gameSettingsA[iPad]->uiFavoriteSkinA[iIndex] = uiSkinID;
    gameSettingsA[iPad]->bSettingsChanged = true;
}

unsigned int SkinManager::getPlayerFavoriteSkin(int iPad, int iIndex,
                                                GAME_SETTINGS** gameSettingsA) {
    return gameSettingsA[iPad]->uiFavoriteSkinA[iIndex];
}

unsigned char SkinManager::getPlayerFavoriteSkinsPos(
    int iPad, GAME_SETTINGS** gameSettingsA) {
    return gameSettingsA[iPad]->ucCurrentFavoriteSkinPos;
}

void SkinManager::setPlayerFavoriteSkinsPos(int iPad, int iPos,
                                            GAME_SETTINGS** gameSettingsA) {
    gameSettingsA[iPad]->ucCurrentFavoriteSkinPos = (unsigned char)iPos;
    gameSettingsA[iPad]->bSettingsChanged = true;
}

unsigned int SkinManager::getPlayerFavoriteSkinsCount(
    int iPad, GAME_SETTINGS** gameSettingsA) {
    unsigned int uiCount = 0;
    for (int i = 0; i < MAX_FAVORITE_SKINS; i++) {
        if (gameSettingsA[iPad]->uiFavoriteSkinA[i] != 0xFFFFFFFF) {
            uiCount++;
        } else {
            break;
        }
    }
    return uiCount;
}

void SkinManager::validateFavoriteSkins(int iPad, GAME_SETTINGS** gameSettingsA,
                                        DLCManager& dlcManager) {
    unsigned int uiCount = getPlayerFavoriteSkinsCount(iPad, gameSettingsA);

    unsigned int uiValidSkin = 0;
    char chars[256];

    for (unsigned int i = 0; i < uiCount; i++) {
        snprintf(chars, 256, "dlcskin%08d.png",
                 getPlayerFavoriteSkin(iPad, i, gameSettingsA));

        DLCPack* pDLCPack = dlcManager.getPackContainingSkin(chars);

        if (pDLCPack != nullptr) {
            DLCSkinFile* pSkinFile = pDLCPack->getSkinFile(chars);

            if (pDLCPack->hasPurchasedFile(DLCManager::e_DLCType_Skin, "") ||
                (pSkinFile && pSkinFile->isFree())) {
                gameSettingsA[iPad]->uiFavoriteSkinA[uiValidSkin++] =
                    gameSettingsA[iPad]->uiFavoriteSkinA[i];
            }
        }
    }

    for (unsigned int i = uiValidSkin; i < MAX_FAVORITE_SKINS; i++) {
        gameSettingsA[iPad]->uiFavoriteSkinA[i] = 0xFFFFFFFF;
    }
}

bool SkinManager::isXuidNotch(PlayerUID xuid) {
    if (m_xuidNotch != INVALID_XUID && xuid != INVALID_XUID) {
        return PlatformProfile.AreXUIDSEqual(xuid, m_xuidNotch);
    }
    return false;
}

bool SkinManager::isXuidDeadmau5(PlayerUID xuid) {
    // Delegates back to static MojangData on Game - this is a simple forwarding
    // wrapper for now; the actual MojangData map stays on Game.
    return app.isXuidDeadmau5(xuid);
}

void SkinManager::addMemoryTextureFile(const std::string& wName,
                                       std::uint8_t* pbData,
                                       unsigned int byteCount) {
    std::lock_guard<std::mutex> lock(csMemFilesLock);
    PMEMDATA pData = nullptr;
    auto it = m_MEM_Files.find(wName);
    if (it != m_MEM_Files.end()) {
#if !defined(_CONTENT_PACKAGE)
        printf("Incrementing the memory texture file count for %s\n",
               wName.c_str());
#endif
        pData = (*it).second;

        if (pData->byteCount == 0 && byteCount != 0) {
            if (pData->pbData != nullptr) delete[] pData->pbData;

            pData->pbData = pbData;
            pData->byteCount = byteCount;
        }

        ++pData->ucRefCount;
        return;
    }

    pData = new MEMDATA();
    pData->pbData = pbData;
    pData->byteCount = byteCount;
    pData->ucRefCount = 1;

    m_MEM_Files[wName] = pData;
}

void SkinManager::removeMemoryTextureFile(const std::string& wName) {
    std::lock_guard<std::mutex> lock(csMemFilesLock);

    auto it = m_MEM_Files.find(wName);
    if (it != m_MEM_Files.end()) {
#if !defined(_CONTENT_PACKAGE)
        printf("Decrementing the memory texture file count for %s\n",
               wName.c_str());
#endif
        PMEMDATA pData = (*it).second;
        --pData->ucRefCount;
        if (pData->ucRefCount <= 0) {
#if !defined(_CONTENT_PACKAGE)
            printf("Erasing the memory texture file data for %s\n",
                   wName.c_str());
#endif
            delete pData;
            m_MEM_Files.erase(wName);
        }
    }
}

bool SkinManager::defaultCapeExists() {
    std::string wTex = "Special_Cape.png";
    bool val = false;

    {
        std::lock_guard<std::mutex> lock(csMemFilesLock);
        auto it = m_MEM_Files.find(wTex);
        if (it != m_MEM_Files.end()) val = true;
    }

    return val;
}

bool SkinManager::isFileInMemoryTextures(const std::string& wName) {
    bool val = false;

    {
        std::lock_guard<std::mutex> lock(csMemFilesLock);
        auto it = m_MEM_Files.find(wName);
        if (it != m_MEM_Files.end()) val = true;
    }

    return val;
}

void SkinManager::getMemFileDetails(const std::string& wName,
                                    std::uint8_t** ppbData,
                                    unsigned int* pByteCount) {
    std::lock_guard<std::mutex> lock(csMemFilesLock);
    auto it = m_MEM_Files.find(wName);
    if (it != m_MEM_Files.end()) {
        PMEMDATA pData = (*it).second;
        *ppbData = pData->pbData;
        *pByteCount = pData->byteCount;
    }
}

void SkinManager::setAdditionalSkinBoxes(std::uint32_t dwSkinID,
                                         SKIN_BOX* SkinBoxA,
                                         unsigned int dwSkinBoxC) {
    EntityRenderer* renderer =
        EntityRenderDispatcher::instance->getRenderer(eTYPE_PLAYER);
    Model* pModel = renderer->getModel();
    std::vector<ModelPart*>* pvModelPart = new std::vector<ModelPart*>;
    std::vector<SKIN_BOX*>* pvSkinBoxes = new std::vector<SKIN_BOX*>;

    {
        std::lock_guard<std::mutex> lock_mp(csAdditionalModelParts);
        std::lock_guard<std::mutex> lock_sb(csAdditionalSkinBoxes);

        app.DebugPrintf(
            "*** SetAdditionalSkinBoxes - Inserting model parts for skin %d "
            "from "
            "array of Skin Boxes\n",
            dwSkinID & 0x0FFFFFFF);

        for (unsigned int i = 0; i < dwSkinBoxC; i++) {
            if (pModel) {
                ModelPart* pModelPart = pModel->AddOrRetrievePart(&SkinBoxA[i]);
                pvModelPart->push_back(pModelPart);
                pvSkinBoxes->push_back(&SkinBoxA[i]);
            }
        }

        m_AdditionalModelParts.insert(
            std::pair<std::uint32_t, std::vector<ModelPart*>*>(dwSkinID,
                                                               pvModelPart));
        m_AdditionalSkinBoxes.insert(
            std::pair<std::uint32_t, std::vector<SKIN_BOX*>*>(dwSkinID,
                                                              pvSkinBoxes));
    }
}

std::vector<ModelPart*>* SkinManager::setAdditionalSkinBoxes(
    std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA) {
    EntityRenderer* renderer =
        EntityRenderDispatcher::instance->getRenderer(eTYPE_PLAYER);
    Model* pModel = renderer->getModel();
    std::vector<ModelPart*>* pvModelPart = new std::vector<ModelPart*>;

    {
        std::lock_guard<std::mutex> lock_mp(csAdditionalModelParts);
        std::lock_guard<std::mutex> lock_sb(csAdditionalSkinBoxes);
        app.DebugPrintf(
            "*** SetAdditionalSkinBoxes - Inserting model parts for skin %d "
            "from "
            "array of Skin Boxes\n",
            dwSkinID & 0x0FFFFFFF);

        for (auto it = pvSkinBoxA->begin(); it != pvSkinBoxA->end(); ++it) {
            if (pModel) {
                ModelPart* pModelPart = pModel->AddOrRetrievePart(*it);
                pvModelPart->push_back(pModelPart);
            }
        }

        m_AdditionalModelParts.insert(
            std::pair<std::uint32_t, std::vector<ModelPart*>*>(dwSkinID,
                                                               pvModelPart));
        m_AdditionalSkinBoxes.insert(
            std::pair<std::uint32_t, std::vector<SKIN_BOX*>*>(dwSkinID,
                                                              pvSkinBoxA));
    }
    return pvModelPart;
}

std::vector<ModelPart*>* SkinManager::getAdditionalModelParts(
    std::uint32_t dwSkinID) {
    std::lock_guard<std::mutex> lock(csAdditionalModelParts);
    std::vector<ModelPart*>* pvModelParts = nullptr;
    if (m_AdditionalModelParts.size() > 0) {
        auto it = m_AdditionalModelParts.find(dwSkinID);
        if (it != m_AdditionalModelParts.end()) {
            pvModelParts = (*it).second;
        }
    }

    return pvModelParts;
}

std::vector<SKIN_BOX*>* SkinManager::getAdditionalSkinBoxes(
    std::uint32_t dwSkinID) {
    std::lock_guard<std::mutex> lock(csAdditionalSkinBoxes);
    std::vector<SKIN_BOX*>* pvSkinBoxes = nullptr;
    if (m_AdditionalSkinBoxes.size() > 0) {
        auto it = m_AdditionalSkinBoxes.find(dwSkinID);
        if (it != m_AdditionalSkinBoxes.end()) {
            pvSkinBoxes = (*it).second;
        }
    }

    return pvSkinBoxes;
}

unsigned int SkinManager::getAnimOverrideBitmask(std::uint32_t dwSkinID) {
    std::lock_guard<std::mutex> lock(csAnimOverrideBitmask);
    unsigned int uiAnimOverrideBitmask = 0L;

    if (m_AnimOverrides.size() > 0) {
        auto it = m_AnimOverrides.find(dwSkinID);
        if (it != m_AnimOverrides.end()) {
            uiAnimOverrideBitmask = (*it).second;
        }
    }

    return uiAnimOverrideBitmask;
}

void SkinManager::setAnimOverrideBitmask(std::uint32_t dwSkinID,
                                         unsigned int uiAnimOverrideBitmask) {
    std::lock_guard<std::mutex> lock(csAnimOverrideBitmask);

    if (m_AnimOverrides.size() > 0) {
        auto it = m_AnimOverrides.find(dwSkinID);
        if (it != m_AnimOverrides.end()) {
            return;  // already in here
        }
    }
    m_AnimOverrides.insert(std::pair<std::uint32_t, unsigned int>(
        dwSkinID, uiAnimOverrideBitmask));
}

std::uint32_t SkinManager::getSkinIdFromPath(const std::string& skin) {
    bool dlcSkin = false;
    unsigned int skinId = 0;

    if (skin.size() >= 14) {
        dlcSkin = skin.substr(0, 3).compare("dlc") == 0;

        std::string skinValue = skin.substr(7, skin.size());
        skinValue = skinValue.substr(0, skinValue.find_first_of('.'));

        std::stringstream ss;
        if (dlcSkin)
            ss << std::dec << skinValue.c_str();
        else
            ss << std::hex << skinValue.c_str();
        ss >> skinId;

        skinId = MAKE_SKIN_BITMASK(dlcSkin, skinId);
    }
    return skinId;
}

std::string SkinManager::getSkinPathFromId(std::uint32_t skinId) {
    char chars[256];
    if (GET_IS_DLC_SKIN_FROM_BITMASK(skinId)) {
        snprintf(chars, 256, "dlcskin%08d.png",
                 GET_DLC_SKIN_ID_FROM_BITMASK(skinId));
    } else {
        std::uint32_t ugcSkinIndex = GET_UGC_SKIN_ID_FROM_BITMASK(skinId);
        std::uint32_t defaultSkinIndex =
            GET_DEFAULT_SKIN_ID_FROM_BITMASK(skinId);
        if (ugcSkinIndex == 0) {
            snprintf(chars, 256, "defskin%08X.png", defaultSkinIndex);
        } else {
            snprintf(chars, 256, "ugcskin%08X.png", ugcSkinIndex);
        }
    }
    return chars;
}
