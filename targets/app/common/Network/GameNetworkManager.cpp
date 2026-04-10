#include "GameNetworkManager.h"
#include "platform/game/game.h"

#include <assert.h>

#include <algorithm>
#include <chrono>
#include <compare>
#include <memory>
#include <thread>
#include <vector>

#include "minecraft/network/Socket.h"
#include "app/common/Game.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_PauseMenu.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "java/File.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/User.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/LevelRenderer.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/network/Connection.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/PreLoginPacket.h"
#include "platform/network/network.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/PlayerList.h"
#include "minecraft/server/ServerAction.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/server/network/PlayerConnection.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/item/crafting/FireworksRecipe.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/chunk/storage/OldChunkStorage.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/XboxStubs.h"
#include "platform/fs/fs.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "platform/storage/storage.h"
#include "strings.h"
#include "util/StringHelpers.h"
#include "platform/network/network.h"

class FriendSessionInfo;
class INVITE_INFO;

// Global instance
CGameNetworkManager g_NetworkManager;
IPlatformNetwork* CGameNetworkManager::s_pPlatformNetworkManager;

// minecraft/-side function accessor for INetworkService.
namespace minecraft::network::platform_internal {
::minecraft::network::INetworkService& NetworkService_get() {
    return g_NetworkManager;
}
}  // namespace minecraft::network::platform_internal

int64_t CGameNetworkManager::messageQueue[512];
int64_t CGameNetworkManager::byteQueue[512];
int CGameNetworkManager::messageQueuePos = 0;

CGameNetworkManager::CGameNetworkManager() {
    m_bInitialised = false;
    m_bLastDisconnectWasLostRoomOnly = false;
    m_bFullSessionMessageOnNextSessionChange = false;
}

void CGameNetworkManager::Initialise() {
    ServerStoppedCreate(false);
    ServerReadyCreate(false);
    int flagIndexSize =
        LevelRenderer::getGlobalChunkCount() /
        (Level::maxBuildHeight /
         16);  // dividing here by number of renderer chunks in one column
    s_pPlatformNetworkManager = &PlatformNetwork;
    s_pPlatformNetworkManager->Initialise(this, flagIndexSize);
    m_bNetworkThreadRunning = false;
    m_bInitialised = true;
}

void CGameNetworkManager::Terminate() {
    if (m_bInitialised) {
        s_pPlatformNetworkManager->Terminate();
    }
}

void CGameNetworkManager::DoWork() { s_pPlatformNetworkManager->DoWork(); }

bool CGameNetworkManager::_RunNetworkGame(void* lpParameter) {
    bool success = true;

    bool isHost = g_NetworkManager.IsHost();
    // Start the network game
    Minecraft* pMinecraft = Minecraft::GetInstance();
    success = StartNetworkGame(pMinecraft, lpParameter);

    if (!success) return false;

    if (isHost) {
        // We do not have a lobby, so the only players in the game at this point
        // are local ones.

        success = s_pPlatformNetworkManager->_RunNetworkGame();
        if (!success) {
            app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_ExitWorld,
                          (void*)true);
            return true;
        }
    }

    if (g_NetworkManager.IsLeavingGame()) return false;

    app.SetGameStarted(true);

    // app.CloseXuiScenes(PlatformProfile.GetPrimaryPad());

    return success;
}

