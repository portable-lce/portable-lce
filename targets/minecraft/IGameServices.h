#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations - minecraft types
class LevelGenerationOptions;
class LevelRuleset;
class LevelChunk;
class ModelPart;

// Forward declarations
class ISkinAssetData;
class DLCPack;

#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/IMenuService.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "platform/PlatformTypes.h"

// eINSTANCEOF lives in java/Class.h which is heavyweight.
using EntityTypeId = int;

class IGameServices {
public:
    virtual ~IGameServices() = default;

    // -- Strings --

    [[nodiscard]] virtual const char* getString(int id) = 0;

    // -- Debug settings --

    [[nodiscard]] virtual bool debugSettingsOn() = 0;
    [[nodiscard]] virtual bool debugArtToolsOn() = 0;
    [[nodiscard]] virtual unsigned int debugGetMask(
        int iPad = -1, bool overridePlayer = false) = 0;
    [[nodiscard]] virtual bool debugMobsDontAttack() = 0;
    [[nodiscard]] virtual bool debugMobsDontTick() = 0;
    [[nodiscard]] virtual bool debugFreezePlayers() = 0;

    // -- Game host options (global settings via stored pointer) --

    [[nodiscard]] virtual unsigned int getGameHostOption(
        eGameHostOption option) = 0;
    virtual void setGameHostOption(eGameHostOption option,
                                   unsigned int value) = 0;

    // -- Level generation --

    [[nodiscard]] virtual LevelGenerationOptions*
    getLevelGenerationOptions() = 0;
    [[nodiscard]] virtual LevelRuleset* getGameRuleDefinitions() = 0;

    // -- Texture cache --

    virtual void addMemoryTextureFile(const std::string& name,
                                      std::uint8_t* data,
                                      unsigned int size) = 0;
    virtual void removeMemoryTextureFile(const std::string& name) = 0;
    virtual void getMemFileDetails(const std::string& name, std::uint8_t** data,
                                   unsigned int* size) = 0;
    [[nodiscard]] virtual bool isFileInMemoryTextures(
        const std::string& name) = 0;

    // -- Player settings --

    [[nodiscard]] virtual unsigned char getGameSettings(int iPad,
                                                        int setting) = 0;
    [[nodiscard]] virtual unsigned char getGameSettings(int setting) = 0;

    // -- App time --

    [[nodiscard]] virtual float getAppTime() = 0;

    // -- Game state --

    [[nodiscard]] virtual bool getGameStarted() = 0;
    virtual void setGameStarted(bool val) = 0;
    [[nodiscard]] virtual bool getTutorialMode() = 0;
    virtual void setTutorialMode(bool val) = 0;
    [[nodiscard]] virtual bool isAppPaused() = 0;
    [[nodiscard]] virtual int getLocalPlayerCount() = 0;
    [[nodiscard]] virtual bool autosaveDue() = 0;
    virtual void setAutosaveTimerTime() = 0;
    [[nodiscard]] virtual int64_t secondsToAutosave() = 0;
    virtual void setDisconnectReason(
        DisconnectPacket::eDisconnectReason reason) = 0;
    virtual void lockSaveNotification() = 0;
    virtual void unlockSaveNotification() = 0;
    [[nodiscard]] virtual bool getResetNether() = 0;
    [[nodiscard]] virtual bool getUseDPadForDebug() = 0;
    [[nodiscard]] virtual bool getWriteSavesToFolderEnabled() = 0;
    [[nodiscard]] virtual bool isLocalMultiplayerAvailable() = 0;
    [[nodiscard]] virtual bool dlcInstallPending() = 0;
    [[nodiscard]] virtual bool dlcInstallProcessCompleted() = 0;
    [[nodiscard]] virtual bool canRecordStatsAndAchievements() = 0;
    [[nodiscard]] virtual bool getTMSGlobalFileListRead() = 0;
    virtual void setRequiredTexturePackID(std::uint32_t id) = 0;
    virtual void setSpecialTutorialCompletionFlag(int iPad, int index) = 0;
    virtual void setBanListCheck(int iPad, bool val) = 0;
    [[nodiscard]] virtual bool getBanListCheck(int iPad) = 0;
    [[nodiscard]] virtual unsigned int getGameNewWorldSize() = 0;
    [[nodiscard]] virtual unsigned int getGameNewWorldSizeUseMoat() = 0;
    [[nodiscard]] virtual unsigned int getGameNewHellScale() = 0;

    // -- UI dispatch --

    virtual void setAction(int iPad, eXuiAction action,
                           void* param = nullptr) = 0;
    [[nodiscard]] virtual eXuiAction getXuiAction(int iPad) = 0;
    virtual void setGlobalXuiAction(eXuiAction action) = 0;
    virtual void handleButtonPresses() = 0;
    virtual void setTMSAction(int iPad, eTMSAction action) = 0;

    // -- Skin / cape / animation --

