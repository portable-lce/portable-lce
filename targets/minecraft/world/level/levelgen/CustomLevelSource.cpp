#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "CustomLevelSource.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "app/common/GameRules/LevelGeneration/LevelGenerationOptions.h"
#include "platform/fs/fs.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#if defined(__linux__)
#include "app/linux/Stubs/winapi_stubs.h"
#endif
#include "java/Random.h"
#include "minecraft/world/entity/MobCategory.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/MobSpawner.h"
#include "minecraft/world/level/biome/BiomeSource.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/levelgen/CanyonFeature.h"
#include "minecraft/world/level/levelgen/LargeCaveFeature.h"
#include "minecraft/world/level/levelgen/LargeFeature.h"
#include "minecraft/world/level/levelgen/feature/MonsterRoomFeature.h"
#include "minecraft/world/level/levelgen/structure/MineShaftFeature.h"
#include "minecraft/world/level/levelgen/structure/RandomScatteredLargeFeature.h"
#include "minecraft/world/level/levelgen/structure/StrongholdFeature.h"
#include "minecraft/world/level/levelgen/structure/VillageFeature.h"
#include "minecraft/world/level/levelgen/synth/PerlinNoise.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/tile/HeavyTile.h"
#include "minecraft/world/level/tile/Tile.h"

const double CustomLevelSource::SNOW_SCALE = 0.3;
const double CustomLevelSource::SNOW_CUTOFF = 0.5;

CustomLevelSource::CustomLevelSource(Level* level, int64_t seed,
                                     bool generateStructures)
    : generateStructures(generateStructures) {
#if defined(_OVERRIDE_HEIGHTMAP)
    m_XZSize = level->getLevelData()->getXZSize();

    m_heightmapOverride =
        std::vector<uint8_t>((m_XZSize * 16) * (m_XZSize * 16));

    {
        const char* path = "GameRules/heightmap.bin";
        auto result = PlatformFilesystem.readFile(
            path, m_heightmapOverride.data(), m_heightmapOverride.size());
        if (result.status == IPlatformFilesystem::ReadStatus::TooLarge) {
            Log::info("Heightmap binary is too large!!\n");
            assert(0);
        } else if (result.status != IPlatformFilesystem::ReadStatus::Ok) {
            gameServices().fatalLoadError();
            assert(false);
        }
    }

    m_waterheightOverride =
        std::vector<uint8_t>((m_XZSize * 16) * (m_XZSize * 16));

    {
        const char* waterHeightPath = "GameRules/waterheight.bin";
        auto result = PlatformFilesystem.readFile(
            waterHeightPath, m_waterheightOverride.data(),
            m_waterheightOverride.size());
        if (result.status == IPlatformFilesystem::ReadStatus::NotFound) {
            memset(m_waterheightOverride.data(), level->seaLevel,
                   m_waterheightOverride.size());
        } else if (result.status == IPlatformFilesystem::ReadStatus::TooLarge) {
            Log::info("waterheight binary is too large!!\n");
            assert(0);
        } else if (result.status != IPlatformFilesystem::ReadStatus::Ok) {
            gameServices().fatalLoadError();
        }
    }

    caveFeature = new LargeCaveFeature();
    strongholdFeature = new StrongholdFeature();
    villageFeature = new VillageFeature(m_XZSize);
    mineShaftFeature = new MineShaftFeature();
    scatteredFeature = new RandomScatteredLargeFeature();
    canyonFeature = new CanyonFeature();

    this->level = level;

    random = new Random(seed);
    pprandom = new Random(
        seed);  // 4J - added, so that we can have a separate random for doing
                // post-processing in parallel with creation
    perlinNoise3 = new PerlinNoise(random, 4);
#endif
}

CustomLevelSource::~CustomLevelSource() {
#if defined(_OVERRIDE_HEIGHTMAP)
    delete caveFeature;
    delete strongholdFeature;
    delete villageFeature;
    delete mineShaftFeature;
    delete canyonFeature;

    delete random;
    delete perlinNoise3;
#endif
}

