#pragma once

#include <cstdint>
#include <mutex>

#include "platform/profile/profile.h"
#include "platform/storage/storage.h"
#include "util/Timer.h"

// using namespace std;

#include "app/common/App_structs.h"
#include "app/common/ArchiveManager.h"
#include "minecraft/sounds/ConsoleSoundEngine.h"
#include "app/common/BannedListManager.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLCController.h"
#include "app/common/DebugOptions.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/common/GameSettingsManager.h"
#include "app/common/LocalizationManager.h"
#include "app/common/MenuController.h"
#include "app/common/NetworkController.h"
#include "app/common/SaveManager.h"
#include "app/common/SkinManager.h"
#include "app/common/TerrainFeatureManager.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "app/common/UI/All Platforms/ArchiveFile.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/locale/StringTable.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/world/entity/item/MinecartHopper.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "platform/NetTypes.h"
#include "platform/XboxStubs.h"

// JoinFromInviteData moved to NetworkController.h

class Player;
class Inventory;
class Level;
class FurnaceTileEntity;
class Container;
class DispenserTileEntity;
class SignTileEntity;
class BrewingStandTileEntity;
class CommandBlockEntity;
class HopperTileEntity;
// class MinecartHopper;
class EntityHorse;
class BeaconTileEntity;
class LocalPlayer;
class DLCPack;
class LevelRuleset;
class ConsoleSchematicFile;
class Model;
class ModelPart;
class StringTable;
class Merchant;

class CMinecraftAudio;

class Game {
public:
    Game();

    static const float fSafeZoneX;  // 5% of 1280
    static const float fSafeZoneY;  // 5% of 720

    typedef std::vector<PMEMDATA> VMEMFILES;
    typedef std::vector<PNOTIFICATION> VNOTIFICATIONS;

    // storing skin files - delegated to SkinManager
    std::vector<std::string>& vSkinNames = m_skinManager.vSkinNames;
    DLCManager m_dlcManager;
    SaveManager m_saveManager;
    BannedListManager m_bannedListManager;
    TerrainFeatureManager m_terrainFeatureManager;
    DebugOptions m_debugOptions;
    LocalizationManager m_localizationManager;
    ArchiveManager m_archiveManager;
    SkinManager m_skinManager;
    GameSettingsManager m_gameSettingsManager;
    DLCController m_dlcController;
    NetworkController m_networkController;
    MenuController m_menuController;

    // storing credits text from the DLC - delegated to DLCController
    std::vector<std::string>& m_vCreditText = m_dlcController.m_vCreditText;

    // In builds prior to TU5, the size of the GAME_SETTINGS struct was 204
    // bytes. We added a few new values to the internal struct in TU5, and even
    // though we changed the size of the ucUnused array to be decreased by the
    // size of the values we added, the packing of the struct has introduced
    // some extra padding that resulted in the GAME_SETTINGS struct being 208
    // bytes. The knock-on effect from this was that all the stats, which come
    // after the game settings in the profile data, we being read offset by 4
    // bytes. We need to ensure that the GAME_SETTINGS struct does not grow
    // larger than 204 bytes or if we need it to then we need to rebuild the
    // profile data completely and increase the profile version. There should be
    // enough free space to grow larger for a few more updates as long as we
    // take into account the padding issues and check that settings are still
    // stored at the same positions when we read them
    static const int GAME_SETTINGS_PROFILE_DATA_BYTES = 204;

#if defined(_EXTENDED_ACHIEVEMENTS)
    /* 4J-JEV:
     * We need more space in the profile data because of the new achievements
     * and statistics necessary for the new expanded achievement set.
     */
    static const int GAME_DEFINED_PROFILE_DATA_BYTES = 2 * 972;  // per user
#else
    static const int GAME_DEFINED_PROFILE_DATA_BYTES = 972;  // per user
#endif
    unsigned int uiGameDefinedDataChangedBitmask;

    void DebugPrintf(const char* szFormat, ...);
    void DebugPrintfVerbose(bool bVerbose, const char* szFormat,
                            ...);  // Conditional printf
    void DebugPrintf(int user, const char* szFormat, ...);

    static const int USER_NONE = 0;  // disables printf
    static const int USER_GENERAL = 1;
    static const int USER_JV = 2;
    static const int USER_MH = 3;
    static const int USER_PB = 4;
    static const int USER_RR = 5;
    static const int USER_SR = 6;
    static const int USER_UI =
        7;  // 4J Stu - This also makes it appear on the UI console

    void HandleButtonPresses() { m_gameSettingsManager.handleButtonPresses(); }
    bool IntroRunning() { return m_bIntroRunning; }
    void SetIntroRunning(bool bSet) { m_bIntroRunning = bSet; }
#if defined(_CONTENT_PACKAGE)
#if !defined(_FINAL_BUILD)
    bool PartnernetPasswordRunning() { return m_bPartnernetPasswordRunning; }
    void SetPartnernetPasswordRunning(bool bSet) {
        m_bPartnernetPasswordRunning = bSet;
    }
#endif
#endif

