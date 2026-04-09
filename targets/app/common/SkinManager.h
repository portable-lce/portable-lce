#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/common/App_structs.h"
#include "minecraft/client/model/SkinBox.h"
#include "platform/XboxStubs.h"

class ModelPart;
class DLCManager;

class SkinManager {
public:
    SkinManager();

    // Skin get/set (require GameSettingsA pointer from Game)
    void setPlayerSkin(int iPad, const std::string& name,
                       GAME_SETTINGS** gameSettingsA);
    void setPlayerSkin(int iPad, std::uint32_t dwSkinId,
                       GAME_SETTINGS** gameSettingsA);
    std::string getPlayerSkinName(int iPad, GAME_SETTINGS** gameSettingsA);
    std::uint32_t getPlayerSkinId(int iPad, GAME_SETTINGS** gameSettingsA,
                                  DLCManager& dlcManager);

    // Cape get/set
    void setPlayerCape(int iPad, const std::string& name,
                       GAME_SETTINGS** gameSettingsA);
    void setPlayerCape(int iPad, std::uint32_t dwCapeId,
                       GAME_SETTINGS** gameSettingsA);
    std::string getPlayerCapeName(int iPad, GAME_SETTINGS** gameSettingsA);
    std::uint32_t getPlayerCapeId(int iPad, GAME_SETTINGS** gameSettingsA);

    // Favorite skins
    void setPlayerFavoriteSkin(int iPad, int iIndex, unsigned int uiSkinID,
                               GAME_SETTINGS** gameSettingsA);
    unsigned int getPlayerFavoriteSkin(int iPad, int iIndex,
                                       GAME_SETTINGS** gameSettingsA);
    unsigned char getPlayerFavoriteSkinsPos(int iPad,
                                            GAME_SETTINGS** gameSettingsA);
    void setPlayerFavoriteSkinsPos(int iPad, int iPos,
                                   GAME_SETTINGS** gameSettingsA);
    unsigned int getPlayerFavoriteSkinsCount(int iPad,
                                             GAME_SETTINGS** gameSettingsA);
    void validateFavoriteSkins(int iPad, GAME_SETTINGS** gameSettingsA,
                               DLCManager& dlcManager);

    // Additional model parts per player
    std::uint32_t getAdditionalModelParts(int iPad);

    // Additional model parts per skin texture
    void setAdditionalSkinBoxes(std::uint32_t dwSkinID, SKIN_BOX* SkinBoxA,
                                unsigned int dwSkinBoxC);
    std::vector<ModelPart*>* setAdditionalSkinBoxes(
        std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA);
    std::vector<ModelPart*>* getAdditionalModelParts(std::uint32_t dwSkinID);
    std::vector<SKIN_BOX*>* getAdditionalSkinBoxes(std::uint32_t dwSkinID);

    // Anim overrides
    void setAnimOverrideBitmask(std::uint32_t dwSkinID,
                                unsigned int uiAnimOverrideBitmask);
    unsigned int getAnimOverrideBitmask(std::uint32_t dwSkinID);

    // Skin path <-> id conversion (static)
    static std::uint32_t getSkinIdFromPath(const std::string& skin);
    static std::string getSkinPathFromId(std::uint32_t skinId);

    // Default cape
    bool defaultCapeExists();

    // Notch/Deadmau5 xuid checks
    bool isXuidNotch(PlayerUID xuid);
    bool isXuidDeadmau5(PlayerUID xuid);

    // Memory texture files for player skins
    void addMemoryTextureFile(const std::string& wName, std::uint8_t* pbData,
                              unsigned int byteCount);
    void removeMemoryTextureFile(const std::string& wName);
    void getMemFileDetails(const std::string& wName, std::uint8_t** ppbData,
                           unsigned int* pByteCount);
    bool isFileInMemoryTextures(const std::string& wName);

    // storing skin files
    std::vector<std::string> vSkinNames;

    // per-player additional model parts
    std::uint32_t m_dwAdditionalModelParts[XUSER_MAX_COUNT];

private:
    PlayerUID m_xuidNotch;

    // Memory texture files
    std::unordered_map<std::string, PMEMDATA> m_MEM_Files;
    std::mutex csMemFilesLock;

    // Additional model parts/skin boxes per skin id
    std::unordered_map<std::uint32_t, std::vector<ModelPart*>*>
        m_AdditionalModelParts;
    std::unordered_map<std::uint32_t, std::vector<SKIN_BOX*>*>
        m_AdditionalSkinBoxes;
    std::unordered_map<std::uint32_t, unsigned int> m_AnimOverrides;
    std::mutex csAdditionalModelParts;
    std::mutex csAdditionalSkinBoxes;
    std::mutex csAnimOverrideBitmask;
};
