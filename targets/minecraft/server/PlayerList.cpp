#include "PlayerList.h"

#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>

#include "MinecraftServer.h"
#include "Settings.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/LevelRuleset.h"
#include "java/Class.h"
#include "java/JavaMath.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/Pos.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/network/Connection.h"
#include "minecraft/network/INetworkService.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/GameEventPacket.h"
#include "minecraft/network/packet/LoginPacket.h"
#include "minecraft/network/packet/PlayerAbilitiesPacket.h"
#include "minecraft/network/packet/PlayerInfoPacket.h"
#include "minecraft/network/packet/RespawnPacket.h"
#include "minecraft/network/packet/SetCarriedItemPacket.h"
#include "minecraft/network/packet/SetExperiencePacket.h"
#include "minecraft/network/packet/SetSpawnPositionPacket.h"
#include "minecraft/network/packet/SetTimePacket.h"
#include "minecraft/network/packet/TextureAndGeometryPacket.h"
#include "minecraft/network/packet/TexturePacket.h"
#include "minecraft/network/packet/UpdateMobEffectPacket.h"
#include "minecraft/network/packet/XZPacket.h"
#include "minecraft/server/level/EntityTracker.h"
#include "minecraft/server/level/PlayerChunkMap.h"
#include "minecraft/server/level/ServerChunkCache.h"
#include "minecraft/server/level/ServerLevel.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/server/level/ServerPlayerGameMode.h"
#include "minecraft/server/network/PendingConnection.h"
#include "minecraft/server/network/PlayerConnection.h"
#include "minecraft/server/network/ServerConnection.h"
#include "minecraft/util/Log.h"
#include "minecraft/util/ProgressListener.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityIO.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/player/SkinTypes.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/ChunkPos.h"
#include "minecraft/world/level/GameRules.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelSettings.h"
#include "minecraft/world/level/PortalForcer.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/saveddata/MapItemSavedData.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/storage/LevelStorage.h"
#include "minecraft/world/level/storage/PlayerIO.h"
#include "minecraft/world/tutorial/ITutorial.h"
#include "nbt/CompoundTag.h"
#include "platform/network/NetTypes.h"
#include "platform/network/network.h"
#include "platform/profile/profile.h"
#include "strings.h"

class MobEffectInstance;

// 4J - this class is fairly substantially altered as there didn't seem any
// point in porting code for banning, whitelisting, ops etc.

PlayerList::PlayerList(MinecraftServer* server) {
    playerIo = nullptr;

    this->server = server;

    sendAllPlayerInfoIn = 0;
    overrideGameMode = nullptr;
    allowCheatsForAllPlayers = false;

#if defined(_LARGE_WORLDS)
    viewDistance = 16;
#else
    viewDistance = 10;
#endif

    // int viewDistance = server->settings->getInt("view-distance", 10);

    maxPlayers = server->settings->getInt("max-players", 20);
    doWhiteList = false;
}

PlayerList::~PlayerList() {
    for (auto it = players.begin(); it < players.end(); it++) {
        (*it)->connection = nullptr;  // Must remove reference to connection, or
                                      // else there is a circular dependency
        delete (*it)->gameMode;  // Gamemode also needs deleted as it references
                                 // back to this player
        (*it)->gameMode = nullptr;
    }
}