    bool IsAppPaused();
    void SetAppPaused(bool val);
    int displaySavingMessage(const IPlatformStorage::ESavingMessage eMsg,
                             int iPad) {
        return m_gameSettingsManager.displaySavingMessage(eMsg, iPad);
    }
    bool GetGameStarted() { return m_bGameStarted; }
    void SetGameStarted(bool bVal) {
        if (bVal)
            DebugPrintf("SetGameStarted - true\n");
        else
            DebugPrintf("SetGameStarted - false\n");
        m_bGameStarted = bVal;
        m_bIsAppPaused = !bVal;
    }
    int GetLocalPlayerCount(void);
    bool LoadInventoryMenu(int iPad, std::shared_ptr<LocalPlayer> player,
                           bool bNavigateBack = false) {
        return m_menuController.loadInventoryMenu(iPad, player, bNavigateBack);
    }
    bool LoadCreativeMenu(int iPad, std::shared_ptr<LocalPlayer> player,
                          bool bNavigateBack = false) {
        return m_menuController.loadCreativeMenu(iPad, player, bNavigateBack);
    }
    bool LoadEnchantingMenu(int iPad, std::shared_ptr<Inventory> inventory,
                            int x, int y, int z, Level* level,
                            const std::string& name) {
        return m_menuController.loadEnchantingMenu(iPad, inventory, x, y, z,
                                                   level, name);
    }
    bool LoadFurnaceMenu(int iPad, std::shared_ptr<Inventory> inventory,
                         std::shared_ptr<FurnaceTileEntity> furnace) {
        return m_menuController.loadFurnaceMenu(iPad, inventory, furnace);
    }
    bool LoadBrewingStandMenu(
        int iPad, std::shared_ptr<Inventory> inventory,
        std::shared_ptr<BrewingStandTileEntity> brewingStand) {
        return m_menuController.loadBrewingStandMenu(iPad, inventory,
                                                     brewingStand);
    }
    bool LoadContainerMenu(int iPad, std::shared_ptr<Container> inventory,
                           std::shared_ptr<Container> container) {
        return m_menuController.loadContainerMenu(iPad, inventory, container);
    }
    bool LoadTrapMenu(int iPad, std::shared_ptr<Container> inventory,
                      std::shared_ptr<DispenserTileEntity> trap) {
        return m_menuController.loadTrapMenu(iPad, inventory, trap);
    }
    bool LoadCrafting2x2Menu(int iPad, std::shared_ptr<LocalPlayer> player) {
        return m_menuController.loadCrafting2x2Menu(iPad, player);
    }
    bool LoadCrafting3x3Menu(int iPad, std::shared_ptr<LocalPlayer> player,
                             int x, int y, int z) {
        return m_menuController.loadCrafting3x3Menu(iPad, player, x, y, z);
    }
    bool LoadFireworksMenu(int iPad, std::shared_ptr<LocalPlayer> player, int x,
                           int y, int z) {
        return m_menuController.loadFireworksMenu(iPad, player, x, y, z);
    }
    bool LoadSignEntryMenu(int iPad, std::shared_ptr<SignTileEntity> sign) {
        return m_menuController.loadSignEntryMenu(iPad, sign);
    }
    bool LoadRepairingMenu(int iPad, std::shared_ptr<Inventory> inventory,
                           Level* level, int x, int y, int z) {
        return m_menuController.loadRepairingMenu(iPad, inventory, level, x, y,
                                                  z);
    }
    bool LoadTradingMenu(int iPad, std::shared_ptr<Inventory> inventory,
                         std::shared_ptr<Merchant> trader, Level* level,
                         const std::string& name) {
        return m_menuController.loadTradingMenu(iPad, inventory, trader, level,
                                                name);
    }

    bool LoadCommandBlockMenu(
        int iPad, std::shared_ptr<CommandBlockEntity> commandBlock) {
        return false;
    }
    bool LoadHopperMenu(int iPad, std::shared_ptr<Inventory> inventory,
                        std::shared_ptr<HopperTileEntity> hopper) {
        return m_menuController.loadHopperMenu(iPad, inventory, hopper);
    }
    bool LoadHopperMenu(int iPad, std::shared_ptr<Inventory> inventory,
                        std::shared_ptr<MinecartHopper> hopper) {
        return m_menuController.loadHopperMenu(iPad, inventory, hopper);
    }
    bool LoadHorseMenu(int iPad, std::shared_ptr<Inventory> inventory,
                       std::shared_ptr<Container> container,
                       std::shared_ptr<EntityHorse> horse) {
        return m_menuController.loadHorseMenu(iPad, inventory, container,
                                              horse);
    }
    bool LoadBeaconMenu(int iPad, std::shared_ptr<Inventory> inventory,
                        std::shared_ptr<BeaconTileEntity> beacon) {
        return m_menuController.loadBeaconMenu(iPad, inventory, beacon);
    }

    bool GetTutorialMode() { return m_bTutorialMode; }
    void SetTutorialMode(bool bSet) { m_bTutorialMode = bSet; }

    void SetSpecialTutorialCompletionFlag(int iPad, int index) {
        m_gameSettingsManager.setSpecialTutorialCompletionFlag(iPad, index);
    }

    static const char* GetString(int iID);
    StringTable* getStringTable() const {
        return m_localizationManager.getStringTable();
    }

    eGameMode GetGameMode() { return m_eGameMode; }
    void SetGameMode(eGameMode eMode) { m_eGameMode = eMode; }

    eXuiAction GetGlobalXuiAction() {
        return m_menuController.getGlobalXuiAction();
    }
    void SetGlobalXuiAction(eXuiAction action) {
        m_menuController.setGlobalXuiAction(action);
    }
    eXuiAction GetXuiAction(int iPad) {
        return m_menuController.getXuiAction(iPad);
    }
    void SetAction(int iPad, eXuiAction action, void* param = nullptr) {
        m_menuController.setAction(iPad, action, param);
    }
    void SetTMSAction(int iPad, eTMSAction action) {
        m_menuController.setTMSAction(iPad, action);
    }
    eTMSAction GetTMSAction(int iPad) {
        return m_menuController.getTMSAction(iPad);
    }
    DisconnectPacket::eDisconnectReason GetDisconnectReason() {
        return m_networkController.getDisconnectReason();
    }
    void SetDisconnectReason(DisconnectPacket::eDisconnectReason bVal) {
        m_networkController.setDisconnectReason(bVal);
    }

    bool GetChangingSessionType() {
        return m_networkController.getChangingSessionType();
    }
    void SetChangingSessionType(bool bVal) {
        m_networkController.setChangingSessionType(bVal);
    }

    bool GetReallyChangingSessionType() {
        return m_networkController.getReallyChangingSessionType();
    }
    void SetReallyChangingSessionType(bool bVal) {
        m_networkController.setReallyChangingSessionType(bVal);
    }

    // 4J Stu - Added so that we can call this when a confirmation box is
    // selected
    static void SetActionConfirmed(void* param) {
        GameSettingsManager::setActionConfirmed(param);
    }
    void HandleXuiActions(void);

    // 4J Stu - Functions used for Minecon and other promo work
    bool GetLoadSavesFromFolderEnabled() {
        return m_debugOptions.getLoadSavesFromFolderEnabled();
    }
    void SetLoadSavesFromFolderEnabled(bool bVal) {
        m_debugOptions.setLoadSavesFromFolderEnabled(bVal);
    }

