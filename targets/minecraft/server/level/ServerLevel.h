#pragma once

#include <stdint.h>

#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SharedConstants.h"
#include "java/JavaIntHash.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/TickNextTickData.h"
#include "minecraft/world/level/TileEventData.h"
#include "minecraft/world/level/biome/Biome.h"
#include "platform/thread/C4JThread.h"

class ServerChunkCache;
class MinecraftServer;
class Node;
class EntityTracker;
class PlayerChunkMap;
class WeighedTreasure;
class LevelSettings;
class LevelStorage;
class MobCategory;
class MobSpawner;
class PortalForcer;
class Pos;
class ProgressListener;
class TileEntity;

class ServerLevel : public Level {
private:
    static const int EMPTY_TIME_NO_TICK =
        SharedConstants::TICKS_PER_SECOND * 60;

    MinecraftServer* server;
    EntityTracker* tracker;
    PlayerChunkMap* chunkMap;

    std::recursive_mutex m_tickNextTickCS;  // 4J added
    std::set<TickNextTickData, TickNextTickDataKeyCompare>
        tickNextTickList;  // 4J Was TreeSet
    std::unordered_set<TickNextTickData, TickNextTickDataKeyHash,
                       TickNextTickDataKeyEq>
        tickNextTickSet;  // 4J Was HashSet

    std::vector<Pos*> m_queuedSendTileUpdates;  // 4J added
    std::recursive_mutex m_csQueueSendTileUpdates;

protected:
    int saveInterval;

public:
    ServerChunkCache* cache;
    bool canEditSpawn;
    bool noSave;

private:
    bool allPlayersSleeping;
    PortalForcer* portalForcer;
    MobSpawner* mobSpawner;
    int emptyTime;
    bool m_bAtLeastOnePlayerSleeping;  // 4J Added
    static std::vector<WeighedTreasure*>
        RANDOM_BONUS_ITEMS;  // 4J - brought forward from 1.3.2

    std::vector<TileEventData> tileEvents[2];
    int activeTileEventsList;

public:
    static void staticCtor();
    ServerLevel(MinecraftServer* server,
                std::shared_ptr<LevelStorage> levelStorage,
                const std::string& levelName, int dimension,
                LevelSettings* levelSettings);
    ~ServerLevel();
    void tick();
    Biome::MobSpawnerData* getRandomMobSpawnAt(MobCategory* mobCategory, int x,
                                               int y, int z);
    void updateSleepingPlayerList();

protected:
    void awakenAllPlayers();

private:
    void stopWeather();

public:
    bool allPlayersAreSleeping();
    void validateSpawn();

protected:
    void tickTiles();

private:
    std::vector<TickNextTickData> toBeTicked;

public:
    bool isTileToBeTickedAt(int x, int y, int z, int tileId);
    void addToTickNextTick(int x, int y, int z, int tileId, int tickDelay);
    void addToTickNextTick(int x, int y, int z, int tileId, int tickDelay,
                           int priorityTilt);
    void forceAddTileTick(int x, int y, int z, int tileId, int tickDelay,
                          int prioTilt);
    void tickEntities();
    void resetEmptyTime();
    bool tickPendingTicks(bool force);
    std::vector<TickNextTickData>* fetchTicksInChunk(LevelChunk* chunk,
                                                     bool remove);
    virtual void tick(std::shared_ptr<Entity> e, bool actual);
    void forceTick(std::shared_ptr<Entity> e, bool actual);
    bool AllPlayersAreSleeping() {
        return allPlayersSleeping;
    }  // 4J added for a message to other players
    bool isAtLeastOnePlayerSleeping() { return m_bAtLeastOnePlayerSleeping; }

protected:
    ChunkSource*
    createChunkSource();  // 4J - was virtual, but was called from parent ctor
public:
    std::vector<std::shared_ptr<TileEntity> >* getTileEntitiesInRegion(
        int x0, int y0, int z0, int x1, int y1, int z1);
    virtual bool mayInteract(std::shared_ptr<Player> player, int xt, int yt,
                             int zt, int content);

protected:
    virtual void initializeLevel(LevelSettings* settings);
    virtual void setInitialSpawn(LevelSettings* settings);
    void generateBonusItemsNearSpawn();  // 4J - brought forward from 1.3.2

public:
    Pos* getDimensionSpecificSpawn();

