#include "McRegionChunkStorage.h"

#include <stdio.h>
#include <string.h>

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

#include "java/InputOutputStream/BufferedOutputStream.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/chunk/storage/OldChunkStorage.h"
#include "minecraft/world/level/chunk/storage/RegionFileCache.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileInputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOutputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"
#include "platform/input/input.h"
#include "platform/thread/C4JThread.h"

class DataInput;

std::mutex McRegionChunkStorage::cs_memory;
std::condition_variable McRegionChunkStorage::s_queueCondition;
std::condition_variable McRegionChunkStorage::s_waitCondition;

std::deque<DataOutputStream*> McRegionChunkStorage::s_chunkDataQueue;
int McRegionChunkStorage::s_runningThreadCount = 0;
C4JThread* McRegionChunkStorage::s_saveThreads[3];

McRegionChunkStorage::McRegionChunkStorage(ConsoleSaveFile* saveFile,
                                           const std::string& prefix)
    : m_prefix(prefix) {
    m_saveFile = saveFile;

    // Make sure that if there are any files for regions to be created, that
    // they are created in the order that suits us for making the initial level
    // save work fast
    if (prefix == "") {
        m_saveFile->createFile(ConsoleSavePath("DIM-1r.-1.-1.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM-1r.0.-1.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM-1r.0.0.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM-1r.-1.0.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM1/r.-1.-1.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM1/r.0.-1.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM1/r.0.0.mcr"));
        m_saveFile->createFile(ConsoleSavePath("DIM1/r.-1.0.mcr"));
        m_saveFile->createFile(ConsoleSavePath("r.-1.-1.mcr"));
        m_saveFile->createFile(ConsoleSavePath("r.0.-1.mcr"));
        m_saveFile->createFile(ConsoleSavePath("r.0.0.mcr"));
        m_saveFile->createFile(ConsoleSavePath("r.-1.0.mcr"));
    }

#if defined(SPLIT_SAVES)
    ConsoleSavePath currentFile =
        ConsoleSavePath(m_prefix + std::string("entities.dat"));

    if (m_saveFile->doesFileExist(currentFile)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(m_saveFile, currentFile);
        DataInputStream dis(&fis);

        int count = dis.readInt();

        for (int i = 0; i < count; ++i) {
            int64_t index = dis.readLong();
            CompoundTag* tag = NbtIo::read(&dis);

            ByteArrayOutputStream bos;
            DataOutputStream dos(&bos);
            NbtIo::write(tag, &dos);
            delete tag;

            std::vector<uint8_t> savedData(bos.size());
            memcpy(savedData.data(), bos.buf.data(), bos.size());

            m_entityData[index] = savedData;
        }
    }
#endif
}

McRegionChunkStorage::~McRegionChunkStorage() {
    // vectors manage their own memory; clearing the map is sufficient
}

LevelChunk* McRegionChunkStorage::load(Level* level, int x, int z) {
    DataInputStream* regionChunkInputStream =
        RegionFileCache::getChunkDataInputStream(m_saveFile, m_prefix, x, z);

#if defined(SPLIT_SAVES)
    // If we can't find the chunk in the save file, then we should remove any
    // entities we might have for that chunk
    if (regionChunkInputStream == nullptr) {
        // 4jcraft fixed cast from int to int64 and taking the mask of the upper
        // bits and cast to unsigned
        uint64_t index =
            ((uint64_t)(uint32_t)(x) << 32) | (((uint64_t)(uint32_t)(z)));

        auto it = m_entityData.find(index);
        if (it != m_entityData.end()) {
            m_entityData.erase(it);
        }
    }
#endif

    LevelChunk* levelChunk = nullptr;

    if (m_saveFile->getOriginalSaveVersion() >=
        SAVE_FILE_VERSION_COMPRESSED_CHUNK_STORAGE) {
        if (regionChunkInputStream != nullptr) {
            levelChunk = OldChunkStorage::load(level, regionChunkInputStream);
            loadEntities(level, levelChunk);
            regionChunkInputStream->deleteChildStream();
            delete regionChunkInputStream;
        }
    } else {
        CompoundTag* chunkData;
        if (regionChunkInputStream != nullptr) {
            chunkData = NbtIo::read((DataInput*)regionChunkInputStream);
        } else {
            return nullptr;
        }

        regionChunkInputStream->deleteChildStream();
        delete regionChunkInputStream;

        if (!chunkData->contains("Level")) {
            char buf[256];
            sprintf(buf,
                    "Chunk file at %d, %d is missing level data, skipping\n", x,
                    z);
            Log::info("%s", buf);
            delete chunkData;
            return nullptr;
        }
        if (!chunkData->getCompound("Level")->contains("Blocks")) {
            char buf[256];
            sprintf(buf,
                    "Chunk file at %d, %d is missing block data, skipping\n", x,
                    z);
            Log::info("%s", buf);
            delete chunkData;
            return nullptr;
        }
        levelChunk =
            OldChunkStorage::load(level, chunkData->getCompound("Level"));
        if (!levelChunk->isAt(x, z)) {
            char buf[256];
            sprintf(buf,
                    "Chunk file at %d, %d is in the wrong location; "
                    "relocating. Expected %d, %d, got %d, %d\n",
                    x, z, x, z, levelChunk->x, levelChunk->z);
            Log::info("%s", buf);
            delete levelChunk;
            delete chunkData;
            return nullptr;

            // 4J Stu - We delete the data within OldChunkStorage::load, so we
            // can never reload from it
            // chunkData->putInt("xPos", x);
            // chunkData->putInt("zPos", z);
            // levelChunk = OldChunkStorage::load(level,
        }
#if defined(SPLIT_SAVES)
        loadEntities(level, levelChunk);
#endif
        delete chunkData;
    }
#if !defined(_CONTENT_PACKAGE)
    if (levelChunk && gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_EnableBiomeOverride)) {
        // 4J Stu - This will force an update of the chunk's biome array
        levelChunk->reloadBiomes();
    }
