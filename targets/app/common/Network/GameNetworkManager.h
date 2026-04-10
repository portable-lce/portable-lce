#pragma once
#include <stdint.h>
// using namespace std;
#include <format>
#include <string>
#include <vector>
#if !defined(__linux__)
#include <qnet.h>
#endif
#include "minecraft/network/INetworkService.h"
#include "platform/C4JThread.h"
#include "platform/NetTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/network/IPlatformNetwork.h"
#include "platform/network/network.h"

class ClientConnection;
class Minecraft;
class FriendSessionInfo;
class INVITE_INFO;
class INetworkPlayer;

// This class implements the game-side interface to the networking system. As
// such, it is platform independent and may contain bits of game-side code where
// appropriate. It shouldn't ever reference any platform specifics of the
// network implementation (eg QNET), rather it should interface with an
// implementation of PlatformNetworkManager to provide this functionality.

class CGameNetworkManager : public ::minecraft::network::INetworkService {
public:
    CGameNetworkManager();
    // Misc high level flow

    typedef enum {
        JOINGAME_SUCCESS,
        JOINGAME_FAIL_GENERAL,
        JOINGAME_FAIL_SERVER_FULL
    } eJoinGameResult;

    void Initialise();
    void Terminate();
    void DoWork();
    bool _RunNetworkGame(void* lpParameter);
    bool StartNetworkGame(Minecraft* minecraft, void* lpParameter);
    int CorrectErrorIDS(int IDS);

    // Player management

    static int GetLocalPlayerMask(int playerIndex);
    int GetPlayerCount();
    int GetOnlinePlayerCount();
    bool AddLocalPlayerByUserIndex(int userIndex);
    bool RemoveLocalPlayerByUserIndex(int userIndex);
    INetworkPlayer* GetLocalPlayerByUserIndex(int userIndex);
    INetworkPlayer* GetPlayerByIndex(int playerIndex);
    INetworkPlayer* GetPlayerByXuid(PlayerUID xuid);
    INetworkPlayer* GetPlayerBySmallId(unsigned char smallId);
    std::string GetDisplayNameByGamertag(std::string gamertag);
    INetworkPlayer* GetHostPlayer();
    void RegisterPlayerChangedCallback(
        int iPad,
        std::function<void(INetworkPlayer* pPlayer, bool leaving)> callback);
    void UnRegisterPlayerChangedCallback(int iPad);
    void HandleSignInChange();
    bool ShouldMessageForFullSession();

    // State management

    bool IsInSession();
    bool IsInGameplay();
    bool IsLeavingGame();
    bool IsReadyToPlayOrIdle();

    // Hosting and game type

