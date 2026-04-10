#include "app/common/Game.h"

#include "app/common/App_structs.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCSkinFile.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Scenes/UIScene_FullscreenProgress.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "java/Class.h"
#include "java/File.h"
#include "java/Random.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameHostOptions.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "platform/network/network.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/stats/StatsCounter.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/item/MinecartHopper.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/crafting/Recipy.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/HopperTileEntity.h"
#include "platform/network/NetTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/XboxStubs.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "platform/storage/storage.h"
#include "strings.h"
#if defined(_WINDOWS64)
#include "app/windows/XML/ATGXmlParser.h"
#include "app/windows/XML/xmlFilesCallback.h"
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <chrono>
#include <compare>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/common/Audio/SoundEngine.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/UI/All Platforms/ArchiveFile.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_PauseMenu.h"
#include "minecraft/Minecraft_Macros.h"
#include "minecraft/client/User.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/locale/StringTable.h"
#include "minecraft/server/PlayerList.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "platform/input/input.h"
#include "util/StringHelpers.h"
#include "util/Timer.h"

class BeaconTileEntity;
class BrewingStandTileEntity;
class DispenserTileEntity;
class EntityHorse;
class FurnaceTileEntity;
class INVITE_INFO;
class Inventory;
class Level;
class LevelChunk;
class LevelGenerationOptions;
class LocalPlayer;
class Merchant;
class ModelPart;
class SignTileEntity;

// Game app;

const float Game::fSafeZoneX = 64.0f;  // 5% of 1280
const float Game::fSafeZoneY = 36.0f;  // 5% of 720

Game::Game() {
    if (GAME_SETTINGS_PROFILE_DATA_BYTES != sizeof(GAME_SETTINGS)) {
        DebugPrintf(
            "WARNING: The size of the profile GAME_SETTINGS struct has "
            "changed, so all stat data is likely incorrect. Is: %d, Should be: "
            "%d\n",
            sizeof(GAME_SETTINGS), GAME_SETTINGS_PROFILE_DATA_BYTES);
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
    }

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        DebugPrintf("Player at index %d has guest number %d\n", i,
                    m_networkController.m_currentSigninInfo[i].dwGuestNumber);
    }

    m_bResourcesLoaded = false;
    m_bGameStarted = false;
    m_bIsAppPaused = false;

    m_bIntroRunning = false;
    m_eGameMode = eMode_Singleplayer;
    m_bTutorialMode = false;

    mfTrialPausedTime = 0.0f;

#if defined(_LARGE_WORLDS)
    m_GameNewWorldSize = 0;
    m_bGameNewWorldSizeUseMoat = false;
    m_GameNewHellScale = 0;
#endif

    m_bResetNether = false;

    LocaleAndLanguageInit();
}

void Game::DebugPrintf(const char* szFormat, ...) {
#if !defined(_FINAL_BUILD)
    char buf[1024];
    va_list ap;
    va_start(ap, szFormat);
    vsnprintf(buf, sizeof(buf), szFormat, ap);
    va_end(ap);
    fputs(buf, stderr);
#endif
}

void Game::DebugPrintf(int user, const char* szFormat, ...) {
#if !defined(_FINAL_BUILD)
    if (user == USER_NONE) return;
    char buf[1024];
    va_list ap;
    va_start(ap, szFormat);
    vsnprintf(buf, sizeof(buf), szFormat, ap);
    va_end(ap);
    fputs(buf, stderr);
    if (user == USER_UI) {
        ui.logDebugString(buf);
    }
#endif
}

const char* Game::GetString(int iID) {
    // return "Değişiklikler ve Yenilikler";
    // return "ÕÕÕÕÖÖÖÖ";
    return app.m_localizationManager.getString(iID);
}

// SetAction moved to MenuController
// HandleButtonPresses moved to GameSettingsManager

bool Game::IsAppPaused() { return m_bIsAppPaused; }

void Game::SetAppPaused(bool val) { m_bIsAppPaused = val; }

// Load*Menu methods moved to MenuController

//////////////////////////////////////////////
// GAME SETTINGS
//////////////////////////////////////////////

// Skin/Cape/FavoriteSkin methods moved to SkinManager

// Mash-up pack worlds

///////////////////////////
//
// Remove the debug settings in the content package build
//
////////////////////////////
#if !defined(_DEBUG_MENUS_ENABLED)

#else

#endif

int Game::BannedLevelDialogReturned(
    void* pParam, int iPad, const IPlatformStorage::EMessageResult result) {
    Game* pApp = (Game*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
    } else {
        if (iPad == PlatformProfile.GetPrimaryPad()) {
            pApp->SetAction(iPad, eAppAction_ExitWorld);
        } else {
            pApp->SetAction(iPad, eAppAction_ExitPlayer);
        }
    }

    return 0;
}

