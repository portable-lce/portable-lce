#include "minecraft/IGameServices.h"
#include "RandomLevelSource.h"

#include <stdlib.h>
#include <string.h>

#include <cstdint>

#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "util/Timer.h"
#include "java/Random.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/MobCategory.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/MobSpawner.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/biome/BiomeSource.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/levelgen/CanyonFeature.h"
#include "minecraft/world/level/levelgen/LargeCaveFeature.h"
#include "minecraft/world/level/levelgen/LargeFeature.h"
#include "minecraft/world/level/levelgen/feature/LakeFeature.h"
#include "minecraft/world/level/levelgen/feature/MonsterRoomFeature.h"
#include "minecraft/world/level/levelgen/structure/MineShaftFeature.h"
#include "minecraft/world/level/levelgen/structure/RandomScatteredLargeFeature.h"
#include "minecraft/world/level/levelgen/structure/StrongholdFeature.h"
#include "minecraft/world/level/levelgen/structure/VillageFeature.h"
#include "minecraft/world/level/levelgen/synth/PerlinNoise.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/tile/HeavyTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/Vec3.h"

const double RandomLevelSource::SNOW_SCALE = 0.3;
const double RandomLevelSource::SNOW_CUTOFF = 0.5;

RandomLevelSource::RandomLevelSource(Level* level, int64_t seed,
                                     bool generateStructures)
    : generateStructures(generateStructures) {
    m_XZSize = level->getLevelData()->getXZSize();
#if defined(_LARGE_WORLDS)
    level->getLevelData()->getMoatFlags(&m_classicEdgeMoat, &m_smallEdgeMoat,
                                        &m_mediumEdgeMoat);
#endif
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
    lperlinNoise1 = new PerlinNoise(random, 16);
    lperlinNoise2 = new PerlinNoise(random, 16);
    perlinNoise1 = new PerlinNoise(random, 8);
    perlinNoise3 = new PerlinNoise(random, 4);

    scaleNoise = new PerlinNoise(random, 10);
    depthNoise = new PerlinNoise(random, 16);

    if (FLOATING_ISLANDS) {
        floatingIslandScale = new PerlinNoise(random, 10);
        floatingIslandNoise = new PerlinNoise(random, 16);
    } else {
        floatingIslandScale = nullptr;
        floatingIslandNoise = nullptr;
    }

    forestNoise = new PerlinNoise(random, 8);
}

RandomLevelSource::~RandomLevelSource() {
    delete caveFeature;
    delete strongholdFeature;
    delete villageFeature;
    delete mineShaftFeature;
    delete scatteredFeature;
    delete canyonFeature;

    delete random;
    ;
    delete lperlinNoise1;
    delete lperlinNoise2;
    delete perlinNoise1;
    delete perlinNoise3;

    delete scaleNoise;
    delete depthNoise;

    if (FLOATING_ISLANDS) {
        delete floatingIslandScale;
        delete floatingIslandNoise;
    }

    delete forestNoise;
}

int g_numPrepareHeightCalls = 0;
time_util::clock::duration g_totalPrepareHeightsTime{};
time_util::clock::duration g_averagePrepareHeightsTime{};

#if defined(_LARGE_WORLDS)

int RandomLevelSource::getMinDistanceToEdge(int xxx, int zzz, int worldSize,
                                            float falloffStart) {
    // Get distance to edges of world in x
    // we have to do a proper line dist check here
    int min = -worldSize / 2;
    int max = (worldSize / 2) - 1;

    // 	// only check if either x or z values are within the falloff
    // 	if(xxx > (min - falloffStart)

    Vec3 topLeft(min, 0, min);
    Vec3 topRight(max, 0, min);
    Vec3 bottomLeft(min, 0, max);
    Vec3 bottomRight(max, 0, max);

    float closest = falloffStart;
    float dist;
    // make sure we're in range of the edges before we do a full distance check
    if ((xxx > (min - falloffStart) && xxx < (min + falloffStart)) ||
        (xxx > (max - falloffStart) && xxx < (max + falloffStart))) {
        Vec3 point(xxx, 0, zzz);

        if (xxx > 0)
            dist = point.distanceFromLine(topRight, bottomRight);
        else
            dist = point.distanceFromLine(topLeft, bottomLeft);
        closest = dist;
    }

    // make sure we're in range of the edges before we do a full distance check
    if ((zzz > (min - falloffStart) && zzz < (min + falloffStart)) ||
        (zzz > (max - falloffStart) && zzz < (max + falloffStart))) {
        Vec3 point(xxx, 0, zzz);

        if (zzz > 0)
            dist = point.distanceFromLine(bottomLeft, bottomRight);
        else
            dist = point.distanceFromLine(topLeft, topRight);
        if (dist < closest) closest = dist;
    }

    return closest;
}

