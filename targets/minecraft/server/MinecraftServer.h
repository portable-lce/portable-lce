#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ConsoleInputSource.h"
#include "ServerAction.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "platform/C4JThread.h"
#include "util/Timer.h"

class ServerConnection;
class Settings;
class PlayerList;
class EntityTracker;
class ConsoleInput;
class ConsoleCommands;
class LevelStorageSource;
class INetworkPlayer;
class LevelRuleset;
class LevelType;
class ProgressRenderer;
class CommandDispatcher;
class LevelGenerationOptions;
class ServerLevel;
class File;
class Level;
class Player;
class Pos;

#define MINECRAFT_SERVER_SLOW_QUEUE_DELAY 250

typedef struct _LoadSaveDataThreadParam {
    void* data;
    int64_t fileSize;
    const std::string saveName;
    _LoadSaveDataThreadParam(void* data, int64_t filesize,
                             const std::string& saveName)
        : data(data), fileSize(filesize), saveName(saveName) {}
} LoadSaveDataThreadParam;

typedef struct _NetworkGameInitData {
    int64_t seed;
    LoadSaveDataThreadParam* saveData;
    std::uint32_t settings;
    LevelGenerationOptions* levelGen;
    std::uint32_t texturePackId;
    bool findSeed;
    unsigned int xzSize;
    unsigned char hellScale;
    ESavePlatform savePlatform;

    _NetworkGameInitData() {
        seed = 0;
        saveData = nullptr;
        settings = 0;
        levelGen = nullptr;
        texturePackId = 0;
        findSeed = false;
        xzSize = LEVEL_LEGACY_WIDTH;
        hellScale = HELL_LEVEL_LEGACY_SCALE;
        savePlatform = SAVE_FILE_PLATFORM_LOCAL;
    }
} NetworkGameInitData;

// 4J Stu - 1.0.1 updates the server to implement the ServerInterface class, but
// I don't think we will use any of the functions that defines so not
// implementing here
class MinecraftServer : public ConsoleInputSource {
public:
    static const std::string VERSION;
    static const int TICK_STATS_SPAN = SharedConstants::TICKS_PER_SECOND * 5;

    //    static Logger logger = Logger.getLogger("Minecraft");
    static std::unordered_map<std::string, int> ironTimers;

private:
    static const int DEFAULT_MINECRAFT_PORT = 25565;
    static const int MS_PER_TICK = 1000 / SharedConstants::TICKS_PER_SECOND;

    // 4J Stu - Added 1.0.1, Not needed
    // std::string localIp;
    // int port;
public:
    ServerConnection* connection;
    Settings* settings;
    std::vector<ServerLevel*> levels;

private:
    PlayerList* players;

    // 4J Stu - Added 1.0.1, Not needed
    // long[] tickTimes = new long[TICK_STATS_SPAN];
    // long[][] levelTickTimes;
private:
    ConsoleCommands* commands;
    bool running;
    bool m_bLoaded;

public:
    bool stopped;
    int tickCount;

public:
    std::string progressStatus;
    int progress;

private:
    //	std::vector<Tickable *> tickables = new ArrayList<Tickable>();	// 4J -
    // removed
    CommandDispatcher* commandDispatcher;
    std::vector<ConsoleInput*>
        consoleInput;  // 4J - was synchronizedList - TODO - investigate
public:
    bool onlineMode;
    bool animals;
    bool npcs;
    bool pvp;
    bool allowFlight;
    std::string motd;
    int maxBuildHeight;
    int playerIdleTimeout;
    bool forceGameType;

private:
    // 4J Added
    // int m_lastSentDifficulty;

public:
    // 4J Stu - This value should be incremented every time the list of players
    // with friends-only UGC settings changes It is sent with PreLoginPacket and
    // compared when it comes back in the LoginPacket
    std::uint32_t m_ugcPlayersVersion;

