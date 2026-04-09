#pragma once

#include <stdint.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "java/JavaIntHash.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/Level.h"

class ClientConnection;
class MultiPlayerChunkCache;
class LevelSettings;
class Minecraft;
class Scoreboard;

class MultiPlayerLevel : public Level {
private:
    static const int TICKS_BEFORE_RESET = 20 * 4;

    class ResetInfo {
    public:
        int x, y, z, ticks, tile, data;
        ResetInfo(int x, int y, int z, int tile, int data);
    };

    std::vector<ResetInfo>
        updatesToReset;  // 4J - was linked list but std::vector seems more
                         // appropriate
    bool m_bEnableResetChanges;  // 4J Added
public:
    void unshareChunkAt(int x, int z);  // 4J - added
    void shareChunkAt(int x, int z);    // 4J - added

    void enableResetChanges(bool enable) {
        m_bEnableResetChanges = enable;
    }  // 4J Added
private:
    int unshareCheckX;   // 4J - added
    int unshareCheckZ;   // 4J - added
    int compressCheckX;  // 4J - added
    int compressCheckZ;  // 4J - added
    std::vector<ClientConnection*>
        connections;  // 4J Stu - Made this a std::vector as we can have more
                      // than one local connection
    MultiPlayerChunkCache* chunkCache;
    Minecraft* minecraft;
    Scoreboard* scoreboard;

public:
    MultiPlayerLevel(ClientConnection* connection, LevelSettings* levelSettings,
                     int dimension, int difficulty);
    virtual ~MultiPlayerLevel();
    virtual void tick();

    void clearResetRegion(int x0, int y0, int z0, int x1, int y1, int z1);

protected:
    ChunkSource*
    createChunkSource();  // 4J - was virtual, but was called from parent ctor
public:
    virtual void validateSpawn();

protected:
    virtual void tickTiles();

public:
    void setChunkVisible(int x, int z, bool visible);

private:
    std::unordered_map<int, std::shared_ptr<Entity>, IntKeyHash2, IntKeyEq>
        entitiesById;  // 4J - was IntHashMap
    std::unordered_set<std::shared_ptr<Entity> > forced;
    std::unordered_set<std::shared_ptr<Entity> > reEntries;

public:
    virtual bool addEntity(std::shared_ptr<Entity> e);
    virtual void removeEntity(std::shared_ptr<Entity> e);

protected:
    virtual void entityAdded(std::shared_ptr<Entity> e);
    virtual void entityRemoved(std::shared_ptr<Entity> e);

public:
    void putEntity(int id, std::shared_ptr<Entity> e);
    std::shared_ptr<Entity> getEntity(int id);
    std::shared_ptr<Entity> removeEntity(int id);
    virtual void removeEntities(
        std::vector<std::shared_ptr<Entity> >* list);  // 4J Added override
    virtual bool setData(int x, int y, int z, int data, int updateFlags,
                         bool forceUpdate = false);
    virtual bool setTileAndData(int x, int y, int z, int tile, int data,
                                int updateFlags);
    bool doSetTileAndData(int x, int y, int z, int tile, int data);
    virtual void disconnect(bool sendDisconnect = true);
    void animateTick(int xt, int yt, int zt);

protected:
    virtual Tickable* makeSoundUpdater(std::shared_ptr<Minecart> minecart);
    virtual void tickWeather();

    static const int ANIMATE_TICK_MAX_PARTICLES = 500;

public:
    void animateTickDoWork();                 // 4J added
    std::unordered_set<int> chunksToAnimate;  // 4J added

public:
    void removeAllPendingEntityRemovals();

    virtual void playSound(std::shared_ptr<Entity> entity, int iSound,
                           float volume, float pitch);

    virtual void playLocalSound(double x, double y, double z, int iSound,
                                float volume, float pitch,
                                bool distanceDelay = false,
                                float fClipSoundDist = 16.0f);

    virtual void createFireworks(double x, double y, double z, double xd,
                                 double yd, double zd, CompoundTag* infoTag);
    virtual void setScoreboard(Scoreboard* scoreboard);
    virtual void setDayTime(int64_t newTime);

    // 4J Stu - Added so we can have multiple local connections
    void addClientConnection(ClientConnection* c) { connections.push_back(c); }
    void removeClientConnection(ClientConnection* c, bool sendDisconnect);

    void tickAllConnections();

    void dataReceivedForChunk(int x, int z);  // 4J added
    void removeUnusedTileEntitiesInRegion(int x0, int y0, int z0, int x1,
                                          int y1, int z1);  // 4J added
};
