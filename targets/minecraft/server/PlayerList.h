#pragma once
#include <cstdint>
#include <deque>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "nbt/CompoundTag.h"
#include "platform/PlatformTypes.h"

class ServerPlayer;
class PlayerChunkMap;
class MinecraftServer;
class PlayerIO;
class PendingConnection;
class Packet;
class ServerLevel;
class TileEntity;
class ProgressListener;
class GameType;
class LoginPacket;
class Connection;
class ServerScoreboard;
class Entity;
class Pos;
class Player;
class Level;
class CompoundTag;

class PlayerList {
private:
    static const int SEND_PLAYER_INFO_INTERVAL =
        20 * 10;  // 4J - brought forward from 1.2.3
    //    public static Logger logger = Logger.getLogger("Minecraft");
public:
    std::vector<std::shared_ptr<ServerPlayer> > players;

private:
    MinecraftServer* server;
    unsigned int maxPlayers;

    // 4J Added
    std::vector<PlayerUID> m_bannedXuids;
    std::deque<std::uint8_t> m_smallIdsToKick;
    std::mutex m_kickPlayersCS;
    std::deque<std::uint8_t> m_smallIdsToClose;
    std::mutex m_closePlayersCS;
    /* 4J - removed
            Set<String> bans = new HashSet<String>();
        Set<String> ipBans = new HashSet<String>();
        Set<String> ops = new HashSet<String>();
        Set<String> whitelist = new HashSet<String>();
        File banFile, ipBanFile, opFile, whiteListFile;
            */
    PlayerIO* playerIo;
    bool doWhiteList;

    GameType* overrideGameMode;
    bool allowCheatsForAllPlayers;
    int viewDistance;

    int sendAllPlayerInfoIn;

    // 4J Added to maintain which players in which dimensions can receive all
    // packet types
    std::vector<std::shared_ptr<ServerPlayer> > receiveAllPlayers[3];

private:
    std::shared_ptr<ServerPlayer> findAlivePlayerOnSystem(
        std::shared_ptr<ServerPlayer> currentPlayer);

public:
    void removePlayerFromReceiving(std::shared_ptr<ServerPlayer> player,
                                   bool usePlayerDimension = true,
                                   int dimension = 0);
    void addPlayerToReceiving(std::shared_ptr<ServerPlayer> player);
    bool canReceiveAllPackets(std::shared_ptr<ServerPlayer> player);

public:
    PlayerList(MinecraftServer* server);
    ~PlayerList();
    void placeNewPlayer(Connection* connection,
                        std::shared_ptr<ServerPlayer> player,
                        std::shared_ptr<LoginPacket> packet);

protected:
    void updateEntireScoreboard(ServerScoreboard* scoreboard,
                                std::shared_ptr<ServerPlayer> player);

public:
    void setLevel(std::vector<ServerLevel*>& levels);
    void changeDimension(std::shared_ptr<ServerPlayer> player,
                         ServerLevel* from);
    int getMaxRange();
    CompoundTag* load(std::shared_ptr<ServerPlayer> player);

protected:
    void save(std::shared_ptr<ServerPlayer> player);

public:
    void validatePlayerSpawnPosition(
        std::shared_ptr<ServerPlayer> player);  // 4J Added
    void add(std::shared_ptr<ServerPlayer> player);
    void move(std::shared_ptr<ServerPlayer> player);
    void remove(std::shared_ptr<ServerPlayer> player);
    std::shared_ptr<ServerPlayer> getPlayerForLogin(
        PendingConnection* pendingConnection, const std::string& userName,
        PlayerUID xuid, PlayerUID OnlineXuid);
    std::shared_ptr<ServerPlayer> respawn(
        std::shared_ptr<ServerPlayer> serverPlayer, int targetDimension,
        bool keepAllPlayerData);
    void toggleDimension(std::shared_ptr<ServerPlayer> player,
                         int targetDimension);
    void repositionAcrossDimension(std::shared_ptr<Entity> entity,
                                   int lastDimension, ServerLevel* oldLevel,
                                   ServerLevel* newLevel);
    void tick();
    bool isTrackingTile(int x, int y, int z, int dimension);         // 4J added
    void prioritiseTileChanges(int x, int y, int z, int dimension);  // 4J added
    void broadcastAll(std::shared_ptr<Packet> packet);
    void broadcastAll(std::shared_ptr<Packet> packet, int dimension);

    std::string getPlayerNames();

public:
    bool isWhiteListed(const std::string& name);
    bool isOp(const std::string& name);
    bool isOp(std::shared_ptr<ServerPlayer> player);  // 4J Added
    std::shared_ptr<ServerPlayer> getPlayer(const std::string& name);
    std::shared_ptr<ServerPlayer> getPlayer(PlayerUID uid);
    std::shared_ptr<ServerPlayer> getNearestPlayer(Pos* position, int range);
    std::vector<ServerPlayer>* getPlayers(
        Pos* position, int rangeMin, int rangeMax, int count, int mode,
        int levelMin, int levelMax,
        std::unordered_map<std::string, int>* scoreRequirements,
        const std::string& playerName, const std::string& teamName,
        Level* level);

private:
    bool meetsScoreRequirements(
        std::shared_ptr<Player> player,
        std::unordered_map<std::string, int> scoreRequirements);

public:
    void sendMessage(const std::string& name, const std::string& message);
    void broadcast(double x, double y, double z, double range, int dimension,
                   std::shared_ptr<Packet> packet);
    void broadcast(std::shared_ptr<Player> except, double x, double y, double z,
                   double range, int dimension, std::shared_ptr<Packet> packet);
    // 4J Added ProgressListener *progressListener param and bDeleteGuestMaps
    // param
    void saveAll(ProgressListener* progressListener,
                 bool bDeleteGuestMaps = false);
    void whiteList(const std::string& playerName);
    void blackList(const std::string& playerName);
    //    Set<String> getWhiteList();		/ 4J removed
    void reloadWhitelist();
    void sendLevelInfo(std::shared_ptr<ServerPlayer> player,
                       ServerLevel* level);
    void sendAllPlayerInfo(std::shared_ptr<ServerPlayer> player);
    int getPlayerCount();
    int getPlayerCount(ServerLevel* level);  // 4J Added
    int getMaxPlayers();
    MinecraftServer* getServer();
    int getViewDistance();
    void setOverrideGameMode(GameType* gameMode);

private:
    void updatePlayerGameMode(std::shared_ptr<ServerPlayer> newPlayer,
                              std::shared_ptr<ServerPlayer> oldPlayer,
                              Level* level);

public:
    void setAllowCheatsForAllPlayers(bool allowCommands);

    // 4J Added
    void kickPlayerByShortId(std::uint8_t networkSmallId);
    void closePlayerConnectionBySmallId(std::uint8_t networkSmallId);
    bool isXuidBanned(PlayerUID xuid);
    // AP added for Vita so the range can be increased once the level starts
    void setViewDistance(int newViewDistance);
};
