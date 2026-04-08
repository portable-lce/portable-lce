#include "MultiPlayerChunkCache.h"

#include <stdint.h>
#include <string.h>

#include "minecraft/network/INetworkService.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "util/StringHelpers.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/level/ServerChunkCache.h"
#include "minecraft/server/level/ServerLevel.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelType.h"
#include "minecraft/world/level/LightLayer.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/chunk/EmptyLevelChunk.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/chunk/WaterLevelChunk.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/tile/Tile.h"

MultiPlayerChunkCache::MultiPlayerChunkCache(Level* level) {
    XZSIZE = level->dimension->getXZSize();  // 4J Added
    XZOFFSET = XZSIZE / 2;                   // 4J Added
    m_XZSize = XZSIZE;
    hasData = new bool[XZSIZE * XZSIZE];
    memset(hasData, 0, sizeof(bool) * XZSIZE * XZSIZE);

    std::vector<uint8_t> emptyBlocks(16 * 16 * Level::maxBuildHeight);
    emptyChunk = new EmptyLevelChunk(level, emptyBlocks, 0, 0);

    // For normal world dimension, create a chunk that can be used to create the
    // illusion of infinite water at the edge of the world
    if (level->dimension->id == 0) {
        std::vector<uint8_t> bytes = std::vector<uint8_t>(16 * 16 * 128);

        // Superflat.... make grass, not water...
        if (level->getLevelData()->getGenerator() == LevelType::lvl_flat) {
            for (int x = 0; x < 16; x++)
                for (int y = 0; y < 128; y++)
                    for (int z = 0; z < 16; z++) {
                        unsigned char tileId = 0;
                        if (y == 3)
                            tileId = Tile::grass_Id;
                        else if (y <= 2)
                            tileId = Tile::dirt_Id;

                        bytes[x << 11 | z << 7 | y] = tileId;
                    }
        } else {
            for (int x = 0; x < 16; x++)
                for (int y = 0; y < 128; y++)
                    for (int z = 0; z < 16; z++) {
                        unsigned char tileId = 0;
                        if (y <= (level->getSeaLevel() - 10))
                            tileId = Tile::stone_Id;
                        else if (y < level->getSeaLevel())
                            tileId = Tile::calmWater_Id;

                        bytes[x << 11 | z << 7 | y] = tileId;
                    }
        }

        waterChunk = new WaterLevelChunk(level, bytes, 0, 0);

        if (level->getLevelData()->getGenerator() == LevelType::lvl_flat) {
            for (int x = 0; x < 16; x++)
                for (int y = 0; y < 128; y++)
                    for (int z = 0; z < 16; z++) {
                        if (y >= 3) {
                            ((WaterLevelChunk*)waterChunk)
                                ->setLevelChunkBrightness(LightLayer::Sky, x, y,
                                                          z, 15);
                        }
                    }
        } else {
            for (int x = 0; x < 16; x++)
                for (int y = 0; y < 128; y++)
                    for (int z = 0; z < 16; z++) {
                        if (y >= (level->getSeaLevel() - 1)) {
                            ((WaterLevelChunk*)waterChunk)
                                ->setLevelChunkBrightness(LightLayer::Sky, x, y,
                                                          z, 15);
                        } else {
                            ((WaterLevelChunk*)waterChunk)
                                ->setLevelChunkBrightness(LightLayer::Sky, x, y,
                                                          z, 2);
                        }
                    }
        }
    } else {
        waterChunk = nullptr;
    }

    this->level = level;

    this->cache = new LevelChunk*[XZSIZE * XZSIZE];
    memset(this->cache, 0, XZSIZE * XZSIZE * sizeof(LevelChunk*));
}

MultiPlayerChunkCache::~MultiPlayerChunkCache() {
    delete emptyChunk;
    delete waterChunk;
    delete cache;
    delete hasData;

    auto itEnd = loadedChunkList.end();
    for (auto it = loadedChunkList.begin(); it != itEnd; it++) delete *it;
}

bool MultiPlayerChunkCache::hasChunk(int x, int z) {
    // This cache always claims to have chunks, although it might actually just
    // return empty data if it doesn't have anything
    return true;
}

// 4J  added - find out if we actually really do have a chunk in our cache
bool MultiPlayerChunkCache::reallyHasChunk(int x, int z) {
    int ix = x + XZOFFSET;
    int iz = z + XZOFFSET;
    // Check we're in range of the stored level - if we aren't, then consider
    // that we do have that chunk as we'll be able to use the water chunk there
    if ((ix < 0) || (ix >= XZSIZE)) return true;
    if ((iz < 0) || (iz >= XZSIZE)) return true;
    int idx = ix * XZSIZE + iz;

    LevelChunk* chunk = cache[idx];
    if (chunk == nullptr) {
        return false;
    }
    return hasData[idx];
}

void MultiPlayerChunkCache::drop(int x, int z) {
    // 4J Stu - We do want to drop any entities in the chunks, especially for
    // the case when a player is dead as they will not get the RemoveEntity
    // packet if an entity is removed.
    LevelChunk* chunk = getChunk(x, z);
    if (!chunk->isEmpty()) {
        // Added parameter here specifies that we don't want to delete tile
        // entities, as they won't get recreated unless they've got update
        // packets The tile entities are in general only created on the client
        // by virtue of the chunk rebuild
        chunk->unload(false);

        // 4J - We just want to clear out the entities in the chunk, but
        // everything else should be valid
        chunk->loaded = true;
    }
}