void PlayerList::placeNewPlayer(Connection* connection,
                                std::shared_ptr<ServerPlayer> player,
                                std::shared_ptr<LoginPacket> packet) {
    CompoundTag* playerTag = load(player);

    bool newPlayer = playerTag == nullptr;

    player->setLevel(server->getLevel(player->dimension));
    player->gameMode->setLevel((ServerLevel*)player->level);

    // Make sure these privileges are always turned off for the host player
    INetworkPlayer* networkPlayer = connection->getSocket()->getPlayer();
    if (networkPlayer != nullptr && networkPlayer->IsHost()) {
        player->enableAllPlayerPrivileges(true);
        player->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_HOST, 1);
    }

    // 4J Stu - TU-1 hotfix
    // Fix for #13150 - When a player loads/joins a game after saving/leaving in
    // the nether, sometimes they are spawned on top of the nether and cannot
    // mine down
    validatePlayerSpawnPosition(player);

    //        logger.info(getName() + " logged in with entity id " +
    //        playerEntity.entityId + " at (" + playerEntity.x + ", " +
    //        playerEntity.y + ", " + playerEntity.z + ")");

    ServerLevel* level = server->getLevel(player->dimension);

    std::uint8_t playerIndex = 0;
    {
        bool usedIndexes[MINECRAFT_NET_MAX_PLAYERS];
        memset(&usedIndexes, 0, MINECRAFT_NET_MAX_PLAYERS * sizeof(bool));
        for (auto it = players.begin(); it < players.end(); ++it) {
            usedIndexes[(int)(*it)->getPlayerIndex()] = true;
        }
        for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i) {
            if (!usedIndexes[i]) {
                playerIndex = i;
                break;
            }
        }
    }
    player->setPlayerIndex(playerIndex);
    player->setCustomSkin(packet->m_playerSkinId);
    player->setCustomCape(packet->m_playerCapeId);

    // 4J-JEV: Moved this here so we can send player-model texture and geometry
    // data.
    std::shared_ptr<PlayerConnection> playerConnection =
        std::shared_ptr<PlayerConnection>(
            new PlayerConnection(server, connection, player));
    // player->connection = playerConnection;	// Used to be assigned in
    // PlayerConnection ctor but moved out so we can use std::shared_ptr

    if (newPlayer) {
        int mapScale = 3;
#if defined(_LARGE_WORLDS)
        int scale = MapItemSavedData::MAP_SIZE * 2 * (1 << mapScale);
        int centreXC = (int)(Math::round(player->x / scale) * scale);
        int centreZC = (int)(Math::round(player->z / scale) * scale);
#else
        // 4J-PB - for Xbox maps, we'll centre them on the origin of the world,
        // since we can fit the whole world in our map
        int centreXC = 0;
        int centreZC = 0;
#endif
        // 4J Added - Give every player a map the first time they join a server
        player->inventory->setItem(
            9, std::make_shared<ItemInstance>(
                   Item::map_Id, 1,
                   level->getAuxValueForMap(player->getXuid(), 0, centreXC,
                                            centreZC, mapScale)));
        if (gameServices().getGameRuleDefinitions() != nullptr) {
            gameServices().getGameRuleDefinitions()->postProcessPlayer(player);
        }
    }

    if (!player->customTextureUrl.empty() &&
        player->customTextureUrl.substr(0, 3).compare("def") != 0 &&
        !gameServices().isFileInMemoryTextures(player->customTextureUrl)) {
        if (server->getConnection()->addPendingTextureRequest(
                player->customTextureUrl)) {
#if !defined(_CONTENT_PACKAGE)
            printf(
                "Sending texture packet to get custom skin %s from player "
                "%s\n",
                player->customTextureUrl.c_str(), player->name.c_str());
#endif
            playerConnection->send(std::shared_ptr<TextureAndGeometryPacket>(
                new TextureAndGeometryPacket(player->customTextureUrl, nullptr,
                                             0)));
        }
    } else if (!player->customTextureUrl.empty() &&
               gameServices().isFileInMemoryTextures(
                   player->customTextureUrl)) {
        // Update the ref count on the memory texture data
        gameServices().addMemoryTextureFile(player->customTextureUrl, nullptr,
                                            0);
    }

    if (!player->customTextureUrl2.empty() &&
        player->customTextureUrl2.substr(0, 3).compare("def") != 0 &&
        !gameServices().isFileInMemoryTextures(player->customTextureUrl2)) {
        if (server->getConnection()->addPendingTextureRequest(
                player->customTextureUrl2)) {
#if !defined(_CONTENT_PACKAGE)
            printf(
                "Sending texture packet to get custom skin %s from player "
                "%s\n",
                player->customTextureUrl2.c_str(), player->name.c_str());
#endif
            playerConnection->send(std::shared_ptr<TexturePacket>(
                new TexturePacket(player->customTextureUrl2, nullptr, 0)));
        }
    } else if (!player->customTextureUrl2.empty() &&
               gameServices().isFileInMemoryTextures(
                   player->customTextureUrl2)) {
        // Update the ref count on the memory texture data
        gameServices().addMemoryTextureFile(player->customTextureUrl2, nullptr,
                                            0);
    }

    player->setIsGuest(packet->m_isGuest);

    Pos* spawnPos = level->getSharedSpawnPos();

    updatePlayerGameMode(player, nullptr, level);

    // Update the privileges with the correct game mode
    GameType* gameType = Player::getPlayerGamePrivilege(
                             player->getAllPlayerGamePrivileges(),
                             Player::ePlayerGamePrivilege_CreativeMode)
                             ? GameType::CREATIVE
                             : GameType::SURVIVAL;
    gameType = LevelSettings::validateGameType(gameType->getId());
    if (player->gameMode->getGameModeForPlayer() != gameType) {
        player->setPlayerGamePrivilege(
            Player::ePlayerGamePrivilege_CreativeMode,
            player->gameMode->getGameModeForPlayer()->getId());
    }

    // std::shared_ptr<PlayerConnection> playerConnection =
    // std::make_shared<PlayerConnection>(server,
    // connection, player);
    player->connection =
        playerConnection;  // Used to be assigned in PlayerConnection ctor but
                           // moved out so we can use std::shared_ptr

    // 4J Added to store UGC settings
    playerConnection->m_friendsOnlyUGC = packet->m_friendsOnlyUGC;
    playerConnection->m_offlineXUID = packet->m_offlineXuid;
    playerConnection->m_onlineXUID = packet->m_onlineXuid;

    // This player is now added to the list, so incrementing this value
    // invalidates all previous PreLogin packets
    if (packet->m_friendsOnlyUGC) ++server->m_ugcPlayersVersion;

    addPlayerToReceiving(player);

    playerConnection->send(std::make_shared<LoginPacket>(
        "", player->entityId, level->getLevelData()->getGenerator(),
        level->getSeed(), player->gameMode->getGameModeForPlayer()->getId(),
        (uint8_t)level->dimension->id, (uint8_t)level->getMaxBuildHeight(),
        (uint8_t)getMaxPlayers(), level->difficulty,
        0 /*TelemetryManager->GetMultiplayerInstanceID()*/,
        (uint8_t)playerIndex, level->useNewSeaLevel(),
        player->getAllPlayerGamePrivileges(),
        level->getLevelData()->getXZSize(),
        level->getLevelData()->getHellScale()));
    playerConnection->send(std::shared_ptr<SetSpawnPositionPacket>(
        new SetSpawnPositionPacket(spawnPos->x, spawnPos->y, spawnPos->z)));
    playerConnection->send(std::shared_ptr<PlayerAbilitiesPacket>(
        new PlayerAbilitiesPacket(&player->abilities)));
    playerConnection->send(std::shared_ptr<SetCarriedItemPacket>(
        new SetCarriedItemPacket(player->inventory->selected)));
    delete spawnPos;

    updateEntireScoreboard((ServerScoreboard*)level->getScoreboard(), player);

    sendLevelInfo(player, level);

    // 4J-PB - removed, since it needs to be localised in the language the
    // client is in
    // server->players->broadcastAll( std::shared_ptr<ChatPacket>( new
    // ChatPacket("§e" + playerEntity->name + " joined the game.") ) );
    broadcastAll(std::shared_ptr<ChatPacket>(
        new ChatPacket(player->name, ChatPacket::e_ChatPlayerJoinedGame)));

    add(player);

    player->doTick(
        true, true,
        false);  // 4J - added - force sending of the nearest chunk before the
                 // player is teleported, so we have somewhere to arrive on...
    playerConnection->teleport(player->x, player->y, player->z, player->yRot,
                               player->xRot);

    server->getConnection()->addPlayerConnection(playerConnection);
    playerConnection->send(std::make_shared<SetTimePacket>(
        level->getGameTime(), level->getDayTime(),
        level->getGameRules()->getBoolean(GameRules::RULE_DAYLIGHT)));

    auto activeEffects = player->getActiveEffects();
    for (auto it = activeEffects->begin(); it != activeEffects->end(); ++it) {
        MobEffectInstance* effect = *it;
        playerConnection->send(std::shared_ptr<UpdateMobEffectPacket>(
            new UpdateMobEffectPacket(player->entityId, effect)));
    }

    player->initMenu();

    if (playerTag != nullptr && playerTag->contains(Entity::RIDING_TAG)) {
        // this player has been saved with a mount tag
        std::shared_ptr<Entity> mount = EntityIO::loadStatic(
            playerTag->getCompound(Entity::RIDING_TAG), level);
        if (mount != nullptr) {
            mount->forcedLoading = true;
            level->addEntity(mount);
            player->ride(mount);
            mount->forcedLoading = false;
        }
    }

    // If we are joining at the same time as someone in the end on this system
    // is travelling through the win portal, then we should set our wonGame flag
    // to true so that respawning works when the EndPoem is closed
    INetworkPlayer* thisPlayer = player->connection->getNetworkPlayer();
    if (thisPlayer != nullptr) {
        for (auto it = players.begin(); it != players.end(); ++it) {
            std::shared_ptr<ServerPlayer> servPlayer = *it;
            INetworkPlayer* checkPlayer =
                servPlayer->connection->getNetworkPlayer();
            if (thisPlayer != checkPlayer && checkPlayer != nullptr &&
                thisPlayer->IsSameSystem(checkPlayer) && servPlayer->wonGame) {
                player->wonGame = true;
                break;
            }
        }
    }
}

void PlayerList::updateEntireScoreboard(ServerScoreboard* scoreboard,
                                        std::shared_ptr<ServerPlayer> player) {
    // unordered_set<Objective *> objectives;

    // for (PlayerTeam team : scoreboard->getPlayerTeams())
    //{
    //	player->connection->send( shared_ptr<SetPlayerTeamPacket>(new
    // SetPlayerTeamPacket(team, SetPlayerTeamPacket::METHOD_ADD)));
    // }

    // for (int slot = 0; slot < Scoreboard::DISPLAY_SLOTS; slot++)
    //{
    //	Objective objective = scoreboard->getDisplayObjective(slot);

    //	if (objective != nullptr && !objectives->contains(objective))
    //	{
    //		vector<shared_ptr<Packet> > *packets =
    // scoreboard->getStartTrackingPackets(objective);

    //		for (Packet packet : packets)
    //		{
    //			player->connection->send(packet);
    //		}

    //		objectives->add(objective);
    //	}
    //}
}

void PlayerList::setLevel(std::vector<ServerLevel*>& levels) {
    playerIo = levels[0]->getLevelStorage()->getPlayerIO();
}

void PlayerList::changeDimension(std::shared_ptr<ServerPlayer> player,
                                 ServerLevel* from) {
    ServerLevel* to = player->getLevel();

    if (from != nullptr) from->getChunkMap()->remove(player);
    to->getChunkMap()->add(player);

    to->cache->create(((int)player->x) >> 4, ((int)player->z) >> 4);
}

int PlayerList::getMaxRange() {
    return PlayerChunkMap::convertChunkRangeToBlock(getViewDistance());
}

CompoundTag* PlayerList::load(std::shared_ptr<ServerPlayer> player) {
    return playerIo->load(player);
}

void PlayerList::save(std::shared_ptr<ServerPlayer> player) {
    playerIo->save(player);
}

