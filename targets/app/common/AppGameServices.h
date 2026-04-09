#pragma once

#include "minecraft/IGameServices.h"

class Game;
class IMenuService;

class AppGameServices : public IGameServices {
public:
    AppGameServices(Game& game, IMenuService& menus);

    // -- Strings --
    const char* getString(int id) override;

    // -- Debug settings --
    bool debugSettingsOn() override;
    bool debugArtToolsOn() override;
    unsigned int debugGetMask(int iPad, bool overridePlayer) override;
    bool debugMobsDontAttack() override;
    bool debugMobsDontTick() override;
    bool debugFreezePlayers() override;

    // -- Game host options --
    unsigned int getGameHostOption(eGameHostOption option) override;
    void setGameHostOption(eGameHostOption option, unsigned int value) override;

    // -- Level generation --
    LevelGenerationOptions* getLevelGenerationOptions() override;
    LevelRuleset* getGameRuleDefinitions() override;

    // -- Texture cache --
    void addMemoryTextureFile(const std::string& name, std::uint8_t* data,
                              unsigned int size) override;
    void removeMemoryTextureFile(const std::string& name) override;
    void getMemFileDetails(const std::string& name, std::uint8_t** data,
                           unsigned int* size) override;
    bool isFileInMemoryTextures(const std::string& name) override;

    // -- Player settings --
    unsigned char getGameSettings(int iPad, int setting) override;
    unsigned char getGameSettings(int setting) override;

    // -- App time --
    float getAppTime() override;

    // -- Game state --
    bool getGameStarted() override;
    void setGameStarted(bool val) override;
    bool getTutorialMode() override;
    void setTutorialMode(bool val) override;
    bool isAppPaused() override;
    int getLocalPlayerCount() override;
    bool autosaveDue() override;
    void setAutosaveTimerTime() override;
    int64_t secondsToAutosave() override;
    void setDisconnectReason(
        DisconnectPacket::eDisconnectReason reason) override;
    void lockSaveNotification() override;
    void unlockSaveNotification() override;
    bool getResetNether() override;
    bool getUseDPadForDebug() override;
    bool getWriteSavesToFolderEnabled() override;
    bool isLocalMultiplayerAvailable() override;
    bool dlcInstallPending() override;
    bool dlcInstallProcessCompleted() override;
    bool canRecordStatsAndAchievements() override;
    bool getTMSGlobalFileListRead() override;
    void setRequiredTexturePackID(std::uint32_t id) override;
    void setSpecialTutorialCompletionFlag(int iPad, int index) override;
    void setBanListCheck(int iPad, bool val) override;
    bool getBanListCheck(int iPad) override;
    unsigned int getGameNewWorldSize() override;
    unsigned int getGameNewWorldSizeUseMoat() override;
    unsigned int getGameNewHellScale() override;

    // -- UI dispatch --
    void setAction(int iPad, eXuiAction action, void* param) override;
    void setXuiServerAction(int iPad, eXuiServerAction action,
                            XuiActionPayload param) override;
    eXuiAction getXuiAction(int iPad) override;
    eXuiServerAction getXuiServerAction(int iPad) override;
    const XuiActionPayload& getXuiServerActionParam(int iPad) override;
    void setGlobalXuiAction(eXuiAction action) override;
    void handleButtonPresses() override;
    void setTMSAction(int iPad, eTMSAction action) override;