#if defined(_DEBUG_MENUS_ENABLED)
bool Game::DebugArtToolsOn() {
    return m_debugOptions.debugArtToolsOn(
        GetGameSettingsDebugMask(PlatformProfile.GetPrimaryPad()));
}
#endif

void Game::SetDebugSequence(const char* pchSeq) {
    PlatformInput.SetDebugSequence(pchSeq, [this]() -> int {
        // printf("sequence matched\n");
        m_debugOptions.setDebugOptions(!m_debugOptions.settingsOn());

        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (app.DebugSettingsOn()) {
                app.ActionDebugMask(i);
            } else {
                // force debug mask off
                app.ActionDebugMask(i, true);
            }
        }

        return 0;
    });
}

int Game::GetLocalPlayerCount(void) {
    int iPlayerC = 0;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        if (pMinecraft != nullptr && pMinecraft->localplayers[i] != nullptr) {
            iPlayerC++;
        }
    }

    return iPlayerC;
}

// Installed DLC callback

// 4J-JEV: For the sake of clarity in DLCMountedCallback.
#if defined(_WINDOWS64)
#define CONTENT_DATA_DISPLAY_NAME(a) (a.szDisplayName)
#else
#define CONTENT_DATA_DISPLAY_NAME(a) (a.wszDisplayName)
#endif

#undef CONTENT_DATA_DISPLAY_NAME

//  void Game::InstallDefaultCape()
//  {
// 	 if(!m_bDefaultCapeInstallAttempted)
// 	 {
// 		 // we only attempt to install the cape once per launch of the
// game 		 m_bDefaultCapeInstallAttempted=true;
//
// 		 std::string wTemp="Default_Cape.png";
// 		 bool bRes=app.IsFileInMemoryTextures(wTemp);
// 		 // if the file is not already in the memory textures, then read
// it from TMS 		 if(!bRes)
// 		 {
// 			 std::uint8_t *pBuffer=nullptr;
// 			 std::uint32_t dwSize=0;
// 			 // 4J-PB - out for now for DaveK so he doesn't get the
// birthday cape #ifdef _CONTENT_PACKAGE
// IPlatformStorage::ETMSStatus eTMSStatus;
// 			 eTMSStatus=PlatformStorage.ReadTMSFile(PlatformProfile.GetPrimaryPad(),IPlatformStorage::eGlobalStorage_Title,IPlatformStorage::eTMS_FileType_Graphic,
// "Default_Cape.png",&pBuffer, &dwSize);
// 			 if(eTMSStatus==IPlatformStorage::ETMSStatus_Idle)
// 			 {
// 				 app.AddMemoryTextureFile(wTemp,pBuffer,dwSize);
// 			 }
// #endif
// 		 }
// 	 }
//  }

//  int Game::DLCReadCallback(void*
//  pParam,IPlatformStorage::DLC_FILE_DETAILS *pDLCData)
//  {
//
//
// 	 return 0;
//  }

//-------------------------------------------------------------------------------------
// Name: InitTime()
// Desc: Initializes the timer variables
//-------------------------------------------------------------------------------------
void Game::InitTime() {
    // Save the start time
    m_Time.qwTime = time_util::clock::now();

    // Zero out the elapsed and total time
    m_Time.qwAppTime = {};
    m_Time.fAppTime = 0.0f;
    m_Time.fElapsedTime = 0.0f;
}

//-------------------------------------------------------------------------------------
// Name: UpdateTime()
// Desc: Updates the elapsed time since our last frame.
//-------------------------------------------------------------------------------------
void Game::UpdateTime() {
    auto qwNewTime = time_util::clock::now();
    auto qwDeltaTime = qwNewTime - m_Time.qwTime;

    m_Time.qwAppTime += qwDeltaTime;
    m_Time.qwTime = qwNewTime;

    m_Time.fElapsedTime = std::chrono::duration<float>(qwDeltaTime).count();
    m_Time.fAppTime = std::chrono::duration<float>(m_Time.qwAppTime).count();
}

bool Game::isXuidDeadmau5(PlayerUID xuid) {
    auto it = DLCController::MojangData.find(
        xuid);  // 4J Stu - The .at and [] accessors
                // insert elements if they don't exist
    if (it != DLCController::MojangData.end()) {
        MOJANG_DATA* pMojangData = DLCController::MojangData[xuid];
        if (pMojangData && pMojangData->eXuid == eXUID_Deadmau5) {
            return true;
        }
    }

    return false;
}

void Game::StoreLaunchData() {}

void Game::ExitGame() {}

// Invites