    // This value is used to store the texture pack id for the currently loaded
    // world
    std::uint32_t m_texturePackId;

public:
    MinecraftServer();
    ~MinecraftServer();

private:
    // 4J Added - LoadSaveDataThreadParam
    bool initServer(int64_t seed, NetworkGameInitData* initData,
                    std::uint32_t initSettings, bool findSeed);
    void postProcessTerminate(ProgressRenderer* mcprogress);
    bool loadLevel(LevelStorageSource* storageSource, const std::string& name,
                   int64_t levelSeed, LevelType* pLevelType,
                   NetworkGameInitData* initData);
    void setProgress(const std::string& status, int progress);
    void endProgress();
    void saveAllChunks();
    void saveGameRules();
    void stopServer(bool didInit);
#if defined(_LARGE_WORLDS)
    void overwriteBordersForNewWorldSize(ServerLevel* level);
    void overwriteHellBordersForNewWorldSize(ServerLevel* level,
                                             int oldHellSize);

#endif
public:
    void setMaxBuildHeight(int maxBuildHeight);
    int getMaxBuildHeight();
    PlayerList* getPlayers();
    void setPlayers(PlayerList* players);
    ServerConnection* getConnection();
    bool isAnimals();
    void setAnimals(bool animals);
    bool isNpcsEnabled();
    void setNpcsEnabled(bool npcs);
    bool isPvpAllowed();
    void setPvpAllowed(bool pvp);
    bool isFlightAllowed();
    void setFlightAllowed(bool allowFlight);
    bool isCommandBlockEnabled();
    bool isNetherEnabled();
    bool isHardcore();
    int getOperatorUserPermissionLevel();
    CommandDispatcher* getCommandDispatcher();
    Pos* getCommandSenderWorldPosition();
    Level* getCommandSenderWorld();
    int getSpawnProtectionRadius();
    bool isUnderSpawnProtection(Level* level, int x, int y, int z,
                                std::shared_ptr<Player> player);
    void setForceGameType(bool forceGameType);
    bool getForceGameType();
    static int64_t getCurrentTimeMillis();
    int getPlayerIdleTimeout();
    void setPlayerIdleTimeout(int playerIdleTimeout);

public:
    void halt();
    void run(int64_t seed, void* lpParameter);

    void broadcastStartSavingPacket();
    void broadcastStopSavingPacket();

private:
    void tick();

public:
    void handleConsoleInput(const std::string& msg, ConsoleInputSource* source);
    void handleConsoleInputs();
    //    void addTickable(Tickable tickable);	// 4J removed
    static void main(int64_t seed, void* lpParameter);
    static void HaltServer(bool bPrimaryPlayerSignedOut = false);

    File* getFile(const std::string& name);
    void info(const std::string& string);
    void warn(const std::string& string);
    std::string getConsoleName();
    ServerLevel* getLevel(int dimension);
    void setLevel(int dimension, ServerLevel* level);         // 4J added
    static MinecraftServer* getInstance() { return server; }  // 4J added
    static bool serverHalted() { return s_bServerHalted; }
    static bool saveOnExitAnswered() { return s_bSaveOnExitAnswered; }
    static void resetFlags() {
        s_bServerHalted = false;
        s_bSaveOnExitAnswered = false;
    }

    bool flagEntitiesToBeRemoved(unsigned int* flags);  // 4J added
private:
    // 4J Added
    static MinecraftServer* server;

    static bool setTimeOfDayAtEndOfTick;
    static int64_t setTimeOfDay;
    static bool setTimeAtEndOfTick;
    static int64_t setTime;

    static bool
        m_bPrimaryPlayerSignedOut;  // 4J-PB added to tell the stopserver not to
                                    // save the game - another player may have
                                    // signed in in their place, so
                                    // PlatformProfile.IsSignedIn isn't enough
    static bool s_bServerHalted;  // 4J Stu Added so that we can halt the server
                                  // even before it's been created properly
    static bool s_bSaveOnExitAnswered;  // 4J Stu Added so that we only ask this
                                        // question once when we exit