bool CGameNetworkManager::StartNetworkGame(Minecraft* minecraft,
                                           void* lpParameter) {
    int64_t seed = 0;
    if (lpParameter != nullptr) {
        NetworkGameInitData* param = (NetworkGameInitData*)lpParameter;
        seed = param->seed;

        app.setLevelGenerationOptions(param->levelGen);
        if (param->levelGen != nullptr) {
            if (app.getLevelGenerationOptions() == nullptr) {
                app.DebugPrintf(
                    "Game rule was not loaded, and seed is required. "
                    "Exiting.\n");
                return false;
            } else {
                param->seed = seed =
                    app.getLevelGenerationOptions()->getLevelSeed();

                if (param->levelGen->isTutorial()) {
                    // Load the tutorial save data here
                    if (param->levelGen->requiresBaseSave() &&
                        !param->levelGen->getBaseSavePath().empty()) {
#if defined(_WINDOWS64)
                        std::string fileRoot =
                            "Windows64Media\\Tutorial\\" +
                            param->levelGen->getBaseSavePath();
                        File root(fileRoot);
                        if (!root.exists())
                            fileRoot = "Windows64\\Tutorial\\" +
                                       param->levelGen->getBaseSavePath();
#else
                        std::string fileRoot =
                            "Tutorial\\" + param->levelGen->getBaseSavePath();
#endif
                        File grf(fileRoot);
                        if (grf.exists()) {
                            std::size_t dwFileSize =
                                PlatformFilesystem.fileSize(grf.getPath());
                            if (dwFileSize > 0) {
                                uint8_t* pbData =
                                    (uint8_t*)new uint8_t[dwFileSize];
                                auto readResult = PlatformFilesystem.readFile(
                                    grf.getPath(), pbData, dwFileSize);
                                if (readResult.status !=
                                    IPlatformFilesystem::ReadStatus::Ok) {
                                    app.FatalLoadError();
                                }

                                // 4J-PB - is it possible that we can get here
                                // after a read fail and it's not an error?
                                param->levelGen->setBaseSaveData(pbData,
                                                                 dwFileSize);
                            }
                        }
                    }
                }
            }
        }
    }

    static int64_t sseed =
        seed;  // Create static version so this will be valid until next call to
               // this function & whilst thread is running
    ServerStoppedCreate(false);
    if (g_NetworkManager.IsHost()) {
        ServerStoppedCreate(true);
        ServerReadyCreate(true);
        // Ready to go - create actual networking thread & start hosting
        C4JThread* thread =
            new C4JThread(&CGameNetworkManager::ServerThreadProc, lpParameter,
                          "Server", 256 * 1024);

        thread->run();

        app.DebugPrintf("[NET] Waiting for server ready...\n");
        ServerReadyWait();
        ServerReadyDestroy();
        app.DebugPrintf("[NET] Server ready! serverHalted=%d\n",
                        MinecraftServer::serverHalted());

        if (MinecraftServer::serverHalted()) return false;

        //		printf("Server ready to go!\n");
    } else {
        Socket::Initialise(nullptr);
    }

    Minecraft* pMinecraft = Minecraft::GetInstance();
    app.DebugPrintf("[NET] IsReadyToPlayOrIdle=%d  IsInSession=%d\n",
                    IsReadyToPlayOrIdle(), IsInSession());
    // Make sure that we have transitioned through any joining/creating stages
    // and are actually playing the game, so that we know the players should be
    // valid
    bool changedMessage = false;
    while (!IsReadyToPlayOrIdle()) {
        changedMessage = true;
        pMinecraft->progressRenderer->progressStage(
            g_NetworkManager.CorrectErrorIDS(
                IDS_PROGRESS_SAVING_TO_DISC));  // "Finalizing..." vaguest
                                                // message I could find
        pMinecraft->progressRenderer->progressStagePercentage(
            g_NetworkManager.GetJoiningReadyPercentage());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (changedMessage) {
        pMinecraft->progressRenderer->progressStagePercentage(100);
    }

    // If we aren't in session, then something bad must have happened - we
    // aren't joining, creating or ready play
    app.DebugPrintf("[NET] Checking IsInSession...=%d\n", IsInSession());
    if (!IsInSession()) {
        app.DebugPrintf("[NET] NOT in session! Halting server.\n");
        MinecraftServer::HaltServer();
        return false;
    }

    app.DebugPrintf("[NET] DLC check: completed=%d pending=%d\n",
                    app.DLCInstallProcessCompleted(), app.DLCInstallPending());
    // 4J Stu - Wait a while to make sure that DLC is loaded. This is the last
    // point before the network communication starts so the latest we can check
    // this
    while (!app.DLCInstallProcessCompleted() && app.DLCInstallPending() &&
           !g_NetworkManager.IsLeavingGame()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (g_NetworkManager.IsLeavingGame()) {
        MinecraftServer::HaltServer();
        return false;
    }

    // PRIMARY PLAYER

    app.DebugPrintf("[NET] Creating ClientConnection (IsHost=%d)...\n",
                    g_NetworkManager.IsHost());
    std::vector<ClientConnection*> createdConnections;
    ClientConnection* connection;

    if (g_NetworkManager.IsHost()) {
        connection = new ClientConnection(minecraft, nullptr);
        app.DebugPrintf("[NET] ClientConnection created, createdOk=%d\n",
                        connection->createdOk);
    } else {
        INetworkPlayer* pNetworkPlayer =
            g_NetworkManager.GetLocalPlayerByUserIndex(
                PlatformProfile.GetLockedProfile());
        if (pNetworkPlayer == nullptr) {
            MinecraftServer::HaltServer();
            app.DebugPrintf("%d\n", PlatformProfile.GetLockedProfile());
            // If the player is nullptr here then something went wrong in the
            // session setup, and continuing will end up in a crash
            return false;
        }

        Socket* socket = pNetworkPlayer->GetSocket();

        // Fix for #13259 - CRASH: Gameplay: loading process is halted when
        // player loads saved data
        if (socket == nullptr) {
            assert(false);
            MinecraftServer::HaltServer();
            // If the socket is nullptr here then something went wrong in the
            // session setup, and continuing will end up in a crash
            return false;
        }

        connection = new ClientConnection(minecraft, socket);
    }

    if (!connection->createdOk) {
        assert(false);
        delete connection;
        connection = nullptr;
        MinecraftServer::HaltServer();
        return false;
    }

    app.DebugPrintf("[NET] Sending PreLoginPacket...\n");
    connection->send(std::shared_ptr<PreLoginPacket>(
        new PreLoginPacket(minecraft->user->name)));
    app.DebugPrintf(
        "[NET] PreLoginPacket sent. Entering connection tick loop...\n");

    // Tick connection until we're ready to go. The stages involved in this are:
    // (1) Creating the ClientConnection sends a prelogin packet to the server
    // (2) the server sends a prelogin back, which is handled by the
    // clientConnection, and returns a login packet (3) the server sends a login
    // back, which is handled by the client connection to start the game
    if (!g_NetworkManager.IsHost()) {
        Minecraft::GetInstance()->progressRenderer->progressStart(
            IDS_PROGRESS_CONNECTING);
    }

    TexturePack* tPack = Minecraft::GetInstance()->skins->getSelected();
    do {
        app.DebugPrintf("ticking connection A\n");
        connection->tick();

        // 4J Stu - We were ticking this way too fast which could cause the
        // connection to time out The connections should tick at 20 per second
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while ((IsInSession() && !connection->isStarted() &&
              !connection->isClosed() && !g_NetworkManager.IsLeavingGame()) ||
             tPack->isLoadingData() ||
             (Minecraft::GetInstance()->skins->needsUIUpdate() ||
              ui.IsReloadingSkin()));
    ui.CleanUpSkinReload();

    // 4J Stu - Fix for #11279 - CRASH: TCR 001: BAS Game Stability: Signing out
    // of game will cause title to crash We need to break out of the above loop
    // if m_bLeavingGame is set, and close the connection
    if (g_NetworkManager.IsLeavingGame() || !IsInSession()) {
        connection->close();
    }

    if (connection->isStarted() && !connection->isClosed()) {
        createdConnections.push_back(connection);

        int primaryPad = PlatformProfile.GetPrimaryPad();
        PlatformGame.SetRichPresenceContext(primaryPad, CONTEXT_GAME_STATE_BLANK);
        if (GetPlayerCount() >
            1)  // Are we offline or online, and how many players are there
        {
            if (IsLocalGame())
                PlatformProfile.SetCurrentGameActivity(
                    primaryPad, CONTEXT_PRESENCE_MULTIPLAYEROFFLINE, false);
            else
                PlatformProfile.SetCurrentGameActivity(
                    primaryPad, CONTEXT_PRESENCE_MULTIPLAYER, false);
        } else {
            if (IsLocalGame())
                PlatformProfile.SetCurrentGameActivity(
                    primaryPad, CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE, false);
            else
                PlatformProfile.SetCurrentGameActivity(
                    primaryPad, CONTEXT_PRESENCE_MULTIPLAYER_1P, false);
        }

        // ALL OTHER LOCAL PLAYERS
        for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
            // Already have setup the primary pad
            if (idx == PlatformProfile.GetPrimaryPad()) continue;

            if (GetLocalPlayerByUserIndex(idx) != nullptr &&
                !PlatformProfile.IsSignedIn(idx)) {
                INetworkPlayer* pNetworkPlayer =
                    g_NetworkManager.GetLocalPlayerByUserIndex(idx);
                Socket* socket = pNetworkPlayer->GetSocket();
                app.DebugPrintf(
                    "Closing socket due to player %d not being signed in any "
                    "more\n", idx);
                if (!socket->close(false)) socket->close(true);

                continue;
            }

            // By default when we host we only have the local player, but
            // currently allow multiple local players to join when joining any
            // other way, so just because they are signed in doesn't mean they
            // are in the session 4J Stu - If they are in the session, then we
            // should add them to the game. Otherwise we won't be able to add
            // them later
            INetworkPlayer* pNetworkPlayer =
                g_NetworkManager.GetLocalPlayerByUserIndex(idx);
            if (pNetworkPlayer == nullptr) continue;

            ClientConnection* connection;

            Socket* socket = pNetworkPlayer->GetSocket();
            connection = new ClientConnection(minecraft, socket, idx);

            minecraft->addPendingLocalConnection(idx, connection);
            // minecraft->createExtraLocalPlayer(idx, (convStringToWstring(
            // PlatformProfile.GetGamertag(idx) )).c_str(), idx, connection);

            // Open the socket on the server end to accept incoming data
            Socket::addIncomingSocket(socket);

            connection->send(std::shared_ptr<PreLoginPacket>(
                new PreLoginPacket(PlatformProfile.GetGamertag(idx))));

            createdConnections.push_back(connection);

            // Tick connection until we're ready to go. The stages involved in
            // this are: (1) Creating the ClientConnection sends a prelogin
            // packet to the server (2) the server sends a prelogin back, which
            // is handled by the clientConnection, and returns a login packet
            // (3) the server sends a login back, which is handled by the client
            // connection to start the game
            do {
                // We need to keep ticking the connections for players that
                // already logged in
                for (auto it = createdConnections.begin();
                     it < createdConnections.end(); ++it) {
                    (*it)->tick();
                }

                // 4J Stu - We were ticking this way too fast which could cause
                // the connection to time out The connections should tick at 20
                // per second
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                app.DebugPrintf("<***> %d %d %d %d %d\n", IsInSession(),
                                !connection->isStarted(),
                                !connection->isClosed(),
                                PlatformProfile.IsSignedIn(idx),
                                !g_NetworkManager.IsLeavingGame());
                // TODO - This SHOULD be something just like the code above but
                // temporarily changing here so that we don't have to depend on
                // the platformprofile behaviour
            } while (IsInSession() && !connection->isStarted() &&
                     !connection->isClosed() &&
                     !g_NetworkManager.IsLeavingGame());

            // 4J Stu - Fix for #11279 - CRASH: TCR 001: BAS Game Stability:
            // Signing out of game will cause title to crash We need to break
            // out of the above loop if m_bLeavingGame is set, and stop creating
            // new connections The connections in the createdConnections vector
            // get closed at the end of the thread
            if (g_NetworkManager.IsLeavingGame() || !IsInSession()) break;

            if (PlatformProfile.IsSignedIn(idx) && !connection->isClosed()) {
                PlatformGame.SetRichPresenceContext(idx, CONTEXT_GAME_STATE_BLANK);
                if (IsLocalGame())
                    PlatformProfile.SetCurrentGameActivity(
                        idx, CONTEXT_PRESENCE_MULTIPLAYEROFFLINE, false);
                else
                    PlatformProfile.SetCurrentGameActivity(
                        idx, CONTEXT_PRESENCE_MULTIPLAYER, false);
            } else {
                connection->close();
                auto it = find(createdConnections.begin(),
                               createdConnections.end(), connection);
                if (it != createdConnections.end())
                    createdConnections.erase(it);
            }
        }

        app.SetGameMode(eMode_Multiplayer);
    } else if (connection->isClosed() || !IsInSession()) {
        //		assert(false);
        MinecraftServer::HaltServer();
        return false;
    }

    if (g_NetworkManager.IsLeavingGame() || !IsInSession()) {
        for (auto it = createdConnections.begin();
             it < createdConnections.end(); ++it) {
            (*it)->close();
        }
        //		assert(false);
        MinecraftServer::HaltServer();
        return false;
    }

    // Catch in-case server has been halted (by a player signout).
    if (MinecraftServer::serverHalted()) return false;

    return true;
}

int CGameNetworkManager::CorrectErrorIDS(int IDS) {
    return s_pPlatformNetworkManager->CorrectErrorIDS(IDS);
}

int CGameNetworkManager::GetLocalPlayerMask(int playerIndex) {
    return s_pPlatformNetworkManager->GetLocalPlayerMask(playerIndex);
}

int CGameNetworkManager::GetPlayerCount() {
    return s_pPlatformNetworkManager->GetPlayerCount();
}

int CGameNetworkManager::GetOnlinePlayerCount() {
    return s_pPlatformNetworkManager->GetOnlinePlayerCount();
}

bool CGameNetworkManager::AddLocalPlayerByUserIndex(int userIndex) {
    return s_pPlatformNetworkManager->AddLocalPlayerByUserIndex(userIndex);
}

bool CGameNetworkManager::RemoveLocalPlayerByUserIndex(int userIndex) {
    return s_pPlatformNetworkManager->RemoveLocalPlayerByUserIndex(userIndex);
}

INetworkPlayer* CGameNetworkManager::GetLocalPlayerByUserIndex(int userIndex) {
    return s_pPlatformNetworkManager->GetLocalPlayerByUserIndex(userIndex);
}

INetworkPlayer* CGameNetworkManager::GetPlayerByIndex(int playerIndex) {
    return s_pPlatformNetworkManager->GetPlayerByIndex(playerIndex);
}

INetworkPlayer* CGameNetworkManager::GetPlayerByXuid(PlayerUID xuid) {
    return s_pPlatformNetworkManager->GetPlayerByXuid(xuid);
}

INetworkPlayer* CGameNetworkManager::GetPlayerBySmallId(unsigned char smallId) {
    return s_pPlatformNetworkManager->GetPlayerBySmallId(smallId);
}

INetworkPlayer* CGameNetworkManager::GetHostPlayer() {
    return s_pPlatformNetworkManager->GetHostPlayer();
}

void CGameNetworkManager::RegisterPlayerChangedCallback(
    int iPad,
    std::function<void(INetworkPlayer* pPlayer, bool leaving)> callback) {
    s_pPlatformNetworkManager->RegisterPlayerChangedCallback(
        iPad, std::move(callback));
}

void CGameNetworkManager::UnRegisterPlayerChangedCallback(int iPad) {
    s_pPlatformNetworkManager->UnRegisterPlayerChangedCallback(iPad);
}

void CGameNetworkManager::HandleSignInChange() {
    s_pPlatformNetworkManager->HandleSignInChange();
}

bool CGameNetworkManager::ShouldMessageForFullSession() {
    return s_pPlatformNetworkManager->ShouldMessageForFullSession();
}

bool CGameNetworkManager::IsInSession() {
    return s_pPlatformNetworkManager->IsInSession();
}

bool CGameNetworkManager::IsInGameplay() {
    return s_pPlatformNetworkManager->IsInGameplay();
}

bool CGameNetworkManager::IsReadyToPlayOrIdle() {
    return s_pPlatformNetworkManager->IsReadyToPlayOrIdle();
}

bool CGameNetworkManager::IsLeavingGame() {
    return s_pPlatformNetworkManager->IsLeavingGame();
}

bool CGameNetworkManager::SetLocalGame(bool isLocal) {
    return s_pPlatformNetworkManager->SetLocalGame(isLocal);
}

bool CGameNetworkManager::IsLocalGame() {
    return s_pPlatformNetworkManager->IsLocalGame();
}

void CGameNetworkManager::SetPrivateGame(bool isPrivate) {
    s_pPlatformNetworkManager->SetPrivateGame(isPrivate);
}

bool CGameNetworkManager::IsPrivateGame() {
    return s_pPlatformNetworkManager->IsPrivateGame();
}

void CGameNetworkManager::HostGame(int localUsersMask, bool bOnlineGame,
                                   bool bIsPrivate, unsigned char publicSlots,
                                   unsigned char privateSlots) {
    // 4J Stu - clear any previous connection errors
    Minecraft::GetInstance()->clearConnectionFailed();

    s_pPlatformNetworkManager->HostGame(localUsersMask, bOnlineGame, bIsPrivate,
                                        publicSlots, privateSlots);
}

bool CGameNetworkManager::IsHost() {
    return (s_pPlatformNetworkManager->IsHost() == true);
}

bool CGameNetworkManager::IsInStatsEnabledSession() {
    return s_pPlatformNetworkManager->IsInStatsEnabledSession();
}

bool CGameNetworkManager::SessionHasSpace(unsigned int spaceRequired) {
    return s_pPlatformNetworkManager->SessionHasSpace(spaceRequired);
}

std::vector<FriendSessionInfo*>* CGameNetworkManager::GetSessionList(
    int iPad, int localPlayers, bool partyOnly) {
    return s_pPlatformNetworkManager->GetSessionList(iPad, localPlayers,
                                                     partyOnly);
}

bool CGameNetworkManager::GetGameSessionInfo(int iPad, SessionID sessionId,
                                             FriendSessionInfo* foundSession) {
    return s_pPlatformNetworkManager->GetGameSessionInfo(iPad, sessionId,
                                                         foundSession);
}

void CGameNetworkManager::SetSessionsUpdatedCallback(
    std::function<void()> callback) {
    s_pPlatformNetworkManager->SetSessionsUpdatedCallback(std::move(callback));
}

void CGameNetworkManager::GetFullFriendSessionInfo(
    FriendSessionInfo* foundSession,
    std::function<void(bool success)> callback) {
    s_pPlatformNetworkManager->GetFullFriendSessionInfo(foundSession,
                                                        std::move(callback));
}

void CGameNetworkManager::ForceFriendsSessionRefresh() {
    s_pPlatformNetworkManager->ForceFriendsSessionRefresh();
}

bool CGameNetworkManager::JoinGameFromInviteInfo(
    int userIndex, int userMask, const INVITE_INFO* pInviteInfo) {
    return s_pPlatformNetworkManager->JoinGameFromInviteInfo(
        userIndex, userMask, pInviteInfo);
}

CGameNetworkManager::eJoinGameResult CGameNetworkManager::JoinGame(
    FriendSessionInfo* searchResult, int localUsersMask) {
    app.SetTutorialMode(false);
    g_NetworkManager.SetLocalGame(false);

    int primaryUserIndex = PlatformProfile.GetLockedProfile();

    // 4J-PB - clear any previous connection errors
    Minecraft::GetInstance()->clearConnectionFailed();

    // Make sure that the Primary Pad is in by default
    localUsersMask |= GetLocalPlayerMask(PlatformProfile.GetPrimaryPad());

    return (eJoinGameResult)(s_pPlatformNetworkManager->JoinGame(
        searchResult, localUsersMask, primaryUserIndex));
}

void CGameNetworkManager::CancelJoinGame(void* lpParam) {}

bool CGameNetworkManager::LeaveGame(bool bMigrateHost) {
    Minecraft::GetInstance()->gui->clearMessages();
    return s_pPlatformNetworkManager->LeaveGame(bMigrateHost);
}

int CGameNetworkManager::JoinFromInvite_SignInReturned(void* pParam,
                                                       bool bContinue,
                                                       int iPad) {
    INVITE_INFO* pInviteInfo = (INVITE_INFO*)pParam;

    if (bContinue == true) {
        app.DebugPrintf("JoinFromInvite_SignInReturned, iPad %d\n", iPad);
        // It's possible that the player has not signed in - they can back out
        if (PlatformProfile.IsSignedIn(iPad) &&
            PlatformProfile.IsSignedInLive(iPad)) {
            app.DebugPrintf(
                "JoinFromInvite_SignInReturned, passed sign-in tests\n");
            int localUsersMask = 0;
            int joiningUsers = 0;

            bool noPrivileges = false;
            for (unsigned int index = 0; index < XUSER_MAX_COUNT; ++index) {
                if (PlatformProfile.IsSignedIn(index)) {
                    ++joiningUsers;
                    if (!PlatformProfile.AllowedToPlayMultiplayer(index))
                        noPrivileges = true;
                    localUsersMask |= GetLocalPlayerMask(index);
                }
            }

            // Check if user-created content is allowed, as we cannot play
            // multiplayer if it's not
            bool noUGC = false;

            if (noUGC) {
                int messageText =
                    IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
                if (joiningUsers > 1)
                    messageText =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;

                ui.RequestUGCMessageBox(IDS_CONNECTION_FAILED, messageText);
            } else if (noPrivileges) {
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                                       IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT,
                                       uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
            } else {
                PlatformProfile.SetLockedProfile(iPad);
                PlatformProfile.SetPrimaryPad(iPad);

                g_NetworkManager.SetLocalGame(false);

                // If the player was signed in before selecting play, we'll not
                // have read the profile yet, so query the sign-in status to get
                // this to happen
                (void)PlatformProfile.QuerySigninStatus();

                // 4J-PB - clear any previous connection errors
                Minecraft::GetInstance()->clearConnectionFailed();

                // change the minecraft player name
                Minecraft::GetInstance()->user->name =
                    PlatformProfile.GetGamertag(
                        PlatformProfile.GetPrimaryPad());

                bool success = g_NetworkManager.JoinGameFromInviteInfo(
                    iPad,            // dwUserIndex
                    localUsersMask,  // dwUserMask
                    pInviteInfo);    // pInviteInfo
                if (!success) {
                    app.DebugPrintf("Failed joining game from invite\n");
                }
            }
        } else {
            app.DebugPrintf(
                "JoinFromInvite_SignInReturned, failed sign-in tests :%d %d\n",
                PlatformProfile.IsSignedIn(iPad),
                PlatformProfile.IsSignedInLive(iPad));
        }
    }
    return 0;
}

void CGameNetworkManager::UpdateAndSetGameSessionData(
    INetworkPlayer* pNetworkPlayerLeaving) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    TexturePack* tPack = pMinecraft->skins->getSelected();
    s_pPlatformNetworkManager->SetSessionTexturePackParentId(
        tPack->getDLCParentPackId());
    s_pPlatformNetworkManager->SetSessionSubTexturePackId(
        tPack->getDLCSubPackId());

    s_pPlatformNetworkManager->UpdateAndSetGameSessionData(
        pNetworkPlayerLeaving);
}

void CGameNetworkManager::SendInviteGUI(int quadrant) {
    s_pPlatformNetworkManager->SendInviteGUI(quadrant);
}

void CGameNetworkManager::ResetLeavingGame() {
    s_pPlatformNetworkManager->ResetLeavingGame();
}

bool CGameNetworkManager::IsNetworkThreadRunning() {
    return m_bNetworkThreadRunning;
    ;
}

int CGameNetworkManager::RunNetworkGameThreadProc(void* lpParameter) {
    // Share AABB & Vec3 pools with default (main thread) - should be ok as long
    // as we don't tick the main thread whilst this thread is running
    Compression::UseDefaultThreadStorage();
    Tile::CreateNewThreadStorage();

    g_NetworkManager.m_bNetworkThreadRunning = true;
    bool success = g_NetworkManager._RunNetworkGame(lpParameter);
    g_NetworkManager.m_bNetworkThreadRunning = false;
    if (!success) {
        TexturePack* tPack = Minecraft::GetInstance()->skins->getSelected();
        while (tPack->isLoadingData() ||
               (Minecraft::GetInstance()->skins->needsUIUpdate() ||
                ui.IsReloadingSkin())) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        ui.CleanUpSkinReload();
        if (app.GetDisconnectReason() == DisconnectPacket::eDisconnect_None) {
            app.SetDisconnectReason(
                DisconnectPacket::eDisconnect_ConnectionCreationFailed);
        }
        // If we failed before the server started, clear the game rules.
        // Otherwise the server will clear it up.
        if (MinecraftServer::getInstance() == nullptr)
            app.m_gameRules.unloadCurrentGameRules();
        Tile::ReleaseThreadStorage();
        return -1;
    }

    Tile::ReleaseThreadStorage();
    return 0;
}

int CGameNetworkManager::ServerThreadProc(void* lpParameter) {
    int64_t seed = 0;
    if (lpParameter != nullptr) {
        NetworkGameInitData* param = (NetworkGameInitData*)lpParameter;
        seed = param->seed;
        app.SetGameHostOption(eGameHostOption_All, param->settings);

        // 4J Stu - If we are loading a DLC save that's separate from the
        // texture pack, load
        if (param->levelGen != nullptr &&
            (param->texturePackId == 0 ||
             param->levelGen->getRequiredTexturePackId() !=
                 param->texturePackId)) {
            while ((Minecraft::GetInstance()->skins->needsUIUpdate() ||
                    ui.IsReloadingSkin())) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            param->levelGen->loadBaseSaveData();
        }
    }

    C4JThread::setThreadName(static_cast<std::uint32_t>(-1),
                             "Minecraft Server thread");
    Compression::UseDefaultThreadStorage();
    OldChunkStorage::UseDefaultThreadStorage();
    Entity::useSmallIds();
    Level::enableLightingCache();
    Tile::CreateNewThreadStorage();
    FireworksRecipe::CreateNewThreadStorage();

    MinecraftServer::main(
        seed,
        lpParameter);  // saveData, app.GetGameHostOption(eGameHostOption_All));

    Tile::ReleaseThreadStorage();
    Level::destroyLightingCache();

    if (lpParameter != nullptr) delete (NetworkGameInitData*)lpParameter;

    return 0;
}

int CGameNetworkManager::ExitAndJoinFromInviteThreadProc(void* lpParam) {
    // Share AABB & Vec3 pools with default (main thread) - should be ok as long
    // as we don't tick the main thread whilst this thread is running
    Compression::UseDefaultThreadStorage();

    // app.SetGameStarted(false);
    UIScene_PauseMenu::_ExitWorld(nullptr);

    while (g_NetworkManager.IsInSession()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Xbox should always be online when receiving invites - on PS3 we need to
    // check & ask the user to sign in
    JoinFromInviteData* inviteData = (JoinFromInviteData*)lpParam;
    app.SetAction(inviteData->dwUserIndex, eAppAction_JoinFromInvite, lpParam);

    return 0;
}

void CGameNetworkManager::_LeaveGame() {
    s_pPlatformNetworkManager->_LeaveGame(false, true);
}

int CGameNetworkManager::ChangeSessionTypeThreadProc(void* lpParam) {
    // Share AABB & Vec3 pools with default (main thread) - should be ok as long
    // as we don't tick the main thread whilst this thread is running
    Compression::UseDefaultThreadStorage();

    Minecraft* pMinecraft = Minecraft::GetInstance();
    MinecraftServer* pServer = MinecraftServer::getInstance();

    pMinecraft->progressRenderer->progressStartNoAbort(
        g_NetworkManager.CorrectErrorIDS(IDS_CONNECTION_LOST_LIVE_NO_EXIT));
    pMinecraft->progressRenderer->progressStage(
        IDS_PROGRESS_CONVERTING_TO_OFFLINE_GAME);

    pServer->queueServerAction(minecraft::server::PauseServer{true});

    // wait for the server to be in a non-ticking state
    pServer->m_serverPausedEvent->waitForSignal(C4JThread::kInfiniteTimeout);

    pMinecraft->progressRenderer->progressStartNoAbort(
        g_NetworkManager.CorrectErrorIDS(IDS_CONNECTION_LOST_LIVE_NO_EXIT));
    pMinecraft->progressRenderer->progressStage(
        IDS_PROGRESS_CONVERTING_TO_OFFLINE_GAME);

    pMinecraft->progressRenderer->progressStagePercentage(25);

    // Null the network player of all the server players that are local, to stop
    // them being removed from the server when removed from the session
    if (pServer != nullptr) {
        PlayerList* players = pServer->getPlayers();
        for (auto it = players->players.begin(); it < players->players.end();
             ++it) {
            std::shared_ptr<ServerPlayer> servPlayer = *it;
            if (servPlayer->connection->isLocal() &&
                !servPlayer->connection->isGuest()) {
                servPlayer->connection->connection->getSocket()->setPlayer(
                    nullptr);
            }
        }
    }

    // delete the current session - if we weren't actually disconnected fully
    // from the network but have just lost our room, then pass a bLeaveRoom flag
    // of false here as by definition we don't need to leave the room (again).
    // This is currently only an issue for sony platforms.
    if (g_NetworkManager.m_bLastDisconnectWasLostRoomOnly) {
        s_pPlatformNetworkManager->_LeaveGame(false, false);
    } else {
        s_pPlatformNetworkManager->_LeaveGame(false, true);
    }

    // wait for the current session to end
    while (g_NetworkManager.IsInSession()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Reset this flag as the we don't need to know that we only lost the room
    // only from this point onwards, the behaviour is exactly the same
    g_NetworkManager.m_bLastDisconnectWasLostRoomOnly = false;
    g_NetworkManager.m_bFullSessionMessageOnNextSessionChange = false;

    pMinecraft->progressRenderer->progressStagePercentage(50);

    // Defaulting to making this a local game
    g_NetworkManager.SetLocalGame(true);

    // Create a new session with all the players that were in the old one
    int localUsersMask = 0;
    char numLocalPlayers = 0;
    for (unsigned int index = 0; index < XUSER_MAX_COUNT; ++index) {
        if (PlatformProfile.IsSignedIn(index) &&
            pMinecraft->localplayers[index] != nullptr) {
            numLocalPlayers++;
            localUsersMask |= GetLocalPlayerMask(index);
        }
    }

    s_pPlatformNetworkManager->_HostGame(localUsersMask);

    pMinecraft->progressRenderer->progressStagePercentage(75);

    // Wait for all the local players to rejoin the session
    while (g_NetworkManager.GetPlayerCount() < numLocalPlayers) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Restore the network player of all the server players that are local
    if (pServer != nullptr) {
        for (unsigned int index = 0; index < XUSER_MAX_COUNT; ++index) {
            if (PlatformProfile.IsSignedIn(index) &&
                pMinecraft->localplayers[index] != nullptr) {
                PlayerUID localPlayerXuid =
                    pMinecraft->localplayers[index]->getXuid();

                PlayerList* players = pServer->getPlayers();
                for (auto it = players->players.begin();
                     it < players->players.end(); ++it) {
                    std::shared_ptr<ServerPlayer> servPlayer = *it;
                    if (servPlayer->getXuid() == localPlayerXuid) {
                        servPlayer->connection->connection->getSocket()
                            ->setPlayer(
                                g_NetworkManager.GetLocalPlayerByUserIndex(
                                    index));
                    }
                }

                // Player might have a pending connection
                if (pMinecraft->m_pendingLocalConnections[index] != nullptr) {
                    // Update the network player
                    pMinecraft->m_pendingLocalConnections[index]
                        ->getConnection()
                        ->getSocket()
                        ->setPlayer(
                            g_NetworkManager.GetLocalPlayerByUserIndex(index));
                } else if (pMinecraft->m_connectionFailed[index] &&
                           (pMinecraft->m_connectionFailedReason[index] ==
                            DisconnectPacket::
                                eDisconnect_ConnectionCreationFailed)) {
                    pMinecraft->removeLocalPlayerIdx(index);
                }
            }
        }
    }

    pMinecraft->progressRenderer->progressStagePercentage(100);

    // Make sure that we have transitioned through any joining/creating stages
    // so we're actually ready to set to play
    while (!s_pPlatformNetworkManager->IsReadyToPlayOrIdle()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    s_pPlatformNetworkManager->_StartGame();

    // Wait until the message box has been closed
    while (ui.IsSceneInStack(XUSER_INDEX_ANY, eUIScene_MessageBox)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Start the game again
    app.SetGameStarted(true);
    MinecraftServer::getInstance()->queueServerAction(
        minecraft::server::PauseServer{false});
    app.SetChangingSessionType(false);
    app.SetReallyChangingSessionType(false);

    return 0;
}

void CGameNetworkManager::SystemFlagSet(INetworkPlayer* pNetworkPlayer,
                                        int index) {
    s_pPlatformNetworkManager->SystemFlagSet(pNetworkPlayer, index);
}

bool CGameNetworkManager::SystemFlagGet(INetworkPlayer* pNetworkPlayer,
                                        int index) {
    return s_pPlatformNetworkManager->SystemFlagGet(pNetworkPlayer, index);
}

std::string CGameNetworkManager::GatherStats() {
    return s_pPlatformNetworkManager->GatherStats();
}

void CGameNetworkManager::renderQueueMeter() {}

std::string CGameNetworkManager::GatherRTTStats() {
    return s_pPlatformNetworkManager->GatherRTTStats();
}

void CGameNetworkManager::StateChange_AnyToHosting() {
    app.DebugPrintf("Disabling Guest Signin\n");
    XEnableGuestSignin(false);
    Minecraft::GetInstance()->clearPendingClientTextureRequests();
}

void CGameNetworkManager::StateChange_AnyToJoining() {
    app.DebugPrintf("Disabling Guest Signin\n");
    XEnableGuestSignin(false);
    Minecraft::GetInstance()->clearPendingClientTextureRequests();

    ConnectionProgressParams* param = new ConnectionProgressParams();
    param->iPad = PlatformProfile.GetPrimaryPad();
    param->stringId = -1;
    param->showTooltips = false;
    param->setFailTimer = true;
    param->timerTime = CONNECTING_PROGRESS_CHECK_TIME;

    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                       eUIScene_ConnectingProgress, param);
}

void CGameNetworkManager::StateChange_JoiningToIdle(
    IPlatformNetwork::eJoinFailedReason reason) {
    DisconnectPacket::eDisconnectReason disconnectReason;
    switch (reason) {
        case IPlatformNetwork::JOIN_FAILED_SERVER_FULL:
            disconnectReason = DisconnectPacket::eDisconnect_ServerFull;
            break;
        case IPlatformNetwork::JOIN_FAILED_INSUFFICIENT_PRIVILEGES:
            disconnectReason =
                DisconnectPacket::eDisconnect_NoMultiplayerPrivilegesJoin;
            app.SetAction(PlatformProfile.GetPrimaryPad(),
                          eAppAction_FailedToJoinNoPrivileges);
            break;
        default:
            disconnectReason =
                DisconnectPacket::eDisconnect_ConnectionCreationFailed;
            break;
    };
    Minecraft::GetInstance()->connectionDisconnected(
        PlatformProfile.GetPrimaryPad(), disconnectReason);
}

void CGameNetworkManager::StateChange_AnyToStarting() {
    if (!g_NetworkManager.IsHost()) {
        LoadingInputParams* loadingParams = new LoadingInputParams();
        loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
        loadingParams->lpParam = nullptr;

        UIFullscreenProgressCompletionData* completionData =
            new UIFullscreenProgressCompletionData();
        completionData->bShowBackground = true;
        completionData->bShowLogo = true;
        completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
        completionData->iPad = PlatformProfile.GetPrimaryPad();
        loadingParams->completionData = completionData;

        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                           eUIScene_FullscreenProgress, loadingParams);
    }
}

void CGameNetworkManager::StateChange_AnyToEnding(bool bStateWasPlaying) {
    // Kick off a stats write for players that are signed into LIVE, if this is
    // a local game
    if (bStateWasPlaying && g_NetworkManager.IsLocalGame()) {
        for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
            INetworkPlayer* pNetworkPlayer =
                g_NetworkManager.GetLocalPlayerByUserIndex(i);
            if (pNetworkPlayer != nullptr && PlatformProfile.IsSignedIn(i)) {
                app.DebugPrintf(
                    "Stats save for an offline game for the player at index "
                    "%d\n",
                    i);
                Minecraft::GetInstance()->forceStatsSave(
                    pNetworkPlayer->GetUserIndex());
            }
        }
    }

    Minecraft::GetInstance()->gui->clearMessages();

    if (!g_NetworkManager.IsHost() && !g_NetworkManager.IsLeavingGame()) {
        // 4J Stu - If the host is saving then it might take a while to quite
        // the session, so do it ourself
        // m_bLeavingGame = true;

        // The host has notified that the game is about to end
        if (app.GetDisconnectReason() == DisconnectPacket::eDisconnect_None)
            app.SetDisconnectReason(DisconnectPacket::eDisconnect_Quitting);
        app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_ExitWorld,
                      (void*)true);
    }
}

void CGameNetworkManager::StateChange_AnyToIdle() {
    app.DebugPrintf("Enabling Guest Signin\n");
    XEnableGuestSignin(true);
    // Reset this here so that we can search for games again
    // 4J Stu - If we are changing session type there is a race between that
    // thread setting the game to local, and this setting it to not local
    if (!app.GetChangingSessionType()) g_NetworkManager.SetLocalGame(false);
}

void CGameNetworkManager::CreateSocket(INetworkPlayer* pNetworkPlayer,
                                       bool localPlayer) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    Socket* socket = nullptr;
    std::shared_ptr<MultiplayerLocalPlayer> mpPlayer =
        pMinecraft->localplayers[pNetworkPlayer->GetUserIndex()];
    if (localPlayer && mpPlayer != nullptr && mpPlayer->connection != nullptr) {
        // If we already have a MultiplayerLocalPlayer here then we are doing a
        // session type change
        socket = mpPlayer->connection->getSocket();

        // Pair this socket and network player
        pNetworkPlayer->SetSocket(socket);
        if (socket) {
            socket->setPlayer(pNetworkPlayer);
        }
    } else {
        socket = new Socket(pNetworkPlayer, g_NetworkManager.IsHost(),
                            g_NetworkManager.IsHost() && localPlayer);
        pNetworkPlayer->SetSocket(socket);

        // 4J Stu - May be other states we want to accept aswell
        // Add this user to the game server if the game is started already
        if (g_NetworkManager.IsHost() && g_NetworkManager.IsInGameplay()) {
            Socket::addIncomingSocket(socket);
        }

        // If this is a local player and we are already in the game, we need to
        // setup a local connection and log the player in to the game server
        if (localPlayer && g_NetworkManager.IsInGameplay()) {
            int idx = pNetworkPlayer->GetUserIndex();
            app.DebugPrintf("Creating new client connection for idx: %d\n",
                            idx);

            ClientConnection* connection;
            connection = new ClientConnection(pMinecraft, socket, idx);

            if (connection->createdOk) {
                connection->send(std::shared_ptr<PreLoginPacket>(
                    new PreLoginPacket(pNetworkPlayer->GetOnlineName())));
                pMinecraft->addPendingLocalConnection(idx, connection);
            } else {
                pMinecraft->connectionDisconnected(
                    idx,
                    DisconnectPacket::eDisconnect_ConnectionCreationFailed);
                delete connection;
                connection = nullptr;
            }
        }
    }
}