//////////////////////////////////////////////////////////////////////////
//
// FatalLoadError
//
// This is called when we can't load one of the required files at startup
// It tends to mean the files have been corrupted.
// We have to assume that we've not been able to load the text for the game.
//
//////////////////////////////////////////////////////////////////////////
void Game::FatalLoadError() {}

// Game Host options

void Game::SetGameHostOption(eGameHostOption eVal, unsigned int uiVal) {
    GameHostOptions::set(m_uiGameHostSettings, eVal, uiVal);
}

unsigned int Game::GetGameHostOption(eGameHostOption eVal) {
    return GameHostOptions::get(m_uiGameHostSettings, eVal);
}

void Game::processSchematics(LevelChunk* levelChunk) {
    m_gameRules.processSchematics(levelChunk);
}

void Game::processSchematicsLighting(LevelChunk* levelChunk) {
    m_gameRules.processSchematicsLighting(levelChunk);
}

void Game::loadDefaultGameRules() { m_gameRules.loadDefaultGameRules(); }

void Game::setLevelGenerationOptions(LevelGenerationOptions* levelGen) {
    m_gameRules.setLevelGenerationOptions(levelGen);
}

const char* Game::GetGameRulesString(const std::string& key) {
    return m_gameRules.GetGameRulesString(key);
}

// PNG_TAG_tEXt, FromBigEndian, GetImageTextData, CreateImageTextData moved to
// MenuController

std::string Game::getEntityName(eINSTANCEOF type) {
    switch (type) {
        case eTYPE_WOLF:
            return app.GetString(IDS_WOLF);
        case eTYPE_CREEPER:
            return app.GetString(IDS_CREEPER);
        case eTYPE_SKELETON:
            return app.GetString(IDS_SKELETON);
        case eTYPE_SPIDER:
            return app.GetString(IDS_SPIDER);
        case eTYPE_ZOMBIE:
            return app.GetString(IDS_ZOMBIE);
        case eTYPE_PIGZOMBIE:
            return app.GetString(IDS_PIGZOMBIE);
        case eTYPE_ENDERMAN:
            return app.GetString(IDS_ENDERMAN);
        case eTYPE_SILVERFISH:
            return app.GetString(IDS_SILVERFISH);
        case eTYPE_CAVESPIDER:
            return app.GetString(IDS_CAVE_SPIDER);
        case eTYPE_GHAST:
            return app.GetString(IDS_GHAST);
        case eTYPE_SLIME:
            return app.GetString(IDS_SLIME);
        case eTYPE_ARROW:
            return app.GetString(IDS_ITEM_ARROW);
        case eTYPE_ENDERDRAGON:
            return app.GetString(IDS_ENDERDRAGON);
        case eTYPE_BLAZE:
            return app.GetString(IDS_BLAZE);
        case eTYPE_LAVASLIME:
            return app.GetString(IDS_LAVA_SLIME);
            // 4J-PB - fix for #107167 - Customer Encountered: TU12: Content:
            // UI: There is no information what killed Player after being slain
            // by Iron Golem.
        case eTYPE_VILLAGERGOLEM:
            return app.GetString(IDS_IRONGOLEM);
        case eTYPE_HORSE:
            return app.GetString(IDS_HORSE);
        case eTYPE_WITCH:
            return app.GetString(IDS_WITCH);
        case eTYPE_WITHERBOSS:
            return app.GetString(IDS_WITHER);
        case eTYPE_BAT:
            return app.GetString(IDS_BAT);
        default:
            break;
    };

    return "";
}

// m_dwContentTypeA moved to DLCController

int32_t Game::RegisterMojangData(char* pXuidName, PlayerUID xuid, char* pSkin,
                                 char* pCape) {
    int32_t hr = 0;
    eXUID eTempXuid = eXUID_Undefined;
    MOJANG_DATA* pMojangData = nullptr;

    // ignore the names if we don't recognize them
    if (pXuidName != nullptr) {
        if (strcmp(pXuidName, "XUID_NOTCH") == 0) {
            eTempXuid =
                eXUID_Notch;  // might be needed for the apple at some point
        } else if (strcmp(pXuidName, "XUID_DEADMAU5") == 0) {
            eTempXuid = eXUID_Deadmau5;  // Needed for the deadmau5 ears
        } else {
            eTempXuid = eXUID_NoName;
        }
    }

    if (eTempXuid != eXUID_Undefined) {
        pMojangData = new MOJANG_DATA;
        memset(pMojangData, 0, sizeof(MOJANG_DATA));
        pMojangData->eXuid = eTempXuid;

        strncpy(pMojangData->wchSkin, pSkin, MAX_CAPENAME_SIZE);
        strncpy(pMojangData->wchCape, pCape, MAX_CAPENAME_SIZE);
        DLCController::MojangData[xuid] = pMojangData;
    }

    return hr;
}