void CustomLevelSource::prepareHeights(int xOffs, int zOffs,
                                       std::vector<uint8_t>& blocks) {
#if defined(_OVERRIDE_HEIGHTMAP)
    int xChunks = 16 / CHUNK_WIDTH;
    int yChunks = Level::maxBuildHeight / CHUNK_HEIGHT;
    int waterHeight = level->seaLevel;

    int xSize = xChunks + 1;
    int ySize = Level::maxBuildHeight / CHUNK_HEIGHT + 1;
    int zSize = xChunks + 1;

    int xMapStart = xOffs + m_XZSize / 2;
    int zMapStart = zOffs + m_XZSize / 2;
    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < yChunks; yc++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int x = 0; x < CHUNK_WIDTH; x++) {
                        for (int z = 0; z < CHUNK_WIDTH; z++) {
                            int mapIndex =
                                (zMapStart * 16 + z + (zc * CHUNK_WIDTH)) *
                                    (m_XZSize * 16) +
                                (xMapStart * 16 + x + (xc * CHUNK_WIDTH));
                            int mapHeight = m_heightmapOverride[mapIndex];
                            waterHeight = m_waterheightOverride[mapIndex];
                            // Log::info("MapHeight = %d, y = %d\n",
                            // mapHeight, yc * CHUNK_HEIGHT + y);
                            ///////////////////////////////////////////////////////////////////
                            // 4J - add this chunk of code to make land
                            // "fall-off" at the edges of a finite world - size
                            // of that world is currently hard-coded in here
                            const int worldSize = m_XZSize * 16;
                            const int falloffStart =
                                32;  // chunks away from edge were we start
                                     // doing fall-off
                            const float falloffMax =
                                128.0f;  // max value we need to get to falloff
                                         // by the edge of the map

                            int xxx = ((xOffs * 16) + x + (xc * CHUNK_WIDTH));
                            int zzz = ((zOffs * 16) + z + (zc * CHUNK_WIDTH));

                            // Get distance to edges of world in x
                            int xxx0 = xxx + (worldSize / 2);
                            if (xxx0 < 0) xxx0 = 0;
                            int xxx1 = ((worldSize / 2) - 1) - xxx;
                            if (xxx1 < 0) xxx1 = 0;

                            // Get distance to edges of world in z
                            int zzz0 = zzz + (worldSize / 2);
                            if (zzz0 < 0) zzz0 = 0;
                            int zzz1 = ((worldSize / 2) - 1) - zzz;
                            if (zzz1 < 0) zzz1 = 0;

                            // Get min distance to any edge
                            int emin = xxx0;
                            if (xxx1 < emin) emin = xxx1;
                            if (zzz0 < emin) emin = zzz0;
                            if (zzz1 < emin) emin = zzz1;

                            float comp = 0.0f;

                            // Calculate how much we want the world to fall
                            // away, if we're in the defined region to do so
                            if (emin < falloffStart) {
                                int falloff = falloffStart - emin;
                                comp = ((float)falloff / (float)falloffStart) *
                                       falloffMax;
                            }
                            // 4J - end of extra code
                            ///////////////////////////////////////////////////////////////////
                            int tileId = 0;
                            // 4J - this comparison used to just be with 0.0f
                            // but is now varied by block above
                            if (yc * CHUNK_HEIGHT + y < mapHeight) {
                                tileId = (uint8_t)Tile::stone_Id;
                            } else if (yc * CHUNK_HEIGHT + y < waterHeight) {
                                tileId = (uint8_t)Tile::calmWater_Id;
                            }

                            // 4J - more extra code to make sure that the column
                            // at the edge of the world is just water & rock, to
                            // match the infinite sea that continues on after
                            // the edge of the world.

                            if (emin == 0) {
                                // This matches code in MultiPlayerChunkCache
                                // that makes the geometry which continues at
                                // the edge of the world
                                if (yc * CHUNK_HEIGHT + y <=
                                    (level->getSeaLevel() - 10))
                                    tileId = Tile::stone_Id;
                                else if (yc * CHUNK_HEIGHT + y <
                                         level->getSeaLevel())
                                    tileId = Tile::calmWater_Id;
                            }

                            int indexY = (yc * CHUNK_HEIGHT + y);
                            int offsAdjustment = 0;
                            if (indexY >=
                                Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                                indexY -=
                                    Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                                offsAdjustment =
                                    Level::COMPRESSED_CHUNK_SECTION_TILES;
                            }
                            int offs =
                                ((x + xc * CHUNK_WIDTH)
                                     << Level::genDepthBitsPlusFour |
                                 (z + zc * CHUNK_WIDTH) << Level::genDepthBits |
                                 indexY) +
                                offsAdjustment;
                            blocks[offs] = tileId;
                        }
                    }
                }
            }
        }
    }
#endif
}