// 4J Stu - TU-1 hotifx
// Add this function to take some of the code from the PlayerList::add function
// with the fixes for checking spawn area, especially in the nether. These
// needed to be done in a different order from before Fix for #13150 - When a
// player loads/joins a game after saving/leaving in the nether, sometimes they
// are spawned on top of the nether and cannot mine down
void PlayerList::validatePlayerSpawnPosition(
    std::shared_ptr<ServerPlayer> player) {
    // 4J Stu - Some adjustments to make sure the current players position is
    // correct Make sure that the player is on the ground, and in the centre x/z
    // of the current column
    Log::info("Original pos is %f, %f, %f in dimension %d\n", player->x,
              player->y, player->z, player->dimension);

    bool spawnForced = player->isRespawnForced();

    double targetX = 0;
    if (player->x < 0)
        targetX = std::ceil(player->x) - 0.5;
    else
        targetX = std::floor(player->x) + 0.5;

    double targetY = floor(player->y);

    double targetZ = 0;
    if (player->z < 0)
        targetZ = std::ceil(player->z) - 0.5;
    else
        targetZ = std::floor(player->z) + 0.5;

    player->setPos(targetX, targetY, targetZ);

    Log::info("New pos is %f, %f, %f in dimension %d\n", player->x, player->y,
              player->z, player->dimension);

    ServerLevel* level = server->getLevel(player->dimension);
    while (level->getCubes(player, &player->bb)->size() != 0) {
        player->setPos(player->x, player->y + 1, player->z);
    }
    Log::info("Final pos is %f, %f, %f in dimension %d\n", player->x, player->y,
              player->z, player->dimension);

    // 4J Stu - If we are in the nether and the above while loop has put us
    // above the nether then we have a problem Finding a valid, safe spawn point
    // is potentially computationally expensive (may have to hunt through a
    // large part of the nether) so move the player to their spawn position in
    // the overworld so that they do not lose their inventory 4J Stu - We also
    // use this mechanism to force a spawn point in the overworld for players
    // who were in the save when the reset nether option was applied
    if (level->dimension->id == -1 && player->y > 125) {
        Log::info(
            "Player in the nether tried to spawn at y = %f, moving to "
            "overworld\n",
            player->y);
        player->setLevel(server->getLevel(0));
        player->gameMode->setLevel(server->getLevel(0));
        player->dimension = 0;

        level = server->getLevel(player->dimension);

        Pos* levelSpawn = level->getSharedSpawnPos();
        player->setPos(levelSpawn->x, levelSpawn->y, levelSpawn->z);
        delete levelSpawn;

        Pos* bedPosition = player->getRespawnPosition();
        if (bedPosition != nullptr) {
            Pos* respawnPosition = Player::checkBedValidRespawnPosition(
                server->getLevel(player->dimension), bedPosition, spawnForced);
            if (respawnPosition != nullptr) {
                player->moveTo(respawnPosition->x + 0.5f,
                               respawnPosition->y + 0.1f,
                               respawnPosition->z + 0.5f, 0, 0);
                player->setRespawnPosition(bedPosition, spawnForced);
            }
            delete bedPosition;
        }
        while (level->getCubes(player, &player->bb)->size() != 0) {
            player->setPos(player->x, player->y + 1, player->z);
        }

        Log::info("Updated pos is %f, %f, %f in dimension %d\n", player->x,
                  player->y, player->z, player->dimension);
    }
}

void PlayerList::add(std::shared_ptr<ServerPlayer> player) {
    // broadcastAll(std::shared_ptr<PlayerInfoPacket>( new
    // PlayerInfoPacket(player->name, true, 1000) ) );
    if (player->connection->getNetworkPlayer()) {
        broadcastAll(std::make_shared<PlayerInfoPacket>(player));
    }

    players.push_back(player);

    // 4J Added
    addPlayerToReceiving(player);

    // Ensure the area the player is spawning in is loaded!
    ServerLevel* level = server->getLevel(player->dimension);

    // 4J Stu - TU-1 hotfix
    // Fix for #13150 - When a player loads/joins a game after saving/leaving in
    // the nether, sometimes they are spawned on top of the nether and cannot
    // mine down Some code from here has been moved to the above
    // validatePlayerSpawnPosition function

    // 4J Stu - Swapped these lines about so that we get the chunk visiblity
    // packet way ahead of all the add tracked entity packets Fix for #9169 -
    // ART : Sign text is replaced with the words Awaiting approval.
    changeDimension(player, nullptr);
    level->addEntity(player);

    for (int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> op = players.at(i);
        // player->connection->send(std::shared_ptr<PlayerInfoPacket>( new
        // PlayerInfoPacket(op->name, true, op->latency) ) );
        if (op->connection->getNetworkPlayer()) {
            player->connection->send(std::make_shared<PlayerInfoPacket>(op));
        }
    }

    if (level->isAtLeastOnePlayerSleeping()) {
        std::shared_ptr<ServerPlayer> firstSleepingPlayer = nullptr;
        for (unsigned int i = 0; i < players.size(); i++) {
            std::shared_ptr<ServerPlayer> thisPlayer = players[i];
            if (thisPlayer->isSleeping()) {
                if (firstSleepingPlayer == nullptr)
                    firstSleepingPlayer = thisPlayer;
                thisPlayer->connection->send(std::make_shared<ChatPacket>(
                    thisPlayer->name, ChatPacket::e_ChatBedMeSleep));
            }
        }
        player->connection->send(std::make_shared<ChatPacket>(
            firstSleepingPlayer->name, ChatPacket::e_ChatBedPlayerSleep));
    }
}

void PlayerList::move(std::shared_ptr<ServerPlayer> player) {
    player->getLevel()->getChunkMap()->move(player);
}

void PlayerList::remove(std::shared_ptr<ServerPlayer> player) {
    save(player);
    // 4J Stu - We don't want to save the map data for guests, so when we are
    // sure that the player is gone delete the map
    if (player->isGuest()) playerIo->deleteMapFilesForPlayer(player);
    ServerLevel* level = player->getLevel();
    if (player->riding != nullptr) {
        // remove mount first because the player unmounts when being
        // removed, also remove mount because it's saved in the player's
        // save tag
        level->removeEntityImmediately(player->riding);
        Log::info("removing player mount");
    }
    level->removeEntity(player);
    level->getChunkMap()->remove(player);
    auto it = find(players.begin(), players.end(), player);
    if (it != players.end()) {
        players.erase(it);
    }
    // broadcastAll(std::shared_ptr<PlayerInfoPacket>( new
    // PlayerInfoPacket(player->name, false, 9999) ) );

    removePlayerFromReceiving(player);
    player->connection = nullptr;  // Must remove reference to connection, or
                                   // else there is a circular dependency
    delete player->gameMode;  // Gamemode also needs deleted as it references
                              // back to this player
    player->gameMode = nullptr;

    // 4J Stu - Save all the players currently in the game, which will also free
    // up unused map id slots if required, and remove old players
    saveAll(nullptr, false);
}

std::shared_ptr<ServerPlayer> PlayerList::getPlayerForLogin(
    PendingConnection* pendingConnection, const std::string& userName,
    PlayerUID xuid, PlayerUID onlineXuid) {
    if (players.size() >= maxPlayers) {
        pendingConnection->disconnect(DisconnectPacket::eDisconnect_ServerFull);
        return std::shared_ptr<ServerPlayer>();
    }

    std::shared_ptr<ServerPlayer> player = std::shared_ptr<ServerPlayer>(
        new ServerPlayer(server, server->getLevel(0), userName,
                         new ServerPlayerGameMode(server->getLevel(0))));
    player->gameMode->player = player;  // 4J added as had to remove this
                                        // assignment from ServerPlayer ctor
    player->setXuid(xuid);              // 4J Added
    player->setOnlineXuid(onlineXuid);  // 4J Added

    // Work out the base server player settings
    INetworkPlayer* networkPlayer =
        pendingConnection->connection->getSocket()->getPlayer();
    if (networkPlayer != nullptr && !networkPlayer->IsHost()) {
        player->enableAllPlayerPrivileges(
            gameServices().getGameHostOption(eGameHostOption_TrustPlayers) > 0);
    }

    // 4J Added
    LevelRuleset* serverRuleDefs = gameServices().getGameRuleDefinitions();
    if (serverRuleDefs != nullptr) {
        player->gameMode->setGameRules(
            GameRuleDefinition::generateNewGameRulesInstance(
                GameRulesInstance::eGameRulesInstanceType_ServerPlayer,
                serverRuleDefs, pendingConnection->connection));
    }

    return player;
}

