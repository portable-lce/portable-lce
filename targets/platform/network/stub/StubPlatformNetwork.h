#pragma once

#include "platform/network/IPlatformNetwork.h"

// True no-op platform network backend. Returns false / 0 / nullptr for
// every operation. The composition root can substitute a real backend
// (QNet, Steam, EOS, custom) at link time. Used as the platform default
// so consumers can call PlatformNetwork.* without nullptr checks.

class StubPlatformNetwork : public IPlatformNetwork {
public:
    bool Initialise(CGameNetworkManager* /*pGameNetworkManager*/,
                    int /*flagIndexSize*/) override {
        return true;
    }
    void Terminate() override {}
    void DoWork() override {}
    [[nodiscard]] int GetJoiningReadyPercentage() override { return 100; }
    [[nodiscard]] int CorrectErrorIDS(int IDS) override { return IDS; }

    [[nodiscard]] int GetPlayerCount() override { return 0; }
    [[nodiscard]] int GetOnlinePlayerCount() override { return 0; }
    [[nodiscard]] int GetLocalPlayerMask(int /*playerIndex*/) override {
        return 0;
    }
    bool AddLocalPlayerByUserIndex(int /*userIndex*/) override { return false; }
    bool RemoveLocalPlayerByUserIndex(int /*userIndex*/) override {
        return false;
    }
    [[nodiscard]] INetworkPlayer* GetLocalPlayerByUserIndex(
        int /*userIndex*/) override {
        return nullptr;
    }
    [[nodiscard]] INetworkPlayer* GetPlayerByIndex(
        int /*playerIndex*/) override {
        return nullptr;
    }
    [[nodiscard]] INetworkPlayer* GetPlayerByXuid(PlayerUID /*xuid*/) override {
        return nullptr;
    }
    [[nodiscard]] INetworkPlayer* GetPlayerBySmallId(
        unsigned char /*smallId*/) override {
        return nullptr;
    }
    [[nodiscard]] INetworkPlayer* GetHostPlayer() override { return nullptr; }
    [[nodiscard]] bool ShouldMessageForFullSession() override { return false; }

    [[nodiscard]] bool IsHost() override { return true; }
    bool JoinGameFromInviteInfo(int /*userIndex*/, int /*userMask*/,
                                const INVITE_INFO* /*pInviteInfo*/) override {
        return false;
    }
    bool LeaveGame(bool /*bMigrateHost*/) override { return true; }
    [[nodiscard]] bool IsInSession() override { return false; }
    [[nodiscard]] bool IsInGameplay() override { return false; }
    [[nodiscard]] bool IsReadyToPlayOrIdle() override { return true; }
    [[nodiscard]] bool IsInStatsEnabledSession() override { return false; }
    [[nodiscard]] bool SessionHasSpace(
        unsigned int /*spaceRequired*/) override {
        return true;
    }
    void SendInviteGUI(int /*quadrant*/) override {}
    [[nodiscard]] bool IsAddingPlayer() override { return false; }

    void HostGame(int /*localUsersMask*/, bool /*bOnlineGame*/,
                  bool /*bIsPrivate*/, unsigned char /*publicSlots*/,
                  unsigned char /*privateSlots*/) override {}
    int JoinGame(FriendSessionInfo* /*searchResult*/, int /*dwLocalUsersMask*/,
                 int /*dwPrimaryUserIndex*/) override {
        return 0;
    }
    bool SetLocalGame(bool /*isLocal*/) override { return true; }
    [[nodiscard]] bool IsLocalGame() override { return true; }
    void SetPrivateGame(bool /*isPrivate*/) override {}
    [[nodiscard]] bool IsPrivateGame() override { return false; }
    [[nodiscard]] bool IsLeavingGame() override { return false; }
    void ResetLeavingGame() override {}

    void RegisterPlayerChangedCallback(
        int /*iPad*/,
        std::function<void(INetworkPlayer*, bool)> /*callback*/) override {}
    void UnRegisterPlayerChangedCallback(int /*iPad*/) override {}

    void HandleSignInChange() override {}

    bool _RunNetworkGame() override { return true; }
    bool _LeaveGame(bool /*bMigrateHost*/, bool /*bLeaveRoom*/) override {
        return true;
    }
    void _HostGame(int /*usersMask*/, unsigned char /*publicSlots*/,
                   unsigned char /*privateSlots*/) override {}
    bool _StartGame() override { return true; }

    void UpdateAndSetGameSessionData(
        INetworkPlayer* /*pNetworkPlayerLeaving*/) override {}
    bool RemoveLocalPlayer(INetworkPlayer* /*pNetworkPlayer*/) override {
        return false;
    }

    void SystemFlagSet(INetworkPlayer* /*pNetworkPlayer*/,
                       int /*index*/) override {}
    [[nodiscard]] bool SystemFlagGet(INetworkPlayer* /*pNetworkPlayer*/,
                                     int /*index*/) override {
        return false;
    }

    [[nodiscard]] std::string GatherStats() override { return {}; }
    [[nodiscard]] std::string GatherRTTStats() override { return {}; }

    void SetSessionTexturePackParentId(int /*id*/) override {}
    void SetSessionSubTexturePackId(int /*id*/) override {}
    void Notify(int /*ID*/, uintptr_t /*Param*/) override {}

    [[nodiscard]] std::vector<FriendSessionInfo*>* GetSessionList(
        int /*iPad*/, int /*localPlayers*/, bool /*partyOnly*/) override {
        return nullptr;
    }
    [[nodiscard]] bool GetGameSessionInfo(
        int /*iPad*/, SessionID /*sessionId*/,
        FriendSessionInfo* /*foundSession*/) override {
        return false;
    }
    void SetSessionsUpdatedCallback(
        std::function<void()> /*callback*/) override {}
    void GetFullFriendSessionInfo(
        FriendSessionInfo* /*foundSession*/,
        std::function<void(bool)> /*callback*/) override {}
    void ForceFriendsSessionRefresh() override {}
};