void CGameNetworkManager::CloseConnection(INetworkPlayer* pNetworkPlayer) {
    MinecraftServer* server = MinecraftServer::getInstance();
    if (server != nullptr) {
        PlayerList* players = server->getPlayers();
        if (players != nullptr) {
            players->closePlayerConnectionBySmallId(
                pNetworkPlayer->GetSmallId());
        }
    }
}

void CGameNetworkManager::PlayerJoining(INetworkPlayer* pNetworkPlayer) {
    if (g_NetworkManager
            .IsInGameplay())  // 4J-JEV: Wait to do this at StartNetworkGame if
                              // not in-game yet.
    {
        // 4J-JEV: Update RichPresence when a player joins the game.
        bool multiplayer = g_NetworkManager.GetPlayerCount() > 1,
             localgame = g_NetworkManager.IsLocalGame();
        for (int iPad = 0; iPad < XUSER_MAX_COUNT; ++iPad) {
            INetworkPlayer* pNetworkPlayer =
                g_NetworkManager.GetLocalPlayerByUserIndex(iPad);
            if (pNetworkPlayer == nullptr) continue;

            PlatformGame.SetRichPresenceContext(iPad, CONTEXT_GAME_STATE_BLANK);
            if (multiplayer) {
                if (localgame)
                    PlatformProfile.SetCurrentGameActivity(
                        iPad, CONTEXT_PRESENCE_MULTIPLAYEROFFLINE, false);
                else
                    PlatformProfile.SetCurrentGameActivity(
                        iPad, CONTEXT_PRESENCE_MULTIPLAYER, false);
            } else {
                if (localgame)
                    PlatformProfile.SetCurrentGameActivity(
                        iPad, CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE, false);
                else
                    PlatformProfile.SetCurrentGameActivity(
                        iPad, CONTEXT_PRESENCE_MULTIPLAYER_1P, false);
            }
        }
    }
}