std::shared_ptr<ServerPlayer> PlayerList::respawn(
    std::shared_ptr<ServerPlayer> serverPlayer, int targetDimension,
    bool keepAllPlayerData) {
    // How we handle the entity tracker depends on whether we are the primary
    // player currently, and whether there will be any player in the same system
    // in the same dimension once we finish respawning.
    bool isPrimary = canReceiveAllPackets(
        serverPlayer);  // Is this the primary player in its current dimension?
    int oldDimension = serverPlayer->dimension;
    bool isEmptying =
        (targetDimension !=
         oldDimension);  // We're not emptying this dimension on this machine if
                         // this player is going back into the same dimension

    // Also consider if there is another player on this machine which is in the
    // same dimension and can take over as primary player
    if (isEmptying) {
        INetworkPlayer* thisPlayer =
            serverPlayer->connection->getNetworkPlayer();

        for (unsigned int i = 0; i < players.size(); i++) {
            std::shared_ptr<ServerPlayer> ep = players[i];
            if (ep == serverPlayer) continue;
            if (ep->dimension != oldDimension) continue;

            INetworkPlayer* otherPlayer = ep->connection->getNetworkPlayer();
            if (otherPlayer != nullptr &&
                thisPlayer->IsSameSystem(otherPlayer)) {
                // There's another player here in the same dimension - we're not
                // the last one out
                isEmptying = false;
            }
        }
    }

    // Now we know where we stand, the actions to take are as follows:
    // (1) if this isn't the primary player, then we just need to remove it from
    // the entity tracker (2) if this Is the primary player then:
    //		(a) if isEmptying is true, then remove the player from the
    // tracker, and send "remove entity" packets for anything seen (this is the
    // original behaviour of the code) 		(b) if isEmptying is false, then
    // we'll be transferring control of entity tracking to another player

    if (isPrimary) {
        if (isEmptying) {
            Log::info("Emptying this dimension\n");
            serverPlayer->getLevel()->getTracker()->clear(serverPlayer);
        } else {
            Log::info("Transferring... storing flags\n");
            serverPlayer->getLevel()->getTracker()->removeEntity(serverPlayer);
        }
    } else {
        Log::info("Not primary player\n");
        serverPlayer->getLevel()->getTracker()->removeEntity(serverPlayer);
    }

    serverPlayer->getLevel()->getChunkMap()->remove(serverPlayer);
    auto it = find(players.begin(), players.end(), serverPlayer);
    if (it != players.end()) {
        players.erase(it);
    }
    server->getLevel(serverPlayer->dimension)
        ->removeEntityImmediately(serverPlayer);

    Pos* bedPosition = serverPlayer->getRespawnPosition();
    bool spawnForced = serverPlayer->isRespawnForced();

    removePlayerFromReceiving(serverPlayer);
    serverPlayer->dimension = targetDimension;

    EDefaultSkins skin = serverPlayer->getPlayerDefaultSkin();
    std::uint8_t playerIndex = serverPlayer->getPlayerIndex();

    PlayerUID playerXuid = serverPlayer->getXuid();
    PlayerUID playerOnlineXuid = serverPlayer->getOnlineXuid();

    std::shared_ptr<ServerPlayer> player = std::shared_ptr<ServerPlayer>(
        new ServerPlayer(server, server->getLevel(serverPlayer->dimension),
                         serverPlayer->getName(),
                         new ServerPlayerGameMode(
                             server->getLevel(serverPlayer->dimension))));
    player->connection = serverPlayer->connection;
    player->restoreFrom(serverPlayer, keepAllPlayerData);
    if (keepAllPlayerData) {
        // Fix for #81759 - TU9: Content: Gameplay: Entering The End Exit Portal
        // replaces the Player's currently held item with the first one from the
        // Quickbar
        player->inventory->selected = serverPlayer->inventory->selected;
    }
    player->gameMode->player = player;  // 4J added as had to remove this
                                        // assignment from ServerPlayer ctor
    player->setXuid(playerXuid);        // 4J Added
    player->setOnlineXuid(playerOnlineXuid);  // 4J Added

    // 4J Stu - Don't reuse the id. If we do, then the player can be re-added
    // after being removed, but the add packet gets sent before the remove
    // packet
    // player->entityId = serverPlayer->entityId;

    player->setPlayerDefaultSkin(skin);
    player->setIsGuest(serverPlayer->isGuest());
    player->setPlayerIndex(playerIndex);
    player->setCustomSkin(serverPlayer->getCustomSkin());
    player->setCustomCape(serverPlayer->getCustomCape());
    player->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_All,
                                   serverPlayer->getAllPlayerGamePrivileges());
    player->gameMode->setGameRules(serverPlayer->gameMode->getGameRules());
    player->dimension = targetDimension;

    // 4J Stu - Added this as we need to know earlier if the player is the
    // player for this connection so that we can work out if they are the
    // primary for the system and can receive all packets
    player->connection->setPlayer(player);

    addPlayerToReceiving(player);

    ServerLevel* level = server->getLevel(serverPlayer->dimension);

    // reset the player's game mode (first pick from old, then copy level if
    // necessary)
    updatePlayerGameMode(player, serverPlayer, level);

    if (serverPlayer->wonGame && targetDimension == oldDimension &&
        serverPlayer->getHealth() > 0) {
        // If the player is still alive and respawning to the same dimension,
        // they are just being added back from someone else viewing the Win
        // screen
        player->moveTo(serverPlayer->x, serverPlayer->y, serverPlayer->z,
                       serverPlayer->yRot, serverPlayer->xRot);
        if (bedPosition != nullptr) {
            player->setRespawnPosition(bedPosition, spawnForced);
            delete bedPosition;
        }
        // Fix for #81759 - TU9: Content: Gameplay: Entering The End Exit Portal
        // replaces the Player's currently held item with the first one from the
        // Quickbar
        player->inventory->selected = serverPlayer->inventory->selected;
    } else if (bedPosition != nullptr) {
        Pos* respawnPosition = Player::checkBedValidRespawnPosition(
            server->getLevel(serverPlayer->dimension), bedPosition,
            spawnForced);
        if (respawnPosition != nullptr) {
            player->moveTo(respawnPosition->x + 0.5f, respawnPosition->y + 0.1f,
                           respawnPosition->z + 0.5f, 0, 0);
            player->setRespawnPosition(bedPosition, spawnForced);
        } else {
            player->connection->send(std::make_shared<GameEventPacket>(
                GameEventPacket::NO_RESPAWN_BED_AVAILABLE, 0));
        }
        delete bedPosition;
    }

    // Ensure the area the player is spawning in is loaded!
    level->cache->create(((int)player->x) >> 4, ((int)player->z) >> 4);

    while (!level->getCubes(player, &player->bb)->empty()) {
        player->setPos(player->x, player->y + 1, player->z);
    }

    player->connection->send(std::make_shared<RespawnPacket>(
        (char)player->dimension, player->level->getSeed(),
        player->level->getMaxBuildHeight(),
        player->gameMode->getGameModeForPlayer(), level->difficulty,
        level->getLevelData()->getGenerator(), player->level->useNewSeaLevel(),
        player->entityId, level->getLevelData()->getXZSize(),
        level->getLevelData()->getHellScale()));
    player->connection->teleport(player->x, player->y, player->z, player->yRot,
                                 player->xRot);
    player->connection->send(std::make_shared<SetExperiencePacket>(
        player->experienceProgress, player->totalExperience,
        player->experienceLevel));

    if (keepAllPlayerData) {
        std::vector<MobEffectInstance*>* activeEffects =
            player->getActiveEffects();
        for (auto it = activeEffects->begin(); it != activeEffects->end();
             ++it) {
            MobEffectInstance* effect = *it;

            player->connection->send(std::shared_ptr<UpdateMobEffectPacket>(
                new UpdateMobEffectPacket(player->entityId, effect)));
        }
        delete activeEffects;
        player->getEntityData()->markDirty(Mob::DATA_EFFECT_COLOR_ID);
    }

    sendLevelInfo(player, level);

    level->getChunkMap()->add(player);
    level->addEntity(player);
    players.push_back(player);

    player->initMenu();
    player->setHealth(player->getHealth());

    // 4J-JEV - Dying before this point in the tutorial is pretty annoying,
    // making sure to remove health/hunger and give you back your meat.
    if (Minecraft::GetInstance()->isTutorial() &&
        (!Minecraft::GetInstance()->gameMode->getTutorial()->isStateCompleted(
            e_Tutorial_State_Food_Bar))) {
        gameServices().getGameRuleDefinitions()->postProcessPlayer(player);
    }

    if (oldDimension == 1 && player->dimension != 1) {
        player->displayClientMessage(IDS_PLAYER_LEFT_END);
    }

    return player;
}

