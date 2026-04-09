#pragma once
#include <stdint.h>
// using namespace std;
#include <string>
#include <vector>

#include "platform/network/IPlatformNetwork.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "minecraft/network/platform/SessionInfo.h"
#include "platform/C4JThread.h"
#include "platform/NetTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/XboxStubs.h"

class C4JThread;
class CGameNetworkManager;
class INetworkPlayer;

class IPlatformNetworkStub : public IPlatformNetwork {
    friend class CGameNetworkManager;

public:
    virtual bool Initialise(CGameNetworkManager* pGameNetworkManager,
                            int flagIndexSize);
    virtual void Terminate();
    virtual int GetJoiningReadyPercentage();
    virtual int CorrectErrorIDS(int IDS);

    virtual void DoWork();
    virtual int GetPlayerCount();
    virtual int GetOnlinePlayerCount();
    virtual int GetLocalPlayerMask(int playerIndex);
    virtual bool AddLocalPlayerByUserIndex(int userIndex);
    virtual bool RemoveLocalPlayerByUserIndex(int userIndex);
    virtual INetworkPlayer* GetLocalPlayerByUserIndex(int userIndex);
    virtual INetworkPlayer* GetPlayerByIndex(int playerIndex);
    virtual INetworkPlayer* GetPlayerByXuid(PlayerUID xuid);
    virtual INetworkPlayer* GetPlayerBySmallId(unsigned char smallId);
    virtual bool ShouldMessageForFullSession();

    virtual INetworkPlayer* GetHostPlayer();
    virtual bool IsHost();
    virtual bool JoinGameFromInviteInfo(int userIndex, int userMask,
                                        const INVITE_INFO* pInviteInfo);
    virtual bool LeaveGame(bool bMigrateHost);

    virtual bool IsInSession();
    virtual bool IsInGameplay();
    virtual bool IsReadyToPlayOrIdle();
    virtual bool IsInStatsEnabledSession();
    virtual bool SessionHasSpace(unsigned int spaceRequired = 1);
    virtual void SendInviteGUI(int quadrant);
    virtual bool IsAddingPlayer();

    virtual void HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                          unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
                          unsigned char privateSlots = 0);
    virtual int JoinGame(FriendSessionInfo* searchResult, int localUsersMask,
                         int primaryUserIndex);
    virtual bool SetLocalGame(bool isLocal);
    virtual bool IsLocalGame() { return m_bIsOfflineGame; }
    virtual void SetPrivateGame(bool isPrivate);
    virtual bool IsPrivateGame() { return m_bIsPrivateGame; }
    virtual bool IsLeavingGame() { return m_bLeavingGame; }
    virtual void ResetLeavingGame() { m_bLeavingGame = false; }

    virtual void RegisterPlayerChangedCallback(
        int iPad,
        std::function<void(INetworkPlayer* pPlayer, bool leaving)> callback);
    virtual void UnRegisterPlayerChangedCallback(int iPad);

    virtual void HandleSignInChange();

    virtual bool _RunNetworkGame();

private:
    bool isSystemPrimaryPlayer(IQNetPlayer* pQNetPlayer);
    virtual bool _LeaveGame(bool bMigrateHost, bool bLeaveRoom);
    virtual void _HostGame(
        int dwUsersMask, unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
        unsigned char privateSlots = 0);
    virtual bool _StartGame();

    IQNet* m_pIQNet;  // pointer to QNet interface

    void* m_notificationListener;

    std::vector<IQNetPlayer*>
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
    virtual void UpdateAndSetGameSessionData(
        INetworkPlayer* pNetworkPlayerLeaving = nullptr);

private:
    std::function<void(INetworkPlayer* pPlayer, bool leaving)>
        playerChangedCallback[XUSER_MAX_COUNT];

    static int RemovePlayerOnSocketClosedThreadProc(void* lpParam);
    virtual bool RemoveLocalPlayer(INetworkPlayer* pNetworkPlayer);

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
    virtual void SystemFlagSet(INetworkPlayer* pNetworkPlayer, int index);
    virtual bool SystemFlagGet(INetworkPlayer* pNetworkPlayer, int index);

    // For telemetry
private:
    float m_lastPlayerEventTimeStart;

public:
    std::string GatherStats();
    std::string GatherRTTStats();

private:
    std::vector<FriendSessionInfo*> friendsSessions[XUSER_MAX_COUNT];
    int m_searchResultsCount[XUSER_MAX_COUNT];
    int m_lastSearchStartTime[XUSER_MAX_COUNT];

    // The results that will be filled in with the current search
    XSESSION_SEARCHRESULT_HEADER* m_pSearchResults[XUSER_MAX_COUNT];
    XNQOS* m_pQoSResult[XUSER_MAX_COUNT];

    // The results from the previous search, which are currently displayed in
    // the game
    XSESSION_SEARCHRESULT_HEADER* m_pCurrentSearchResults[XUSER_MAX_COUNT];
    XNQOS* m_pCurrentQoSResult[XUSER_MAX_COUNT];
    int m_currentSearchResultsCount[XUSER_MAX_COUNT];

    int m_lastSearchPad;
    bool m_bSearchResultsReady;
    bool m_bSearchPending;
    std::function<void()> m_SessionsUpdatedCallback;

    C4JThread* m_SearchingThread;

    void TickSearch();
    void SearchForGames();
    static int SearchForGamesThreadProc(void* lpParameter);

    void SetSearchResultsReady(int resultCount = 0);

    std::vector<INetworkPlayer*> currentNetworkPlayers;
    INetworkPlayer* addNetworkPlayer(IQNetPlayer* pQNetPlayer);
    void removeNetworkPlayer(IQNetPlayer* pQNetPlayer);
    static INetworkPlayer* getNetworkPlayer(IQNetPlayer* pQNetPlayer);

    virtual void SetSessionTexturePackParentId(int id);
    virtual void SetSessionSubTexturePackId(int id);
    virtual void Notify(int ID, uintptr_t Param);

public:
    virtual std::vector<FriendSessionInfo*>* GetSessionList(int iPad,
                                                            int localPlayers,
                                                            bool partyOnly);
    virtual bool GetGameSessionInfo(int iPad, SessionID sessionId,
                                    FriendSessionInfo* foundSession);
    virtual void SetSessionsUpdatedCallback(std::function<void()> callback);
    virtual void GetFullFriendSessionInfo(
        FriendSessionInfo* foundSession,
        std::function<void(bool success)> callback);
    virtual void ForceFriendsSessionRefresh();

private:
    void NotifyPlayerJoined(IQNetPlayer* pQNetPlayer);

    void FakeLocalPlayerJoined() {
        NotifyPlayerJoined(m_pIQNet->GetLocalPlayerByUserIndex(0));
    }
};