void CGameNetworkManager::PlayerLeaving(INetworkPlayer* pNetworkPlayer) {
    if (pNetworkPlayer->IsLocal()) {
        PlatformProfile.SetCurrentGameActivity(pNetworkPlayer->GetUserIndex(),
                                               CONTEXT_PRESENCE_IDLE, false);
    }
}

void CGameNetworkManager::HostChanged() {
    // Disable host migration
    app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_ExitWorld,
                  (void*)true);
}

void CGameNetworkManager::WriteStats(INetworkPlayer* pNetworkPlayer) {
    Minecraft::GetInstance()->forceStatsSave(pNetworkPlayer->GetUserIndex());
}

void CGameNetworkManager::GameInviteReceived(int userIndex,
                                             const INVITE_INFO* pInviteInfo) {
    int localUsersMask = 0;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    int joiningUsers = 0;

    bool noPrivileges = false;
    for (unsigned int index = 0; index < XUSER_MAX_COUNT; ++index) {
        if (PlatformProfile.IsSignedIn(index)) {
            // 4J-PB we shouldn't bring any inactive players into the game,
            // except for the invited player (who may be an inactive player) 4J
            // Stu - If we are not in a game, then bring in all players signed
            // in
            if (index == userIndex ||
                pMinecraft->localplayers[index] != nullptr) {
                ++joiningUsers;
                if (!PlatformProfile.AllowedToPlayMultiplayer(index))
                    noPrivileges = true;
                localUsersMask |= GetLocalPlayerMask(index);
            }
        }
    }

    // Check if user-created content is allowed, as we cannot play multiplayer
    // if it's not
    bool noUGC = false;
    bool bContentRestricted = false;
    bool pccAllowed = true;
    bool pccFriendsAllowed = true;
    PlatformProfile.AllowedPlayerCreatedContent(PlatformProfile.GetPrimaryPad(),
                                                false, &pccAllowed,
                                                &pccFriendsAllowed);
    if (!pccAllowed && !pccFriendsAllowed) noUGC = true;

    if (noUGC) {
        int messageText = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
        if (joiningUsers > 1)
            messageText = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;

        ui.RequestUGCMessageBox(IDS_CONNECTION_FAILED, messageText,
                                XUSER_INDEX_ANY);
    } else if (noPrivileges) {
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_CONFIRM_OK;

        // 4J-PB - it's possible there is no primary pad here, when accepting an
        // invite from the dashboard
        // PlatformStorage.RequestMessageBox(
        // IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
        // IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT,
        // uiIDA,1,PlatformProfile.GetPrimaryPad(),nullptr,nullptr,
        // app.GetStringTable());
        ui.RequestErrorMessage(IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                               IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT, uiIDA, 1,
                               XUSER_INDEX_ANY);
    } else {
        if (!g_NetworkManager.IsInSession()) {
            HandleInviteWhenInMenus(userIndex, pInviteInfo);
        } else {
            app.DebugPrintf(
                "We are already in a multiplayer game...need to leave it\n");

            // 			JoinFromInviteData *joinData = new
            // JoinFromInviteData(); 			joinData->dwUserIndex =
            // dwUserIndex; 			joinData->dwLocalUsersMask =
            // dwLocalUsersMask; 			joinData->pInviteInfo =
            // pInviteInfo;

            // tell the app to process this
            { app.ProcessInvite(userIndex, localUsersMask, pInviteInfo); }
        }
    }
}