void PlayerList::toggleDimension(std::shared_ptr<ServerPlayer> player,
                                 int targetDimension) {
    int lastDimension = player->dimension;
    // How we handle the entity tracker depends on whether we are the primary
    // player currently, and whether there will be any player in the same system
    // in the same dimension once we finish respawning.
    bool isPrimary = canReceiveAllPackets(
        player);  // Is this the primary player in its current dimension?
    bool isEmptying = true;

    // Also consider if there is another player on this machine which is in the
    // same dimension and can take over as primary player
    INetworkPlayer* thisPlayer = player->connection->getNetworkPlayer();

    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> ep = players[i];
        if (ep == player) continue;
        if (ep->dimension != lastDimension) continue;

        INetworkPlayer* otherPlayer = ep->connection->getNetworkPlayer();
        if (otherPlayer != nullptr && thisPlayer->IsSameSystem(otherPlayer)) {
            // There's another player here in the same dimension - we're not the
            // last one out
            isEmptying = false;
        }
    }

    // Now we know where we stand, the actions to take are as follows:
    // (1) if this isn't the primary player, then we just need to remove it from
    // the entity tracker (2) if this Is the primary player then:
    //		(a) if isEmptying is true, then remove the player from the
    // tracker, and send "remove entity" packets for anything seen (this is the
    // original behaviour of the code) 		(b) if isEmptying is false, then
    // we'll be transferring control of entity tracking to another player

    if (isPrimary) {
        if (isEmptying) {
            Log::info("Toggle... Emptying this dimension\n");
            player->getLevel()->getTracker()->clear(player);
        } else {
            Log::info("Toggle...  transferring\n");
            player->getLevel()->getTracker()->removeEntity(player);
        }
    } else {
        Log::info("Toggle...  Not primary player\n");
        player->getLevel()->getTracker()->removeEntity(player);
    }

    ServerLevel* oldLevel = server->getLevel(player->dimension);

    // 4J Stu - Do this much earlier so we don't end up unloading chunks in the
    // wrong dimension
    player->getLevel()->getChunkMap()->remove(player);

    if (player->dimension != 1 && targetDimension == 1) {
        player->displayClientMessage(IDS_PLAYER_ENTERED_END);
    } else if (player->dimension == 1) {
        player->displayClientMessage(IDS_PLAYER_LEFT_END);
    }

    player->dimension = targetDimension;

    ServerLevel* newLevel = server->getLevel(player->dimension);

    // 4J Stu - Fix for #46423 - TU5: Art: Code: No burning animation visible
    // after entering The Nether while burning
    player->clearFire();  // Stop burning if travelling through a portal

    // 4J Stu Added so that we remove entities from the correct level, after the
    // respawn packet we will be in the wrong level
    player->flushEntitiesToRemove();

    player->connection->send(std::make_shared<RespawnPacket>(
        (char)player->dimension, newLevel->getSeed(),
        newLevel->getMaxBuildHeight(), player->gameMode->getGameModeForPlayer(),
        newLevel->difficulty, newLevel->getLevelData()->getGenerator(),
        newLevel->useNewSeaLevel(), player->entityId,
        newLevel->getLevelData()->getXZSize(),
        newLevel->getLevelData()->getHellScale()));

    oldLevel->removeEntityImmediately(player);
    player->removed = false;

    repositionAcrossDimension(player, lastDimension, oldLevel, newLevel);
    changeDimension(player, oldLevel);

    player->gameMode->setLevel(newLevel);

    // Resend the teleport if we haven't yet sent the chunk they will land on
    if (!NetworkService.SystemFlagGet(
            player->connection->getNetworkPlayer(),
            ServerPlayer::getFlagIndexForChunk(
                ChunkPos(player->xChunk, player->zChunk),
                player->level->dimension->id))) {
        player->connection->teleport(player->x, player->y, player->z,
                                     player->yRot, player->xRot, false);
        // Force sending of the current chunk
        player->doTick(true, true, true);
    }

    player->connection->teleport(player->x, player->y, player->z, player->yRot,
                                 player->xRot);

    // 4J Stu - Fix for #64683 - Customer Encountered: TU7: Content: Gameplay:
    // Potion effects are removed after using the Nether Portal
    std::vector<MobEffectInstance*>* activeEffects = player->getActiveEffects();
    for (auto it = activeEffects->begin(); it != activeEffects->end(); ++it) {
        MobEffectInstance* effect = *it;

        player->connection->send(std::shared_ptr<UpdateMobEffectPacket>(
            new UpdateMobEffectPacket(player->entityId, effect)));
    }
    delete activeEffects;
    player->getEntityData()->markDirty(Mob::DATA_EFFECT_COLOR_ID);

    sendLevelInfo(player, newLevel);
    sendAllPlayerInfo(player);
}

void PlayerList::repositionAcrossDimension(std::shared_ptr<Entity> entity,
                                           int lastDimension,
                                           ServerLevel* oldLevel,
                                           ServerLevel* newLevel) {
    double xt = entity->x;
    double zt = entity->z;
    double xOriginal = entity->x;
    double yOriginal = entity->y;
    double zOriginal = entity->z;
    float yRotOriginal = entity->yRot;
    double scale =
        newLevel->getLevelData()
            ->getHellScale();  // 4J Scale was 8 but this is all we can fit in
    if (entity->dimension == -1) {
        xt /= scale;
        zt /= scale;
        entity->moveTo(xt, entity->y, zt, entity->yRot, entity->xRot);
        if (entity->isAlive()) {
            oldLevel->tick(entity, false);
        }
    } else if (entity->dimension == 0) {
        xt *= scale;
        zt *= scale;
        entity->moveTo(xt, entity->y, zt, entity->yRot, entity->xRot);
        if (entity->isAlive()) {
            oldLevel->tick(entity, false);
        }
    } else {
        Pos* p;

        if (lastDimension == 1) {
            // Coming from the end
            p = newLevel->getSharedSpawnPos();
        } else {
            // Going to the end
            p = newLevel->getDimensionSpecificSpawn();
        }

        xt = p->x;
        entity->y = p->y;
        zt = p->z;
        delete p;
        entity->moveTo(xt, entity->y, zt, 90, 0);
        if (entity->isAlive()) {
            oldLevel->tick(entity, false);
        }
    }

    if (entity->GetType() == eTYPE_SERVERPLAYER) {
        std::shared_ptr<ServerPlayer> player =
            std::dynamic_pointer_cast<ServerPlayer>(entity);
        removePlayerFromReceiving(player, false, lastDimension);
        addPlayerToReceiving(player);
    }

    if (lastDimension != 1) {
        xt = (double)std::clamp((int)xt, -Level::MAX_LEVEL_SIZE + 128,
                                Level::MAX_LEVEL_SIZE - 128);
        zt = (double)std::clamp((int)zt, -Level::MAX_LEVEL_SIZE + 128,
                                Level::MAX_LEVEL_SIZE - 128);
        if (entity->isAlive()) {
            newLevel->addEntity(entity);
            entity->moveTo(xt, entity->y, zt, entity->yRot, entity->xRot);
            newLevel->tick(entity, false);
            newLevel->cache->autoCreate = true;
            newLevel->getPortalForcer()->force(entity, xOriginal, yOriginal,
                                               zOriginal, yRotOriginal);
            newLevel->cache->autoCreate = false;
        }
    }

    entity->setLevel(newLevel);
}

