#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "ServerConnection.h"

#include <algorithm>

#include "PendingConnection.h"
#include "PlayerConnection.h"
#include "util/StringHelpers.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/network/Connection.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/ServerSettingsChangedPacket.h"
#include "minecraft/server/level/ServerPlayer.h"

ServerConnection::ServerConnection(MinecraftServer* server) {
    // 4J - added initialiser
    connectionCounter = 0;

    this->server = server;
}

ServerConnection::~ServerConnection() {}

// 4J - added to handle incoming connections, to replace thread that original
// used to have
void ServerConnection::NewIncomingSocket(Socket* socket) {
    std::shared_ptr<PendingConnection> unconnectedClient =
        std::make_shared<PendingConnection>(
            server, socket,
            "Connection #" + toWString<int>(connectionCounter++));
    handleConnection(unconnectedClient);
}

void ServerConnection::addPlayerConnection(
    std::shared_ptr<PlayerConnection> uc) {
    players.push_back(uc);
}

void ServerConnection::handleConnection(std::shared_ptr<PendingConnection> uc) {
    {
        std::lock_guard<std::mutex> lock(pending_cs);
        pending.push_back(uc);
    }
}

void ServerConnection::stop() {
    {
        std::lock_guard<std::mutex> lock(pending_cs);
        for (unsigned int i = 0; i < pending.size(); i++) {
            std::shared_ptr<PendingConnection> uc = pending[i];
            uc->connection->close(DisconnectPacket::eDisconnect_Closed);
        }
    }

    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<PlayerConnection> player = players[i];
        player->connection->close(DisconnectPacket::eDisconnect_Closed);
    }
}

void ServerConnection::tick() {
    {
        // MGH - changed this so that the the CS lock doesn't cover the tick
        // (was causing a lockup when 2 players tried to join)
        std::vector<std::shared_ptr<PendingConnection> > tempPending;
        {
            std::lock_guard<std::mutex> lock(pending_cs);
            tempPending = pending;
        }

        for (unsigned int i = 0; i < tempPending.size(); i++) {
            std::shared_ptr<PendingConnection> uc = tempPending[i];
            //        try {	// 4J - removed try/catch
            uc->tick();
            //        } catch (Exception e) {
            //            uc.disconnect("Internal server error");
            //            logger.log(Level.WARNING, "Failed to handle packet: "
            //            + e, e);
            //        }
            if (uc->connection != nullptr) uc->connection->flush();
        }
    }

    // now remove from the pending list
    {
        std::lock_guard<std::mutex> lock(pending_cs);
        for (unsigned int i = 0; i < pending.size(); i++)
            if (pending[i]->done) {
                pending.erase(pending.begin() + i);
                i--;
            }
    }

    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<PlayerConnection> player = players[i];
        std::shared_ptr<ServerPlayer> serverPlayer = player->getPlayer();
        if (serverPlayer) {
            serverPlayer->updateFrameTick();
            serverPlayer->doChunkSendingTick(false);
        }
        player->tick();
        if (player->done) {
            players.erase(players.begin() + i);
            i--;
        }
        player->connection->flush();
    }
}

bool ServerConnection::addPendingTextureRequest(
    const std::string& textureName) {
    auto it = find(m_pendingTextureRequests.begin(),
                   m_pendingTextureRequests.end(), textureName);
    if (it == m_pendingTextureRequests.end()) {
        m_pendingTextureRequests.push_back(textureName);
        return true;
    }

    // 4J Stu - We want to request this texture from everyone, if we have a
    // duplicate it's most likely because the first person we asked for it
    // didn't have it eg They selected a skin then deleted the skin pack. The
    // side effect of this change is that in certain cases we can send a few
    // more requests, and receive a few more responses if people join with the
    // same skin in a short space of time
    return true;
}

void ServerConnection::handleTextureReceived(const std::string& textureName) {
    auto it = find(m_pendingTextureRequests.begin(),
                   m_pendingTextureRequests.end(), textureName);
    if (it != m_pendingTextureRequests.end()) {
        m_pendingTextureRequests.erase(it);
    }
    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<PlayerConnection> player = players[i];
        if (!player->done) {
            player->handleTextureReceived(textureName);
        }
    }
}

void ServerConnection::handleTextureAndGeometryReceived(
    const std::string& textureName) {
    auto it = find(m_pendingTextureRequests.begin(),
                   m_pendingTextureRequests.end(), textureName);
    if (it != m_pendingTextureRequests.end()) {
        m_pendingTextureRequests.erase(it);
    }
    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<PlayerConnection> player = players[i];
        if (!player->done) {
            player->handleTextureAndGeometryReceived(textureName);
        }
    }
}

void ServerConnection::handleServerSettingsChanged(
    std::shared_ptr<ServerSettingsChangedPacket> packet) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (packet->action == ServerSettingsChangedPacket::HOST_DIFFICULTY) {
        for (unsigned int i = 0; i < pMinecraft->levels.size(); ++i) {
            if (pMinecraft->levels[i] != nullptr) {
                Log::info(
                    "ClientConnection::handleServerSettingsChanged - "
                    "Difficulty = %d",
                    packet->data);
                pMinecraft->levels[i]->difficulty = packet->data;
            }
        }
    }
    // 	else
    // if(packet->action==ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS)//
    // options
    // 	{
    // 		gameServices().setGameHostOption(eGameHostOption_All,packet->m_serverSettings)
    // 	}
    // 	else
    // 	{
    // 		unsigned char ucData=(unsigned char)packet->data;
    // 		if(ucData&1)
    // 		{
    // 			// hide gamertags
    // 			pMinecraft->options->SetGamertagSetting(true);
    // 		}
    // 		else
    // 		{
    // 			pMinecraft->options->SetGamertagSetting(false);
    // 		}
    //
    // 		for (unsigned int i = 0; i < players.size(); i++)
    // 		{
    // 			shared_ptr<PlayerConnection> playerconnection =
    // players[i];
    // 			playerconnection->setShowOnMaps(pMinecraft->options->GetGamertagSetting());
    // 		}
    // 	}
}

std::vector<std::shared_ptr<PlayerConnection> >*
ServerConnection::getPlayers() {
    return &players;
}