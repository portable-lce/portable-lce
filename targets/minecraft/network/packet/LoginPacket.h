#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Packet.h"
#include "minecraft/network/packet/Packet.h"
#include "platform/PlatformTypes.h"

class LevelType;

class LoginPacket : public Packet,
                    public std::enable_shared_from_this<LoginPacket> {
public:
    int clientVersion;
    std::string userName;
    int64_t seed;
    char dimension;
    PlayerUID m_offlineXuid, m_onlineXuid;         // 4J Added
    char difficulty;                               // 4J Added
    bool m_friendsOnlyUGC;                         // 4J Added
    std::uint32_t m_ugcPlayersVersion;             // 4J Added
    int m_multiplayerInstanceId;                   // 4J Added for sentient
    std::uint8_t m_playerIndex;                    // 4J Added
    std::uint32_t m_playerSkinId, m_playerCapeId;  // 4J Added
    bool m_isGuest;                                // 4J Added
    bool m_newSeaLevel;                            // 4J Added
    LevelType* m_pLevelType;
    unsigned int m_uiGamePrivileges;
    int m_xzSize;     // 4J Added
    int m_hellScale;  // 4J Added

    // 1.8.2
    int gameType;
    std::uint8_t mapHeight;
    std::uint8_t maxPlayers;

    LoginPacket();
    LoginPacket(const std::string& userName, int clientVersion,
                LevelType* pLevelType, int64_t seed, int gameType,
                char dimension, std::uint8_t mapHeight, std::uint8_t maxPlayers,
                char difficulty, int m_multiplayerInstanceId,
                std::uint8_t playerIndex, bool newSeaLevel,
                unsigned int uiGamePrivileges, int xzSize,
                int hellScale);  // Server -> Client
    LoginPacket(const std::string& userName, int clientVersion,
                PlayerUID offlineXuid, PlayerUID onlineXuid,
                bool friendsOnlyUGC, std::uint32_t ugcPlayersVersion,
                std::uint32_t skinId, std::uint32_t capeId,
                bool isGuest);  // Client -> Server

    virtual void read(DataInputStream* dis);
    virtual void write(DataOutputStream* dos);
    virtual void handle(PacketListener* listener);
    virtual int getEstimatedSize();

public:
    static std::shared_ptr<Packet> create() {
        return std::make_shared<LoginPacket>();
    }
    virtual int getId() { return 1; }
};