void PlayerList::tick() {
    // 4J - brought changes to how often this is sent forward from 1.2.3
    if (++sendAllPlayerInfoIn > SEND_PLAYER_INFO_INTERVAL) {
        sendAllPlayerInfoIn = 0;
    }

    if (sendAllPlayerInfoIn < players.size()) {
        std::shared_ptr<ServerPlayer> op = players[sendAllPlayerInfoIn];
        // broadcastAll(std::shared_ptr<PlayerInfoPacket>( new
        // PlayerInfoPacket(op->name, true, op->latency) ) );
        if (op->connection->getNetworkPlayer()) {
            broadcastAll(std::make_shared<PlayerInfoPacket>(op));
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_closePlayersCS);
        while (!m_smallIdsToClose.empty()) {
            std::uint8_t smallId = m_smallIdsToClose.front();
            m_smallIdsToClose.pop_front();

            std::shared_ptr<ServerPlayer> player = nullptr;

            for (unsigned int i = 0; i < players.size(); i++) {
                std::shared_ptr<ServerPlayer> p = players.at(i);
                // 4J Stu - May be being a bit overprotective with all the
                // nullptr checks, but adding late in TU7 so want to be safe
                if (p != nullptr && p->connection != nullptr &&
                    p->connection->connection != nullptr &&
                    p->connection->connection->getSocket() != nullptr &&
                    p->connection->connection->getSocket()->getSmallId() ==
                        smallId) {
                    player = p;
                    break;
                }
            }

            if (player != nullptr) {
                player->connection->disconnect(
                    DisconnectPacket::eDisconnect_Closed);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_kickPlayersCS);
        while (!m_smallIdsToKick.empty()) {
            std::uint8_t smallId = m_smallIdsToKick.front();
            m_smallIdsToKick.pop_front();
            INetworkPlayer* selectedPlayer =
                NetworkService.GetPlayerBySmallId(smallId);
            if (selectedPlayer != nullptr) {
                if (selectedPlayer->IsLocal() != true) {
                    // #if 0
                    PlayerUID xuid = selectedPlayer->GetUID();
                    // Kick this player from the game
                    std::shared_ptr<ServerPlayer> player = nullptr;

                    for (unsigned int i = 0; i < players.size(); i++) {
                        std::shared_ptr<ServerPlayer> p = players.at(i);
                        PlayerUID playersXuid = p->getOnlineXuid();
                        if (p != nullptr &&
                            PlatformProfile.AreXUIDSEqual(playersXuid, xuid)) {
                            player = p;
                            break;
                        }
                    }

                    if (player != nullptr) {
                        m_bannedXuids.push_back(player->getOnlineXuid());
                        // 4J Stu - If we have kicked a player, make sure that
                        // they have no privileges if they later try to join the
                        // world when trust players is off
                        player->enableAllPlayerPrivileges(false);
                        player->connection->setWasKicked();
                        player->connection->send(
                            std::shared_ptr<DisconnectPacket>(
                                new DisconnectPacket(
                                    DisconnectPacket::eDisconnect_Kicked)));
                    }
                    // #endif
                }
            }
        }
    }

    // Check our receiving players, and if they are dead see if we can replace
    // them
    for (unsigned int dim = 0; dim < 2; ++dim) {
        for (unsigned int i = 0; i < receiveAllPlayers[dim].size(); ++i) {
            std::shared_ptr<ServerPlayer> currentPlayer =
                receiveAllPlayers[dim][i];
            if (currentPlayer->removed) {
                std::shared_ptr<ServerPlayer> newPlayer =
                    findAlivePlayerOnSystem(currentPlayer);
                if (newPlayer != nullptr) {
                    receiveAllPlayers[dim][i] = newPlayer;
                    Log::info(
                        "Replacing primary player %s with %s in dimension "
                        "%d\n",
                        currentPlayer->name.c_str(), newPlayer->name.c_str(),
                        dim);
                }
            }
        }
    }
}

bool PlayerList::isTrackingTile(int x, int y, int z, int dimension) {
    return server->getLevel(dimension)->getChunkMap()->isTrackingTile(x, y, z);
}

// 4J added - make sure that any tile updates for the chunk at this location get
// prioritised for sending
void PlayerList::prioritiseTileChanges(int x, int y, int z, int dimension) {
    server->getLevel(dimension)->getChunkMap()->prioritiseTileChanges(x, y, z);
}

void PlayerList::broadcastAll(std::shared_ptr<Packet> packet) {
    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> player = players[i];
        player->connection->send(packet);
    }
}

void PlayerList::broadcastAll(std::shared_ptr<Packet> packet, int dimension) {
    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> player = players[i];
        if (player->dimension == dimension) player->connection->send(packet);
    }
}

std::string PlayerList::getPlayerNames() {
    std::string msg;
    for (unsigned int i = 0; i < players.size(); i++) {
        if (i > 0) msg += ", ";
        msg += players[i]->name;
    }
    return msg;
}

bool PlayerList::isWhiteListed(const std::string& name) { return true; }

bool PlayerList::isOp(const std::string& name) { return false; }

bool PlayerList::isOp(std::shared_ptr<ServerPlayer> player) {
    bool cheatsEnabled =
        gameServices().getGameHostOption(eGameHostOption_CheatsEnabled);
#if defined(_DEBUG_MENUS_ENABLED)
    cheatsEnabled = cheatsEnabled || gameServices().getUseDPadForDebug();
#endif
    INetworkPlayer* networkPlayer = player->connection->getNetworkPlayer();
    bool isOp = cheatsEnabled &&
                (player->isModerator() ||
                 (networkPlayer != nullptr && networkPlayer->IsHost()));
    return isOp;
}

std::shared_ptr<ServerPlayer> PlayerList::getPlayer(const std::string& name) {
    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> p = players[i];
        if (p->name ==
            name)  // 4J - used to be case insensitive (using equalsIgnoreCase)
                   // - imagine we'll be shifting to XUIDs anyway
        {
            return p;
        }
    }
    return nullptr;
}

// 4J Added
std::shared_ptr<ServerPlayer> PlayerList::getPlayer(PlayerUID uid) {
    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> p = players[i];
        if (p->getXuid() == uid ||
            p->getOnlineXuid() == uid)  // 4J - used to be case insensitive
                                        // (using equalsIgnoreCase) - imagine
                                        // we'll be shifting to XUIDs anyway
        {
            return p;
        }
    }
    return nullptr;
}

std::shared_ptr<ServerPlayer> PlayerList::getNearestPlayer(Pos* position,
                                                           int range) {
    if (players.empty()) return nullptr;
    if (position == nullptr) return players.at(0);
    std::shared_ptr<ServerPlayer> current = nullptr;
    double dist = -1;
    int rangeSqr = range * range;

    for (int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> next = players.at(i);
        double newDist =
            position->distSqr(next->getCommandSenderWorldPosition());

        if ((dist == -1 || newDist < dist) &&
            (range <= 0 || newDist <= rangeSqr)) {
            dist = newDist;
            current = next;
        }
    }

    return current;
}

std::vector<ServerPlayer>* PlayerList::getPlayers(
    Pos* position, int rangeMin, int rangeMax, int count, int mode,
    int levelMin, int levelMax,
    std::unordered_map<std::string, int>* scoreRequirements,
    const std::string& playerName, const std::string& teamName, Level* level) {
    Log::info("getPlayers NOT IMPLEMENTED!");
    return nullptr;

    /*if (players.empty()) return nullptr;
    vector<shared_ptr<ServerPlayer> > result = new
    vector<shared_ptr<ServerPlayer> >(); bool reverse = count < 0; bool
    playerNameNot = !playerName.empty() && playerName.startsWith("!"); bool
    teamNameNot = !teamName.empty() && teamName.startsWith("!"); int rangeMinSqr
    = rangeMin * rangeMin; int rangeMaxSqr = rangeMax * rangeMax; count =
    Mth.abs(count);

    if (playerNameNot) playerName = playerName.substring(1);
    if (teamNameNot) teamName = teamName.substring(1);

    for (int i = 0; i < players.size(); i++) {
    ServerPlayer player = players.get(i);

    if (level != null && player.level != level) continue;
    if (playerName != null) {
    if (playerNameNot == playerName.equalsIgnoreCase(player.getAName()))
    continue;
    }
    if (teamName != null) {
    Team team = player.getTeam();
    String actualName = team == null ? "" : team.getName();
    if (teamNameNot == teamName.equalsIgnoreCase(actualName)) continue;
    }

    if (position != null && (rangeMin > 0 || rangeMax > 0)) {
    float distance = position.distSqr(player.getCommandSenderWorldPosition());
    if (rangeMin > 0 && distance < rangeMinSqr) continue;
    if (rangeMax > 0 && distance > rangeMaxSqr) continue;
    }

    if (!meetsScoreRequirements(player, scoreRequirements)) continue;

    if (mode != GameType.NOT_SET.getId() && mode !=
    player.gameMode.getGameModeForPlayer().getId()) continue; if (levelMin > 0
    && player.experienceLevel < levelMin) continue; if (player.experienceLevel >
    levelMax) continue;

    result.add(player);
    }

    if (position != null) Collections.sort(result, new
    PlayerDistanceComparator(position)); if (reverse)
    Collections.reverse(result); if (count > 0) result = result.subList(0,
    Math.min(count, result.size()));

    return result;*/
}