    // -- Skin / cape / animation --
    std::string getPlayerSkinName(int iPad) override;
    std::uint32_t getPlayerSkinId(int iPad) override;
    std::string getPlayerCapeName(int iPad) override;
    std::uint32_t getPlayerCapeId(int iPad) override;
    std::uint32_t getAdditionalModelPartsForPad(int iPad) override;
    void setAdditionalSkinBoxes(std::uint32_t dwSkinID, SKIN_BOX* boxA,
                                unsigned int boxC) override;
    std::vector<SKIN_BOX*>* getAdditionalSkinBoxes(
        std::uint32_t dwSkinID) override;
    std::vector<ModelPart*>* getAdditionalModelParts(
        std::uint32_t dwSkinID) override;
    std::vector<ModelPart*>* setAdditionalSkinBoxesFromVec(
        std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA) override;
    void setAnimOverrideBitmask(std::uint32_t dwSkinID,
                                unsigned int bitmask) override;
    unsigned int getAnimOverrideBitmask(std::uint32_t dwSkinID) override;
    std::uint32_t getSkinIdFromPath(const std::string& skin) override;
    std::string getSkinPathFromId(std::uint32_t skinId) override;
    bool defaultCapeExists() override;
    bool isXuidNotch(PlayerUID xuid) override;
    bool isXuidDeadmau5(PlayerUID xuid) override;

    // -- Platform features --
    void fatalLoadError() override;
    void setRichPresenceContext(int iPad, int contextId) override;
    void captureSaveThumbnail() override;
    void getSaveThumbnail(std::uint8_t** data, unsigned int* size) override;
    void readBannedList(int iPad, eTMSAction action, bool bCallback) override;
    void updatePlayerInfo(std::uint8_t networkSmallId,
                          int16_t playerColourIndex,
                          unsigned int playerPrivileges) override;
    unsigned int getPlayerPrivileges(std::uint8_t networkSmallId) override;
    void setGameSettingsDebugMask(int iPad, unsigned int uiVal) override;

    // -- Schematics / terrain --
    void processSchematics(LevelChunk* chunk) override;
    void processSchematicsLighting(LevelChunk* chunk) override;
    void addTerrainFeaturePosition(_eTerrainFeatureType type, int x,
                                   int z) override;
    bool getTerrainFeaturePosition(_eTerrainFeatureType type, int* pX,
                                   int* pZ) override;
    void loadDefaultGameRules() override;

    // -- Archive / resources --
    bool hasArchiveFile(const std::string& filename) override;
    std::vector<std::uint8_t> getArchiveFile(
        const std::string& filename) override;

    // -- Strings / formatting / misc queries --
    int getHTMLColour(eMinecraftColour colour) override;
    std::string getEntityName(EntityTypeId type) override;
    const char* getGameRulesString(const std::string& key) override;
    unsigned int createImageTextData(std::uint8_t* textMetadata, int64_t seed,
                                     bool hasSeed, unsigned int uiHostOptions,
                                     unsigned int uiTexturePackId) override;
    std::string getFilePath(std::uint32_t packId, std::string filename,
                            bool bAddDataFolder,
                            std::string mountPoint) override;
    char* getUniqueMapName() override;
    void setUniqueMapName(char* name) override;
    unsigned int getOpacityTimer(int iPad) override;
    void setOpacityTimer(int iPad) override;
    void tickOpacityTimer(int iPad) override;
    bool isInBannedLevelList(int iPad, PlayerUID xuid,
                             char* levelName) override;
    MOJANG_DATA* getMojangDataForXuid(PlayerUID xuid) override;
    void debugPrintf(const char* msg) override;

    // -- DLC --
    DLCSkinFile* getDLCSkinFile(const std::string& name) override;
    bool dlcNeedsCorruptCheck() override;
    unsigned int dlcCheckForCorrupt(bool showMessage) override;
    bool dlcReadDataFile(unsigned int& filesProcessed, const std::string& path,
                         DLCPack* pack, bool fromArchive) override;
    void dlcRemovePack(DLCPack* pack) override;

    // -- Game rules --
    LevelGenerationOptions* loadGameRules(std::uint8_t* data,
                                          unsigned int size) override;
    void saveGameRules(std::uint8_t** data, unsigned int* size) override;
    void unloadCurrentGameRules() override;
    void setLevelGenerationOptions(LevelGenerationOptions* levelGen) override;

    // -- Shared data --
    std::vector<std::string>& getSkinNames() override;
    std::vector<FEATURE_DATA*>& getTerrainFeatures() override;

    // -- Menu service --
    IMenuService& menus() override;

private:
    Game& game_;
    IMenuService& menus_;
};
