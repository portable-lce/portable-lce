#pragma once

#include <stdint.h>

#include <condition_variable>  // 4jcraft: im pretty sure there's a better alternative to this.
#include <deque>
#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ChunkStorage.h"
#include "OldChunkStorage.h"
#include "RegionFileCache.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "nbt/NbtIo.h"

class ConsoleSaveFile;
class C4JThread;
class DataOutputStream;
class Level;
class LevelChunk;

class McRegionChunkStorage : public ChunkStorage {
private:
    const std::string m_prefix;
    ConsoleSaveFile* m_saveFile;
    static std::mutex cs_memory;

    std::unordered_map<int64_t, std::vector<uint8_t>> m_entityData;

    static std::deque<DataOutputStream*> s_chunkDataQueue;
    static int s_runningThreadCount;
    static C4JThread* s_saveThreads[3];

public:
    McRegionChunkStorage(ConsoleSaveFile* saveFile, const std::string& prefix);
    ~McRegionChunkStorage();
    static void staticCtor();

    static std::condition_variable s_queueCondition;
    static std::condition_variable s_waitCondition;
    virtual LevelChunk* load(Level* level, int x, int z);
    virtual void save(Level* level, LevelChunk* levelChunk);
    virtual void saveEntities(Level* level, LevelChunk* levelChunk);
    virtual void loadEntities(Level* level, LevelChunk* levelChunk);
    virtual void tick();
    virtual void flush();
    virtual void WaitForAll();                 // 4J Added
    virtual void WaitIfTooManyQueuedChunks();  // 4J Added

private:
    static void WaitForAllSaves();
    static void WaitForSaves();
    static int runSaveThreadProc(void* lpParam);
};
