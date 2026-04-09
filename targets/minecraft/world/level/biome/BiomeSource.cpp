#include "BiomeSource.h"

#include <assert.h>

#include <algorithm>

#include "java/Random.h"
#include "java/System.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/ChunkPos.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/TilePos.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/biome/BiomeCache.h"
#include "minecraft/world/level/newbiome/layer/Layer.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "platform/input/input.h"
#include "strings.h"

// 4J - removal of separate temperature & downfall layers brought forward
// from 1.2.3
void BiomeSource::_init() {
    layer = nullptr;
    zoomedLayer = nullptr;

    cache = new BiomeCache(this);

    playerSpawnBiomes.push_back(Biome::forest);
    playerSpawnBiomes.push_back(Biome::taiga);
    // 4J-PB - Moving forward plains as a spawnable biome (mainly for the
    // Superflat world)
    playerSpawnBiomes.push_back(Biome::plains);
    playerSpawnBiomes.push_back(Biome::taigaHills);
    playerSpawnBiomes.push_back(Biome::forestHills);
    playerSpawnBiomes.push_back(Biome::jungle);
    playerSpawnBiomes.push_back(Biome::jungleHills);
}

void BiomeSource::_init(int64_t seed, LevelType* generator) {
    _init();

    std::vector<std::shared_ptr<Layer>> layers =
        Layer::getDefaultLayers(seed, generator);
    layer = layers[0];
    zoomedLayer = layers[1];
}

BiomeSource::BiomeSource() { _init(); }

// 4J added
BiomeSource::BiomeSource(int64_t seed, LevelType* generator) {
    _init(seed, generator);
}

// 4J - removal of separate temperature & downfall layers brought forward
// from 1.2.3
BiomeSource::BiomeSource(Level* level) {
    _init(level->getSeed(), level->getLevelData()->getGenerator());
}

BiomeSource::~BiomeSource() { delete cache; }

Biome* BiomeSource::getBiome(ChunkPos* cp) {
    return getBiome(cp->x << 4, cp->z << 4);
}

Biome* BiomeSource::getBiome(int x, int z) { return cache->getBiome(x, z); }

float BiomeSource::getDownfall(int x, int z) const {
    return cache->getDownfall(x, z);
}

// 4J - note that caller is responsible for deleting returned array.
// temperatures array is for output only.
std::vector<float> BiomeSource::getDownfallBlock(int x, int z, int w,
                                                 int h) const {
    std::vector<float> downfalls;
    getDownfallBlock(downfalls, x, z, w, h);
    return downfalls;
}

// 4J - note that caller is responsible for deleting returned array.
// temperatures array is for output only. 4J - removal of separate temperature &
// downfall layers brought forward from 1.2.3
void BiomeSource::getDownfallBlock(std::vector<float>& downfalls, int x, int z,
                                   int w, int h) const {
    // if (downfalls == nullptr || downfalls->length < w * h)
    if (downfalls.empty() || downfalls.size() < (unsigned int)(w * h)) {
        downfalls = std::vector<float>(w * h);
    }

    std::vector<int> result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++) {
        float d = (float)Biome::biomes[result[i]]->getDownfallInt() / 65536.0f;
        if (d > 1) d = 1;
        downfalls[i] = d;
    }
}

BiomeCache::Block* BiomeSource::getBlockAt(int x, int y) {
    return cache->getBlockAt(x, y);
}

float BiomeSource::getTemperature(int x, int y, int z) const {
    return scaleTemp(cache->getTemperature(x, z), y);
}

// 4J - brought forward from 1.2.3
float BiomeSource::scaleTemp(float temp, int y) const { return temp; }

std::vector<float> BiomeSource::getTemperatureBlock(int x, int z, int w,
                                                    int h) const {
    std::vector<float> temperatures;
    getTemperatureBlock(temperatures, x, z, w, h);
    return temperatures;
}

// 4J - note that caller is responsible for deleting returned array.
// temperatures array is for output only. 4J - removal of separate temperature &
// downfall layers brought forward from 1.2.3
void BiomeSource::getTemperatureBlock(std::vector<float>& temperatures, int x,
                                      int z, int w, int h) const {
    // if (temperatures == null || temperatures.size() < w * h) {
    if (temperatures.empty() || temperatures.size() < (unsigned int)(w * h)) {
        temperatures = std::vector<float>(w * h);
    }

    std::vector<int> result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++) {
        float t =
            (float)Biome::biomes[result[i]]->getTemperatureInt() / 65536.0f;
        if (t > 1) t = 1;
        temperatures[i] = t;
    }
}

std::vector<Biome*> BiomeSource::getRawBiomeBlock(int x, int z, int w,
                                                  int h) const {
    std::vector<Biome*> biomes;
    getRawBiomeBlock(biomes, x, z, w, h);
    return biomes;
}