float RandomLevelSource::getHeightFalloff(int xxx, int zzz, int* pEMin) {
    ///////////////////////////////////////////////////////////////////
    // 4J - add this chunk of code to make land "fall-off" at the edges of
    // a finite world - size of that world is currently hard-coded in here
    const int worldSize = m_XZSize * 16;
    const int falloffStart =
        32;  // chunks away from edge were we start doing fall-off
    const float falloffMax =
        128.0f;  // max value we need to get to falloff by the edge of the map

    float comp = 0.0f;
    int emin = getMinDistanceToEdge(xxx, zzz, worldSize, falloffStart);
    // check if we have a larger world that should have moats
    int expandedWorldSizes[3] = {LEVEL_WIDTH_CLASSIC * 16,
                                 LEVEL_WIDTH_SMALL * 16,
                                 LEVEL_WIDTH_MEDIUM * 16};
    bool expandedMoatValues[3] = {m_classicEdgeMoat, m_smallEdgeMoat,
                                  m_mediumEdgeMoat};
    for (int i = 0; i < 3; i++) {
        if (expandedMoatValues[i] && (worldSize > expandedWorldSizes[i])) {
            // this world has been expanded, with moat settings, so we need
            // fallofs at this edges too
            int eminMoat = getMinDistanceToEdge(xxx, zzz, expandedWorldSizes[i],
                                                falloffStart);
            if (eminMoat < emin) {
                emin = eminMoat;
            }
        }
    }

    // Calculate how much we want the world to fall away, if we're in the
    // defined region to do so
    if (emin < falloffStart) {
        int falloff = falloffStart - emin;
        comp = ((float)falloff / (float)falloffStart) * falloffMax;
    }
    *pEMin = emin;
    return comp;
    // 4J - end of extra code
    ///////////////////////////////////////////////////////////////////
}

#else

// MGH  - go back to using the simpler version for PS3/vita/360, as it was
// causing a lot of slow down on the tuturial generation
float RandomLevelSource::getHeightFalloff(int xxx, int zzz, int* pEMin) {
    ///////////////////////////////////////////////////////////////////
    // 4J - add this chunk of code to make land "fall-off" at the edges of
    // a finite world - size of that world is currently hard-coded in here
    const int worldSize = m_XZSize * 16;
    const int falloffStart =
        32;  // chunks away from edge were we start doing fall-off
    const float falloffMax =
        128.0f;  // max value we need to get to falloff by the edge of the map

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

    // Calculate how much we want the world to fall away, if we're in the
    // defined region to do so
    if (emin < falloffStart) {
        int falloff = falloffStart - emin;
        comp = ((float)falloff / (float)falloffStart) * falloffMax;
    }
    // 4J - end of extra code
    ///////////////////////////////////////////////////////////////////
    *pEMin = emin;
    return comp;
}

#endif