void CustomLevelSource::buildSurfaces(int xOffs, int zOffs,
                                      std::vector<uint8_t>& blocks,
                                      std::vector<Biome*>& biomes) {
#if defined(_OVERRIDE_HEIGHTMAP)
    int waterHeight = level->seaLevel;
    int xMapStart = xOffs + m_XZSize / 2;
    int zMapStart = zOffs + m_XZSize / 2;

    double s = 1 / 32.0;

    std::vector<double> depthBuffer(
        16 * 16);  // 4J - used to be declared with class level
                   // scope but moved here for thread safety

    depthBuffer = perlinNoise3->getRegion(depthBuffer, xOffs * 16, zOffs * 16,
                                          0, 16, 16, 1, s * 2, s * 2, s * 2);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            int mapIndex =
                (zMapStart * 16 + z) * (m_XZSize * 16) + (xMapStart * 16 + x);
            waterHeight = m_waterheightOverride[mapIndex];

            Biome* b = biomes[z + x * 16];
            float temp = b->getTemperature();
            int runDepth = (int)(depthBuffer[x + z * 16] / 3 + 3 +
                                 random->nextDouble() * 0.25);

            int run = -1;

            uint8_t top = b->topMaterial;
            uint8_t material = b->material;

            LevelGenerationOptions* lgo = gameServices().getLevelGenerationOptions();
            if (lgo != nullptr) {
                lgo->getBiomeOverride(b->id, material, top);
            }

            for (int y = Level::maxBuildHeight - 1; y >= 0; y--) {
                int indexY = y;
                int offsAdjustment = 0;
                if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                    indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                    offsAdjustment = Level::COMPRESSED_CHUNK_SECTION_TILES;
                }
                int offs = (x << Level::genDepthBitsPlusFour |
                            z << Level::genDepthBits | indexY) +
                           offsAdjustment;

                if (y <= 1 + random->nextInt(
                                 2))  // 4J - changed to make the bedrock not
                                      // have bits you can get stuck in
                                      //                if (y <= 0 +
                                      //                random->nextInt(5))
                {
                    blocks[offs] = (uint8_t)Tile::unbreakable_Id;
                } else {
                    int old = blocks[offs];

                    if (old == 0) {
                        run = -1;
                    } else if (old == Tile::stone_Id) {
                        if (run == -1) {
                            if (runDepth <= 0) {
                                top = 0;
                                material = (uint8_t)Tile::stone_Id;
                            } else if (y >= waterHeight - 4 &&
                                       y <= waterHeight + 1) {
                                top = b->topMaterial;
                                material = b->material;
                                if (lgo != nullptr) {
                                    lgo->getBiomeOverride(b->id, material, top);
                                }
                            }

                            if (y < waterHeight && top == 0) {
                                if (temp < 0.15f)
                                    top = (uint8_t)Tile::ice_Id;
                                else
                                    top = (uint8_t)Tile::calmWater_Id;
                            }

                            run = runDepth;
                            if (y >= waterHeight - 1)
                                blocks[offs] = top;
                            else
                                blocks[offs] = material;
                        } else if (run > 0) {
                            run--;
                            blocks[offs] = material;

                            // place a few sandstone blocks beneath sand
                            // runs
                            if (run == 0 && material == Tile::sand_Id) {
                                run = random->nextInt(4);
                                material = (uint8_t)Tile::sandStone_Id;
                            }
                        }
                    }
                }
            }
        }
    }

#endif
}

LevelChunk* CustomLevelSource::create(int x, int z) {
#if defined(_OVERRIDE_HEIGHTMAP)
    return getChunk(x, z);
#else
    return nullptr;
#endif
}