    // 4J - added so that we can have a separate thread for post processing
    // chunks on level creation
    static int runPostUpdate(void* lpParam);
    C4JThread* m_postUpdateThread;
    bool m_postUpdateTerminate;
    class postProcessRequest {
    public:
        int x, z;
        ChunkSource* chunkSource;
        postProcessRequest(int x, int z, ChunkSource* chunkSource)
            : x(x), z(z), chunkSource(chunkSource) {}
    };
    std::vector<postProcessRequest> m_postProcessRequests;
    std::mutex m_postProcessCS;

public:
    void addPostProcessRequest(ChunkSource* chunkSource, int x, int z);

public:
    static PlayerList* getPlayerList() {
        if (server != nullptr)
            return server->players;
        else
            return nullptr;
    }
    static void SetTimeOfDay(int64_t time) {
        setTimeOfDayAtEndOfTick = true;
        setTimeOfDay = time;
    }
    static void SetTime(int64_t time) {
        setTimeAtEndOfTick = true;
        setTime = time;
    }

    C4JThread::Event* m_serverPausedEvent;

private:
    // 4J Added
    bool m_isServerPaused;

    // 4J Added - A static that stores the QNet index of the player that is next
    // allowed to send a packet in the slow queue
#if defined(_ACK_CHUNK_SEND_THROTTLING)
    static bool s_hasSentEnoughPackets;
    static int64_t s_tickStartTime;
    static std::vector<INetworkPlayer*> s_sentTo;
    static const int MAX_TICK_TIME_FOR_PACKET_SENDS = 35;
#else
    static int s_slowQueuePlayerIndex;
    static time_util::time_point s_slowQueueLastTime;
    static bool s_slowQueuePacketSent;
#endif

    bool IsServerPaused() { return m_isServerPaused; }

private:
    // Drain the action queue and dispatch each one. Called from the
    // server tick loop. The drain takes the mutex briefly to swap the
    // queue out, then dispatches without holding the lock.
    void drainServerActions();

    void handleServerAction(const minecraft::server::SaveGame& a);
    void handleServerAction(const minecraft::server::DropDebugItem& a);
    void handleServerAction(const minecraft::server::SpawnDebugMob& a);
    void handleServerAction(const minecraft::server::PauseServer& a);
    void handleServerAction(const minecraft::server::ToggleRain& a);
    void handleServerAction(const minecraft::server::ToggleThunder& a);
    void handleServerAction(
        const minecraft::server::BroadcastSettingChanged& a);
    void handleServerAction(const minecraft::server::ExportSchematic& a);
    void handleServerAction(const minecraft::server::SetCameraLocation& a);

    std::mutex m_actionQueueMutex;
    std::vector<minecraft::server::ServerAction> m_actionQueue;

    // 4J Added
    bool m_saveOnExit;
    bool m_suspending;

public:
    static bool chunkPacketManagement_CanSendTo(INetworkPlayer* player);
    static void chunkPacketManagement_DidSendTo(INetworkPlayer* player);
#if !defined(_ACK_CHUNK_SEND_THROTTLING)
    static void cycleSlowQueueIndex();
#endif

    void chunkPacketManagement_PreTick();
    void chunkPacketManagement_PostTick();

    // Queue a typed action for the server to handle on its next tick.
    // Safe to call from any thread; the queue is mutex-protected and
    // drained from the server tick loop.
    void queueServerAction(minecraft::server::ServerAction action);

    void setSaveOnExit(bool save) {
        m_saveOnExit = save;
        s_bSaveOnExitAnswered = true;
    }
    void Suspend();
    bool IsSuspending();

    // 4J Stu - A load of functions were all added in 1.0.1 in the
    // ServerInterface, but I don't think we need any of them
};