    void Suspend();  // 4j Added for XboxOne PLM

    void save(bool force, ProgressListener* progressListener,
              bool bAutosave = false);
    void saveToDisc(ProgressListener* progressListener,
                    bool autosave);  // 4J Added

private:
    void saveLevelData();

    typedef std::unordered_map<int, std::shared_ptr<Entity>, IntKeyHash2,
                               IntKeyEq>
        intEntityMap;
    intEntityMap entitiesById;  // 4J - was IntHashMap, using same hashing
                                // function as this uses
protected:
    virtual void entityAdded(std::shared_ptr<Entity> e);
    virtual void entityRemoved(std::shared_ptr<Entity> e);

public:
    std::shared_ptr<Entity> getEntity(int id);
    virtual bool addGlobalEntity(std::shared_ptr<Entity> e);
    void broadcastEntityEvent(std::shared_ptr<Entity> e, uint8_t event);
    virtual std::shared_ptr<Explosion> explode(std::shared_ptr<Entity> source,
                                               double x, double y, double z,
                                               float r, bool fire,
                                               bool destroyBlocks);
    virtual void tileEvent(int x, int y, int z, int tile, int b0, int b1);

private:
    void runTileEvents();
    bool doTileEvent(TileEventData* te);

public:
    void closeLevelStorage();

protected:
    virtual void tickWeather();

public:
    MinecraftServer* getServer();
    EntityTracker* getTracker();
    void setTimeAndAdjustTileTicks(int64_t newTime);
    PlayerChunkMap* getChunkMap();
    PortalForcer* getPortalForcer();
    void sendParticles(const std::string& name, double x, double y, double z,
                       int count);
    void sendParticles(const std::string& name, double x, double y, double z,
                       int count, double xDist, double yDist, double zDist,
                       double speed);

    void queueSendTileUpdate(int x, int y, int z);  // 4J Added
private:
    void runQueuedSendTileUpdates();  // 4J Added

    // 4J - added for implementation of finite limit to number of item entities,
    // tnt and falling block entities
public:
    static const int MAX_HANGING_ENTITIES = 400;
    static const int MAX_ITEM_ENTITIES = 200;
    static const int MAX_ARROW_ENTITIES = 200;
    static const int MAX_EXPERIENCEORB_ENTITIES = 50;
    static const int MAX_PRIMED_TNT = 20;
    static const int MAX_FALLING_TILE = 20;

    int m_primedTntCount;
    int m_fallingTileCount;
    std::recursive_mutex m_limiterCS;
    std::list<std::shared_ptr<Entity> > m_itemEntities;
    std::list<std::shared_ptr<Entity> > m_hangingEntities;
    std::list<std::shared_ptr<Entity> > m_arrowEntities;
    std::list<std::shared_ptr<Entity> > m_experienceOrbEntities;

    virtual bool addEntity(std::shared_ptr<Entity> e);
    void entityAddedExtra(std::shared_ptr<Entity> e);
    void entityRemovedExtra(std::shared_ptr<Entity> e);

    bool atEntityLimit(std::shared_ptr<Entity> e);  // 4J: Added

    virtual bool newPrimedTntAllowed();
    virtual bool newFallingTileAllowed();

    void flagEntitiesToBeRemoved(unsigned int* flags,
                                 bool* removedFound);  // 4J added

    // 4J added
    static const int MAX_UPDATES = 256;

    // Each of these need to be duplicated for each level in the current game.
    // As we currently only have 2 (over/nether), making this constant
    static Level* m_level[3];
    static int m_updateChunkX[3][LEVEL_CHUNKS_TO_UPDATE_MAX];
    static int m_updateChunkZ[3][LEVEL_CHUNKS_TO_UPDATE_MAX];
    static int m_updateChunkCount[3];
    static int m_updateTileX[3][MAX_UPDATES];
    static int m_updateTileY[3][MAX_UPDATES];
    static int m_updateTileZ[3][MAX_UPDATES];
    static int m_updateTileCount[3];
    static int m_randValue[3];

    static C4JThread::EventArray* m_updateTrigger;
    static std::recursive_mutex m_updateCS[3];

    static C4JThread* m_updateThread;
    static int runUpdate(void* lpParam);
};
