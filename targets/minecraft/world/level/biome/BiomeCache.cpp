#include "BiomeCache.h"

#include <utility>

#include "BiomeSource.h"
#include "minecraft/IGameServices.h"
#include "minecraft/world/level/biome/Biome.h"

BiomeCache::Block::Block(int x, int z, BiomeCache* parent) {
    // 	temps = std::vector<float>(ZONE_SIZE * ZONE_SIZE, false);
    // // MGH - added "no clear" flag to arrayWithLength 	downfall =
    // std::vector<float>(ZONE_SIZE
    // * ZONE_SIZE, false); 	biomes = std::vector<Biome*>(ZONE_SIZE *
    // ZONE_SIZE, false);
    biomeIndices = std::vector<uint8_t>(ZONE_SIZE * ZONE_SIZE, false);

    lastUse = 0;
    this->x = x;
    this->z = z;
    // 	parent->source->getTemperatureBlock(temps, x << ZONE_SIZE_BITS, z <<
    // ZONE_SIZE_BITS, ZONE_SIZE, ZONE_SIZE);
    // 	parent->source->getDownfallBlock(downfall, x << ZONE_SIZE_BITS, z <<
    // ZONE_SIZE_BITS, ZONE_SIZE, ZONE_SIZE);
    // 	parent->source->getBiomeBlock(biomes, x << ZONE_SIZE_BITS, z <<
    // ZONE_SIZE_BITS, ZONE_SIZE, ZONE_SIZE, false); 4jcraft added cast to
    // unsigned
    parent->source->getBiomeIndexBlock(
        biomeIndices, (unsigned)x << ZONE_SIZE_BITS,
        (unsigned)z << ZONE_SIZE_BITS, ZONE_SIZE, ZONE_SIZE, false);
}

BiomeCache::Block::~Block() {}

Biome* BiomeCache::Block::getBiome(int x, int z) {
    //	return biomes[(x & ZONE_SIZE_MASK) | ((z & ZONE_SIZE_MASK) <<
    // ZONE_SIZE_BITS)];

    int biomeIndex = biomeIndices[(x & ZONE_SIZE_MASK) |
                                  ((z & ZONE_SIZE_MASK) << ZONE_SIZE_BITS)];
    return Biome::biomes[biomeIndex];
}

float BiomeCache::Block::getTemperature(int x, int z) {
    //	return temps[(x & ZONE_SIZE_MASK) | ((z & ZONE_SIZE_MASK) <<
    // ZONE_SIZE_BITS)];

    int biomeIndex = biomeIndices[(x & ZONE_SIZE_MASK) |
                                  ((z & ZONE_SIZE_MASK) << ZONE_SIZE_BITS)];
    return Biome::biomes[biomeIndex]->getTemperature();
}

float BiomeCache::Block::getDownfall(int x, int z) {
    // 	return downfall[(x & ZONE_SIZE_MASK) | ((z & ZONE_SIZE_MASK) <<
    // ZONE_SIZE_BITS)];

    int biomeIndex = biomeIndices[(x & ZONE_SIZE_MASK) |
                                  ((z & ZONE_SIZE_MASK) << ZONE_SIZE_BITS)];
    return Biome::biomes[biomeIndex]->getDownfall();
}

BiomeCache::BiomeCache(BiomeSource* source) {
    // 4J Initialisors
    lastUpdateTime = 0;

    this->source = source;
}

BiomeCache::~BiomeCache() {
    // 4J Stu - Delete source?
    // delete source;

    for (auto it = all.begin(); it != all.end(); ++it) {
        delete (*it);
    }
}

BiomeCache::Block* BiomeCache::getBlockAt(int x, int z) {
    std::lock_guard<std::mutex> lock(m_CS);
    x >>= ZONE_SIZE_BITS;
    z >>= ZONE_SIZE_BITS;
    int64_t slot =
        (((int64_t)x) & 0xffffffffl) | ((((int64_t)z) & 0xffffffffl) << 32l);
    auto it = cached.find(slot);
    Block* block = nullptr;
    if (it == cached.end()) {
        block = new Block(x, z, this);
        cached[slot] = block;
        all.push_back(block);
    } else {
        block = it->second;
    }
    block->lastUse = gameServices().getAppTime();
    return block;
}

Biome* BiomeCache::getBiome(int x, int z) {
    return getBlockAt(x, z)->getBiome(x, z);
}

float BiomeCache::getTemperature(int x, int z) {
    return getBlockAt(x, z)->getTemperature(x, z);
}

float BiomeCache::getDownfall(int x, int z) {
    return getBlockAt(x, z)->getDownfall(x, z);
}

void BiomeCache::update() {
    std::lock_guard<std::mutex> lock(m_CS);
    int64_t now = gameServices().getAppTime();
    int64_t utime = now - lastUpdateTime;
    if (utime > DECAY_TIME / 4 || utime < 0) {
        lastUpdateTime = now;

        for (auto it = all.begin(); it != all.end();) {
            Block* block = *it;
            int64_t time = now - block->lastUse;
            if (time > DECAY_TIME || time < 0) {
                it = all.erase(it);
                int64_t slot = (((int64_t)block->x) & 0xffffffffl) |
                               ((((int64_t)block->z) & 0xffffffffl) << 32l);
                cached.erase(slot);
                delete block;
            } else {
                ++it;
            }
        }
    }
}

std::vector<Biome*> BiomeCache::getBiomeBlockAt(int x, int z) {
    std::vector<uint8_t> indices = getBlockAt(x, z)->biomeIndices;
    std::vector<Biome*> biomes(indices.size());
    for (int i = 0; i < indices.size(); i++)
        biomes[i] = Biome::biomes[indices[i]];
    return biomes;
}

std::vector<uint8_t> BiomeCache::getBiomeIndexBlockAt(int x, int z) {
    return getBlockAt(x, z)->biomeIndices;
}