LevelChunk* CustomLevelSource::getChunk(int xOffs, int zOffs) {
#if defined(_OVERRIDE_HEIGHTMAP)
    random->setSeed(xOffs * 341873128712l + zOffs * 132897987541l);

    // 4J - now allocating this with a physical alloc & bypassing general memory
    // management so that it will get cleanly freed
    int blocksSize = Level::maxBuildHeight * 16 * 16;
    uint8_t* tileData = (uint8_t*)malloc(blocksSize);
    memset(tileData, 0, blocksSize);
    std::vector<uint8_t> blocks =
        std::vector<uint8_t>(tileData, tileData + blocksSize);
    //    std::vector<uint8_t> blocks = std::vector<uint8_t>(16 * level->depth *
    //    16);

    // LevelChunk *levelChunk = new LevelChunk(level, blocks, xOffs, zOffs);
    // // 4J - moved to below

    prepareHeights(xOffs, zOffs, blocks);

    // 4J - Some changes made here to how biomes, temperatures and downfalls are
    // passed around for thread safety
    std::vector<Biome*> biomes;
    level->getBiomeSource()->getBiomeBlock(biomes, xOffs * 16, zOffs * 16, 16,
                                           16, true);

    buildSurfaces(xOffs, zOffs, blocks, biomes);

    caveFeature->apply(this, level, xOffs, zOffs, blocks);
    // 4J Stu Design Change - 1.8 gen goes stronghold, mineshaft, village,
    // canyon this changed in 1.2 to canyon, mineshaft, village, stronghold This
    // change makes sense as it stops canyons running through other structures
    canyonFeature->apply(this, level, xOffs, zOffs, blocks);
    if (generateStructures) {
        mineShaftFeature->apply(this, level, xOffs, zOffs, blocks);
        villageFeature->apply(this, level, xOffs, zOffs, blocks);
        strongholdFeature->apply(this, level, xOffs, zOffs, blocks);
        scatteredFeature->apply(this, level, xOffs, zOffs, blocks);
    }
    //        canyonFeature.apply(this, level, xOffs, zOffs, blocks);
    // townFeature.apply(this, level, xOffs, zOffs, blocks);
    // addCaves(xOffs, zOffs, blocks);
    // addTowns(xOffs, zOffs, blocks);

    //    levelChunk->recalcHeightmap();		// 4J - removed & moved
    //    into its own method

    // 4J - this now creates compressed block data from the blocks array passed
    // in, so moved it until after the blocks are actually finalised. We also
    // now need to free the passed in blocks as the LevelChunk doesn't use the
    // passed in allocation anymore.
    LevelChunk* levelChunk = new LevelChunk(level, blocks, xOffs, zOffs);
    free(tileData);

    return levelChunk;
#else
    return nullptr;
#endif
}

// 4J - removed & moved into its own method from getChunk, so we can call
// recalcHeightmap after the chunk is added into the cache. Without doing this,
// then loads of the lightgaps() calls will fail to add any lights, because
// adding a light checks if the cache has this chunk in. lightgaps also does
// light 1 block into the neighbouring chunks, and maybe that is somehow enough
// to get lighting to propagate round the world, but this just doesn't seem
// right - this isn't a new fault in the 360 version, have checked that java
// does the same.
void CustomLevelSource::lightChunk(LevelChunk* lc) {
#if defined(_OVERRIDE_HEIGHTMAP)
    lc->recalcHeightmap();
#endif
}

bool CustomLevelSource::hasChunk(int x, int y) { return true; }

void CustomLevelSource::calcWaterDepths(ChunkSource* parent, int xt, int zt) {
#if defined(_OVERRIDE_HEIGHTMAP)
    int xo = xt * 16;
    int zo = zt * 16;
    for (int x = 0; x < 16; x++) {
        int y = level->getSeaLevel();
        for (int z = 0; z < 16; z++) {
            int xp = xo + x + 7;
            int zp = zo + z + 7;
            int h = level->getHeightmap(xp, zp);
            if (h <= 0) {
                if (level->getHeightmap(xp - 1, zp) > 0 ||
                    level->getHeightmap(xp + 1, zp) > 0 ||
                    level->getHeightmap(xp, zp - 1) > 0 ||
                    level->getHeightmap(xp, zp + 1) > 0) {
                    bool hadWater = false;
                    if (hadWater ||
                        (level->getTile(xp - 1, y, zp) == Tile::calmWater_Id &&
                         level->getData(xp - 1, y, zp) < 7))
                        hadWater = true;
                    if (hadWater ||
                        (level->getTile(xp + 1, y, zp) == Tile::calmWater_Id &&
                         level->getData(xp + 1, y, zp) < 7))
                        hadWater = true;
                    if (hadWater ||
                        (level->getTile(xp, y, zp - 1) == Tile::calmWater_Id &&
                         level->getData(xp, y, zp - 1) < 7))
                        hadWater = true;
                    if (hadWater ||
                        (level->getTile(xp, y, zp + 1) == Tile::calmWater_Id &&
                         level->getData(xp, y, zp + 1) < 7))
                        hadWater = true;
                    if (hadWater) {
                        for (int x2 = -5; x2 <= 5; x2++) {
                            for (int z2 = -5; z2 <= 5; z2++) {
                                int d =
                                    (x2 > 0 ? x2 : -x2) + (z2 > 0 ? z2 : -z2);

                                if (d <= 5) {
                                    d = 6 - d;
                                    if (level->getTile(xp + x2, y, zp + z2) ==
                                        Tile::calmWater_Id) {
                                        int od =
                                            level->getData(xp + x2, y, zp + z2);
                                        if (od < 7 && od < d) {
                                            level->setData(
                                                xp + x2, y, zp + z2, d,
                                                Tile::UPDATE_CLIENTS);
                                        }
                                    }
                                }
                            }
                        }
                        if (hadWater) {
                            level->setTileAndData(xp, y, zp, Tile::calmWater_Id,
                                                  7, Tile::UPDATE_CLIENTS);
                            for (int y2 = 0; y2 < y; y2++) {
                                level->setTileAndData(xp, y2, zp,
                                                      Tile::calmWater_Id, 8,
                                                      Tile::UPDATE_CLIENTS);
                            }
                        }
                    }
                }
            }
        }
    }
#endif
}