bool PlayerList::meetsScoreRequirements(
    std::shared_ptr<Player> player,
    std::unordered_map<std::string, int> scoreRequirements) {
    Log::info("meetsScoreRequirements NOT IMPLEMENTED!");
    return false;

    // if (scoreRequirements == null || scoreRequirements.size() == 0) return
    // true;

    // for (Map.Entry<String, Integer> requirement :
    // scoreRequirements.entrySet()) { 	String name = requirement.getKey();
    //	bool min = false;

    //	if (name.endsWith("_min") && name.length() > 4) {
    //		min = true;
    //		name = name.substring(0, name.length() - 4);
    //	}

    //	Scoreboard scoreboard = player.getScoreboard();
    //	Objective objective = scoreboard.getObjective(name);
    //	if (objective == null) return false;
    //	Score score = player.getScoreboard().getPlayerScore(player.getAName(),
    // objective); 	int value = score.getScore();

    //	if (value < requirement.getValue() && min) {
    //		return false;
    //	} else if (value > requirement.getValue() && !min) {
    //		return false;
    //	}
    //}

    // return true;
}

void PlayerList::sendMessage(const std::string& name,
                             const std::string& message) {
    std::shared_ptr<ServerPlayer> player = getPlayer(name);
    if (player != nullptr) {
        player->connection->send(std::make_shared<ChatPacket>(message));
    }
}

void PlayerList::broadcast(double x, double y, double z, double range,
                           int dimension, std::shared_ptr<Packet> packet) {
    broadcast(nullptr, x, y, z, range, dimension, packet);
}

void PlayerList::broadcast(std::shared_ptr<Player> except, double x, double y,
                           double z, double range, int dimension,
                           std::shared_ptr<Packet> packet) {
    // 4J - altered so that we don't send to the same machine more than once.
    // Add the source player to the machines we have "sent" to as it doesn't
    // need to go to that machine either
    std::vector<std::shared_ptr<ServerPlayer> > sentTo;
    if (except != nullptr) {
        sentTo.push_back(std::dynamic_pointer_cast<ServerPlayer>(except));
    }

    for (unsigned int i = 0; i < players.size(); i++) {
        std::shared_ptr<ServerPlayer> p = players[i];
        if (p == except) continue;
        if (p->dimension != dimension) continue;

        // 4J - don't send to the same machine more than once
        bool dontSend = false;
        if (sentTo.size()) {
            INetworkPlayer* thisPlayer = p->connection->getNetworkPlayer();
            if (thisPlayer == nullptr) {
                dontSend = true;
            } else {
                for (unsigned int j = 0; j < sentTo.size(); j++) {
                    std::shared_ptr<ServerPlayer> player2 = sentTo[j];
                    INetworkPlayer* otherPlayer =
                        player2->connection->getNetworkPlayer();
                    if (otherPlayer != nullptr &&
                        thisPlayer->IsSameSystem(otherPlayer)) {
                        dontSend = true;
                    }
                }
            }
        }
        if (dontSend) {
            continue;
        }

        double xd = x - p->x;
        double yd = y - p->y;
        double zd = z - p->z;
        if (xd * xd + yd * yd + zd * zd < range * range) {
            p->connection->send(packet);
            sentTo.push_back(p);
        }
    }
}

void PlayerList::saveAll(ProgressListener* progressListener,
                         bool bDeleteGuestMaps /*= false*/) {
    if (progressListener != nullptr)
        progressListener->progressStart(IDS_PROGRESS_SAVING_PLAYERS);
    // 4J - playerIo can be nullptr if we have have to exit a game really early
    // on due to network failure
    if (playerIo) {
        playerIo->saveAllCachedData();
        for (unsigned int i = 0; i < players.size(); i++) {
            playerIo->save(players[i]);

            // 4J Stu - We don't want to save the map data for guests, so when
            // we are sure that the player is gone delete the map
            if (bDeleteGuestMaps && players[i]->isGuest())
                playerIo->deleteMapFilesForPlayer(players[i]);

            if (progressListener != nullptr)
                progressListener->progressStagePercentage(
                    (i * 100) / ((int)players.size()));
        }
        playerIo->clearOldPlayerFiles();
        playerIo->saveMapIdLookup();
    }
}

void PlayerList::whiteList(const std::string& playerName) {}

void PlayerList::blackList(const std::string& playerName) {}

void PlayerList::reloadWhitelist() {}

void PlayerList::sendLevelInfo(std::shared_ptr<ServerPlayer> player,
                               ServerLevel* level) {
    player->connection->send(std::make_shared<SetTimePacket>(
        level->getGameTime(), level->getDayTime(),
        level->getGameRules()->getBoolean(GameRules::RULE_DAYLIGHT)));
    if (level->isRaining()) {
        player->connection->send(std::shared_ptr<GameEventPacket>(
            new GameEventPacket(GameEventPacket::START_RAINING, 0)));
    } else {
        // 4J Stu - Fix for #44836 - Customer Encountered: Out of Sync Weather
        // [A-10] If it was raining when the player left the level, and is now
        // not raining we need to make sure that state is updated
        player->connection->send(std::shared_ptr<GameEventPacket>(
            new GameEventPacket(GameEventPacket::STOP_RAINING, 0)));
    }

    // send the stronghold position if there is one
    if ((level->dimension->id == 0) &&
        level->getLevelData()->getHasStronghold()) {
        player->connection->send(std::make_shared<XZPacket>(
            XZPacket::STRONGHOLD, level->getLevelData()->getXStronghold(),
            level->getLevelData()->getZStronghold()));
    }
}

void PlayerList::sendAllPlayerInfo(std::shared_ptr<ServerPlayer> player) {
    player->refreshContainer(player->inventoryMenu);
    player->resetSentInfo();
    player->connection->send(std::shared_ptr<SetCarriedItemPacket>(
        new SetCarriedItemPacket(player->inventory->selected)));
}

int PlayerList::getPlayerCount() { return (int)players.size(); }

int PlayerList::getPlayerCount(ServerLevel* level) {
    int count = 0;

    for (auto it = players.begin(); it != players.end(); ++it) {
        if ((*it)->level == level) ++count;
    }

    return count;
}

int PlayerList::getMaxPlayers() { return maxPlayers; }

MinecraftServer* PlayerList::getServer() { return server; }

int PlayerList::getViewDistance() { return viewDistance; }

void PlayerList::setOverrideGameMode(GameType* gameMode) {
    overrideGameMode = gameMode;
}

void PlayerList::updatePlayerGameMode(std::shared_ptr<ServerPlayer> newPlayer,
                                      std::shared_ptr<ServerPlayer> oldPlayer,
                                      Level* level) {
    // reset the player's game mode (first pick from old, then copy level if
    // necessary)
    if (oldPlayer != nullptr) {
        newPlayer->gameMode->setGameModeForPlayer(
            oldPlayer->gameMode->getGameModeForPlayer());
    } else if (overrideGameMode != nullptr) {
        newPlayer->gameMode->setGameModeForPlayer(overrideGameMode);
    }
    newPlayer->gameMode->updateGameMode(level->getLevelData()->getGameType());
}