    // 4J Stu - Useful for debugging
    bool GetWriteSavesToFolderEnabled() {
        return m_debugOptions.getWriteSavesToFolderEnabled();
    }
    void SetWriteSavesToFolderEnabled(bool bVal) {
        m_debugOptions.setWriteSavesToFolderEnabled(bVal);
    }
    bool GetMobsDontAttackEnabled() {
        return m_debugOptions.getMobsDontAttack();
    }
    void SetMobsDontAttackEnabled(bool bVal) {
        m_debugOptions.setMobsDontAttack(bVal);
    }
    bool GetUseDPadForDebug() { return m_debugOptions.getUseDPadForDebug(); }
    void SetUseDPadForDebug(bool bVal) {
        m_debugOptions.setUseDPadForDebug(bVal);
    }
    bool GetMobsDontTickEnabled() { return m_debugOptions.getMobsDontTick(); }
    void SetMobsDontTickEnabled(bool bVal) {
        m_debugOptions.setMobsDontTick(bVal);
    }

    bool GetFreezePlayers() { return m_debugOptions.getFreezePlayers(); }
    void SetFreezePlayers(bool bVal) { m_debugOptions.setFreezePlayers(bVal); }

    // debug -0 show safe area
    void ShowSafeArea(bool show) {}
    // 4J-PB - to capture the social post screenshot
    virtual void CaptureScreenshot(int iPad) {};
    // void			GetPreviewImage(int iPad,XSOCIAL_PREVIEWIMAGE
    // *preview);

    void InitGameSettings() { m_gameSettingsManager.initGameSettings(); }
    static int OldProfileVersionCallback(void* pParam, unsigned char* pucData,
                                         const unsigned short usVersion,
                                         const int iPad) {
        return GameSettingsManager::oldProfileVersionCallback(pParam, pucData,
                                                              usVersion, iPad);
    }

    static int DefaultOptionsCallback(
        void* pParam, IPlatformProfile::PROFILESETTINGS* pSettings,
        const int iPad) {
        return GameSettingsManager::defaultOptionsCallback(pParam, pSettings,
                                                           iPad);
    }
    int SetDefaultOptions(IPlatformProfile::PROFILESETTINGS* pSettings,
                          const int iPad) {
        return m_gameSettingsManager.setDefaultOptions(pSettings, iPad);
    }

    void SetGameSettings(int iPad, eGameSetting eVal, unsigned char ucVal) {
        m_gameSettingsManager.setGameSettings(iPad, eVal, ucVal);
    }
    unsigned char GetGameSettings(int iPad, eGameSetting eVal) {
        return m_gameSettingsManager.getGameSettings(iPad, eVal);
    }
    unsigned char GetGameSettings(eGameSetting eVal) {
        return m_gameSettingsManager.getGameSettings(eVal);
    }
    void SetPlayerSkin(int iPad, const std::string& name) {
        m_skinManager.setPlayerSkin(iPad, name, GameSettingsA);
    }
    void SetPlayerSkin(int iPad, std::uint32_t dwSkinId) {
        m_skinManager.setPlayerSkin(iPad, dwSkinId, GameSettingsA);
    }
    void SetPlayerCape(int iPad, const std::string& name) {
        m_skinManager.setPlayerCape(iPad, name, GameSettingsA);
    }
    void SetPlayerCape(int iPad, std::uint32_t dwCapeId) {
        m_skinManager.setPlayerCape(iPad, dwCapeId, GameSettingsA);
    }
    void SetPlayerFavoriteSkin(int iPad, int iIndex, unsigned int uiSkinID) {
        m_skinManager.setPlayerFavoriteSkin(iPad, iIndex, uiSkinID,
                                            GameSettingsA);
    }
    unsigned int GetPlayerFavoriteSkin(int iPad, int iIndex) {
        return m_skinManager.getPlayerFavoriteSkin(iPad, iIndex, GameSettingsA);
    }
    unsigned char GetPlayerFavoriteSkinsPos(int iPad) {
        return m_skinManager.getPlayerFavoriteSkinsPos(iPad, GameSettingsA);
    }
    void SetPlayerFavoriteSkinsPos(int iPad, int iPos) {
        m_skinManager.setPlayerFavoriteSkinsPos(iPad, iPos, GameSettingsA);
    }
    unsigned int GetPlayerFavoriteSkinsCount(int iPad) {
        return m_skinManager.getPlayerFavoriteSkinsCount(iPad, GameSettingsA);
    }
    void ValidateFavoriteSkins(int iPad) {
        m_skinManager.validateFavoriteSkins(iPad, GameSettingsA, m_dlcManager);
    }

    // Mash-up pack worlds hide/display - delegated to GameSettingsManager
    void HideMashupPackWorld(int iPad, unsigned int iMashupPackID) {
        m_gameSettingsManager.hideMashupPackWorld(iPad, iMashupPackID);
    }
    void EnableMashupPackWorlds(int iPad) {
        m_gameSettingsManager.enableMashupPackWorlds(iPad);
    }
    unsigned int GetMashupPackWorlds(int iPad) {
        return m_gameSettingsManager.getMashupPackWorlds(iPad);
    }

    // Minecraft language select - delegated to GameSettingsManager
    void SetMinecraftLanguage(int iPad, unsigned char ucLanguage) {
        m_gameSettingsManager.setMinecraftLanguage(iPad, ucLanguage);
    }
    unsigned char GetMinecraftLanguage(int iPad) {
        return m_gameSettingsManager.getMinecraftLanguage(iPad);
    }
    void SetMinecraftLocale(int iPad, unsigned char ucLanguage) {
        m_gameSettingsManager.setMinecraftLocale(iPad, ucLanguage);
    }
    unsigned char GetMinecraftLocale(int iPad) {
        return m_gameSettingsManager.getMinecraftLocale(iPad);
    }