volatile bool waitHere = true;

void CGameNetworkManager::HandleInviteWhenInMenus(
    int userIndex, const INVITE_INFO* pInviteInfo) {
    // We are in the root menus somewhere

    {
        PlatformProfile.SetPrimaryPad(userIndex);

        // 4J Stu - If we accept an invite from the main menu before going to
        // play game we need to load the DLC These checks are done within the
        // StartInstallDLCProcess - (!app.DLCInstallProcessCompleted() &&
        // !app.DLCInstallPending()) app.StartInstallDLCProcess(dwUserIndex);
        app.StartInstallDLCProcess(userIndex);

        // 4J Stu - Fix for #10936 - MP Lab: TCR 001: Matchmaking: Player is
        // stuck in a soft-locked state after selecting the guest account when
        // prompted The locked profile should not be changed if we are in menus
        // as the main player might sign out in the sign-in ui
        // PlatformProfile.SetLockedProfile(-1);

        if (!app.IsLocalMultiplayerAvailable()) {
            bool noPrivileges =
                !PlatformProfile.AllowedToPlayMultiplayer(userIndex);

            if (noPrivileges) {
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                                       IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT,
                                       uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
            } else {
                PlatformProfile.SetLockedProfile(userIndex);
                PlatformProfile.SetPrimaryPad(userIndex);

                int localUsersMask = 0;
                localUsersMask |= GetLocalPlayerMask(userIndex);

                // If the player was signed in before selecting play, we'll not
                // have read the profile yet, so query the sign-in status to get
                // this to happen
                (void)PlatformProfile.QuerySigninStatus();

                // 4J-PB - clear any previous connection errors
                Minecraft::GetInstance()->clearConnectionFailed();

                g_NetworkManager.SetLocalGame(false);

                // change the minecraft player name
                Minecraft::GetInstance()->user->name =
                    PlatformProfile.GetGamertag(
                        PlatformProfile.GetPrimaryPad());

                bool success = g_NetworkManager.JoinGameFromInviteInfo(
                    userIndex, localUsersMask, pInviteInfo);
                if (!success) {
                    app.DebugPrintf("Failed joining game from invite\n");
                }
            }
        } else {
            // the FromInvite will make the lib decide how many panes to display
            // based on connected pads/signed in players
            SignInInfo info;
            info.Func = [pInviteInfo](bool bContinue, int pad) {
                return JoinFromInvite_SignInReturned(
                    const_cast<INVITE_INFO*>(pInviteInfo), bContinue, pad);
            };
            info.requireOnline = true;
            app.DebugPrintf("Using fullscreen layer\n");
            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_QuadrantSignin, &info, eUILayer_Alert,
                               eUIGroup_Fullscreen);
        }
    }
}