LevelChunk* MultiPlayerChunkCache::create(int x, int z) {
    int ix = x + XZOFFSET;
    int iz = z + XZOFFSET;
    // Check we're in range of the stored level
    if ((ix < 0) || (ix >= XZSIZE))
        return (waterChunk ? waterChunk : emptyChunk);
    if ((iz < 0) || (iz >= XZSIZE))
        return (waterChunk ? waterChunk : emptyChunk);
    int idx = ix * XZSIZE + iz;
    LevelChunk* chunk = cache[idx];
    LevelChunk* lastChunk = chunk;

    if (chunk == nullptr) {
        {
            std::unique_lock<std::mutex> lock(m_csLoadCreate);

            // LevelChunk *chunk;
            if (NetworkService
                    .IsHost())  // force here to disable sharing of data
            {
                // 4J-JEV: We are about to use shared data, abort if the server
                // is stopped and the data is deleted.
                if (MinecraftServer::getInstance()->serverHalted())
                    return nullptr;

                // If we're the host, then don't create the chunk, share data
                // from the server's copy
#ifdef _LARGE_WORLDS
                LevelChunk* serverChunk =
                    MinecraftServer::getInstance()
                        ->getLevel(level->dimension->id)
                        ->cache->getChunkLoadedOrUnloaded(x, z);
#else
                LevelChunk* serverChunk = MinecraftServer::getInstance()
                                              ->getLevel(level->dimension->id)
                                              ->cache->getChunk(x, z);
#endif
                chunk = new LevelChunk(level, x, z, serverChunk);
                // Let renderer know that this chunk has been created - it might
                // have made render data from the EmptyChunk if it got to a
                // chunk before the server sent it
                level->setTilesDirty(x * 16, 0, z * 16, x * 16 + 15, 127,
                                     z * 16 + 15);
                hasData[idx] = true;
            } else {
                // Passing an empty array into the LevelChunk ctor, which it now
                // detects and sets up the chunk as compressed & empty
                std::vector<uint8_t> bytes;

                chunk = new LevelChunk(level, bytes, x, z);

                // 4J - changed to use new methods for lighting
                chunk->setSkyLightDataAllBright();
                //			Arrays::fill(chunk->skyLight->data,
                //(byte) 255);
            }

            chunk->loaded = true;
        }

#if (defined _WIN64 || defined __LP64__)
        if (InterlockedCompareExchangeRelease64(
                (int64_t*)&cache[idx], (int64_t)chunk, (int64_t)lastChunk) ==
            (int64_t)lastChunk)
#else
        if (InterlockedCompareExchangeRelease(
                (int32_t*)&cache[idx], (int32_t)chunk, (int32_t)lastChunk) ==
            (int32_t)lastChunk)
#endif  // 0
        {
            // If we're sharing with the server, we'll need to calculate our
            // heightmap now, which isn't shared. If we aren't sharing with the
            // server, then this will be calculated when the chunk data arrives.
            if (NetworkService.IsHost()) {
                chunk->recalcHeightmapOnly();
            }

            // Successfully updated the cache
            {
                std::lock_guard<std::mutex> lock(m_csLoadCreate);
                loadedChunkList.push_back(chunk);
            }
        } else {
            // Something else must have updated the cache. Return that chunk and
            // discard this one. This really shouldn't be happening in
            // multiplayer
            delete chunk;
            return cache[idx];
        }

    } else {
        chunk->load();
    }

    return chunk;
}

LevelChunk* MultiPlayerChunkCache::getChunk(int x, int z) {
    int ix = x + XZOFFSET;
    int iz = z + XZOFFSET;
    // Check we're in range of the stored level
    if ((ix < 0) || (ix >= XZSIZE))
        return (waterChunk ? waterChunk : emptyChunk);
    if ((iz < 0) || (iz >= XZSIZE))
        return (waterChunk ? waterChunk : emptyChunk);
    int idx = ix * XZSIZE + iz;

    LevelChunk* chunk = cache[idx];
    if (chunk == nullptr) {
        return emptyChunk;
    } else {
        return chunk;
    }
}

bool MultiPlayerChunkCache::save(bool force,
                                 ProgressListener* progressListener) {
    return true;
}

bool MultiPlayerChunkCache::tick() { return false; }

bool MultiPlayerChunkCache::shouldSave() { return false; }

void MultiPlayerChunkCache::postProcess(ChunkSource* parent, int x, int z) {}

std::vector<Biome::MobSpawnerData*>* MultiPlayerChunkCache::getMobsAt(
    MobCategory* mobCategory, int x, int y, int z) {
    return nullptr;
}

TilePos* MultiPlayerChunkCache::findNearestMapFeature(
    Level* level, const std::string& featureName, int x, int y, int z) {
    return nullptr;
}

void MultiPlayerChunkCache::recreateLogicStructuresForChunk(int chunkX,
                                                            int chunkZ) {}

std::string MultiPlayerChunkCache::gatherStats() {
    int size;
    {
        std::lock_guard<std::mutex> lock(m_csLoadCreate);
        size = (int)loadedChunkList.size();
    }
    return "MultiplayerChunkCache: " + toWString<int>(size);
}

void MultiPlayerChunkCache::dataReceived(int x, int z) {
    int ix = x + XZOFFSET;
    int iz = z + XZOFFSET;
    // Check we're in range of the stored level
    if ((ix < 0) || (ix >= XZSIZE)) return;
    if ((iz < 0) || (iz >= XZSIZE)) return;
    int idx = ix * XZSIZE + iz;
    hasData[idx] = true;
}