void RandomLevelSource::prepareHeights(int xOffs, int zOffs,
                                       std::vector<uint8_t>& blocks) {
    int xChunks = 16 / CHUNK_WIDTH;
    int yChunks = Level::genDepth / CHUNK_HEIGHT;
    int waterHeight = level->seaLevel;

    int xSize = xChunks + 1;
    int ySize = Level::genDepth / CHUNK_HEIGHT + 1;
    int zSize = xChunks + 1;

    std::vector<Biome*> biomes;  // 4J created locally here for thread safety,
                                 // java has this as a class member

    level->getBiomeSource()->getRawBiomeBlock(biomes, xOffs * CHUNK_WIDTH - 2,
                                              zOffs * CHUNK_WIDTH - 2,
                                              xSize + 5, zSize + 5);

    std::vector<double>
        buffer;  // 4J - used to be declared with class level scope but
                 // tidying up for thread safety reasons
    buffer = getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks, xSize,
                        ySize, zSize, biomes);

    time_util::Timer timer;
    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < yChunks; yc++) {
                double yStep = 1 / (double)CHUNK_HEIGHT;
                double s0 =
                    buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
                double s1 =
                    buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
                double s2 =
                    buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
                double s3 =
                    buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

                double s0a =
                    (buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] -
                     s0) *
                    yStep;
                double s1a =
                    (buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] -
                     s1) *
                    yStep;
                double s2a =
                    (buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] -
                     s2) *
                    yStep;
                double s3a =
                    (buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] -
                     s3) *
                    yStep;

                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    double xStep = 1 / (double)CHUNK_WIDTH;

                    double _s0 = s0;
                    double _s1 = s1;
                    double _s0a = (s2 - s0) * xStep;
                    double _s1a = (s3 - s1) * xStep;

                    for (int x = 0; x < CHUNK_WIDTH; x++) {
                        int offs = (unsigned)(x + (unsigned)xc * CHUNK_WIDTH)
                                       << Level::genDepthBitsPlusFour |
                                   ((unsigned)zc * CHUNK_WIDTH)
                                       << Level::genDepthBits |
                                   (yc * CHUNK_HEIGHT + y);
                        int step = 1 << Level::genDepthBits;
                        offs -= step;
                        double zStep = 1 / (double)CHUNK_WIDTH;

                        double val = _s0;
                        double vala = (_s1 - _s0) * zStep;
                        val -= vala;
                        for (int z = 0; z < CHUNK_WIDTH; z++) {
                            // 4J Stu - I have removed all uses of the new
                            // getHeightFalloff function for now as we had some
                            // problems with PS3/PSVita world generation I have
                            // fixed the non large worlds method, however we
                            // will be happier if the current builds go out with
                            // completely old code We can put the new code back
                            // in mid-november 2014 once those PS3/Vita builds
                            // are gone (and the PS4 doesn't have world
                            // enlarging in these either anyway)
                            int xxx = ((xOffs * 16) + x + (xc * CHUNK_WIDTH));
                            int zzz = ((zOffs * 16) + z + (zc * CHUNK_WIDTH));
                            int emin;
                            float comp = getHeightFalloff(xxx, zzz, &emin);

                            // 4J - slightly rearranged this code (as of
                            // java 1.0.1 merge) to better fit with changes
                            // we've made edge-of-world things - original sets
                            // blocks[offs += step] directly here rather than
                            // setting a tileId
                            int tileId = 0;
                            // 4J - this comparison used to just be with 0.0f
                            // but is now varied by block above
                            if ((val += vala) > comp) {
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

                            blocks[offs += step] = tileId;
                        }
                        _s0 += _s0a;
                        _s1 += _s1a;
                    }

                    s0 += s0a;
                    s1 += s1a;
                    s2 += s2a;
                    s3 += s3a;
                }
            }
        }
    }
    g_numPrepareHeightCalls++;
    g_totalPrepareHeightsTime += timer.elapsed();
    g_averagePrepareHeightsTime =
        g_totalPrepareHeightsTime / g_numPrepareHeightCalls;
}

