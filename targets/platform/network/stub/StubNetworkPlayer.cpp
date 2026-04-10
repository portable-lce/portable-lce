#include "StubNetworkPlayer.h"

#include "StubPlatformNetwork.h"
#include "java/System.h"
#include "platform/PlatformTypes.h"
#include "platform/network/NetTypes.h"

StubNetworkPlayer::StubNetworkPlayer() { m_pSocket = nullptr; }

uint8_t StubNetworkPlayer::GetSmallId() { return 0; }
void StubNetworkPlayer::SendData(INetworkPlayer* player, const void* pvData,
                                 int dataSize, bool lowPriority, bool ack) {}
bool StubNetworkPlayer::IsSameSystem(INetworkPlayer* player) { return true; }
int StubNetworkPlayer::GetOutstandingAckCount() { return 0; }
int StubNetworkPlayer::GetSendQueueSizeBytes(INetworkPlayer* player,
                                             bool lowPriority) {
    return 0;
}
int StubNetworkPlayer::GetSendQueueSizeMessages(INetworkPlayer* player,
                                                bool lowPriority) {
    return 0;
}
int StubNetworkPlayer::GetCurrentRtt() { return 0; }
bool StubNetworkPlayer::IsHost() {
    return this == &StubPlatformNetwork::m_players[0];
}
bool StubNetworkPlayer::IsGuest() { return false; }
bool StubNetworkPlayer::IsLocal() { return true; }
int StubNetworkPlayer::GetSessionIndex() { return 0; }
bool StubNetworkPlayer::IsTalking() { return false; }
bool StubNetworkPlayer::IsMutedByLocalUser(int dwUserIndex) { return false; }
bool StubNetworkPlayer::HasVoice() { return false; }
bool StubNetworkPlayer::HasCamera() { return false; }
void StubNetworkPlayer::SetSocket(Socket* pSocket) { m_pSocket = pSocket; }
Socket* StubNetworkPlayer::GetSocket() { return m_pSocket; }
PlayerUID StubNetworkPlayer::GetUID() { return INVALID_XUID; }
const char* StubNetworkPlayer::GetOnlineName() { return "stub"; }
std::string StubNetworkPlayer::GetDisplayName() { return "stub"; }
int StubNetworkPlayer::GetUserIndex() {
    return this - &StubPlatformNetwork::m_players[0];
}
void StubNetworkPlayer::SentChunkPacket() {
    m_lastChunkPacketTime = System::currentTimeMillis();
}
int StubNetworkPlayer::GetTimeSinceLastChunkPacket_ms() {
    // If we haven't ever sent a packet, return maximum
    if (m_lastChunkPacketTime == 0) {
        return INT_MAX;
    }

    const int64_t currentTime = System::currentTimeMillis();
    return static_cast<int>(currentTime - m_lastChunkPacketTime);
}
