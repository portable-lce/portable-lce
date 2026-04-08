#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "PendingConnection.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "platform/PlatformTypes.h"
#include "platform/storage/storage.h"
#include "minecraft/GameEnums.h"
#include "app/common/BuildVer/BuildVer.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "platform/IPlatformNetwork.h"
#include "platform/NetTypes.h"
#include "PlayerConnection.h"
#include "ServerConnection.h"
#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/network/Connection.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/LoginPacket.h"
#include "minecraft/network/packet/PreLoginPacket.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/PlayerList.h"
#include "minecraft/server/level/ServerPlayer.h"

class Packet;
// #if 0
// #include "PS3/Network/NetworkPlayerSony.h"
// #endif

Random* PendingConnection::random = new Random();

PendingConnection::PendingConnection(MinecraftServer* server, Socket* socket,
                                     const std::string& id) {
    // 4J - added initialisers
    done = false;
    _tick = 0;
    name = "";
    acceptedLogin = nullptr;
    loginKey = "";

    this->server = server;
    connection = new Connection(socket, id, this);
    connection->fakeLag = FAKE_LAG;
}

PendingConnection::~PendingConnection() { delete connection; }

void PendingConnection::tick() {
    if (acceptedLogin != nullptr) {
        this->handleAcceptedLogin(acceptedLogin);
        acceptedLogin = nullptr;
    }
    if (_tick++ == MAX_TICKS_BEFORE_LOGIN) {
        disconnect(DisconnectPacket::eDisconnect_LoginTooLong);
    } else {
        connection->tick();
    }
}

void PendingConnection::disconnect(DisconnectPacket::eDisconnectReason reason) {
    //   try {	// 4J - removed try/catch
    //        logger.info("Disconnecting " + getName() + ": " + reason);
    fprintf(stderr, "[PENDING] disconnect called with reason=%d at tick=%d\n",
            reason, _tick);
    Log::info("Pending connection disconnect: %d\n", reason);
    connection->send(std::make_shared<DisconnectPacket>(reason));
    connection->sendAndQuit();
    done = true;
    //    } catch (Exception e) {
    //        e.printStackTrace();
    //    }
}

void PendingConnection::handlePreLogin(std::shared_ptr<PreLoginPacket> packet) {
    if (packet->m_netcodeVersion != MINECRAFT_NET_VERSION) {
        Log::info("Netcode version is %d not equal to %d\n",
                        packet->m_netcodeVersion, MINECRAFT_NET_VERSION);
        if (packet->m_netcodeVersion > MINECRAFT_NET_VERSION) {
            disconnect(DisconnectPacket::eDisconnect_OutdatedServer);
        } else {
            disconnect(DisconnectPacket::eDisconnect_OutdatedClient);
        }
        return;
    }
    //	printf("Server: handlePreLogin\n");
    name =
        packet->loginKey;  // 4J Stu - Change from the login packet as we know
                           // better on client end during the pre-login packet
    sendPreLoginResponse();
}

void PendingConnection::sendPreLoginResponse() {
    // 4J Stu - Calculate the players with UGC privileges set
    PlayerUID* ugcXuids = new PlayerUID[MINECRAFT_NET_MAX_PLAYERS];
    std::uint8_t ugcXuidCount = 0;
    std::uint8_t hostIndex = 0;
    std::uint8_t ugcFriendsOnlyBits = 0;
    char szUniqueMapName[14];

    PlatformStorage.GetSaveUniqueFilename(szUniqueMapName);

    PlayerList* playerList = MinecraftServer::getInstance()->getPlayers();
    for (auto it = playerList->players.begin(); it != playerList->players.end();
         ++it) {
        std::shared_ptr<ServerPlayer> player = *it;
        // If the offline Xuid is invalid but the online one is not then that's
        // guest which we should ignore If the online Xuid is invalid but the
        // offline one is not then we are definitely an offline game so dont
        // care about UGC

        // PADDY - this is failing when a local player with chat restrictions
        // joins an online game

        if (player != nullptr &&
            player->connection->m_offlineXUID != INVALID_XUID &&
            player->connection->m_onlineXUID != INVALID_XUID) {
            if (player->connection->m_friendsOnlyUGC) {
                ugcFriendsOnlyBits |= (1 << ugcXuidCount);
            }
            // Need to use the online XUID otherwise friend checks will fail on
            // the client
            ugcXuids[ugcXuidCount] = player->connection->m_onlineXUID;

            if (player->connection->getNetworkPlayer() != nullptr &&
                player->connection->getNetworkPlayer()->IsHost())
                hostIndex = ugcXuidCount;

            ++ugcXuidCount;
        }
    }

    {
        connection->send(std::shared_ptr<PreLoginPacket>(
            new PreLoginPacket("-", ugcXuids, ugcXuidCount, ugcFriendsOnlyBits,
                               server->m_ugcPlayersVersion, szUniqueMapName,
                               gameServices().getGameHostOption(eGameHostOption_All),
                               hostIndex, server->m_texturePackId)));
    }
}

