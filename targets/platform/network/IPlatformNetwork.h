#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "platform/NetTypes.h"
#include "platform/PlatformTypes.h"

#ifndef VER_NETWORK
#define VER_NETWORK 560
#endif
#define MINECRAFT_NET_VERSION VER_NETWORK

class INetworkPlayer;
class CGameNetworkManager;
struct FriendSessionInfo;

struct SearchForGamesData {
    unsigned int sessionIDCount;
    XSESSION_SEARCHRESULT_HEADER* searchBuffer;
    XNQOS** ppQos;
    SessionID* sessionIDList;
    XOVERLAPPED* pOverlapped;
};

class IPlatformNetwork {
public:
    enum eJoinFailedReason {
        JOIN_FAILED_SERVER_FULL,
        JOIN_FAILED_INSUFFICIENT_PRIVILEGES,
        JOIN_FAILED_NONSPECIFIC,
    };

    virtual ~IPlatformNetwork() = default;

    // Lifecycle
    virtual bool Initialise(CGameNetworkManager* pGameNetworkManager,
                            int flagIndexSize) = 0;
    virtual void Terminate() = 0;
    virtual void DoWork() = 0;
    [[nodiscard]] virtual int GetJoiningReadyPercentage() = 0;
    [[nodiscard]] virtual int CorrectErrorIDS(int IDS) = 0;

    // Players
    [[nodiscard]] virtual int GetPlayerCount() = 0;
    [[nodiscard]] virtual int GetOnlinePlayerCount() = 0;
    [[nodiscard]] virtual int GetLocalPlayerMask(int playerIndex) = 0;
    virtual bool AddLocalPlayerByUserIndex(int userIndex) = 0;
    virtual bool RemoveLocalPlayerByUserIndex(int userIndex) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetLocalPlayerByUserIndex(
        int userIndex) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetPlayerByIndex(int playerIndex) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetPlayerByXuid(PlayerUID xuid) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetPlayerBySmallId(
        unsigned char smallId) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetHostPlayer() = 0;
    [[nodiscard]] virtual bool ShouldMessageForFullSession() = 0;

    // Session state
    [[nodiscard]] virtual bool IsHost() = 0;
    virtual bool JoinGameFromInviteInfo(int userIndex, int userMask,
                                        const INVITE_INFO* pInviteInfo) = 0;
    virtual bool LeaveGame(bool bMigrateHost) = 0;
    [[nodiscard]] virtual bool IsInSession() = 0;
    [[nodiscard]] virtual bool IsInGameplay() = 0;
    [[nodiscard]] virtual bool IsReadyToPlayOrIdle() = 0;
    [[nodiscard]] virtual bool IsInStatsEnabledSession() = 0;
    [[nodiscard]] virtual bool SessionHasSpace(
        unsigned int spaceRequired = 1) = 0;
    virtual void SendInviteGUI(int quadrant) = 0;
    [[nodiscard]] virtual bool IsAddingPlayer() = 0;

    // Hosting / joining
    virtual void HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                          unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
                          unsigned char privateSlots = 0) = 0;
    virtual int JoinGame(FriendSessionInfo* searchResult, int dwLocalUsersMask,
                         int dwPrimaryUserIndex) = 0;
    virtual void CancelJoinGame() {}
    virtual bool SetLocalGame(bool isLocal) = 0;
    [[nodiscard]] virtual bool IsLocalGame() = 0;
    virtual void SetPrivateGame(bool isPrivate) = 0;
    [[nodiscard]] virtual bool IsPrivateGame() = 0;
    [[nodiscard]] virtual bool IsLeavingGame() = 0;
    virtual void ResetLeavingGame() = 0;

    // Callbacks
    virtual void RegisterPlayerChangedCallback(
        int iPad, std::function<void(INetworkPlayer* pPlayer, bool leaving)>
                      callback) = 0;
    virtual void UnRegisterPlayerChangedCallback(int iPad) = 0;

    virtual void HandleSignInChange() = 0;

    // Game loop
    virtual bool _RunNetworkGame() = 0;
    virtual bool _LeaveGame(bool bMigrateHost, bool bLeaveRoom) = 0;
    virtual void _HostGame(
        int usersMask, unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
        unsigned char privateSlots = 0) = 0;
    virtual bool _StartGame() = 0;

    // Session data
    virtual void UpdateAndSetGameSessionData(
        INetworkPlayer* pNetworkPlayerLeaving = nullptr) = 0;
    virtual bool RemoveLocalPlayer(INetworkPlayer* pNetworkPlayer) = 0;

    // System flags
    virtual void SystemFlagSet(INetworkPlayer* pNetworkPlayer, int index) = 0;
    [[nodiscard]] virtual bool SystemFlagGet(INetworkPlayer* pNetworkPlayer,
                                             int index) = 0;

    // Stats
    [[nodiscard]] virtual std::string GatherStats() = 0;
    [[nodiscard]] virtual std::string GatherRTTStats() = 0;

    // Session internals
    virtual void SetSessionTexturePackParentId(int id) = 0;
    virtual void SetSessionSubTexturePackId(int id) = 0;
    virtual void Notify(int ID, uintptr_t Param) = 0;

    // Session list
    [[nodiscard]] virtual std::vector<FriendSessionInfo*>* GetSessionList(
        int iPad, int localPlayers, bool partyOnly) = 0;
    [[nodiscard]] virtual bool GetGameSessionInfo(
        int iPad, SessionID sessionId, FriendSessionInfo* foundSession) = 0;
    virtual void SetSessionsUpdatedCallback(std::function<void()> callback) = 0;
    virtual void GetFullFriendSessionInfo(
        FriendSessionInfo* foundSession,
        std::function<void(bool success)> callback) = 0;
    virtual void ForceFriendsSessionRefresh() = 0;

    virtual void FakeLocalPlayerJoined() {}
};