// 4J added
void BiomeSource::getRawBiomeIndices(std::vector<int>& biomes, int x, int z,
                                     int w, int h) const {
    std::vector<int> result = layer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++) {
        biomes[i] = result[i];
    }
}

void BiomeSource::getRawBiomeBlock(std::vector<Biome*>& biomes, int x, int z,
                                   int w, int h) const {
    // if (biomes == null || biomes.size() < w * h)
    if (biomes.empty() || biomes.size() < (unsigned int)(w * h)) {
        biomes = std::vector<Biome*>(w * h);
    }

    std::vector<int> result = layer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++) {
        biomes[i] = Biome::biomes[result[i]];
#if !defined(_CONTENT_PACKAGE)
        if (biomes[i] == nullptr) {
            Log::info("Tried to assign null biome %d\n", result[i]);
            assert(0);
        }
#endif
    }
}

std::vector<Biome*> BiomeSource::getBiomeBlock(int x, int z, int w,
                                               int h) const {
    if (w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0) {
        return cache->getBiomeBlockAt(x, z);
    }
    std::vector<Biome*> biomes;
    getBiomeBlock(biomes, x, z, w, h, true);
    return biomes;
}

// 4J - caller is responsible for deleting biomes array
void BiomeSource::getBiomeBlock(std::vector<Biome*>& biomes, int x, int z,
                                int w, int h, bool useCache) const {
    // if (biomes == null || biomes.size() < w * h)
    if (biomes.empty() || biomes.size() < w * h) {
        biomes = std::vector<Biome*>(w * h);
    }

    if (useCache && w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0) {
        std::vector<Biome*> tmp = cache->getBiomeBlockAt(x, z);
        std::copy(tmp.begin(), tmp.begin() + w * h, biomes.begin());
        // the indices now. //4jcraft made it array delete
        // return biomes;
    }

    std::vector<int> result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++) {
        biomes[i] = Biome::biomes[result[i]];
    }
}

std::vector<uint8_t> BiomeSource::getBiomeIndexBlock(int x, int z, int w,
                                                     int h) const {
    if (w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0) {
        return cache->getBiomeIndexBlockAt(x, z);
    }
    std::vector<uint8_t> biomeIndices;
    getBiomeIndexBlock(biomeIndices, x, z, w, h, true);
    return biomeIndices;
}

// 4J - caller is responsible for deleting biomes array
void BiomeSource::getBiomeIndexBlock(std::vector<uint8_t>& biomeIndices, int x,
                                     int z, int w, int h, bool useCache) const {
    // if (biomes == null || biomes.size() < w * h)
    if (biomeIndices.empty() || biomeIndices.size() < w * h) {
        biomeIndices = std::vector<uint8_t>(w * h);
    }

    if (useCache && w == 16 && h == 16 && (x & 0xf) == 0 && (z & 0xf) == 0) {
        std::vector<uint8_t> tmp = cache->getBiomeIndexBlockAt(x, z);
        std::copy(tmp.begin(), tmp.begin() + w * h, biomeIndices.begin());
        // return biomes;
    }

    std::vector<int> result = zoomedLayer->getArea(x, z, w, h);
    for (int i = 0; i < w * h; i++) {
        biomeIndices[i] = (uint8_t)result[i];
    }
}

/**
 * Checks if an area around a block contains only the specified biomes.
 * Useful for placing elements like towns.
 *
 * This is a bit of a rough check, to make it as fast as possible. To ensure
 * NO other biomes, add a margin of at least four blocks to the radius
 */
bool BiomeSource::containsOnly(int x, int z, int r,
                               const std::vector<Biome*>& allowed) {
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;

    std::vector<int> biomes = layer->getArea(x0, z0, w, h);
    for (int i = 0; i < w * h; i++) {
        Biome* b = Biome::biomes[biomes[i]];
        if (find(allowed.begin(), allowed.end(), b) == allowed.end())
            return false;
    }

    return true;
}

/**
 * Checks if an area around a block contains only the specified biome.
 * Useful for placing elements like towns.
 *
 * This is a bit of a rough check, to make it as fast as possible. To ensure
 * NO other biomes, add a margin of at least four blocks to the radius
 */
bool BiomeSource::containsOnly(int x, int z, int r, Biome* allowed) {
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0;
    int h = z1 - z0;
    int biomesCount = w * h;
    std::vector<int> biomes = layer->getArea(x0, z0, w, h);
    for (unsigned int i = 0; i < biomesCount; i++) {
        Biome* b = Biome::biomes[biomes[i]];
        if (allowed != b) return false;
    }

    return true;
}