#endif
    return levelChunk;
}

void McRegionChunkStorage::save(Level* level, LevelChunk* levelChunk) {
    level->checkSession();

    // 4J - removed try/catch
    //    try {

    // Note - have added use of a mutex round sections of code that
    // do a lot of memory alloc/free operations. This is because when we are
    // running saves on multiple threads these sections have a lot of
    // contention. Better to let each thread have its turn at a higher level of
    // granularity.
    DataOutputStream* output = RegionFileCache::getChunkDataOutputStream(
        m_saveFile, m_prefix, levelChunk->x, levelChunk->z);

    if (m_saveFile->getOriginalSaveVersion() >=
        SAVE_FILE_VERSION_COMPRESSED_CHUNK_STORAGE) {
        OldChunkStorage::save(levelChunk, level, output);

        {
            std::lock_guard<std::mutex> lock(cs_memory);
            s_chunkDataQueue.push_back(output);
        }
        // 4jcraft: WAKE UP, WAKE THE FUCK.. UP
        s_queueCondition.notify_one();

    } else {
        CompoundTag* tag;
        {
            std::lock_guard<std::mutex> lock(cs_memory);
            tag = new CompoundTag();
            CompoundTag* levelData = new CompoundTag();
            tag->put("Level", levelData);
            OldChunkStorage::save(levelChunk, level, levelData);

            NbtIo::write(tag, output);
        }
        output->close();

        // 4J Stu - getChunkDataOutputStream makes a new DataOutputStream that
        // points to a new ChunkBuffer( ByteArrayOutputStream ) We should clean
        // these up when we are done
        {
            std::lock_guard<std::mutex> lock(cs_memory);
            output->deleteChildStream();
            delete output;
            delete tag;
        }
    }

    LevelData* levelInfo = level->getLevelData();

    // 4J Stu - Override this with our save file size to stop all the
    // RegionFileCache lookups
    // levelInfo->setSizeOnDisk(levelInfo->getSizeOnDisk() +
    // RegionFileCache::getSizeDelta(m_saveFile, m_prefix, levelChunk->x,
    // levelChunk->z));
    levelInfo->setSizeOnDisk(this->m_saveFile->getSizeOnDisk());
    //    } catch (Exception e) {
    //        e.printStackTrace();
    //    }
}

void McRegionChunkStorage::saveEntities(Level* level, LevelChunk* levelChunk) {
#if defined(SPLIT_SAVES)
    // 4j added cast to unsigned and changed index to u
    uint64_t index = ((uint64_t)(uint32_t)(levelChunk->x) << 32) |
                     (((uint64_t)(uint32_t)(levelChunk->z)));

    CompoundTag* newTag = new CompoundTag();
    bool savedEntities =
        OldChunkStorage::saveEntities(levelChunk, level, newTag);

    if (savedEntities) {
        ByteArrayOutputStream bos;
        DataOutputStream dos(&bos);
        NbtIo::write(newTag, &dos);

        std::vector<uint8_t> savedData(bos.size());
        memcpy(savedData.data(), bos.buf.data(), bos.size());

        m_entityData[index] = savedData;
    } else {
        auto it = m_entityData.find(index);
        if (it != m_entityData.end()) {
            m_entityData.erase(it);
        }
    }
    delete newTag;

#endif
}