// 4J - changed this to used pprandom rather than random, so that we can run it
// concurrently with getChunk
void CustomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
#if defined(_OVERRIDE_HEIGHTMAP)
    HeavyTile::instaFall = true;
    int xo = xt * 16;
    int zo = zt * 16;

    Biome* biome = level->getBiome(xo + 16, zo + 16);

    if (CustomLevelSource::FLOATING_ISLANDS) {
        calcWaterDepths(parent, xt, zt);
    }

    pprandom->setSeed(level->getSeed());
    int64_t xScale = pprandom->nextLong() / 2 * 2 + 1;
    int64_t zScale = pprandom->nextLong() / 2 * 2 + 1;
    pprandom->setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

    bool hasVillage = false;

    if (generateStructures) {
        mineShaftFeature->postProcess(level, pprandom, xt, zt);
        hasVillage = villageFeature->postProcess(level, pprandom, xt, zt);
        strongholdFeature->postProcess(level, pprandom, xt, zt);
        scatteredFeature->postProcess(level, random, xt, zt);
    }

    for (int i = 0; i < 8; i++) {
        int x = xo + pprandom->nextInt(16) + 8;
        int y = pprandom->nextInt(Level::maxBuildHeight);
        int z = zo + pprandom->nextInt(16) + 8;
        MonsterRoomFeature* mrf = new MonsterRoomFeature();
        if (mrf->place(level, pprandom, x, y, z)) {
        }
        delete mrf;
    }

    biome->decorate(level, pprandom, xo, zo);

    gameServices().processSchematics(parent->getChunk(xt, zt));

    MobSpawner::postProcessSpawnMobs(level, biome, xo + 8, zo + 8, 16, 16,
                                     pprandom);

    // 4J - brought forward from 1.2.3 to get snow back in taiga biomes
    xo += 8;
    zo += 8;
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            int y = level->getTopRainBlock(xo + x, zo + z);

            if (level->shouldFreezeIgnoreNeighbors(x + xo, y - 1, z + zo)) {
                level->setTileAndData(
                    x + xo, y - 1, z + zo, Tile::ice_Id, 0,
                    Tile::UPDATE_INVISIBLE);  // 4J - changed from setTile,
                                              // otherwise we end up creating a
                                              // *lot* of dynamic water tiles as
                                              // these ice tiles are set
            }
            if (level->shouldSnow(x + xo, y, z + zo)) {
                level->setTileAndData(x + xo, y, z + zo, Tile::topSnow_Id, 0,
                                      Tile::UPDATE_CLIENTS);
            }
        }
    }

    HeavyTile::instaFall = false;
#endif
}

bool CustomLevelSource::save(bool force, ProgressListener* progressListener) {
    return true;
}

bool CustomLevelSource::tick() { return false; }

bool CustomLevelSource::shouldSave() { return true; }

std::string CustomLevelSource::gatherStats() { return "CustomLevelSource"; }

std::vector<Biome::MobSpawnerData*>* CustomLevelSource::getMobsAt(
    MobCategory* mobCategory, int x, int y, int z) {
#if defined(_OVERRIDE_HEIGHTMAP)
    Biome* biome = level->getBiome(x, z);
    if (biome == nullptr) {
        return nullptr;
    }
    if (mobCategory == MobCategory::monster &&
        scatteredFeature->isSwamphut(x, y, z)) {
        return scatteredFeature->getSwamphutEnemies();
    }
    return biome->getMobs(mobCategory);
#else
    return nullptr;
#endif
}

TilePos* CustomLevelSource::findNearestMapFeature(
    Level* level, const std::string& featureName, int x, int y, int z) {
#if defined(_OVERRIDE_HEIGHTMAP)
    if (LargeFeature::STRONGHOLD == featureName &&
        strongholdFeature != nullptr) {
        return strongholdFeature->getNearestGeneratedFeature(level, x, y, z);
    }
#endif
    return nullptr;
}

void CustomLevelSource::recreateLogicStructuresForChunk(int chunkX,
                                                        int chunkZ) {
    if (generateStructures) {
#if defined(_OVERRIDE_HEIGHTMAP)
        std::vector<uint8_t> emptyBlocks;
        mineShaftFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
        villageFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
        strongholdFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
        scatteredFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
#endif
    }
}
