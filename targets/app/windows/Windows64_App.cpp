
#include "WindowsGame.h"
#include "platform/game/game.h"
#include "app/common/Game.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/User.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/PlayerList.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelSettings.h"
#include "minecraft/world/level/LevelType.h"
#include "minecraft/world/level/biome/BiomeSource.h"

WindowsGame app;

#define CONTEXT_GAME_STATE 0

WindowsGame::WindowsGame() : Game() {}

void WindowsGame::SetRichPresenceContext(int iPad, int contextId) {
    PlatformProfile.SetRichPresenceContextValue(iPad, CONTEXT_GAME_STATE,
                                                contextId);
}

void WindowsGame::StoreLaunchData() {}
void WindowsGame::ExitGame() {}
void WindowsGame::FatalLoadError() {}

void WindowsGame::CaptureSaveThumbnail() {}
void WindowsGame::GetSaveThumbnail(std::uint8_t** thumbnailData,
                                   unsigned int* thumbnailSize) {}
void WindowsGame::ReleaseSaveThumbnail() {}

void WindowsGame::GetScreenshot(int iPad, std::uint8_t** screenshotData,
                                unsigned int* screenshotSize) {}

void WindowsGame::TemporaryCreateGameStart() {
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// From CScene_Main::OnInit

    app.setLevelGenerationOptions(nullptr);

    // From CScene_Main::RunPlayGame
    Minecraft* pMinecraft = Minecraft::GetInstance();
    PlatformGame.ReleaseSaveThumbnail();
    PlatformProfile.SetLockedProfile(0);
    pMinecraft->user->name = "Windows";
    app.ApplyGameSettingsChanged(0);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// From CScene_MultiGameJoinLoad::OnInit
    MinecraftServer::resetFlags();

    // From CScene_MultiGameJoinLoad::OnNotifyPressEx
    app.SetTutorialMode(false);
    app.SetCorruptSaveDeleted(false);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// From CScene_MultiGameCreate::CreateGame

    app.ClearTerrainFeaturePosition();
    std::string wWorldName = "TestWorld";

    PlatformStorage.ResetSaveData();
    PlatformStorage.SetSaveTitle(wWorldName.c_str());

    bool isFlat = false;
    int64_t seedValue =
        0;  // BiomeSource::findSeed(isFlat?LevelType::lvl_flat:LevelType::lvl_normal);
            // // 4J - was (new Random())->nextLong() - now trying to actually
            // find a seed to suit our requirements

    NetworkGameInitData* param = new NetworkGameInitData();
    param->seed = seedValue;
    param->saveData = nullptr;

    app.SetGameHostOption(eGameHostOption_Difficulty, 0);
    app.SetGameHostOption(eGameHostOption_FriendsOfFriends, 0);
    app.SetGameHostOption(eGameHostOption_Gamertags, 1);
    app.SetGameHostOption(eGameHostOption_BedrockFog, 1);

    app.SetGameHostOption(
        eGameHostOption_GameType,
        GameType::CREATIVE->getId());  // LevelSettings::GAMETYPE_SURVIVAL
    app.SetGameHostOption(eGameHostOption_LevelType, 0);
    app.SetGameHostOption(eGameHostOption_Structures, 1);
    app.SetGameHostOption(eGameHostOption_BonusChest, 0);

    app.SetGameHostOption(eGameHostOption_PvP, 1);
    app.SetGameHostOption(eGameHostOption_TrustPlayers, 1);
    app.SetGameHostOption(eGameHostOption_FireSpreads, 1);
    app.SetGameHostOption(eGameHostOption_TNT, 1);
    app.SetGameHostOption(eGameHostOption_HostCanFly, 1);
    app.SetGameHostOption(eGameHostOption_HostCanChangeHunger, 1);
    app.SetGameHostOption(eGameHostOption_HostCanBeInvisible, 1);

    param->settings = app.GetGameHostOption(eGameHostOption_All);

    g_NetworkManager.FakeLocalPlayerJoined();

    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
    loadingParams->lpParam = param;

    // Reset the autosave time
    app.SetAutosaveTimerTime();

    C4JThread* thread = new C4JThread(loadingParams->func,
                                      loadingParams->lpParam, "RunNetworkGame");
    thread->run();
}

int WindowsGame::GetLocalTMSFileIndex(char* wchTMSFile,
                                      bool bFilenameIncludesExtension,
                                      eFileExtensionType eEXT) {
    return -1;
}

int WindowsGame::LoadLocalTMSFile(char* wchTMSFile) { return -1; }

int WindowsGame::LoadLocalTMSFile(char* wchTMSFile, eFileExtensionType eExt) {
    return -1;
}

void WindowsGame::FreeLocalTMSFiles(eTMSFileType eType) {}