void RandomLevelSource::buildSurfaces(int xOffs, int zOffs,
                                      std::vector<uint8_t>& blocks,
                                      std::vector<Biome*>& biomes) {
    int waterHeight = level->seaLevel;

    double s = 1 / 32.0;

    std::vector<double> depthBuffer(
        16 * 16);  // 4J - used to be declared with class level
                   // scope but moved here for thread safety

    depthBuffer = perlinNoise3->getRegion(depthBuffer, xOffs * 16, zOffs * 16,
                                          0, 16, 16, 1, s * 2, s * 2, s * 2);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
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

            for (int y = Level::genDepthMinusOne; y >= 0; y--) {
                int offs = (z * 16 + x) * Level::genDepth + y;

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

                            // place a few sandstone blocks beneath sand runs
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
}

LevelChunk* RandomLevelSource::create(int x, int z) { return getChunk(x, z); }

LevelChunk* RandomLevelSource::getChunk(int xOffs, int zOffs) {
    random->setSeed(xOffs * 341873128712l + zOffs * 132897987541l);

    // 4J - now allocating this with a physical alloc & bypassing general memory
    // management so that it will get cleanly freed
    int blocksSize = Level::genDepth * 16 * 16;
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
}

// 4J - removed & moved into its own method from getChunk, so we can call
// recalcHeightmap after the chunk is added into the cache. Without doing this,
// then loads of the lightgaps() calls will fail to add any lights, because
// adding a light checks if the cache has this chunk in. lightgaps also does
// light 1 block into the neighbouring chunks, and maybe that is somehow enough
// to get lighting to propagate round the world, but this just doesn't seem
// right - this isn't a new fault in the 360 version, have checked that java
// does the same.
void RandomLevelSource::lightChunk(LevelChunk* lc) { lc->recalcHeightmap(); }

std::vector<double> RandomLevelSource::getHeights(std::vector<double>& buffer,
                                                  int x, int y, int z,
                                                  int xSize, int ySize,
                                                  int zSize,
                                                  std::vector<Biome*>& biomes) {
    if (buffer.empty()) {
        buffer = std::vector<double>(xSize * ySize * zSize);
    }
    if (pows.empty()) {
        pows = std::vector<float>(5 * 5);
        for (int xb = -2; xb <= 2; xb++) {
            for (int zb = -2; zb <= 2; zb++) {
                float ppp = 10.0f / Mth::sqrt(xb * xb + zb * zb + 0.2f);
                pows[xb + 2 + (zb + 2) * 5] = ppp;
            }
        }
    }

    double s = 1 * 684.412;
    double hs = 1 * 684.412;

    std::vector<double> pnr, ar, br, sr, dr, fi,
        fis;  // 4J - used to be declared with class level scope but moved here
              // for thread safety

    if (FLOATING_ISLANDS) {
        fis = floatingIslandScale->getRegion(fis, x, y, z, xSize, 1, zSize, 1.0,
                                             0, 1.0);
        fi = floatingIslandNoise->getRegion(fi, x, y, z, xSize, 1, zSize, 500.0,
                                            0, 500.0);
    }

    sr = scaleNoise->getRegion(sr, x, z, xSize, zSize, 1.121, 1.121, 0.5);
    dr = depthNoise->getRegion(dr, x, z, xSize, zSize, 200.0, 200.0, 0.5);
    pnr = perlinNoise1->getRegion(pnr, x, y, z, xSize, ySize, zSize, s / 80.0,
                                  hs / 160.0, s / 80.0);
    ar = lperlinNoise1->getRegion(ar, x, y, z, xSize, ySize, zSize, s, hs, s);
    br = lperlinNoise2->getRegion(br, x, y, z, xSize, ySize, zSize, s, hs, s);

    x = z = 0;

    int p = 0;
    int pp = 0;

    for (int xx = 0; xx < xSize; xx++) {
        for (int zz = 0; zz < zSize; zz++) {
            float sss = 0;
            float ddd = 0;
            float pow = 0;

            int rr = 2;

            Biome* mb = biomes[(xx + 2) + (zz + 2) * (xSize + 5)];
            for (int xb = -rr; xb <= rr; xb++) {
                for (int zb = -rr; zb <= rr; zb++) {
                    Biome* b =
                        biomes[(xx + xb + 2) + (zz + zb + 2) * (xSize + 5)];
                    float ppp = pows[xb + 2 + (zb + 2) * 5] / (b->depth + 2);
                    if (b->depth > mb->depth) {
                        ppp /= 2;
                    }
                    sss += b->scale * ppp;
                    ddd += b->depth * ppp;
                    pow += ppp;
                }
            }
            sss /= pow;
            ddd /= pow;

            sss = sss * 0.9f + 0.1f;
            ddd = (ddd * 4 - 1) / 8.0f;

            double rdepth = (dr[pp] / 8000.0);
            if (rdepth < 0) rdepth = -rdepth * 0.3;
            rdepth = rdepth * 3.0 - 2.0;

            if (rdepth < 0) {
                rdepth = rdepth / 2;
                if (rdepth < -1) rdepth = -1;
                rdepth = rdepth / 1.4;
                rdepth /= 2;
            } else {
                if (rdepth > 1) rdepth = 1;
                rdepth = rdepth / 8;
            }

            pp++;

            for (int yy = 0; yy < ySize; yy++) {
                double depth = ddd;
                double scale = sss;

                depth += rdepth * 0.2;
                depth = depth * ySize / 16.0;

                double yCenter = ySize / 2.0 + depth * 4;

                double val = 0;

                double yOffs =
                    (yy - (yCenter)) * 12 * 128 / Level::genDepth / scale;

                if (yOffs < 0) yOffs *= 4;

                double bb = ar[p] / 512;
                double cc = br[p] / 512;

                double v = (pnr[p] / 10 + 1) / 2;
                if (v < 0)
                    val = bb;
                else if (v > 1)
                    val = cc;
                else
                    val = bb + (cc - bb) * v;
                val -= yOffs;

                if (yy > ySize - 4) {
                    double slide = (yy - (ySize - 4)) / (4 - 1.0f);
                    val = val * (1 - slide) + -10 * slide;
                }

                buffer[p] = val;
                p++;
            }
        }
    }

    return buffer;
}

bool RandomLevelSource::hasChunk(int x, int y) { return true; }

void RandomLevelSource::calcWaterDepths(ChunkSource* parent, int xt, int zt) {
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
                                            level->setData(xp + x2, y, zp + z2,
                                                           d, Tile::UPDATE_ALL);
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
}

// 4J - changed this to used pprandom rather than random, so that we can run it
// concurrently with getChunk
void RandomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
    HeavyTile::instaFall = true;
    int xo = xt * 16;
    int zo = zt * 16;

    Biome* biome = level->getBiome(xo + 16, zo + 16);

    if (FLOATING_ISLANDS) {
        calcWaterDepths(parent, xt, zt);
    }

    pprandom->setSeed(level->getSeed());
    int64_t xScale = pprandom->nextLong() / 2 * 2 + 1;
    int64_t zScale = pprandom->nextLong() / 2 * 2 + 1;
    // 4jcraft added casts to a higher int and unsigned
    pprandom->setSeed((((uint64_t)xt * (uint64_t)xScale) +
                       ((uint64_t)zt * (uint64_t)zScale)) ^
                      level->getSeed());

    bool hasVillage = false;

    if (generateStructures) {
        mineShaftFeature->postProcess(level, pprandom, xt, zt);
        hasVillage = villageFeature->postProcess(level, pprandom, xt, zt);
        strongholdFeature->postProcess(level, pprandom, xt, zt);
        scatteredFeature->postProcess(level, random, xt, zt);
    }

    if (biome != Biome::desert && biome != Biome::desertHills) {
        if (!hasVillage && pprandom->nextInt(4) == 0) {
            int x = xo + pprandom->nextInt(16) + 8;
            int y = pprandom->nextInt(Level::genDepth);
            int z = zo + pprandom->nextInt(16) + 8;

            LakeFeature calmWater(Tile::calmWater_Id);
            calmWater.place(level, pprandom, x, y, z);
        }
    }

    if (!hasVillage && pprandom->nextInt(8) == 0) {
        int x = xo + pprandom->nextInt(16) + 8;
        int y = pprandom->nextInt(pprandom->nextInt(Level::genDepth - 8) + 8);
        int z = zo + pprandom->nextInt(16) + 8;
        if (y < level->seaLevel || pprandom->nextInt(10) == 0) {
            LakeFeature calmLava(Tile::calmLava_Id);
            calmLava.place(level, pprandom, x, y, z);
        }
    }

    for (int i = 0; i < 8; i++) {
        int x = xo + pprandom->nextInt(16) + 8;
        int y = pprandom->nextInt(Level::genDepth);
        int z = zo + pprandom->nextInt(16) + 8;
        MonsterRoomFeature mrf;
        mrf.place(level, pprandom, x, y, z);
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
                level->setTileAndData(x + xo, y - 1, z + zo, Tile::ice_Id, 0,
                                      Tile::UPDATE_CLIENTS);
            }
            if (level->shouldSnow(x + xo, y, z + zo)) {
                level->setTileAndData(x + xo, y, z + zo, Tile::topSnow_Id, 0,
                                      Tile::UPDATE_CLIENTS);
            }
        }
    }

    HeavyTile::instaFall = false;
}

