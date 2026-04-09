#include "app/common/AppGameServices.h"

#include "app/common/DLC/DLCSkinFile.h"
#include "app/common/Game.h"
#include "java/Class.h"  // eINSTANCEOF
#include "platform/game/game.h"

AppGameServices::AppGameServices(Game& game, IMenuService& menus)
    : game_(game), menus_(menus) {}

// -- Strings --

const char* AppGameServices::getString(int id) { return Game::GetString(id); }

// -- Debug settings --

bool AppGameServices::debugSettingsOn() { return game_.DebugSettingsOn(); }

bool AppGameServices::debugArtToolsOn() { return game_.DebugArtToolsOn(); }

unsigned int AppGameServices::debugGetMask(int iPad, bool overridePlayer) {
    return game_.GetGameSettingsDebugMask(iPad, overridePlayer);
}

bool AppGameServices::debugMobsDontAttack() {
    return game_.GetMobsDontAttackEnabled();
}

bool AppGameServices::debugMobsDontTick() {
    return game_.GetMobsDontTickEnabled();
}

bool AppGameServices::debugFreezePlayers() { return game_.GetFreezePlayers(); }

// -- Game host options --

unsigned int AppGameServices::getGameHostOption(eGameHostOption option) {
    return game_.GetGameHostOption(option);
}

void AppGameServices::setGameHostOption(eGameHostOption option,
                                        unsigned int value) {
    game_.SetGameHostOption(option, value);
}

// -- Level generation --

LevelGenerationOptions* AppGameServices::getLevelGenerationOptions() {
    return game_.getLevelGenerationOptions();
}

LevelRuleset* AppGameServices::getGameRuleDefinitions() {
    return game_.getGameRuleDefinitions();
}

// -- Texture cache --

void AppGameServices::addMemoryTextureFile(const std::string& name,
                                           std::uint8_t* data,
                                           unsigned int size) {
    game_.AddMemoryTextureFile(name, data, size);
}

void AppGameServices::removeMemoryTextureFile(const std::string& name) {
    game_.RemoveMemoryTextureFile(name);
}

void AppGameServices::getMemFileDetails(const std::string& name,
                                        std::uint8_t** data,
                                        unsigned int* size) {
    game_.GetMemFileDetails(name, data, size);
}

bool AppGameServices::isFileInMemoryTextures(const std::string& name) {
    return game_.IsFileInMemoryTextures(name);
}

// -- Player settings --

unsigned char AppGameServices::getGameSettings(int iPad, int setting) {
    return game_.GetGameSettings(iPad, static_cast<eGameSetting>(setting));
}

unsigned char AppGameServices::getGameSettings(int setting) {
    return game_.GetGameSettings(static_cast<eGameSetting>(setting));
}

// -- App time --

float AppGameServices::getAppTime() { return game_.getAppTime(); }

// -- Game state --

bool AppGameServices::getGameStarted() { return game_.GetGameStarted(); }
void AppGameServices::setGameStarted(bool val) { game_.SetGameStarted(val); }
bool AppGameServices::getTutorialMode() { return game_.GetTutorialMode(); }
void AppGameServices::setTutorialMode(bool val) { game_.SetTutorialMode(val); }
bool AppGameServices::isAppPaused() { return game_.IsAppPaused(); }
int AppGameServices::getLocalPlayerCount() {
    return game_.GetLocalPlayerCount();
}
bool AppGameServices::autosaveDue() { return game_.AutosaveDue(); }
void AppGameServices::setAutosaveTimerTime() { game_.SetAutosaveTimerTime(); }
int64_t AppGameServices::secondsToAutosave() {
    return game_.SecondsToAutosave();
}

void AppGameServices::setDisconnectReason(
    DisconnectPacket::eDisconnectReason reason) {
    game_.SetDisconnectReason(reason);
}

void AppGameServices::lockSaveNotification() { game_.lockSaveNotification(); }
void AppGameServices::unlockSaveNotification() {
    game_.unlockSaveNotification();
}
bool AppGameServices::getResetNether() { return game_.GetResetNether(); }
bool AppGameServices::getUseDPadForDebug() {
    return game_.GetUseDPadForDebug();
}

bool AppGameServices::getWriteSavesToFolderEnabled() {
    return game_.GetWriteSavesToFolderEnabled();
}

bool AppGameServices::isLocalMultiplayerAvailable() {
    return game_.IsLocalMultiplayerAvailable();
}

bool AppGameServices::dlcInstallPending() { return game_.DLCInstallPending(); }

bool AppGameServices::dlcInstallProcessCompleted() {
    return game_.DLCInstallProcessCompleted();
}

bool AppGameServices::canRecordStatsAndAchievements() {
    return game_.CanRecordStatsAndAchievements();
}

bool AppGameServices::getTMSGlobalFileListRead() {
    return game_.GetTMSGlobalFileListRead();
}

void AppGameServices::setRequiredTexturePackID(std::uint32_t id) {
    game_.SetRequiredTexturePackID(id);
}

void AppGameServices::setSpecialTutorialCompletionFlag(int iPad, int index) {
    game_.SetSpecialTutorialCompletionFlag(iPad, index);
}