    bool SetLocalGame(bool isLocal);
    bool IsLocalGame();
    void SetPrivateGame(bool isPrivate);
    bool IsPrivateGame();
    void HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                  unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
                  unsigned char privateSlots = 0);
    bool IsHost();
    bool IsInStatsEnabledSession();

    // Client session discovery

    bool SessionHasSpace(unsigned int spaceRequired = 1);
    std::vector<FriendSessionInfo*>* GetSessionList(int iPad, int localPlayers,
                                                    bool partyOnly);
    bool GetGameSessionInfo(int iPad, SessionID sessionId,
                            FriendSessionInfo* foundSession);
    void SetSessionsUpdatedCallback(std::function<void()> callback);
    void GetFullFriendSessionInfo(FriendSessionInfo* foundSession,
                                  std::function<void(bool success)> callback);
    void ForceFriendsSessionRefresh();

    // Session joining and leaving

    bool JoinGameFromInviteInfo(int userIndex, int userMask,
                                const INVITE_INFO* pInviteInfo);
    eJoinGameResult JoinGame(FriendSessionInfo* searchResult,
                             int localUsersMask);
    static void CancelJoinGame(
        void* lpParam);  // Not part of the shared interface
    bool LeaveGame(bool bMigrateHost);
    static int JoinFromInvite_SignInReturned(void* pParam, bool bContinue,
                                             int iPad);
    void UpdateAndSetGameSessionData(
        INetworkPlayer* pNetworkPlayerLeaving = nullptr);
    void SendInviteGUI(int iPad);
    void ResetLeavingGame();

    // Threads

    bool IsNetworkThreadRunning();
    static int RunNetworkGameThreadProc(void* lpParameter);
    static int ServerThreadProc(void* lpParameter);
    static int ExitAndJoinFromInviteThreadProc(void* lpParam);

    static void _LeaveGame();
    static int ChangeSessionTypeThreadProc(void* lpParam);

    // System flags

    void SystemFlagSet(INetworkPlayer* pNetworkPlayer, int index);
    bool SystemFlagGet(INetworkPlayer* pNetworkPlayer, int index);

    // Events

    void ServerReadyCreate(
        bool create);           // Create the signal (or set to nullptr)
    void ServerReady();         // Signal that we are ready
    void ServerReadyWait();     // Wait for the signal
    void ServerReadyDestroy();  // Destroy signal
    bool ServerReadyValid();    // Is non-nullptr

    void ServerStoppedCreate(bool create);  // Create the signal
    void ServerStopped();                   // Signal that we are ready
    void ServerStoppedWait();               // Wait for the signal
    void ServerStoppedDestroy();            // Destroy signal
    bool ServerStoppedValid();              // Is non-nullptr

    // Debug output

    std::string GatherStats();
    void renderQueueMeter();
    std::string GatherRTTStats();

    // GUI debug output

    // Used for debugging output
    static const int messageQueue_length = 512;
    static int64_t messageQueue[messageQueue_length];
    static const int byteQueue_length = 512;
    static int64_t byteQueue[byteQueue_length];
    static int messageQueuePos;

    // Methods called from PlatformNetworkManager
    // 4jcraft: made these public, we can't friend class StubPlatformNetwork
    // here like before because that would be naming an opaque platform backend
    // class, plus this API is shit so i dont care
public:
    void StateChange_AnyToHosting();
    void StateChange_AnyToJoining();
    void StateChange_JoiningToIdle(IPlatformNetwork::eJoinFailedReason reason);
    void StateChange_AnyToStarting();
    void StateChange_AnyToEnding(bool bStateWasPlaying);
    void StateChange_AnyToIdle();
    void CreateSocket(INetworkPlayer* pNetworkPlayer, bool localPlayer);
    void CloseConnection(INetworkPlayer* pNetworkPlayer);
    void PlayerJoining(INetworkPlayer* pNetworkPlayer);
    void PlayerLeaving(INetworkPlayer* pNetworkPlayer);
    void HostChanged();
    void WriteStats(INetworkPlayer* pNetworkPlayer);
    void GameInviteReceived(int userIndex, const INVITE_INFO* pInviteInfo);
    void HandleInviteWhenInMenus(int userIndex, const INVITE_INFO* pInviteInfo);
    void AddLocalPlayerFailed(int idx, bool serverFull = false);
    void HandleDisconnect(bool bLostRoomOnly);

    int GetPrimaryPad();
    int GetLockedProfile();
    bool IsSignedInLive(int playerIdx);
    bool AllowedToPlayMultiplayer(int playerIdx);
    char* GetOnlineName(int playerIdx);

    C4JThread::Event* m_hServerStoppedEvent;
    C4JThread::Event* m_hServerReadyEvent;
    bool m_bInitialised;

private:
    static IPlatformNetwork* s_pPlatformNetworkManager;
    bool m_bNetworkThreadRunning;
    int GetJoiningReadyPercentage();
    bool m_bLastDisconnectWasLostRoomOnly;
    bool m_bFullSessionMessageOnNextSessionChange;

public:
    void FakeLocalPlayerJoined();  // Temporary method whilst we don't have real
                                   // networking to make this happen
};

extern CGameNetworkManager g_NetworkManager;