bool RandomLevelSource::save(bool force, ProgressListener* progressListener) {
    return true;
}

bool RandomLevelSource::tick() { return false; }

bool RandomLevelSource::shouldSave() { return true; }

std::string RandomLevelSource::gatherStats() { return "RandomLevelSource"; }

std::vector<Biome::MobSpawnerData*>* RandomLevelSource::getMobsAt(
    MobCategory* mobCategory, int x, int y, int z) {
    Biome* biome = level->getBiome(x, z);
    if (biome == nullptr) {
        return nullptr;
    }
    if (mobCategory == MobCategory::monster &&
        scatteredFeature->isSwamphut(x, y, z)) {
        return scatteredFeature->getSwamphutEnemies();
    }
    return biome->getMobs(mobCategory);
}

TilePos* RandomLevelSource::findNearestMapFeature(
    Level* level, const std::string& featureName, int x, int y, int z) {
    if (LargeFeature::STRONGHOLD == featureName &&
        strongholdFeature != nullptr) {
        return strongholdFeature->getNearestGeneratedFeature(level, x, y, z);
    }
    return nullptr;
}

void RandomLevelSource::recreateLogicStructuresForChunk(int chunkX,
                                                        int chunkZ) {
    if (generateStructures) {
        std::vector<uint8_t> emptyBlocks;
        mineShaftFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
        villageFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
        strongholdFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
        scatteredFeature->apply(this, level, chunkX, chunkZ, emptyBlocks);
    }
}