void AppGameServices::setBanListCheck(int iPad, bool val) {
    game_.SetBanListCheck(iPad, val);
}

bool AppGameServices::getBanListCheck(int iPad) {
    return game_.GetBanListCheck(iPad);
}

unsigned int AppGameServices::getGameNewWorldSize() {
    return game_.GetGameNewWorldSize();
}

unsigned int AppGameServices::getGameNewWorldSizeUseMoat() {
    return game_.GetGameNewWorldSizeUseMoat();
}

unsigned int AppGameServices::getGameNewHellScale() {
    return game_.GetGameNewHellScale();
}

// -- UI dispatch --

void AppGameServices::setAction(int iPad, eXuiAction action, void* param) {
    game_.SetAction(iPad, action, param);
}

eXuiAction AppGameServices::getXuiAction(int iPad) {
    return game_.GetXuiAction(iPad);
}

void AppGameServices::setGlobalXuiAction(eXuiAction action) {
    game_.SetGlobalXuiAction(action);
}

void AppGameServices::handleButtonPresses() { game_.HandleButtonPresses(); }

void AppGameServices::setTMSAction(int iPad, eTMSAction action) {
    game_.SetTMSAction(iPad, action);
}

// -- Skin / cape / animation --

std::string AppGameServices::getPlayerSkinName(int iPad) {
    return game_.GetPlayerSkinName(iPad);
}

std::uint32_t AppGameServices::getPlayerSkinId(int iPad) {
    return game_.GetPlayerSkinId(iPad);
}

std::string AppGameServices::getPlayerCapeName(int iPad) {
    return game_.GetPlayerCapeName(iPad);
}

std::uint32_t AppGameServices::getPlayerCapeId(int iPad) {
    return game_.GetPlayerCapeId(iPad);
}

std::uint32_t AppGameServices::getAdditionalModelPartsForPad(int iPad) {
    return game_.GetAdditionalModelParts(iPad);
}

void AppGameServices::setAdditionalSkinBoxes(std::uint32_t dwSkinID,
                                             SKIN_BOX* boxA,
                                             unsigned int boxC) {
    game_.SetAdditionalSkinBoxes(dwSkinID, boxA, boxC);
}

std::vector<SKIN_BOX*>* AppGameServices::getAdditionalSkinBoxes(
    std::uint32_t dwSkinID) {
    return game_.GetAdditionalSkinBoxes(dwSkinID);
}

std::vector<ModelPart*>* AppGameServices::getAdditionalModelParts(
    std::uint32_t dwSkinID) {
    return game_.GetAdditionalModelParts(dwSkinID);
}

std::vector<ModelPart*>* AppGameServices::setAdditionalSkinBoxesFromVec(
    std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA) {
    return game_.SetAdditionalSkinBoxes(dwSkinID, pvSkinBoxA);
}

void AppGameServices::setAnimOverrideBitmask(std::uint32_t dwSkinID,
                                             unsigned int bitmask) {
    game_.SetAnimOverrideBitmask(dwSkinID, bitmask);
}

unsigned int AppGameServices::getAnimOverrideBitmask(std::uint32_t dwSkinID) {
    return game_.GetAnimOverrideBitmask(dwSkinID);
}

std::uint32_t AppGameServices::getSkinIdFromPath(const std::string& skin) {
    return Game::getSkinIdFromPath(skin);
}

std::string AppGameServices::getSkinPathFromId(std::uint32_t skinId) {
    return Game::getSkinPathFromId(skinId);
}

bool AppGameServices::defaultCapeExists() { return game_.DefaultCapeExists(); }

bool AppGameServices::isXuidNotch(PlayerUID xuid) {
    return game_.isXuidNotch(xuid);
}

bool AppGameServices::isXuidDeadmau5(PlayerUID xuid) {
    return game_.isXuidDeadmau5(xuid);
}

// -- Platform features --

void AppGameServices::fatalLoadError() { game_.FatalLoadError(); }

void AppGameServices::setRichPresenceContext(int iPad, int contextId) {
    PlatformGame.SetRichPresenceContext(iPad, contextId);
}

void AppGameServices::captureSaveThumbnail() {
    PlatformGame.CaptureSaveThumbnail();
}

void AppGameServices::getSaveThumbnail(std::uint8_t** data,
                                       unsigned int* size) {
    PlatformGame.GetSaveThumbnail(data, size);
}

void AppGameServices::readBannedList(int iPad, eTMSAction action,
                                     bool bCallback) {
    PlatformGame.ReadBannedList(iPad, action, bCallback);
}

void AppGameServices::updatePlayerInfo(std::uint8_t networkSmallId,
                                       int16_t playerColourIndex,
                                       unsigned int playerPrivileges) {
    game_.UpdatePlayerInfo(networkSmallId, playerColourIndex, playerPrivileges);
}

unsigned int AppGameServices::getPlayerPrivileges(std::uint8_t networkSmallId) {
    return game_.GetPlayerPrivileges(networkSmallId);
}