void PlayerList::setAllowCheatsForAllPlayers(bool allowCommands) {
    this->allowCheatsForAllPlayers = allowCommands;
}

std::shared_ptr<ServerPlayer> PlayerList::findAlivePlayerOnSystem(
    std::shared_ptr<ServerPlayer> player) {
    int dimIndex, playerDim;
    dimIndex = playerDim = player->dimension;
    if (dimIndex == -1)
        dimIndex = 1;
    else if (dimIndex == 1)
        dimIndex = 2;

    INetworkPlayer* thisPlayer = player->connection->getNetworkPlayer();
    if (thisPlayer != nullptr) {
        for (auto itP = players.begin(); itP != players.end(); ++itP) {
            std::shared_ptr<ServerPlayer> newPlayer = *itP;

            INetworkPlayer* otherPlayer =
                newPlayer->connection->getNetworkPlayer();

            if (!newPlayer->removed && newPlayer != player &&
                newPlayer->dimension == playerDim && otherPlayer != nullptr &&
                otherPlayer->IsSameSystem(thisPlayer)) {
                return newPlayer;
            }
        }
    }

    return nullptr;
}

void PlayerList::removePlayerFromReceiving(std::shared_ptr<ServerPlayer> player,
                                           bool usePlayerDimension /*= true*/,
                                           int dimension /*= 0*/) {
    int dimIndex, playerDim;
    dimIndex = playerDim = usePlayerDimension ? player->dimension : dimension;
    if (dimIndex == -1)
        dimIndex = 1;
    else if (dimIndex == 1)
        dimIndex = 2;

#if !defined(_CONTENT_PACKAGE)
    Log::info("Requesting remove player %s as primary in dimension %d\n",
              player->name.c_str(), dimIndex);
#endif
    bool playerRemoved = false;

    auto it = find(receiveAllPlayers[dimIndex].begin(),
                   receiveAllPlayers[dimIndex].end(), player);
    if (it != receiveAllPlayers[dimIndex].end()) {
#if !defined(_CONTENT_PACKAGE)
        Log::info("Remove: Removing player %s as primary in dimension %d\n",
                  player->name.c_str(), dimIndex);
#endif
        receiveAllPlayers[dimIndex].erase(it);
        playerRemoved = true;
    }

    INetworkPlayer* thisPlayer = player->connection->getNetworkPlayer();
    if (thisPlayer != nullptr && playerRemoved) {
        for (auto itP = players.begin(); itP != players.end(); ++itP) {
            std::shared_ptr<ServerPlayer> newPlayer = *itP;

            INetworkPlayer* otherPlayer =
                newPlayer->connection->getNetworkPlayer();

            if (newPlayer != player && newPlayer->dimension == playerDim &&
                otherPlayer != nullptr &&
                otherPlayer->IsSameSystem(thisPlayer)) {
#if !defined(_CONTENT_PACKAGE)
                Log::info(
                    "Remove: Adding player %s as primary in dimension %d\n",
                    newPlayer->name.c_str(), dimIndex);
#endif
                receiveAllPlayers[dimIndex].push_back(newPlayer);
                break;
            }
        }
    } else if (thisPlayer == nullptr) {
#if !defined(_CONTENT_PACKAGE)
        Log::info(
            "Remove: Qnet player for %s was nullptr so re-checking all "
            "players\n",
            player->name.c_str());
#endif
        // 4J Stu - Something went wrong, or possibly the QNet player left
        // before we got here. Re-check all active players and make sure they
        // have someone on their system to receive all packets
        for (auto itP = players.begin(); itP != players.end(); ++itP) {
            std::shared_ptr<ServerPlayer> newPlayer = *itP;
            INetworkPlayer* checkingPlayer =
                newPlayer->connection->getNetworkPlayer();

            if (checkingPlayer != nullptr) {
                int newPlayerDim = 0;
                if (newPlayer->dimension == -1)
                    newPlayerDim = 1;
                else if (newPlayer->dimension == 1)
                    newPlayerDim = 2;
                bool foundPrimary = false;
                for (auto it = receiveAllPlayers[newPlayerDim].begin();
                     it != receiveAllPlayers[newPlayerDim].end(); ++it) {
                    std::shared_ptr<ServerPlayer> primaryPlayer = *it;
                    INetworkPlayer* primPlayer =
                        primaryPlayer->connection->getNetworkPlayer();
                    if (primPlayer != nullptr &&
                        checkingPlayer->IsSameSystem(primPlayer)) {
                        foundPrimary = true;
                        break;
                    }
                }
                if (!foundPrimary) {
#if !defined(_CONTENT_PACKAGE)
                    Log::info(
                        "Remove: Adding player %s as primary in dimension "
                        "%d\n",
                        newPlayer->name.c_str(), newPlayerDim);
#endif
                    receiveAllPlayers[newPlayerDim].push_back(newPlayer);
                }
            }
        }
    }
}

void PlayerList::addPlayerToReceiving(std::shared_ptr<ServerPlayer> player) {
    int playerDim = 0;
    if (player->dimension == -1)
        playerDim = 1;
    else if (player->dimension == 1)
        playerDim = 2;

#if !defined(_CONTENT_PACKAGE)
    Log::info("Requesting add player %s as primary in dimension %d\n",
              player->name.c_str(), playerDim);
#endif

    bool shouldAddPlayer = true;

    INetworkPlayer* thisPlayer = player->connection->getNetworkPlayer();

    if (thisPlayer == nullptr) {
#if !defined(_CONTENT_PACKAGE)
        Log::info(
            "Add: Qnet player for player %s is nullptr so not adding them\n",
            player->name.c_str());
#endif
        shouldAddPlayer = false;
    } else {
        for (auto it = receiveAllPlayers[playerDim].begin();
             it != receiveAllPlayers[playerDim].end(); ++it) {
            std::shared_ptr<ServerPlayer> oldPlayer = *it;
            INetworkPlayer* checkingPlayer =
                oldPlayer->connection->getNetworkPlayer();
            if (checkingPlayer != nullptr &&
                checkingPlayer->IsSameSystem(thisPlayer)) {
                shouldAddPlayer = false;
                break;
            }
        }
    }

    if (shouldAddPlayer) {
#if !defined(_CONTENT_PACKAGE)
        Log::info("Add: Adding player %s as primary in dimension %d\n",
                  player->name.c_str(), playerDim);
#endif
        receiveAllPlayers[playerDim].push_back(player);
    }
}

bool PlayerList::canReceiveAllPackets(std::shared_ptr<ServerPlayer> player) {
    int playerDim = 0;
    if (player->dimension == -1)
        playerDim = 1;
    else if (player->dimension == 1)
        playerDim = 2;
    for (auto it = receiveAllPlayers[playerDim].begin();
         it != receiveAllPlayers[playerDim].end(); ++it) {
        std::shared_ptr<ServerPlayer> newPlayer = *it;
        if (newPlayer == player) {
            return true;
        }
    }
    return false;
}

void PlayerList::kickPlayerByShortId(std::uint8_t networkSmallId) {
    {
        std::lock_guard<std::mutex> lock(m_kickPlayersCS);
        m_smallIdsToKick.push_back(networkSmallId);
    }
}

void PlayerList::closePlayerConnectionBySmallId(std::uint8_t networkSmallId) {
    {
        std::lock_guard<std::mutex> lock(m_closePlayersCS);
        m_smallIdsToClose.push_back(networkSmallId);
    }
}

bool PlayerList::isXuidBanned(PlayerUID xuid) {
    if (xuid == INVALID_XUID) return false;

    bool banned = false;

    for (auto it = m_bannedXuids.begin(); it != m_bannedXuids.end(); ++it) {
        if (PlatformProfile.AreXUIDSEqual(xuid, *it)) {
            banned = true;
            break;
        }
    }

    return banned;
}

// AP added for Vita so the range can be increased once the level starts
void PlayerList::setViewDistance(int newViewDistance) {
    viewDistance = newViewDistance;
}
