#pragma once

#include <cstdint>
#include <string>

#include "platform/PlatformTypes.h"

// Minimal interface that minecraft/ code uses to talk to the network
// subsystem. The concrete implementation lives in app/common/Network/
// (CGameNetworkManager). Same shape as IGameServices: minecraft/ code
// calls the interface; the implementation can sit in a higher layer
// without minecraft/ needing to include its header.
//
// Method names match the existing CGameNetworkManager surface so the
// concrete class can implement the interface without rename churn.

class INetworkPlayer;

namespace minecraft::network {

class INetworkService {
public:
    virtual ~INetworkService() = default;

    // Player management
    [[nodiscard]] virtual int GetPlayerCount() = 0;
    virtual bool AddLocalPlayerByUserIndex(int userIndex) = 0;
    virtual bool RemoveLocalPlayerByUserIndex(int userIndex) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetLocalPlayerByUserIndex(
        int userIndex) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetPlayerByIndex(int playerIndex) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetPlayerBySmallId(
        unsigned char smallId) = 0;
    [[nodiscard]] virtual INetworkPlayer* GetHostPlayer() = 0;

    // Hosting / state
    [[nodiscard]] virtual bool IsHost() = 0;
    [[nodiscard]] virtual bool IsInSession() = 0;
    [[nodiscard]] virtual bool IsLeavingGame() = 0;
    [[nodiscard]] virtual bool IsLocalGame() = 0;
    [[nodiscard]] virtual bool SessionHasSpace(
        unsigned int spaceRequired = 1) = 0;
    virtual void HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                          unsigned char publicSlots,
                          unsigned char privateSlots) = 0;
    virtual void FakeLocalPlayerJoined() = 0;
    virtual void UpdateAndSetGameSessionData(
        INetworkPlayer* pNetworkPlayerLeaving = nullptr) = 0;

    // System flags
    virtual void SystemFlagSet(INetworkPlayer* pNetworkPlayer, int index) = 0;
    [[nodiscard]] virtual bool SystemFlagGet(INetworkPlayer* pNetworkPlayer,
                                             int index) = 0;

    // Server lifecycle events
    virtual void ServerReady() = 0;
    virtual void ServerStopped() = 0;

    // Stats / debug
    [[nodiscard]] virtual std::string GatherStats() = 0;
    [[nodiscard]] virtual std::string GatherRTTStats() = 0;
    virtual void renderQueueMeter() = 0;
};

namespace platform_internal {
INetworkService& NetworkService_get();
}

}  // namespace minecraft::network

#define NetworkService \
    (::minecraft::network::platform_internal::NetworkService_get())
