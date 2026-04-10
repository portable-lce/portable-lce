#pragma once

#include <stdint.h>

#include <string>

#include "platform/network/network.h"
#include "platform/NetTypes.h"
#include "platform/PlatformTypes.h"

class Socket;

// This is an implementation of the INetworkPlayer interface for the supported
// QNet-backed path. It
// effectively wraps the StubNetworkPlayer class in a non-platform-specific way. It is
// managed by PlatformNetworkManagerStub.

class StubNetworkPlayer : public INetworkPlayer {
public:
    StubNetworkPlayer();

    // Common player interface
    unsigned char GetSmallId();
    void SendData(INetworkPlayer* player, const void* pvData,
                          int dataSize, bool lowPriority, bool ack);
    bool IsSameSystem(INetworkPlayer* player);
    int GetOutstandingAckCount();
    int GetSendQueueSizeBytes(INetworkPlayer* player, bool lowPriority);
    int GetSendQueueSizeMessages(INetworkPlayer* player,
                                         bool lowPriority);
    int GetCurrentRtt();
    bool IsHost();
    bool IsGuest();
    bool IsLocal();
    int GetSessionIndex();
    bool IsTalking();
    bool IsMutedByLocalUser(int userIndex);
    bool HasVoice();
    bool HasCamera();
    int GetUserIndex();
    void SetSocket(Socket* pSocket);
    Socket* GetSocket();
    const char* GetOnlineName();
    std::string GetDisplayName();
    PlayerUID GetUID();
    void SentChunkPacket();
    int GetTimeSinceLastChunkPacket_ms();

private:
    int64_t m_lastChunkPacketTime;
    Socket* m_pSocket;
};