void McRegionChunkStorage::loadEntities(Level* level, LevelChunk* levelChunk) {
#if defined(SPLIT_SAVES)
    int64_t index = ((int64_t)(levelChunk->x) << 32) |
                    (((int64_t)(levelChunk->z)) & 0x00000000FFFFFFFF);

    auto it = m_entityData.find(index);
    if (it != m_entityData.end()) {
        ByteArrayInputStream bais(it->second);
        DataInputStream dis(&bais);
        CompoundTag* tag = NbtIo::read(&dis);
        OldChunkStorage::loadEntities(levelChunk, level, tag);
        bais.reset();
        delete tag;
    }
#endif
}

void McRegionChunkStorage::tick() { m_saveFile->tick(); }

void McRegionChunkStorage::flush() {
#if defined(SPLIT_SAVES)
    ConsoleSavePath currentFile =
        ConsoleSavePath(m_prefix + std::string("entities.dat"));
    ConsoleSaveFileOutputStream fos =
        ConsoleSaveFileOutputStream(m_saveFile, currentFile);
    BufferedOutputStream bos(&fos, 1024 * 1024);
    DataOutputStream dos(&bos);

    dos.writeInt(m_entityData.size());

    for (auto it = m_entityData.begin(); it != m_entityData.end(); ++it) {
        dos.writeLong(it->first);
        dos.write(it->second, 0, it->second.size());
    }
    bos.flush();

#endif
}

void McRegionChunkStorage::staticCtor() {
    for (unsigned int i = 0; i < 3; ++i) {
        char threadName[256];
        sprintf(threadName, "McRegion Save thread %d\n", i);
        C4JThread::setThreadName(0, threadName);

        // saveThreads[j] =
        // CreateThread(nullptr,0,runSaveThreadProc,&threadData[j],CREATE_SUSPENDED,&threadId[j]);
        s_saveThreads[i] =
            new C4JThread(runSaveThreadProc, nullptr, threadName);

        // Log::info("Created new thread: %s\n",threadName);

        // ResumeThread( saveThreads[j] );
        s_saveThreads[i]->run();
    }
}

// 4jcraft: removed the wasting 100ms chunk loading part.
int McRegionChunkStorage::runSaveThreadProc(void* lpParam) {
    Compression::CreateNewThreadStorage();

    bool running = true;
    DataOutputStream* dos = nullptr;
    while (running) {
        {
            std::unique_lock<std::mutex> lock(cs_memory);
            s_queueCondition.wait(lock,
                                  [] { return !s_chunkDataQueue.empty(); });
            dos = s_chunkDataQueue.front();
            s_chunkDataQueue.pop_front();
            s_runningThreadCount++;
        }  // Unlock so the main thread can keep working

        if (dos) {
            dos->close();
            dos->deleteChildStream();
            delete dos;
            dos = nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(cs_memory);
            s_runningThreadCount--;
        }

        // Tell the main thread we finished a chunk
        s_waitCondition.notify_all();
    }

    Compression::ReleaseThreadStorage();
    return 0;
}

void McRegionChunkStorage::WaitForAll() { WaitForAllSaves(); }

void McRegionChunkStorage::WaitIfTooManyQueuedChunks() { WaitForSaves(); }

// Static
// 4jcraft: Better waiting system
void McRegionChunkStorage::WaitForAllSaves() {
    std::unique_lock<std::mutex> lock(cs_memory);
    // Pause the main thread instantly until queue is 0 AND workers are done
    s_waitCondition.wait(lock, [] {
        return s_chunkDataQueue.empty() && s_runningThreadCount == 0;
    });
}

// Static
void McRegionChunkStorage::WaitForSaves() {
    static const int MAX_QUEUE_SIZE = 12;
    static const int DESIRED_QUEUE_SIZE = 6;

    std::unique_lock<std::mutex> lock(cs_memory);
    if (s_chunkDataQueue.size() > MAX_QUEUE_SIZE) {
        // Pause until the queue drains down to the desired size
        s_waitCondition.wait(
            lock, [] { return s_chunkDataQueue.size() <= DESIRED_QUEUE_SIZE; });
    }
}