void CGameNetworkManager::AddLocalPlayerFailed(int idx,
                                               bool serverFull /* = false*/) {
    Minecraft::GetInstance()->connectionDisconnected(
        idx, serverFull
                 ? DisconnectPacket::eDisconnect_ServerFull
                 : DisconnectPacket::eDisconnect_ConnectionCreationFailed);
}

void CGameNetworkManager::HandleDisconnect(bool bLostRoomOnly) {
    int iPrimaryPlayer = g_NetworkManager.GetPrimaryPad();

    if ((g_NetworkManager.GetLockedProfile() != -1) && iPrimaryPlayer != -1 &&
        g_NetworkManager.IsInSession()) {
        m_bLastDisconnectWasLostRoomOnly = bLostRoomOnly;
        app.SetAction(iPrimaryPlayer, eAppAction_EthernetDisconnected);
    } else {
        m_bLastDisconnectWasLostRoomOnly = false;
    }
}

int CGameNetworkManager::GetPrimaryPad() {
    return PlatformProfile.GetPrimaryPad();
}

int CGameNetworkManager::GetLockedProfile() {
    return PlatformProfile.GetLockedProfile();
}

bool CGameNetworkManager::IsSignedInLive(int playerIdx) {
    return PlatformProfile.IsSignedInLive(playerIdx);
}

