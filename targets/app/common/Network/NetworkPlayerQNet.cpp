#include "NetworkPlayerQNet.h"

#include <limits.h>

#include "java/System.h"
#include "platform/NetTypes.h"

NetworkPlayerQNet::NetworkPlayerQNet(IQNetPlayer* qnetPlayer) {
    m_qnetPlayer = qnetPlayer;
    m_pSocket = nullptr;
}

unsigned char NetworkPlayerQNet::GetSmallId() {
    return m_qnetPlayer->GetSmallId();
}

void NetworkPlayerQNet::SendData(INetworkPlayer* player, const void* pvData,
                                 int dataSize, bool lowPriority, bool ack) {
    uint32_t flags;
    flags = QNET_SENDDATA_RELIABLE | QNET_SENDDATA_SEQUENTIAL;
    if (lowPriority)
        flags |= QNET_SENDDATA_LOW_PRIORITY | QNET_SENDDATA_SECONDARY;
    m_qnetPlayer->SendData(
        static_cast<NetworkPlayerQNet*>(player)->m_qnetPlayer, pvData, dataSize,
        flags);
}

int NetworkPlayerQNet::GetOutstandingAckCount() { return 0; }

bool NetworkPlayerQNet::IsSameSystem(INetworkPlayer* player) {
    return (m_qnetPlayer->IsSameSystem(
                static_cast<NetworkPlayerQNet*>(player)->m_qnetPlayer) == true);
}

int NetworkPlayerQNet::GetSendQueueSizeBytes(INetworkPlayer* player,
                                             bool lowPriority) {
    uint32_t flags = QNET_GETSENDQUEUESIZE_BYTES;
    if (lowPriority) flags |= QNET_GETSENDQUEUESIZE_SECONDARY_TYPE;
    return m_qnetPlayer->GetSendQueueSize(
        player ? static_cast<NetworkPlayerQNet*>(player)->m_qnetPlayer
               : nullptr,
        flags);
}

int NetworkPlayerQNet::GetSendQueueSizeMessages(INetworkPlayer* player,
                                                bool lowPriority) {
    uint32_t flags = QNET_GETSENDQUEUESIZE_MESSAGES;
    if (lowPriority) flags |= QNET_GETSENDQUEUESIZE_SECONDARY_TYPE;
    return m_qnetPlayer->GetSendQueueSize(
        player ? static_cast<NetworkPlayerQNet*>(player)->m_qnetPlayer
               : nullptr,
        flags);
}

int NetworkPlayerQNet::GetCurrentRtt() { return m_qnetPlayer->GetCurrentRtt(); }

bool NetworkPlayerQNet::IsHost() { return (m_qnetPlayer->IsHost() == true); }

bool NetworkPlayerQNet::IsGuest() { return (m_qnetPlayer->IsGuest() == true); }

bool NetworkPlayerQNet::IsLocal() { return (m_qnetPlayer->IsLocal() == true); }

int NetworkPlayerQNet::GetSessionIndex() {
    return m_qnetPlayer->GetSessionIndex();
}

bool NetworkPlayerQNet::IsTalking() {
    return (m_qnetPlayer->IsTalking() == true);
}

bool NetworkPlayerQNet::IsMutedByLocalUser(int userIndex) {
    return (m_qnetPlayer->IsMutedByLocalUser(userIndex) == true);
}

bool NetworkPlayerQNet::HasVoice() {
    return (m_qnetPlayer->HasVoice() == true);
}

bool NetworkPlayerQNet::HasCamera() {
    return (m_qnetPlayer->HasCamera() == true);
}

int NetworkPlayerQNet::GetUserIndex() { return m_qnetPlayer->GetUserIndex(); }

void NetworkPlayerQNet::SetSocket(Socket* pSocket) { m_pSocket = pSocket; }

Socket* NetworkPlayerQNet::GetSocket() { return m_pSocket; }

PlayerUID NetworkPlayerQNet::GetUID() { return m_qnetPlayer->GetXuid(); }

const char* NetworkPlayerQNet::GetOnlineName() {
    return m_qnetPlayer->GetGamertag();
}

std::string NetworkPlayerQNet::GetDisplayName() {
    return m_qnetPlayer->GetGamertag();
}

IQNetPlayer* NetworkPlayerQNet::GetQNetPlayer() { return m_qnetPlayer; }

void NetworkPlayerQNet::SentChunkPacket() {
    m_lastChunkPacketTime = System::currentTimeMillis();
}

int NetworkPlayerQNet::GetTimeSinceLastChunkPacket_ms() {
    // If we haven't ever sent a packet, return maximum
    if (m_lastChunkPacketTime == 0) {
        return INT_MAX;
    }

    const int64_t currentTime = System::currentTimeMillis();
    return static_cast<int>(currentTime - m_lastChunkPacketTime);
}