    // 4J-PB - set a timer when the user navigates the quickselect, so we can
    // bring the opacity back to defaults for a short time
    unsigned int GetOpacityTimer(int iPad) {
        return m_menuController.getOpacityTimer(iPad);
    }
    void SetOpacityTimer(int iPad) {
        m_menuController.setOpacityTimer(iPad);
    }  // 6 seconds
    void TickOpacityTimer(int iPad) { m_menuController.tickOpacityTimer(iPad); }

public:
    std::string GetPlayerSkinName(int iPad) {
        return m_skinManager.getPlayerSkinName(iPad, GameSettingsA);
    }
    std::uint32_t GetPlayerSkinId(int iPad) {
        return m_skinManager.getPlayerSkinId(iPad, GameSettingsA, m_dlcManager);
    }
    std::string GetPlayerCapeName(int iPad) {
        return m_skinManager.getPlayerCapeName(iPad, GameSettingsA);
    }
    std::uint32_t GetPlayerCapeId(int iPad) {
        return m_skinManager.getPlayerCapeId(iPad, GameSettingsA);
    }
    std::uint32_t GetAdditionalModelParts(int iPad) {
        return m_skinManager.getAdditionalModelParts(iPad);
    }
    void CheckGameSettingsChanged(bool bOverride5MinuteTimer = false,
                                  int iPad = XUSER_INDEX_ANY) {
        m_gameSettingsManager.checkGameSettingsChanged(bOverride5MinuteTimer,
                                                       iPad);
    }
    void ApplyGameSettingsChanged(int iPad) {
        m_gameSettingsManager.applyGameSettingsChanged(iPad);
    }
    void ClearGameSettingsChangedFlag(int iPad) {
        m_gameSettingsManager.clearGameSettingsChangedFlag(iPad);
    }
    void ActionGameSettings(int iPad, eGameSetting eVal) {
        m_gameSettingsManager.actionGameSettings(iPad, eVal);
    }
    unsigned int GetGameSettingsDebugMask(int iPad = -1,
                                          bool bOverridePlayer = false) {
        return m_gameSettingsManager.getGameSettingsDebugMask(iPad,
                                                              bOverridePlayer);
    }
    void SetGameSettingsDebugMask(int iPad, unsigned int uiVal) {
        m_gameSettingsManager.setGameSettingsDebugMask(iPad, uiVal);
    }
    void ActionDebugMask(int iPad, bool bSetAllClear = false) {
        m_gameSettingsManager.actionDebugMask(iPad, bSetAllClear);
    }

    //
    bool IsLocalMultiplayerAvailable();

    // for sign in change monitoring - delegated to NetworkController
    static void SignInChangeCallback(void* pParam, bool bVal,
                                     unsigned int uiSignInData) {
        NetworkController::signInChangeCallback(pParam, bVal, uiSignInData);
    }
    static void ClearSignInChangeUsersMask() {
        NetworkController::clearSignInChangeUsersMask();
    }
    static int SignoutExitWorldThreadProc(void* lpParameter) {
        return NetworkController::signoutExitWorldThreadProc(lpParameter);
    }
    static int PrimaryPlayerSignedOutReturned(
        void* pParam, int iPad, const IPlatformStorage::EMessageResult result) {
        return NetworkController::primaryPlayerSignedOutReturned(pParam, iPad,
                                                                 result);
    }
    static int EthernetDisconnectReturned(
        void* pParam, int iPad, const IPlatformStorage::EMessageResult result) {
        return NetworkController::ethernetDisconnectReturned(pParam, iPad,
                                                             result);
    }
    static void ProfileReadErrorCallback(void* pParam) {
        NetworkController::profileReadErrorCallback(pParam);
    }

    // FATAL LOAD ERRORS
    virtual void FatalLoadError();

    // Notifications from the game listener to be passed to the qnet listener
    static void NotificationsCallback(void* pParam,
                                      std::uint32_t dwNotification,
                                      unsigned int uiParam) {
        NetworkController::notificationsCallback(pParam, dwNotification,
                                                 uiParam);
    }

    // for the ethernet being disconnected
    static void LiveLinkChangeCallback(void* pParam, bool bConnected) {
        NetworkController::liveLinkChangeCallback(pParam, bConnected);
    }
    bool GetLiveLinkRequired() {
        return m_networkController.getLiveLinkRequired();
    }
    void SetLiveLinkRequired(bool required) {
        m_networkController.setLiveLinkRequired(required);
    }

#if defined(_DEBUG_MENUS_ENABLED)
    bool DebugSettingsOn() { return m_debugOptions.settingsOn(); }
    bool DebugArtToolsOn();
#else
    bool DebugSettingsOn() { return false; }
    bool DebugArtToolsOn() { return false; }
#endif
    void SetDebugSequence(const char* pchSeq);
    // bool			UploadFileToGlobalStorage(int iQuadrant,
    // IPlatformStorage::eGlobalStorage eStorageFacility, std::string *wsFile );

    // Installed DLC - delegated to DLCController
    bool StartInstallDLCProcess(int iPad) {
        return m_dlcController.startInstallDLCProcess(iPad);
    }
    int dlcInstalledCallback(int iOfferC, int iPad) {
        return m_dlcController.dlcInstalledCallback(iOfferC, iPad);
    }
    void HandleDLCLicenseChange();
    int dlcMountedCallback(int iPad, std::uint32_t dwErr,
                           std::uint32_t dwLicenceMask) {
        return m_dlcController.dlcMountedCallback(iPad, dwErr, dwLicenceMask);
    }
    void MountNextDLC(int iPad) { m_dlcController.mountNextDLC(iPad); }
    void HandleDLC(DLCPack* pack) { m_dlcController.handleDLC(pack); }
    bool DLCInstallPending() { return m_dlcController.dlcInstallPending(); }
    bool DLCInstallProcessCompleted() {
        return m_dlcController.dlcInstallProcessCompleted();
    }
    void ClearDLCInstalled() { m_dlcController.clearDLCInstalled(); }
    static int MarketplaceCountsCallback(
        void* pParam, IPlatformStorage::DLC_TMS_DETAILS* details, int iPad) {
        return DLCController::marketplaceCountsCallback(pParam, details, iPad);
    }

    bool AlreadySeenCreditText(const std::string& wstemp) {
        return m_dlcController.alreadySeenCreditText(wstemp);
    }

    void ClearNewDLCAvailable(void) { m_dlcController.clearNewDLCAvailable(); }
    bool GetNewDLCAvailable() { return m_dlcController.getNewDLCAvailable(); }
    void DisplayNewDLCTipAgain() { m_dlcController.displayNewDLCTipAgain(); }
    bool DisplayNewDLCTip() { return m_dlcController.displayNewDLCTip(); }

    // functions to store launch data, and to exit the game - required due to
    // possibly being on a demo disc
    virtual void StoreLaunchData();
    virtual void ExitGame();

    bool isXuidNotch(PlayerUID xuid) { return m_skinManager.isXuidNotch(xuid); }
    bool isXuidDeadmau5(PlayerUID xuid);

