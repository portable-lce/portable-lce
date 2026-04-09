#pragma once
#include <stdint.h>

#include <format>
#include <memory>
#include <vector>

#include "Biome.h"
#include "BiomeCache.h"
#include "BiomeSource.h"
#include "minecraft/world/level/biome/BiomeSource.h"

class ChunkPos;
class Level;
class Layer;
class TilePos;
class LevelType;
class Random;

class BiomeSource {
private:
    std::shared_ptr<Layer> layer;
    std::shared_ptr<Layer> zoomedLayer;

public:
    static const int CACHE_DIAMETER = 256;

private:
    BiomeCache* cache;

    std::vector<Biome*> playerSpawnBiomes;

protected:
    void _init();
    void _init(int64_t seed, LevelType* generator);
    BiomeSource();

public:
    BiomeSource(int64_t seed, LevelType* generator);
    BiomeSource(Level* level);

private:
    static bool getIsMatch(float* frac);                            // 4J added
    static void getFracs(std::vector<int>& indices, float* fracs);  // 4J added
public:
    static int64_t findSeed(LevelType* generator);  // 4J added
    virtual ~BiomeSource();

public:
    std::vector<Biome*> getPlayerSpawnBiomes() { return playerSpawnBiomes; }
    virtual Biome* getBiome(ChunkPos* cp);
    virtual Biome* getBiome(int x, int z);

    // 4J - changed the interface for these methods, mainly for thread safety
    virtual float getDownfall(int x, int z) const;
    virtual std::vector<float> getDownfallBlock(int x, int z, int w,
                                                int h) const;
    virtual void getDownfallBlock(std::vector<float>& downfalls, int x, int z,
                                  int w, int h) const;

    // 4J - changed the interface for these methods, mainly for thread safety
    virtual BiomeCache::Block* getBlockAt(int x, int y);
    virtual float getTemperature(int x, int y, int z) const;
    float scaleTemp(float temp,
                    int y) const;  // 4J - brought forward from 1.2.3
    virtual std::vector<float> getTemperatureBlock(int x, int z, int w,
                                                   int h) const;
    virtual void getTemperatureBlock(std::vector<float>& temperatures, int x,
                                     int z, int w, int h) const;

    virtual std::vector<Biome*> getRawBiomeBlock(int x, int z, int w,
                                                 int h) const;
    virtual void getRawBiomeBlock(std::vector<Biome*>& biomes, int x, int z,
                                  int w, int h) const;
    virtual void getRawBiomeIndices(std::vector<int>& biomes, int x, int z,
                                    int w,
                                    int h) const;  // 4J added
    virtual std::vector<Biome*> getBiomeBlock(int x, int z, int w, int h) const;
    virtual void getBiomeBlock(std::vector<Biome*>& biomes, int x, int z, int w,
                               int h, bool useCache) const;

    virtual std::vector<uint8_t> getBiomeIndexBlock(int x, int z, int w,
                                                    int h) const;
    virtual void getBiomeIndexBlock(std::vector<uint8_t>& biomeIndices, int x,
                                    int z, int w, int h, bool useCache) const;

    /**
     * Checks if an area around a block contains only the specified biomes.
     * Useful for placing elements like towns.
     *
     * This is a bit of a rough check, to make it as fast as possible. To ensure
     * NO other biomes, add a margin of at least four blocks to the radius
     */
    virtual bool containsOnly(int x, int z, int r,
                              const std::vector<Biome*>& allowed);

    /**
     * Checks if an area around a block contains only the specified biome.
     * Useful for placing elements like towns.
     *
     * This is a bit of a rough check, to make it as fast as possible. To ensure
     * NO other biomes, add a margin of at least four blocks to the radius
     */
    virtual bool containsOnly(int x, int z, int r, Biome* allowed);

    /**
     * Finds the specified biome within the radius. This will return a random
     * position if several are found. This test is fairly rough.
     *
     * Returns null if the biome wasn't found
     */
    virtual TilePos* findBiome(int x, int z, int r, Biome* toFind,
                               Random* random);

    /**
     * Finds one of the specified biomes within the radius. This will return a
     * random position if several are found. This test is fairly rough.
     *
     * Returns null if the biome wasn't found
     */
    virtual TilePos* findBiome(int x, int z, int r,
                               const std::vector<Biome*>& allowed,
                               Random* random);

    void update();
};