void AppGameServices::setGameSettingsDebugMask(int iPad, unsigned int uiVal) {
    game_.SetGameSettingsDebugMask(iPad, uiVal);
}

// -- Schematics / terrain --

void AppGameServices::processSchematics(LevelChunk* chunk) {
    game_.processSchematics(chunk);
}

void AppGameServices::processSchematicsLighting(LevelChunk* chunk) {
    game_.processSchematicsLighting(chunk);
}

void AppGameServices::addTerrainFeaturePosition(_eTerrainFeatureType type,
                                                int x, int z) {
    game_.AddTerrainFeaturePosition(type, x, z);
}

bool AppGameServices::getTerrainFeaturePosition(_eTerrainFeatureType type,
                                                int* pX, int* pZ) {
    return game_.GetTerrainFeaturePosition(type, pX, pZ);
}

void AppGameServices::loadDefaultGameRules() { game_.loadDefaultGameRules(); }

// -- Archive / resources --

bool AppGameServices::hasArchiveFile(const std::string& filename) {
    return game_.hasArchiveFile(filename);
}

std::vector<std::uint8_t> AppGameServices::getArchiveFile(
    const std::string& filename) {
    return game_.getArchiveFile(filename);
}

// -- Strings / formatting / misc queries --

int AppGameServices::getHTMLColour(eMinecraftColour colour) {
    return game_.GetHTMLColour(colour);
}

std::string AppGameServices::getEntityName(EntityTypeId type) {
    return game_.getEntityName(static_cast<eINSTANCEOF>(type));
}

const char* AppGameServices::getGameRulesString(const std::string& key) {
    return game_.GetGameRulesString(key);
}

unsigned int AppGameServices::createImageTextData(
    std::uint8_t* textMetadata, int64_t seed, bool hasSeed,
    unsigned int uiHostOptions, unsigned int uiTexturePackId) {
    return game_.CreateImageTextData(textMetadata, seed, hasSeed, uiHostOptions,
                                     uiTexturePackId);
}

std::string AppGameServices::getFilePath(std::uint32_t packId,
                                         std::string filename,
                                         bool bAddDataFolder,
                                         std::string mountPoint) {
    return game_.getFilePath(packId, filename, bAddDataFolder, mountPoint);
}

char* AppGameServices::getUniqueMapName() { return game_.GetUniqueMapName(); }

void AppGameServices::setUniqueMapName(char* name) {
    game_.SetUniqueMapName(name);
}

unsigned int AppGameServices::getOpacityTimer(int iPad) {
    return game_.GetOpacityTimer(iPad);
}

void AppGameServices::setOpacityTimer(int iPad) { game_.SetOpacityTimer(iPad); }

void AppGameServices::tickOpacityTimer(int iPad) {
    game_.TickOpacityTimer(iPad);
}

bool AppGameServices::isInBannedLevelList(int iPad, PlayerUID xuid,
                                          char* levelName) {
    return game_.IsInBannedLevelList(iPad, xuid, levelName);
}

MOJANG_DATA* AppGameServices::getMojangDataForXuid(PlayerUID xuid) {
    return game_.GetMojangDataForXuid(xuid);
}

void AppGameServices::debugPrintf(const char* msg) {
    game_.DebugPrintf("%s", msg);
}

// -- DLC --

ISkinAssetData* AppGameServices::getSkinAssetData(const std::string& name) {
    return game_.m_dlcManager.getSkinFile(name);
}
bool AppGameServices::dlcNeedsCorruptCheck() {
    return game_.m_dlcManager.NeedsCorruptCheck();
}
unsigned int AppGameServices::dlcCheckForCorrupt(bool showMessage) {
    return game_.m_dlcManager.checkForCorruptDLCAndAlert(showMessage);
}
bool AppGameServices::dlcReadDataFile(unsigned int& filesProcessed,
                                      const std::string& path, DLCPack* pack,
                                      bool fromArchive) {
    return game_.m_dlcManager.readDLCDataFile(filesProcessed, path, pack,
                                              fromArchive);
}
void AppGameServices::dlcRemovePack(DLCPack* pack) {
    game_.m_dlcManager.removePack(pack);
}

// -- Game rules --

LevelGenerationOptions* AppGameServices::loadGameRules(std::uint8_t* data,
                                                       unsigned int size) {
    return game_.m_gameRules.loadGameRules(data, size);
}
void AppGameServices::saveGameRules(std::uint8_t** data, unsigned int* size) {
    game_.m_gameRules.saveGameRules(data, size);
}
void AppGameServices::unloadCurrentGameRules() {
    game_.m_gameRules.unloadCurrentGameRules();
}
void AppGameServices::setLevelGenerationOptions(
    LevelGenerationOptions* levelGen) {
    game_.m_gameRules.setLevelGenerationOptions(levelGen);
}

// -- Shared data --

std::vector<std::string>& AppGameServices::getSkinNames() {
    return game_.vSkinNames;
}

std::vector<FEATURE_DATA*>& AppGameServices::getTerrainFeatures() {
    return *game_.m_terrainFeatureManager.features();
}

// -- Menu service --

IMenuService& AppGameServices::menus() { return menus_; }