bool CGameNetworkManager::AllowedToPlayMultiplayer(int playerIdx) {
    return PlatformProfile.AllowedToPlayMultiplayer(playerIdx);
}

char* CGameNetworkManager::GetOnlineName(int playerIdx) {
    return PlatformProfile.GetGamertag(playerIdx);
}

void CGameNetworkManager::ServerReadyCreate(bool create) {
    m_hServerReadyEvent = (create ? (new C4JThread::Event) : nullptr);
}

void CGameNetworkManager::ServerReady() {
    if (m_hServerReadyEvent != nullptr) {
        m_hServerReadyEvent->set();
    } else {
        app.DebugPrintf(
            "[NET] Warning: ServerReady() called but m_hServerReadyEvent is "
            "nullptr\n");
    }
}

void CGameNetworkManager::ServerReadyWait() {
    if (m_hServerReadyEvent != nullptr) {
        m_hServerReadyEvent->waitForSignal(C4JThread::kInfiniteTimeout);
    } else {
        app.DebugPrintf(
            "[NET] Warning: ServerReadyWait() called but m_hServerReadyEvent "
            "is nullptr\n");
    }
}

void CGameNetworkManager::ServerReadyDestroy() {
    delete m_hServerReadyEvent;
    m_hServerReadyEvent = nullptr;
}

