#pragma once
#include <stdint.h>
// using namespace std;
#include <string>
#include <vector>

#include "StubNetworkPlayer.h"
#include "minecraft/client/model/SkinBox.h"
#include "platform/PlatformTypes.h"
#include "platform/XboxStubs.h"
#include "platform/network/IPlatformNetwork.h"
#include "platform/network/NetTypes.h"
#include "platform/network/network.h"

class CGameNetworkManager;
class INetworkPlayer;

class StubPlatformNetwork : public IPlatformNetwork {
public:
    static StubNetworkPlayer m_players[4];

    bool Initialise(CGameNetworkManager* pGameNetworkManager,
                    int flagIndexSize);
    void Terminate();
    int GetJoiningReadyPercentage();
    int CorrectErrorIDS(int IDS);

    void DoWork();
    int GetPlayerCount();
    int GetOnlinePlayerCount();
    int GetLocalPlayerMask(int playerIndex);
    bool AddLocalPlayerByUserIndex(int userIndex);
    bool RemoveLocalPlayerByUserIndex(int userIndex);
    INetworkPlayer* GetLocalPlayerByUserIndex(int userIndex);
    INetworkPlayer* GetPlayerByIndex(int playerIndex);
    INetworkPlayer* GetPlayerByXuid(PlayerUID xuid);
    INetworkPlayer* GetPlayerBySmallId(unsigned char smallId);
    bool ShouldMessageForFullSession();

    INetworkPlayer* GetHostPlayer();
    bool IsHost();
    bool JoinGameFromInviteInfo(int userIndex, int userMask,
                                const INVITE_INFO* pInviteInfo);
    bool LeaveGame(bool bMigrateHost);

    bool IsInSession();
    bool IsInGameplay();
    bool IsReadyToPlayOrIdle();
    bool IsInStatsEnabledSession();
    bool SessionHasSpace(unsigned int spaceRequired = 1);
    void SendInviteGUI(int quadrant);
    bool IsAddingPlayer();

    void HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                  unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
                  unsigned char privateSlots = 0);
    int JoinGame(FriendSessionInfo* searchResult, int localUsersMask,
                 int primaryUserIndex);
    bool SetLocalGame(bool isLocal);
    bool IsLocalGame() { return m_bIsOfflineGame; }
    void SetPrivateGame(bool isPrivate);
    bool IsPrivateGame() { return m_bIsPrivateGame; }
    bool IsLeavingGame() { return m_bLeavingGame; }
    void ResetLeavingGame() { m_bLeavingGame = false; }

    void RegisterPlayerChangedCallback(
        int iPad,
        std::function<void(INetworkPlayer* pPlayer, bool leaving)> callback);
    void UnRegisterPlayerChangedCallback(int iPad);

    void HandleSignInChange();

    bool _RunNetworkGame();

private:
    bool isSystemPrimaryPlayer(INetworkPlayer* pQNetPlayer);
    bool _LeaveGame(bool bMigrateHost, bool bLeaveRoom);
    void _HostGame(int dwUsersMask,
                   unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
                   unsigned char privateSlots = 0);
    bool _StartGame();

    void* m_notificationListener;

    std::vector<INetworkPlayer*>
        m_machineQNetPrimaryPlayers;  // collection of players that we deem to
                                      // be the main one for that system

    bool m_bLeavingGame;
    bool m_bLeaveGameOnTick;
    bool m_migrateHostOnLeave;
    bool m_bHostChanged;

    bool m_bIsOfflineGame;
    bool m_bIsPrivateGame;
    int m_flagIndexSize;

    // This is only maintained by the host, and is not valid on client machines
    GameSessionData m_hostGameSessionData;
    CGameNetworkManager* m_pGameNetworkManager;

public:
    void UpdateAndSetGameSessionData(
        INetworkPlayer* pNetworkPlayerLeaving = nullptr);

private:
    std::function<void(INetworkPlayer* pPlayer, bool leaving)>
        playerChangedCallback[XUSER_MAX_COUNT];

    bool RemoveLocalPlayer(INetworkPlayer* pNetworkPlayer);

    // Things for handling per-system flags
    class PlayerFlags {
    public:
        INetworkPlayer* m_pNetworkPlayer;
        unsigned char* flags;
        unsigned int count;
        PlayerFlags(INetworkPlayer* pNetworkPlayer, unsigned int count);
        ~PlayerFlags();
    };
    std::vector<PlayerFlags*> m_playerFlags;
    void SystemFlagAddPlayer(INetworkPlayer* pNetworkPlayer);
    void SystemFlagRemovePlayer(INetworkPlayer* pNetworkPlayer);
    void SystemFlagReset();

public:
    void SystemFlagSet(INetworkPlayer* pNetworkPlayer, int index);
    bool SystemFlagGet(INetworkPlayer* pNetworkPlayer, int index);

public:
    std::string GatherStats();
    std::string GatherRTTStats();

private:
    std::vector<FriendSessionInfo*> friendsSessions[XUSER_MAX_COUNT];
    int m_searchResultsCount[XUSER_MAX_COUNT];
    int m_lastSearchStartTime[XUSER_MAX_COUNT];

    std::function<void()> m_SessionsUpdatedCallback;

    void TickSearch();
    void SearchForGames();

    void SetSearchResultsReady(int resultCount = 0);

    std::vector<INetworkPlayer*> currentNetworkPlayers;
    INetworkPlayer* addNetworkPlayer(INetworkPlayer* pQNetPlayer);
    void removeNetworkPlayer(INetworkPlayer* pQNetPlayer);
    static INetworkPlayer* getNetworkPlayer(INetworkPlayer* pQNetPlayer);

    void SetSessionTexturePackParentId(int id);
    void SetSessionSubTexturePackId(int id);
    void Notify(int ID, uintptr_t Param);

public:
    std::vector<FriendSessionInfo*>* GetSessionList(int iPad, int localPlayers,
                                                    bool partyOnly);
    bool GetGameSessionInfo(int iPad, SessionID sessionId,
                            FriendSessionInfo* foundSession);
    void SetSessionsUpdatedCallback(std::function<void()> callback);
    void GetFullFriendSessionInfo(FriendSessionInfo* foundSession,
                                  std::function<void(bool success)> callback);
    void ForceFriendsSessionRefresh();

private:
    void NotifyPlayerJoined(INetworkPlayer* pQNetPlayer);
};