void PendingConnection::handleLogin(std::shared_ptr<LoginPacket> packet) {
    fprintf(stderr, "[LOGIN-SRV] handleLogin called! clientVersion=%d\n",
            packet->clientVersion);
    // name = packet->userName;
    if (packet->clientVersion != SharedConstants::NETWORK_PROTOCOL_VERSION) {
        Log::info("Client version is %d not equal to %d\n",
                        packet->clientVersion,
                        SharedConstants::NETWORK_PROTOCOL_VERSION);
        if (packet->clientVersion > SharedConstants::NETWORK_PROTOCOL_VERSION) {
            disconnect(DisconnectPacket::eDisconnect_OutdatedServer);
        } else {
            disconnect(DisconnectPacket::eDisconnect_OutdatedClient);
        }
        return;
    }

    // if (true)// 4J removed !server->onlineMode)
    bool sentDisconnect = false;

    if (sentDisconnect) {
        // Do nothing
    } else if (server->getPlayers()->isXuidBanned(packet->m_onlineXuid)) {
        disconnect(DisconnectPacket::eDisconnect_Banned);
    } else {
        handleAcceptedLogin(packet);
    }
    // else
    {
        // 4J - removed
    }
}

void PendingConnection::handleAcceptedLogin(
    std::shared_ptr<LoginPacket> packet) {
    if (packet->m_ugcPlayersVersion != server->m_ugcPlayersVersion) {
        // Send the pre-login packet again with the new list of players
        sendPreLoginResponse();
        return;
    }

    // Guests use the online xuid, everyone else uses the offline one
    PlayerUID playerXuid = packet->m_offlineXuid;
    if (playerXuid == INVALID_XUID) playerXuid = packet->m_onlineXuid;

    std::shared_ptr<ServerPlayer> playerEntity =
        server->getPlayers()->getPlayerForLogin(this, name, playerXuid,
                                                packet->m_onlineXuid);
    if (playerEntity != nullptr) {
        server->getPlayers()->placeNewPlayer(connection, playerEntity, packet);
        connection = nullptr;  // We've moved responsibility for this over to
                               // the new PlayerConnection, nullptr so we don't
                               // delete our reference to it here in our dtor
    }
    done = true;
}

void PendingConnection::onDisconnect(DisconnectPacket::eDisconnectReason reason,
                                     void* reasonObjects) {
    //    logger.info(getName() + " lost connection");
    done = true;
}

void PendingConnection::handleGetInfo(std::shared_ptr<GetInfoPacket> packet) {
    // try {
    // String message = server->motd + "§" + server->players->getPlayerCount() +
    // "§" + server->players->getMaxPlayers(); connection->send(new
    // DisconnectPacket(message));
    connection->send(std::shared_ptr<DisconnectPacket>(
        new DisconnectPacket(DisconnectPacket::eDisconnect_ServerFull)));
    connection->sendAndQuit();
    server->connection->removeSpamProtection(connection->getSocket());
    done = true;
    //} catch (Exception e) {
    //	e.printStackTrace();
    //}
}

void PendingConnection::handleKeepAlive(
    std::shared_ptr<KeepAlivePacket> packet) {
    // Ignore
}

void PendingConnection::onUnhandledPacket(std::shared_ptr<Packet> packet) {
    disconnect(DisconnectPacket::eDisconnect_UnexpectedPacket);
}

void PendingConnection::send(std::shared_ptr<Packet> packet) {
    connection->send(packet);
}

std::string PendingConnection::getName() {
    return "Unimplemented";
    //        if (name != null) return name + " [" +
    //        connection.getRemoteAddress().toString() + "]"; return
    //        connection.getRemoteAddress().toString();
}

bool PendingConnection::isServerPacketListener() { return true; }

bool PendingConnection::isDisconnected() { return done; }