    void AddMemoryTextureFile(const std::string& wName, std::uint8_t* pbData,
                              unsigned int byteCount) {
        m_skinManager.addMemoryTextureFile(wName, pbData, byteCount);
    }
    void RemoveMemoryTextureFile(const std::string& wName) {
        m_skinManager.removeMemoryTextureFile(wName);
    }
    void GetMemFileDetails(const std::string& wName, std::uint8_t** ppbData,
                           unsigned int* pByteCount) {
        m_skinManager.getMemFileDetails(wName, ppbData, pByteCount);
    }
    bool IsFileInMemoryTextures(const std::string& wName) {
        return m_skinManager.isFileInMemoryTextures(wName);
    }

    // Texture Pack Data files (icon, banner, comparison shot & text)
    void AddMemoryTPDFile(int iConfig, std::uint8_t* pbData,
                          unsigned int byteCount) {
        m_archiveManager.addMemoryTPDFile(iConfig, pbData, byteCount);
    }
    void RemoveMemoryTPDFile(int iConfig) {
        m_archiveManager.removeMemoryTPDFile(iConfig);
    }
    bool IsFileInTPD(int iConfig) {
        return m_archiveManager.isFileInTPD(iConfig);
    }
    void GetTPD(int iConfig, std::uint8_t** ppbData, unsigned int* pByteCount) {
        m_archiveManager.getTPD(iConfig, ppbData, pByteCount);
    }
    int GetTPDSize() { return m_archiveManager.getTPDSize(); }
    int GetTPConfigVal(char* pwchDataFile) {
        return m_archiveManager.getTPConfigVal(pwchDataFile);
    }

    bool DefaultCapeExists() { return m_skinManager.defaultCapeExists(); }
    // void InstallDefaultCape(); // attempt  to install the default cape once
    // per game launch

    // invites - delegated to NetworkController
    void ProcessInvite(std::uint32_t dwUserIndex,
                       std::uint32_t dwLocalUsersMask,
                       const INVITE_INFO* pInviteInfo) {
        m_networkController.processInvite(dwUserIndex, dwLocalUsersMask,
                                          pInviteInfo);
    }

    // Add credits for DLC installed - delegated to DLCController
    void AddCreditText(const char* lpStr) {
        m_dlcController.addCreditText(lpStr);
    }

private:
    std::unordered_map<PlayerUID, std::uint8_t*> m_GTS_Files;

public:
    // launch data
    std::uint8_t* m_pLaunchData;
    unsigned int m_dwLaunchDataSize;

public:
    // BAN LIST
    void AddLevelToBannedLevelList(int iPad, PlayerUID xuid, char* pszLevelName,
                                   bool bWriteToTMS) {
        m_bannedListManager.addLevel(iPad, xuid, pszLevelName, bWriteToTMS);
    }
    bool IsInBannedLevelList(int iPad, PlayerUID xuid, char* pszLevelName) {
        return m_bannedListManager.isInList(iPad, xuid, pszLevelName);
    }
    void RemoveLevelFromBannedLevelList(int iPad, PlayerUID xuid,
                                        char* pszLevelName) {
        m_bannedListManager.removeLevel(iPad, xuid, pszLevelName);
    }
    void InvalidateBannedList(int iPad) {
        m_bannedListManager.invalidate(iPad);
    }
    void SetUniqueMapName(char* pszUniqueMapName) {
        m_bannedListManager.setUniqueMapName(pszUniqueMapName);
    }
    char* GetUniqueMapName(void) {
        return m_bannedListManager.getUniqueMapName();
    }

public:
    bool GetResourcesLoaded() { return m_bResourcesLoaded; }
    void SetResourcesLoaded(bool bVal) { m_bResourcesLoaded = bVal; }

public:
    bool m_bGameStarted;
    bool m_bIntroRunning;
    bool m_bTutorialMode;
    bool m_bIsAppPaused;

    // m_bChangingSessionType and m_bReallyChangingSessionType moved to
    // NetworkController

    // trial, and trying to unlock full
    // version on an upsell

    void loadMediaArchive() { m_archiveManager.loadMediaArchive(); }
    void loadStringTable() {
        m_localizationManager.loadStringTable(
            m_archiveManager.getMediaArchive());
    }

public:
    int getArchiveFileSize(const std::string& filename) {
        return m_archiveManager.getArchiveFileSize(filename);
    }
    bool hasArchiveFile(const std::string& filename) {
        return m_archiveManager.hasArchiveFile(filename);
    }
    std::vector<uint8_t> getArchiveFile(const std::string& filename) {
        return m_archiveManager.getArchiveFile(filename);
    }

private:
    static int BannedLevelDialogReturned(
        void* pParam, int iPad, const IPlatformStorage::EMessageResult);
    static int TexturePackDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return MenuController::texturePackDialogReturned(pParam, iPad, result);
    }

    bool m_bResourcesLoaded;

    // Global string table for this application.
    // CXuiStringTable StringTable;

    // Container scene for some menu

    //	CXuiScene debugContainerScene;

    // bool m_bSplitScreenEnabled;

#if defined(_CONTENT_PACKAGE)
#if !defined(_FINAL_BUILD)
    bool m_bPartnernetPasswordRunning;
#endif
#endif

    eGameMode m_eGameMode;  // single or multiplayer

    // GameSettingsA reference alias into GameSettingsManager
    GAME_SETTINGS* (&GameSettingsA)[XUSER_MAX_COUNT] =
        m_gameSettingsManager.GameSettingsA;

    // m_uiLastSignInData moved to NetworkController

    // Debug options now in m_debugOptions

public:
    virtual void RunFrame() {};

    static constexpr unsigned int m_dwOfferID = 0x00000001;

    // timer
    void InitTime();
    void UpdateTime();

    // trial timer
    void SetTrialTimerStart(void);
    float getTrialTimer(void);

    // notifications from the game for qnet - delegated to NetworkController
    NetworkController::VNOTIFICATIONS* GetNotifications() {
        return m_networkController.getNotifications();
    }

