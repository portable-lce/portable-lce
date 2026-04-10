#pragma once
#include <deque>
#include <format>
#include <mutex>
#include <string>
#include <vector>

#include "java/File.h"
#include "java/JavaIntHash.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/levelgen/RandomLevelSource.h"
#include "platform/thread/C4JThread.h"

class ServerLevel;
class ChunkStorage;
class LevelChunk;

class ServerChunkCache : public ChunkSource {
private:
    //	unordered_set<int,IntKeyHash, IntKeyEq> toDrop;
private:
    LevelChunk* emptyChunk;
    ChunkSource* source;
    ChunkStorage* storage;

public:
    bool autoCreate;

private:
    LevelChunk** cache;
    std::vector<LevelChunk*> m_loadedChunkList;
    ServerLevel* level;

#ifdef _LARGE_WORLDS
    std::deque<LevelChunk*> m_toDrop;
    LevelChunk** m_unloadedCache;
#endif

    // 4J - added for multithreaded support
    std::recursive_mutex m_csLoadCreate;
    // 4J - size of cache is defined by size of one side - must be even
    int XZSIZE;
    int XZOFFSET;

public:
    ServerChunkCache(ServerLevel* level, ChunkStorage* storage,
                     ChunkSource* source);
    virtual ~ServerChunkCache();
    virtual bool hasChunk(int x, int z);
    std::vector<LevelChunk*>* getLoadedChunkList();
    void drop(int x, int z);
    void dropAll();
    virtual LevelChunk* create(int x, int z);
    LevelChunk* create(int x, int z, bool asyncPostProcess);  // 4J added
    virtual LevelChunk* getChunk(int x, int z);
#ifdef _LARGE_WORLDS
    LevelChunk* getChunkLoadedOrUnloaded(int x, int z);  // 4J added
    void overwriteLevelChunkFromSource(
        int x, int z);  // 4J MGH added, for expanding worlds, to kill any
                        // player changes and reset the chunk
    void overwriteHellLevelChunkFromSource(
        int x, int z, int minVal,
        int maxVal);  // 4J MGH added, for expanding worlds, to reset the outer
                      // tiles in the chunk
    void updateOverwriteHellChunk(LevelChunk* origChunk,
                                  LevelChunk* playerChunk, int xMin, int xMax,
                                  int zMin, int zMax);

#endif
    virtual LevelChunk** getCache() { return cache; }  // 4J added

    // 4J-JEV Added; Remove chunk from the toDrop queue.
#ifdef _LARGE_WORLDS
    void dontDrop(int x, int z);
#endif

private:
    LevelChunk* load(int x, int z);
    void saveEntities(LevelChunk* levelChunk);
    void save(LevelChunk* levelChunk);

    void updatePostProcessFlag(short flag, int x, int z, int xo, int zo,
                               LevelChunk* lc);              // 4J added
    void updatePostProcessFlags(int x, int z);               // 4J added
    void flagPostProcessComplete(short flag, int x, int z);  // 4J added
public:
    virtual void postProcess(ChunkSource* parent, int x, int z);

private:
#ifdef _LARGE_WORLDS
    static const int MAX_SAVES = 20;
#else
    // 4J Stu - Was 24, but lowering it drastically so that we can trickle save
    // chunks
    static const int MAX_SAVES = 1;
#endif

public:
    virtual bool saveAllEntities();
    virtual bool save(bool force, ProgressListener* progressListener);
    virtual bool tick();
    virtual bool shouldSave();
    virtual std::string gatherStats();

    virtual std::vector<Biome::MobSpawnerData*>* getMobsAt(
        MobCategory* mobCategory, int x, int y, int z);
    virtual TilePos* findNearestMapFeature(Level* level,
                                           const std::string& featureName,
                                           int x, int y, int z);
    virtual void recreateLogicStructuresForChunk(int chunkX, int chunkZ);

private:
    typedef struct _SaveThreadData {
        ServerChunkCache* cache;
        LevelChunk* chunkToSave;
        bool saveEntities;
        bool useSharedThreadStorage;
        C4JThread::Event* notificationEvent;
        C4JThread::Event* wakeEvent;  // This is a handle to the one fired by
                                      // the producer thread
    } SaveThreadData;

public:
    static int runSaveThreadProc(void* lpParam);
};