/**
 * Finds the specified biome within the radius. This will return a random
 * position if several are found. This test is fairly rough.
 *
 * Returns null if the biome wasn't found
 */
TilePos* BiomeSource::findBiome(int x, int z, int r, Biome* toFind,
                                Random* random) {
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;
    std::vector<int> biomes = layer->getArea(x0, z0, w, h);
    TilePos* res = nullptr;
    int found = 0;
    int biomesCount = w * h;
    for (unsigned int i = 0; i < biomesCount; i++) {
        int xx = x0 + i % w;
        int zz = z0 + i / w;
        Biome* b = Biome::biomes[biomes[i]];
        if (b == toFind) {
            if (res == nullptr || random->nextInt(found + 1) == 0) {
                res = new TilePos(xx, 0, zz);
                found++;
            }
        }
    }

    return res;
}

/**
 * Finds one of the specified biomes within the radius. This will return a
 * random position if several are found. This test is fairly rough.
 *
 * Returns null if the biome wasn't found
 */
TilePos* BiomeSource::findBiome(int x, int z, int r,
                                const std::vector<Biome*>& allowed,
                                Random* random) {
    int x0 = ((x - r) >> 2);
    int z0 = ((z - r) >> 2);
    int x1 = ((x + r) >> 2);
    int z1 = ((z + r) >> 2);

    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;
    std::vector<int> biomes = layer->getArea(x0, z0, w, h);
    TilePos* res = nullptr;
    int found = 0;
    for (unsigned int i = 0; i < w * h; i++) {
        int xx = (x0 + i % w) << 2;
        int zz = (z0 + i / w) << 2;
        Biome* b = Biome::biomes[biomes[i]];
        if (find(allowed.begin(), allowed.end(), b) != allowed.end()) {
            if (res == nullptr || random->nextInt(found + 1) == 0) {
                delete res;
                res = new TilePos(xx, 0, zz);
                found++;
            }
        }
    }

    return res;
}

void BiomeSource::update() { cache->update(); }

// #define DEBUG_SEEDS 50

// 4J added - find a seed for this biomesource that matches certain criteria
int64_t BiomeSource::findSeed(LevelType* generator) {
    int64_t bestSeed = 0;

    ProgressRenderer* mcprogress = Minecraft::GetInstance()->progressRenderer;
    mcprogress->progressStage(IDS_PROGRESS_NEW_WORLD_SEED);

#if !defined(_CONTENT_PACKAGE)
    if (gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_EnableBiomeOverride)) {
        // Do nothing
    } else
#endif
    {
#if defined(DEBUG_SEEDS)
        for (int k = 0; k < DEBUG_SEEDS; k++)
#endif
        {
            // Try and genuinely random this search up
            Random* pr = new Random(System::nanoTime());

            // Raw biome data has one result per 4x4 group of tiles.
            // Removing a border of 8 from each side since we'll be doing
            // special things at the edge to turn our world into an island, and
            // so don't want to count things in the edge region in case they
            // later get removed
            static const int biomeWidth =
                (54 * 4) - 16;  // Should be even so we can offset evenly
            static const int biomeOffset = -(biomeWidth / 2);

            // Storage for our biome indices
            std::vector<int> indices =
                std::vector<int>(biomeWidth * biomeWidth);

            // Storage for the fractional amounts of each biome that will be
            // calculated
            float toCompare[Biome::BIOME_COUNT];

            bool matchFound = false;
            int tryCount = 0;

            // Just keeping trying to generate seeds until we find one that
            // matches our criteria
            do {
                int64_t seed = pr->nextLong();
                BiomeSource* biomeSource = new BiomeSource(seed, generator);

                biomeSource->getRawBiomeIndices(
                    indices, biomeOffset, biomeOffset, biomeWidth, biomeWidth);
                getFracs(indices, toCompare);

                matchFound = getIsMatch(toCompare);

                if (matchFound) bestSeed = seed;

                delete biomeSource;
                tryCount++;

                mcprogress->progressStagePercentage(tryCount % 100);
            } while (!matchFound);

            // Clean up
            delete pr;

#if defined(DEBUG_SEEDS)
            Log::info("%d: %d tries taken, seed used is %lld\n", k, tryCount,
                      bestSeed);

            BiomeSource* biomeSource = new BiomeSource(bestSeed);
            std::vector<Biome*> biomes = biomeSource->getBiomeBlock(
                -27 * 16, -27 * 16, 54 * 16, 54 * 16);

            unsigned int* pixels = new unsigned int[54 * 16 * 54 * 16];
            for (int i = 0; i < 54 * 16 * 54 * 16; i++) {
                int id = biomes[i]->id;

                // Create following colours:
                // 0	ocean				0000	black
                // 1	plains				0001	pastel cyan
                // 2	desert				0010	green
                // 3	extreme hills		0011	yellow
                // 4	forest				0100	blue
                // 5	taiga				0101	magenta
                // 6	swamps				0110	cyan
                // 7	river				0111	white
                // 8	hell				1000	grey
                // 9	end biome			1001	white
                // 10	frozen ocean		1010	pastel green
                // 11	frozen river		1011	pastel yellow
                // 12	ice flats			1100	pastel blue
                // 13	ice mountains		1101	pastel magenta
                // 14	mushroom island		1110	red
                // 15   mushroom shore		1111	pastel red

                if (id == 1)
                    id = 14;
                else if (id == 14)
                    id = 1;
                else if (id == 9)
                    id = 15;
                else if (id == 15)
                    id = 9;
                pixels[i] = 0xff000000;
                if (id & 1) pixels[i] |= 0x00ff0000;
                if (id & 2) pixels[i] |= 0x0000ff00;
                if (id & 4) pixels[i] |= 0x000000ff;
                if (id & 8) pixels[i] |= 0x00808080;
            }
            D3DXIMAGE_INFO srcInfo;
            srcInfo.Format = D3DFMT_LIN_A8R8G8B8;
            srcInfo.ImageFileFormat = D3DXIFF_BMP;
            srcInfo.Width = 54 * 16;
            srcInfo.Height = 54 * 16;

            char buf[256];
            sprintf(buf, "GAME:\\BiomeTest%d.bmp", k);
            PlatformRender.SaveTextureData(buf, &srcInfo, (int*)pixels);

            delete[] pixels;
            delete biomeSource;
#endif
        }
    }

    return bestSeed;
}