private:
    static int UnlockFullExitReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result) {
        return MenuController::unlockFullExitReturned(pParam, iPad, result);
    }
    static int UnlockFullSaveReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result) {
        return MenuController::unlockFullSaveReturned(pParam, iPad, result);
    }
    static int UnlockFullInviteReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return MenuController::unlockFullInviteReturned(pParam, iPad, result);
    }
    static int TrialOverReturned(void* pParam, int iPad,
                                 IPlatformStorage::EMessageResult result) {
        return MenuController::trialOverReturned(pParam, iPad, result);
    }
    static int ExitAndJoinFromInvite(void* pParam, int iPad,
                                     IPlatformStorage::EMessageResult result) {
        return NetworkController::exitAndJoinFromInvite(pParam, iPad, result);
    }
    static int ExitAndJoinFromInviteSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return NetworkController::exitAndJoinFromInviteSaveDialogReturned(
            pParam, iPad, result);
    }
    static int ExitAndJoinFromInviteAndSaveReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return NetworkController::exitAndJoinFromInviteAndSaveReturned(
            pParam, iPad, result);
    }
    static int ExitAndJoinFromInviteDeclineSaveReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return NetworkController::exitAndJoinFromInviteDeclineSaveReturned(
            pParam, iPad, result);
    }
    static int FatalErrorDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int WarningTrialTexturePackReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return NetworkController::warningTrialTexturePackReturned(pParam, iPad,
                                                                  result);
    }

    JoinFromInviteData& m_InviteData = m_networkController.m_InviteData;
    // m_bDebugOptions moved to m_debugOptions

    // Trial timer
    float m_fTrialTimerStart, mfTrialPausedTime;
    typedef struct TimeInfo {
        time_util::time_point qwTime;
        time_util::clock::duration qwAppTime{};

        float fAppTime;
        float fElapsedTime;
    } TIMEINFO;

    TimeInfo m_Time;

public:
    void InitialiseTips() { m_localizationManager.initialiseTips(); }
    int GetNextTip() { return m_localizationManager.getNextTip(); }
    int GetHTMLColour(eMinecraftColour colour) {
        return m_localizationManager.getHTMLColour(colour);
    }
    int GetHTMLColor(eMinecraftColour colour) { return GetHTMLColour(colour); }
    int GetHTMLFontSize(EHTMLFontSize size) {
        return m_localizationManager.getHTMLFontSize(size);
    }
    std::string FormatHTMLString(int iPad, const std::string& desc,
                                 int shadowColour = 0xFFFFFFFF) {
        return m_localizationManager.formatHTMLString(iPad, desc, shadowColour);
    }
    std::string GetActionReplacement(int iPad, unsigned char ucAction) {
        return m_localizationManager.getActionReplacement(iPad, ucAction);
    }
    std::string GetVKReplacement(unsigned int uiVKey) {
        return m_localizationManager.getVKReplacement(uiVKey);
    }
    std::string GetIconReplacement(unsigned int uiIcon) {
        return m_localizationManager.getIconReplacement(uiIcon);
    }

    float getAppTime() { return m_Time.fAppTime; }
    void UpdateTrialPausedTimer() { mfTrialPausedTime += m_Time.fElapsedTime; }

    static int RemoteSaveThreadProc(void* lpParameter) {
        return MenuController::remoteSaveThreadProc(lpParameter);
    }
    static void ExitGameFromRemoteSave(void* lpParameter) {
        MenuController::exitGameFromRemoteSave(lpParameter);
    }
    static int ExitGameFromRemoteSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
        return MenuController::exitGameFromRemoteSaveDialogReturned(
            pParam, iPad, result);
    }

    // XML
public:
    // Hold a vector of terrain feature positions
    void AddTerrainFeaturePosition(_eTerrainFeatureType eType, int x, int z) {
        m_terrainFeatureManager.add(eType, x, z);
    }
    void ClearTerrainFeaturePosition() { m_terrainFeatureManager.clear(); }
    _eTerrainFeatureType IsTerrainFeature(int x, int z) {
        return m_terrainFeatureManager.isFeature(x, z);
    }
    bool GetTerrainFeaturePosition(_eTerrainFeatureType eType, int* pX,
                                   int* pZ) {
        return m_terrainFeatureManager.getPosition(eType, pX, pZ);
    }

    static int32_t RegisterMojangData(char*, PlayerUID, char*, char*);
    MOJANG_DATA* GetMojangDataForXuid(PlayerUID xuid);
    static int32_t RegisterConfigValues(char* pType, int iValue);

    static int32_t RegisterDLCData(char* a, char* b, int c, uint64_t d,
                                   uint64_t e, char* f, unsigned int g, int h,
                                   char* pDataFile) {
        return DLCController::registerDLCData(a, b, c, d, e, f, g, h,
                                              pDataFile);
    }
    bool GetDLCFullOfferIDForSkinID(const std::string& FirstSkin,
                                    uint64_t* pullVal) {
        return m_dlcController.getDLCFullOfferIDForSkinID(FirstSkin, pullVal);
    }
    DLC_INFO* GetDLCInfoForTrialOfferID(uint64_t ullOfferID_Trial) {
        return m_dlcController.getDLCInfoForTrialOfferID(ullOfferID_Trial);
    }
    DLC_INFO* GetDLCInfoForFullOfferID(uint64_t ullOfferID_Full) {
        return m_dlcController.getDLCInfoForFullOfferID(ullOfferID_Full);
    }

    unsigned int GetDLCCreditsCount() {
        return m_dlcController.getDLCCreditsCount();
    }
    SCreditTextItemDef* GetDLCCredits(int iIndex) {
        return m_dlcController.getDLCCredits(iIndex);
    }

    // TMS
    void ReadDLCFileFromTMS(int iPad, eTMSAction action,
                            bool bCallback = false);
    void ReadXuidsFileFromTMS(int iPad, eTMSAction action,
                              bool bCallback = false);

    // DLC data members moved to DLCController
    // Sign-in info moved to NetworkController

public:
    // void OverrideFontRenderer(bool set, bool immediate = true);
    //	void ToggleFontRenderer() {
    // OverrideFontRenderer(!m_bFontRendererOverridden,false); }
    BANNEDLIST(&BannedListA)
    [XUSER_MAX_COUNT] = m_bannedListManager.BannedListA;

public:
    void SetBanListCheck(int iPad, bool bVal) {
        m_bannedListManager.setBanListCheck(iPad, bVal);
    }
    bool GetBanListCheck(int iPad) {
        return m_bannedListManager.getBanListCheck(iPad);
    }
    // AUTOSAVE
public:
    void SetAutosaveTimerTime(void);
    bool AutosaveDue(void) { return m_saveManager.autosaveDue(); }
    int64_t SecondsToAutosave() { return m_saveManager.secondsToAutosave(); }

    // m_uiOpacityCountDown moved to MenuController
    // DLC flags moved to DLCController
    // Host options - m_uiGameHostSettings moved to GameSettingsManager
    unsigned int& m_uiGameHostSettings =
        m_gameSettingsManager.m_uiGameHostSettings;