bool CGameNetworkManager::ServerReadyValid() {
    return (m_hServerReadyEvent != nullptr);
}

void CGameNetworkManager::ServerStoppedCreate(bool create) {
    m_hServerStoppedEvent = (create ? (new C4JThread::Event) : nullptr);
}

void CGameNetworkManager::ServerStopped() {
    if (m_hServerStoppedEvent != nullptr) {
        m_hServerStoppedEvent->set();
    } else {
        app.DebugPrintf(
            "[NET] Warning: ServerStopped() called but m_hServerStoppedEvent "
            "is nullptr\n");
    }
}

void CGameNetworkManager::ServerStoppedWait() {
    // If this is called from the main thread, then this won't be ticking
    // anything which can mean that the storage manager state can't progress.
    // This means that the server thread we are waiting on won't ever finish, as
    // it might be locked waiting for this to complete itself. Do some ticking
    // here then if this is the case.
    if (C4JThread::isMainThread()) {
        int result = C4JThread::WaitResult::Timeout;
        do {
            PlatformRenderer.StartFrame();
            result = m_hServerStoppedEvent->waitForSignal(20);
            // Tick some simple things
            PlatformProfile.Tick();
            PlatformStorage.Tick();
            PlatformInput.Tick();
            PlatformRenderer.Tick();
            ui.tick();
            ui.render();
            PlatformRenderer.Present();
        } while (result == C4JThread::WaitResult::Timeout);
    } else {
        if (m_hServerStoppedEvent != nullptr) {
            m_hServerStoppedEvent->waitForSignal(C4JThread::kInfiniteTimeout);
        } else {
            app.DebugPrintf(
                "[NET] Warning: ServerStoppedWait() called but "
                "m_hServerStoppedEvent is nullptr\n");
        }
    }
}

void CGameNetworkManager::ServerStoppedDestroy() {
    delete m_hServerStoppedEvent;
    m_hServerStoppedEvent = nullptr;
}

bool CGameNetworkManager::ServerStoppedValid() {
    return (m_hServerStoppedEvent != nullptr);
}

int CGameNetworkManager::GetJoiningReadyPercentage() {
    return s_pPlatformNetworkManager->GetJoiningReadyPercentage();
}

void CGameNetworkManager::FakeLocalPlayerJoined() {
    s_pPlatformNetworkManager->FakeLocalPlayerJoined();
}