MOJANG_DATA* Game::GetMojangDataForXuid(PlayerUID xuid) {
    return DLCController::MojangData[xuid];
}

int32_t Game::RegisterConfigValues(char* pType, int iValue) {
    int32_t hr = 0;

    // #ifdef 0
    // 	if(pType!=nullptr)
    // 	{
    // 		if(strcmp(pType,"XboxOneTransfer")==0)
    // 		{
    // 			if(iValue>0)
    // 			{
    // 				app.m_bTransferSavesToXboxOne=true;
    // 			}
    // 			else
    // 			{
    // 				app.m_bTransferSavesToXboxOne=false;
    // 			}
    // 		}
    // 		else if(strcmp(pType,"TransferSlotCount")==0)
    // 		{
    // 			app.m_uiTransferSlotC=iValue;
    // 		}
    //
    // 	}
    // #endif

    return hr;
}

#if defined(_WINDOWS64)
#elif defined(__linux__)
#else

#endif

// DLC

// AUTOSAVE
void Game::SetAutosaveTimerTime(void) {
    int settingValue =
        GetGameSettings(PlatformProfile.GetPrimaryPad(), eGameSetting_Autosave);
    m_saveManager.setAutosaveTimerTime(settingValue);
}

void Game::SetTrialTimerStart(void) {
    m_fTrialTimerStart = m_Time.fAppTime;
    mfTrialPausedTime = 0.0f;
}

float Game::getTrialTimer(void) {
    return m_Time.fAppTime - m_fTrialTimerStart - mfTrialPausedTime;
}

bool Game::IsLocalMultiplayerAvailable() {
    unsigned int connectedControllers = 0;
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (PlatformInput.IsPadConnected(i) || PlatformProfile.IsSignedIn(i))
            ++connectedControllers;
    }

    bool available = PlatformRenderer.IsHiDef() && connectedControllers > 1;

    return available;

    // Found this in GameNetworkManager?
    // #ifdef 0
    //		iOtherConnectedControllers =
    // PlatformInput.GetConnectedGamepadCount();
    //		if((PlatformInput.IsPadConnected(userIndex) ||
    // PlatformProfile.IsSignedIn(userIndex)))
    //		{
    //			--iOtherConnectedControllers;
    //		}
    // #else
    //		for(unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
    //		{
    //			if( (i!=userIndex) && (PlatformInput.IsPadConnected(i)
    //||
    // PlatformProfile.IsSignedIn(i)) )
    //			{
    //				iOtherConnectedControllers++;
    //			}
    //		}
    // #endif
}

// 4J-PB - language and locale function

// (moved to manager class)

std::string Game::getFilePath(std::uint32_t packId, std::string filename,
                              bool bAddDataFolder, std::string mountPoint) {
    std::string path =
        getRootPath(packId, true, bAddDataFolder, mountPoint) + filename;
    File f(path);
    if (f.exists()) {
        return path;
    }
    return getRootPath(packId, false, true, mountPoint) + filename;
}

enum ETitleUpdateTexturePacks {
    // eTUTP_MassEffect = 0x400,
    // eTUTP_Skyrim = 0x401,
    // eTUTP_Halo = 0x402,
    // eTUTP_Festive = 0x405,

    // eTUTP_Plastic = 0x801,
    // eTUTP_Candy = 0x802,
    // eTUTP_Fantasy = 0x803,
    eTUTP_Halloween = 0x804,
    // eTUTP_Natural = 0x805,
    // eTUTP_City = 0x01000806, // 4J Stu - The released City pack had a
    // sub-pack ID eTUTP_Cartoon = 0x807, eTUTP_Steampunk = 0x01000808, // 4J
    // Stu - The released Steampunk pack had a sub-pack ID
};

#if defined(_WINDOWS64)
std::string titleUpdateTexturePackRoot = "Windows64\\DLC\\";
#else
std::string titleUpdateTexturePackRoot = "CU\\DLC\\";
#endif

std::string Game::getRootPath(std::uint32_t packId, bool allowOverride,
                              bool bAddDataFolder, std::string mountPoint) {
    std::string path = mountPoint;
    if (allowOverride) {
        switch (packId) {
            case eTUTP_Halloween:
                path = titleUpdateTexturePackRoot + "Halloween Texture Pack";
                break;
        };
        File folder(path);
        if (!folder.exists()) {
            path = mountPoint;
        }
    }

    if (bAddDataFolder) {
        return path + "\\Data\\";
    } else {
        return path + "\\";
    }
}