#if defined(_LARGE_WORLDS)
    unsigned int m_GameNewWorldSize;
    bool m_bGameNewWorldSizeUseMoat;
    unsigned int m_GameNewHellScale;
#endif

public:
    void SetGameHostOption(eGameHostOption eVal, unsigned int uiVal);
    void SetGameHostOption(unsigned int& uiHostSettings, eGameHostOption eVal,
                           unsigned int uiVal) {
        m_gameSettingsManager.setGameHostOption(uiHostSettings, eVal, uiVal);
    }
    unsigned int GetGameHostOption(eGameHostOption eVal);
    unsigned int GetGameHostOption(unsigned int uiHostSettings,
                                   eGameHostOption eVal) {
        return m_gameSettingsManager.getGameHostOption(uiHostSettings, eVal);
    }

#if defined(_LARGE_WORLDS)
    void SetGameNewWorldSize(unsigned int newSize, bool useMoat) {
        m_GameNewWorldSize = newSize;
        m_bGameNewWorldSizeUseMoat = useMoat;
    }
    unsigned int GetGameNewWorldSize() { return m_GameNewWorldSize; }
    unsigned int GetGameNewWorldSizeUseMoat() {
        return m_bGameNewWorldSizeUseMoat;
    }
    void SetGameNewHellScale(unsigned int newScale) {
        m_GameNewHellScale = newScale;
    }
    unsigned int GetGameNewHellScale() { return m_GameNewHellScale; }
#endif
    void SetResetNether(bool bResetNether) { m_bResetNether = bResetNether; }
    bool GetResetNether() { return m_bResetNether; }
    bool CanRecordStatsAndAchievements() {
        return m_gameSettingsManager.canRecordStatsAndAchievements();
    }

    // World seed from png image - delegated to MenuController
    void GetImageTextData(std::uint8_t* imageData, unsigned int imageBytes,
                          unsigned char* seedText, unsigned int& uiHostOptions,
                          bool& bHostOptionsRead,
                          std::uint32_t& uiTexturePack) {
        m_menuController.getImageTextData(imageData, imageBytes, seedText,
                                          uiHostOptions, bHostOptionsRead,
                                          uiTexturePack);
    }
    unsigned int CreateImageTextData(std::uint8_t* textMetadata, int64_t seed,
                                     bool hasSeed, unsigned int uiHostOptions,
                                     unsigned int uiTexturePackId) {
        return m_menuController.createImageTextData(
            textMetadata, seed, hasSeed, uiHostOptions, uiTexturePackId);
    }

    // Game rules
    GameRuleManager m_gameRules;

public:
    void processSchematics(LevelChunk* levelChunk);
    void processSchematicsLighting(LevelChunk* levelChunk);
    void loadDefaultGameRules();
    std::vector<LevelGenerationOptions*>* getLevelGenerators() {
        return m_gameRules.getLevelGenerators();
    }
    void setLevelGenerationOptions(LevelGenerationOptions* levelGen);
    LevelRuleset* getGameRuleDefinitions() {
        return m_gameRules.getGameRuleDefinitions();
    }
    LevelGenerationOptions* getLevelGenerationOptions() {
        return m_gameRules.getLevelGenerationOptions();
    }
    const char* GetGameRulesString(const std::string& key);

    // m_playerColours and m_playerGamePrivileges moved to NetworkController

public:
    void UpdatePlayerInfo(std::uint8_t networkSmallId,
                          int16_t playerColourIndex,
                          unsigned int playerGamePrivileges) {
        m_networkController.updatePlayerInfo(networkSmallId, playerColourIndex,
                                             playerGamePrivileges);
    }
    short GetPlayerColour(std::uint8_t networkSmallId) {
        return m_networkController.getPlayerColour(networkSmallId);
    }
    unsigned int GetPlayerPrivileges(std::uint8_t networkSmallId) {
        return m_networkController.getPlayerPrivileges(networkSmallId);
    }

    std::string getEntityName(eINSTANCEOF type);

    unsigned int AddDLCRequest(eDLCMarketplaceType eContentType,
                               bool bPromote = false) {
        return m_dlcController.addDLCRequest(eContentType, bPromote);
    }
    bool RetrieveNextDLCContent() {
        return m_dlcController.retrieveNextDLCContent();
    }
    bool CheckTMSDLCCanStop() { return m_dlcController.checkTMSDLCCanStop(); }
    int dlcOffersReturned(int iOfferC, std::uint32_t dwType, int iPad) {
        return m_dlcController.dlcOffersReturned(iOfferC, dwType, iPad);
    }
    std::uint32_t GetDLCContentType(eDLCContentType eType) {
        return m_dlcController.getDLCContentType(eType);
    }
    eDLCContentType Find_eDLCContentType(std::uint32_t dwType) {
        return m_dlcController.find_eDLCContentType(dwType);
    }
    int GetDLCOffersCount() { return m_dlcController.getDLCOffersCount(); }
    bool DLCContentRetrieved(eDLCMarketplaceType eType) {
        return m_dlcController.dlcContentRetrieved(eType);
    }
    void TickDLCOffersRetrieved() { m_dlcController.tickDLCOffersRetrieved(); }
    void ClearAndResetDLCDownloadQueue() {
        m_dlcController.clearAndResetDLCDownloadQueue();
    }
    bool RetrieveNextTMSPPContent() {
        return m_dlcController.retrieveNextTMSPPContent();
    }
    void TickTMSPPFilesRetrieved() {
        m_dlcController.tickTMSPPFilesRetrieved();
    }
    void ClearTMSPPFilesRetrieved() {
        m_dlcController.clearTMSPPFilesRetrieved();
    }
    unsigned int AddTMSPPFileTypeRequest(eDLCContentType eType,
                                         bool bPromote = false) {
        return m_dlcController.addTMSPPFileTypeRequest(eType, bPromote);
    }
    int GetDLCInfoTexturesOffersCount() {
        return m_dlcController.getDLCInfoTexturesOffersCount();
    }

    static int TMSPPFileReturned(void* pParam, int iPad, int iUserData,
                                 IPlatformStorage::PTMSPP_FILEDATA pFileData,
                                 const char* szFilename) {
        return DLCController::tmsPPFileReturned(pParam, iPad, iUserData,
                                                pFileData, szFilename);
    }
    DLC_INFO* GetDLCInfoTrialOffer(int iIndex) {
        return m_dlcController.getDLCInfoTrialOffer(iIndex);
    }
    DLC_INFO* GetDLCInfoFullOffer(int iIndex) {
        return m_dlcController.getDLCInfoFullOffer(iIndex);
    }

    int GetDLCInfoTrialOffersCount() {
        return m_dlcController.getDLCInfoTrialOffersCount();
    }
    int GetDLCInfoFullOffersCount() {
        return m_dlcController.getDLCInfoFullOffersCount();
    }
    bool GetDLCFullOfferIDForPackID(const int iPackID, uint64_t* pullVal) {
        return m_dlcController.getDLCFullOfferIDForPackID(iPackID, pullVal);
    }
    uint64_t GetDLCInfoTexturesFullOffer(int iIndex) {
        return m_dlcController.getDLCInfoTexturesFullOffer(iIndex);
    }

    void SetCorruptSaveDeleted(bool bVal) { m_bCorruptSaveDeleted = bVal; }
    bool GetCorruptSaveDeleted(void) { return m_bCorruptSaveDeleted; }

    void lockSaveNotification() { m_saveManager.lock(); }
    void unlockSaveNotification() { m_saveManager.unlock(); }

    // Download status members moved to DLCController
    bool m_bCorruptSaveDeleted;

    std::uint8_t*& m_pBannedListFileBuffer =
        m_bannedListManager.m_pBannedListFileBuffer;
    unsigned int& m_dwBannedListFileSize =
        m_bannedListManager.m_dwBannedListFileSize;

