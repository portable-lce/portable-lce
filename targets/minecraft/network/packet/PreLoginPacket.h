#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Packet.h"
#include "minecraft/network/packet/Packet.h"
#include "platform/PlatformTypes.h"

class PreLoginPacket : public Packet,
                       public std::enable_shared_from_this<PreLoginPacket> {
    // the login key is username client->server and sessionid server->client
public:
    static const int m_iSaveNameLen = 14;
    // 4J Added more info to this packet so that we can check if anyone has a
    // UGC privilege that won't let us
    //  join, and so that we can inform the server if we have that privilege
    //  set. Anyone with UGC turned off completely can't play the game online at
    //  all, so we only need to specify players with friends only set
    PlayerUID* m_playerXuids;
    std::uint8_t m_dwPlayerCount;
    std::uint8_t m_friendsOnlyBits;
    std::uint32_t m_ugcPlayersVersion;
    char m_szUniqueSaveName[m_iSaveNameLen];  // added for checking if the level
                                              // is in the ban list
    std::uint32_t
        m_serverSettings;  // A bitfield of server settings constructed with the
                           // MAKE_SERVER_SETTINGS macro
    std::uint8_t
        m_hostIndex;  // Rather than sending the xuid of the host again, send an
                      // index into the m_playerXuids array
    std::uint32_t m_texturePackId;
    std::int16_t m_netcodeVersion;

    std::string loginKey;

    PreLoginPacket();
    PreLoginPacket(std::string userName);
    PreLoginPacket(std::string userName, PlayerUID* playerXuids,
                   std::uint8_t playerCount, std::uint8_t friendsOnlyBits,
                   std::uint32_t ugcPlayersVersion,
                   const char* pszUniqueSaveName, std::uint32_t serverSettings,
                   std::uint8_t hostIndex, std::uint32_t texturePackId);
    ~PreLoginPacket();

    virtual void read(DataInputStream* dis);
    virtual void write(DataOutputStream* dos);
    virtual void handle(PacketListener* listener);
    virtual int getEstimatedSize();

public:
    static std::shared_ptr<Packet> create() {
        return std::make_shared<PreLoginPacket>();
    }
    virtual int getId() { return 2; }
};