    [[nodiscard]] virtual std::string getPlayerSkinName(int iPad) = 0;
    [[nodiscard]] virtual std::uint32_t getPlayerSkinId(int iPad) = 0;
    [[nodiscard]] virtual std::string getPlayerCapeName(int iPad) = 0;
    [[nodiscard]] virtual std::uint32_t getPlayerCapeId(int iPad) = 0;
    [[nodiscard]] virtual std::uint32_t getAdditionalModelPartsForPad(
        int iPad) = 0;
    virtual void setAdditionalSkinBoxes(std::uint32_t dwSkinID, SKIN_BOX* boxA,
                                        unsigned int boxC) = 0;
    [[nodiscard]] virtual std::vector<SKIN_BOX*>* getAdditionalSkinBoxes(
        std::uint32_t dwSkinID) = 0;
    [[nodiscard]] virtual std::vector<ModelPart*>* getAdditionalModelParts(
        std::uint32_t dwSkinID) = 0;
    virtual std::vector<ModelPart*>* setAdditionalSkinBoxesFromVec(
        std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA) = 0;
    virtual void setAnimOverrideBitmask(std::uint32_t dwSkinID,
                                        unsigned int bitmask) = 0;
    [[nodiscard]] virtual unsigned int getAnimOverrideBitmask(
        std::uint32_t dwSkinID) = 0;
    [[nodiscard]] virtual std::uint32_t getSkinIdFromPath(
        const std::string& skin) = 0;
    [[nodiscard]] virtual std::string getSkinPathFromId(
        std::uint32_t skinId) = 0;
    [[nodiscard]] virtual bool defaultCapeExists() = 0;
    [[nodiscard]] virtual bool isXuidNotch(PlayerUID xuid) = 0;
    [[nodiscard]] virtual bool isXuidDeadmau5(PlayerUID xuid) = 0;

    // -- Platform features --

    virtual void fatalLoadError() = 0;
    virtual void setRichPresenceContext(int iPad, int contextId) = 0;
    virtual void captureSaveThumbnail() = 0;
    virtual void getSaveThumbnail(std::uint8_t** data, unsigned int* size) = 0;
    virtual void readBannedList(int iPad, eTMSAction action = (eTMSAction)0,
                                bool bCallback = false) = 0;
    virtual void updatePlayerInfo(std::uint8_t networkSmallId,
                                  int16_t playerColourIndex,
                                  unsigned int playerPrivileges) = 0;
    [[nodiscard]] virtual unsigned int getPlayerPrivileges(
        std::uint8_t networkSmallId) = 0;
    virtual void setGameSettingsDebugMask(int iPad, unsigned int uiVal) = 0;

    // -- Schematics / terrain --

    virtual void processSchematics(LevelChunk* chunk) = 0;
    virtual void processSchematicsLighting(LevelChunk* chunk) = 0;
    virtual void addTerrainFeaturePosition(_eTerrainFeatureType type, int x,
                                           int z) = 0;
    [[nodiscard]] virtual bool getTerrainFeaturePosition(
        _eTerrainFeatureType type, int* pX, int* pZ) = 0;
    virtual void loadDefaultGameRules() = 0;

    // -- Archive / resources --

    [[nodiscard]] virtual bool hasArchiveFile(const std::string& filename) = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> getArchiveFile(
        const std::string& filename) = 0;

    // -- Strings / formatting / misc queries --

    [[nodiscard]] virtual int getHTMLColour(eMinecraftColour colour) = 0;
    [[nodiscard]] virtual std::string getEntityName(EntityTypeId type) = 0;
    [[nodiscard]] virtual const char* getGameRulesString(
        const std::string& key) = 0;
    [[nodiscard]] virtual unsigned int createImageTextData(
        std::uint8_t* textMetadata, int64_t seed, bool hasSeed,
        unsigned int uiHostOptions, unsigned int uiTexturePackId) = 0;
    [[nodiscard]] virtual std::string getFilePath(
        std::uint32_t packId, std::string filename, bool bAddDataFolder,
        std::string mountPoint = "TPACK:") = 0;
    [[nodiscard]] virtual char* getUniqueMapName() = 0;
    virtual void setUniqueMapName(char* name) = 0;
    [[nodiscard]] virtual unsigned int getOpacityTimer(int iPad) = 0;
    virtual void setOpacityTimer(int iPad) = 0;
    virtual void tickOpacityTimer(int iPad) = 0;
    [[nodiscard]] virtual bool isInBannedLevelList(int iPad, PlayerUID xuid,
                                                   char* levelName) = 0;
    [[nodiscard]] virtual MOJANG_DATA* getMojangDataForXuid(PlayerUID xuid) = 0;
    virtual void debugPrintf(const char* msg) = 0;

    // -- DLC --

    [[nodiscard]] virtual ISkinAssetData* getSkinAssetData(
        const std::string& name) = 0;
    [[nodiscard]] virtual bool dlcNeedsCorruptCheck() = 0;
    virtual unsigned int dlcCheckForCorrupt(bool showMessage = true) = 0;
    [[nodiscard]] virtual bool dlcReadDataFile(unsigned int& filesProcessed,
                                               const std::string& path,
                                               DLCPack* pack,
                                               bool fromArchive = false) = 0;
    virtual void dlcRemovePack(DLCPack* pack) = 0;

    // -- Game rules --

    virtual LevelGenerationOptions* loadGameRules(std::uint8_t* data,
                                                  unsigned int size) = 0;
    virtual void saveGameRules(std::uint8_t** data, unsigned int* size) = 0;
    virtual void unloadCurrentGameRules() = 0;
    virtual void setLevelGenerationOptions(
        LevelGenerationOptions* levelGen) = 0;

    // -- Shared data --

    [[nodiscard]] virtual std::vector<std::string>& getSkinNames() = 0;
    [[nodiscard]] virtual std::vector<FEATURE_DATA*>& getTerrainFeatures() = 0;

    // -- Menu service --

    [[nodiscard]] virtual IMenuService& menus() = 0;
};

// Global accessor - set once at startup, used everywhere in minecraft/
void initGameServices(IGameServices* services);
IGameServices& gameServices();