public:
    unsigned int& m_dwDLCFileSize = m_dlcController.m_dwDLCFileSize;
    std::uint8_t*& m_pDLCFileBuffer = m_dlcController.m_pDLCFileBuffer;

    // 	static int CallbackReadXuidsFileFromTMS(void* lpParam, char
    // *wchFilename, int iPad, bool bResult, int iAction); 	static int
    // CallbackDLCFileFromTMS(void* lpParam, char *wchFilename, int iPad,
    // bool bResult, int iAction); 	static int
    // CallbackBannedListFileFromTMS(void* lpParam, char *wchFilename, int
    // iPad, bool bResult, int iAction);

    // Storing additional model parts per skin texture
    void SetAdditionalSkinBoxes(std::uint32_t dwSkinID, SKIN_BOX* SkinBoxA,
                                unsigned int dwSkinBoxC) {
        m_skinManager.setAdditionalSkinBoxes(dwSkinID, SkinBoxA, dwSkinBoxC);
    }
    std::vector<ModelPart*>* SetAdditionalSkinBoxes(
        std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA) {
        return m_skinManager.setAdditionalSkinBoxes(dwSkinID, pvSkinBoxA);
    }
    std::vector<ModelPart*>* GetAdditionalModelParts(std::uint32_t dwSkinID) {
        return m_skinManager.getAdditionalModelParts(dwSkinID);
    }
    std::vector<SKIN_BOX*>* GetAdditionalSkinBoxes(std::uint32_t dwSkinID) {
        return m_skinManager.getAdditionalSkinBoxes(dwSkinID);
    }
    void SetAnimOverrideBitmask(std::uint32_t dwSkinID,
                                unsigned int uiAnimOverrideBitmask) {
        m_skinManager.setAnimOverrideBitmask(dwSkinID, uiAnimOverrideBitmask);
    }
    unsigned int GetAnimOverrideBitmask(std::uint32_t dwSkinID) {
        return m_skinManager.getAnimOverrideBitmask(dwSkinID);
    }

    static std::uint32_t getSkinIdFromPath(const std::string& skin) {
        return SkinManager::getSkinIdFromPath(skin);
    }
    static std::string getSkinPathFromId(std::uint32_t skinId) {
        return SkinManager::getSkinPathFromId(skinId);
    }

    virtual bool GetTMSGlobalFileListRead() { return true; }
    virtual bool GetTMSDLCInfoRead() { return true; }
    virtual bool GetTMSXUIDsFileRead() { return true; }

    bool GetBanListRead(int iPad) {
        return m_bannedListManager.getBanListRead(iPad);
    }
    void SetBanListRead(int iPad, bool bVal) {
        m_bannedListManager.setBanListRead(iPad, bVal);
    }
    void ClearBanList(int iPad) { m_bannedListManager.clearBanList(iPad); }

    std::uint32_t GetRequiredTexturePackID() {
        return m_archiveManager.getRequiredTexturePackID();
    }
    void SetRequiredTexturePackID(std::uint32_t texturePackId) {
        m_archiveManager.setRequiredTexturePackID(texturePackId);
    }

    virtual void GetFileFromTPD(eTPDFileType eType, std::uint8_t* pbData,
                                unsigned int byteCount, std::uint8_t** ppbData,
                                unsigned int* pByteCount) {
        m_archiveManager.getFileFromTPD(eType, pbData, byteCount, ppbData,
                                        pByteCount);
    }

    // XTITLE_DEPLOYMENT_TYPE getDeploymentType() { return
    // m_titleDeploymentType; }

private:
    bool m_bResetNether;

    // 4J-PB - language and locale functions
public:
    void LocaleAndLanguageInit() {
        m_localizationManager.localeAndLanguageInit();
    }
    void getLocale(std::vector<std::string>& vecWstrLocales) {
        m_localizationManager.getLocale(vecWstrLocales);
    }
    [[nodiscard]] std::vector<std::string> getLocale() {
        std::vector<std::string> v;
        m_localizationManager.getLocale(v);
        return v;
    }
    int get_eMCLang(char* pwchLocale) {
        return m_localizationManager.get_eMCLang(pwchLocale);
    }
    int get_xcLang(char* pwchLocale) {
        return m_localizationManager.get_xcLang(pwchLocale);
    }

    void SetTickTMSDLCFiles(bool bVal) {
        m_dlcController.setTickTMSDLCFiles(bVal);
    }

    std::string getFilePath(std::uint32_t packId, std::string filename,
                            bool bAddDataFolder,
                            std::string mountPoint = "TPACK:");

private:
    std::string getRootPath(std::uint32_t packId, bool allowOverride,
                            bool bAddDataFolder, std::string mountPoint);

public:
#if defined(_WINDOWS64)
    // CMinecraftAudio audio;
#else

#endif
};

// singleton
// extern CMinecraftApp app;