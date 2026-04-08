#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "PreLoginPacket.h"

#include <cstdint>
#include <cstring>

#include "minecraft/BuildVer.h"
#include "platform/IPlatformNetwork.h"
#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"

PreLoginPacket::PreLoginPacket() {
    loginKey = "";
    m_playerXuids = nullptr;
    m_dwPlayerCount = 0;
    m_friendsOnlyBits = 0;
    m_ugcPlayersVersion = 0;
    memset(m_szUniqueSaveName, 0, m_iSaveNameLen);
    m_serverSettings = 0;
    m_hostIndex = 0;
    m_texturePackId = 0;
    m_netcodeVersion = 0;
}

PreLoginPacket::PreLoginPacket(std::string userName) {
    this->loginKey = userName;
    m_playerXuids = nullptr;
    m_dwPlayerCount = 0;
    m_friendsOnlyBits = 0;
    m_ugcPlayersVersion = 0;
    memset(m_szUniqueSaveName, 0, m_iSaveNameLen);
    m_serverSettings = 0;
    m_hostIndex = 0;
    m_texturePackId = 0;
    m_netcodeVersion = 0;
}

PreLoginPacket::PreLoginPacket(
    std::string userName, PlayerUID* playerXuids, std::uint8_t playerCount,
    std::uint8_t friendsOnlyBits, std::uint32_t ugcPlayersVersion,
    const char* pszUniqueSaveName, std::uint32_t serverSettings,
    std::uint8_t hostIndex, std::uint32_t texturePackId) {
    this->loginKey = userName;
    m_playerXuids = playerXuids;
    m_dwPlayerCount = playerCount;
    m_friendsOnlyBits = friendsOnlyBits;
    m_ugcPlayersVersion = ugcPlayersVersion;
    memcpy(m_szUniqueSaveName, pszUniqueSaveName, m_iSaveNameLen);
    m_serverSettings = serverSettings;
    m_hostIndex = hostIndex;
    m_texturePackId = texturePackId;
    m_netcodeVersion = 0;
}

PreLoginPacket::~PreLoginPacket() {
    if (m_playerXuids != nullptr) delete[] m_playerXuids;
}

void PreLoginPacket::read(DataInputStream* dis)  // throws IOException
{
    m_netcodeVersion = dis->readShort();

    loginKey = readUtf(dis, 32);

    m_friendsOnlyBits = dis->readByte();
    m_ugcPlayersVersion = static_cast<std::uint32_t>(dis->readInt());
    m_dwPlayerCount = dis->readByte();
    if (m_dwPlayerCount > 0) {
        m_playerXuids = new PlayerUID[m_dwPlayerCount];
        for (std::uint32_t i = 0; i < m_dwPlayerCount; ++i) {
            m_playerXuids[i] = dis->readPlayerUID();
        }
    }
    for (int i = 0; i < m_iSaveNameLen; ++i) {
        m_szUniqueSaveName[i] = static_cast<char>(dis->readByte());
    }
    m_serverSettings = static_cast<std::uint32_t>(dis->readInt());
    m_hostIndex = dis->readByte();

    m_texturePackId = static_cast<std::uint32_t>(dis->readInt());

    // Set the name of the map so we can check it for players banned lists
    gameServices().setUniqueMapName((char*)m_szUniqueSaveName);
}

void PreLoginPacket::write(DataOutputStream* dos)  // throws IOException
{
    dos->writeShort(MINECRAFT_NET_VERSION);

    writeUtf(loginKey, dos);

    dos->writeByte(m_friendsOnlyBits);
    dos->writeInt(static_cast<int>(m_ugcPlayersVersion));
    dos->writeByte((std::uint8_t)m_dwPlayerCount);
    for (std::uint32_t i = 0; i < m_dwPlayerCount; ++i) {
        dos->writePlayerUID(m_playerXuids[i]);
    }

    Log::info("*** PreLoginPacket::write - %s\n", m_szUniqueSaveName);
    for (int i = 0; i < m_iSaveNameLen; ++i) {
        dos->writeByte(static_cast<std::uint8_t>(m_szUniqueSaveName[i]));
    }
    dos->writeInt(static_cast<int>(m_serverSettings));
    dos->writeByte(m_hostIndex);
    dos->writeInt(static_cast<int>(m_texturePackId));
}

void PreLoginPacket::handle(PacketListener* listener) {
    listener->handlePreLogin(shared_from_this());
}

int PreLoginPacket::getEstimatedSize() {
    return 4 + 4 + (int)loginKey.length() + 4 + 14 + 4 + 1 + 4;
}