// 4J added - get the fractional amounts of each biome type in the given indices
void BiomeSource::getFracs(std::vector<int>& indices, float* fracs) {
    for (int i = 0; i < Biome::BIOME_COUNT; i++) {
        fracs[i] = 0.0f;
    }

    for (int i = 0; i < indices.size(); i++) {
        fracs[indices[i]] += 1.0f;
    }

    for (int i = 0; i < Biome::BIOME_COUNT; i++) {
        fracs[i] /= (float)(indices.size());
    }
}

// 4J added - determine if this particular set of fractional amounts of biome
// types matches are requirements
bool BiomeSource::getIsMatch(float* frac) {
    // A true for a particular biome type here marks it as one that *has* to be
    // present
    static const bool critical[Biome::BIOME_COUNT] = {
        true,   // ocean
        true,   // plains
        true,   // desert
        false,  // extreme hills
        true,   // forest
        true,   // taiga
        true,   // swamps
        false,  // river
        false,  // hell
        false,  // end biome
        false,  // frozen ocean
        false,  // frozen river
        false,  // ice flats
        false,  // ice mountains
        true,   // mushroom island / shore
        false,  // mushroom shore (combined with above)
        false,  // beach
        false,  // desert hills (combined with desert)
        false,  // forest hills (combined with forest)
        false,  // taiga hills (combined with taga)
        false,  // small extreme hills
        true,   // jungle
        false,  // jungle hills (combined with jungle)
    };

    // Don't want more than 15% ocean
    if (frac[0] > 0.15f) {
        return false;
    }

    // Consider mushroom shore & islands as the same by finding max
    frac[14] = ((frac[15] > frac[14]) ? frac[15] : frac[14]);

    // Merge desert and desert hills
    frac[2] = ((frac[17] > frac[2]) ? frac[17] : frac[2]);

    // Merge forest and forest hills
    frac[4] = ((frac[18] > frac[4]) ? frac[18] : frac[4]);

    // Merge taiga and taiga hills
    frac[5] = ((frac[19] > frac[5]) ? frac[19] : frac[5]);

    // Merge jungle and jungle hills
    frac[21] = ((frac[22] > frac[21]) ? frac[22] : frac[21]);

    // Loop through all biome types, and:
    // (1) count them
    // (2) give up if one of the critical ones is missing

    int typeCount = 0;
    for (int i = 0; i < Biome::BIOME_COUNT; i++) {
        // We want to skip some where we have merged with another type
        if (i == 15 || i == 17 || i == 18 || i == 19 || i == 22) continue;

        // Consider 0.1% as being "present" - this equates an area of about 3
        // chunks
        if (frac[i] > 0.001f) {
            typeCount++;
        } else {
            // If a critical biome is missing, just give up
            if (critical[i]) {
                return false;
            }
        }
    }

    // Consider as suitable if we've got all the critical ones, and in total 9
    // or more - currently there's 8 critical so this just forces at least 1
    // more others
    return (typeCount >= 9);
}
