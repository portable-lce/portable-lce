#include "minecraft/IGameServices.h"
#include "minecraft/world/level/newbiome/layer/Layer.h"

#include <stdint.h>

#include <memory>
#include <vector>

#include "BiomeOverrideLayer.h"
#include "platform/input/input.h"
#include "app/common/Console_Debug_enum.h"
#include "minecraft/world/level/LevelType.h"
#include "minecraft/world/level/newbiome/layer/AddIslandLayer.h"
#include "minecraft/world/level/newbiome/layer/AddMushroomIslandLayer.h"
#include "minecraft/world/level/newbiome/layer/AddSnowLayer.h"
#include "minecraft/world/level/newbiome/layer/BiomeInitLayer.h"
#include "minecraft/world/level/newbiome/layer/FuzzyZoomLayer.h"
#include "minecraft/world/level/newbiome/layer/GrowMushroomIslandLayer.h"
#include "minecraft/world/level/newbiome/layer/IslandLayer.h"
#include "minecraft/world/level/newbiome/layer/RegionHillsLayer.h"
#include "minecraft/world/level/newbiome/layer/RiverInitLayer.h"
#include "minecraft/world/level/newbiome/layer/RiverLayer.h"
#include "minecraft/world/level/newbiome/layer/RiverMixerLayer.h"
#include "minecraft/world/level/newbiome/layer/ShoreLayer.h"
#include "minecraft/world/level/newbiome/layer/SmoothLayer.h"
#include "minecraft/world/level/newbiome/layer/SwampRiversLayer.h"
#include "minecraft/world/level/newbiome/layer/VoronoiZoom.h"
#include "minecraft/world/level/newbiome/layer/ZoomLayer.h"

std::vector<std::shared_ptr<Layer>> Layer::getDefaultLayers(
    int64_t seed, LevelType* levelType) {
    // 4J - Some changes moved here from 1.2.3. Temperature & downfall layers
    // are no longer created & returned, and a debug layer is isn't. For
    // reference with regard to future merging, things NOT brought forward from
    // the 1.2.3 version are new layer types that we don't have yet (shores,
    // swamprivers, region hills etc.)
    std::shared_ptr<Layer> islandLayer = std::make_shared<IslandLayer>(1);
    islandLayer = std::make_shared<FuzzyZoomLayer>(2000, islandLayer);
    islandLayer = std::make_shared<AddIslandLayer>(1, islandLayer);
    islandLayer = std::make_shared<ZoomLayer>(2001, islandLayer);
    islandLayer = std::make_shared<AddIslandLayer>(2, islandLayer);
    islandLayer = std::make_shared<AddSnowLayer>(2, islandLayer);
    islandLayer = std::make_shared<ZoomLayer>(2002, islandLayer);
    islandLayer = std::make_shared<AddIslandLayer>(3, islandLayer);
    islandLayer = std::make_shared<ZoomLayer>(2003, islandLayer);
    islandLayer = std::make_shared<AddIslandLayer>(4, islandLayer);
    //	islandLayer = std::make_shared<AddMushroomIslandLayer>(5,
    // islandLayer);		// 4J - old position of mushroom island layer

    int zoomLevel = 4;
    if (levelType == LevelType::lvl_largeBiomes) {
        zoomLevel = 6;
    }

    std::shared_ptr<Layer> riverLayer = islandLayer;
    riverLayer = ZoomLayer::zoom(1000, riverLayer, 0);
    riverLayer = std::make_shared<RiverInitLayer>(100, riverLayer);
    riverLayer = ZoomLayer::zoom(1000, riverLayer, zoomLevel + 2);
    riverLayer = std::make_shared<RiverLayer>(1, riverLayer);
    riverLayer = std::make_shared<SmoothLayer>(1000, riverLayer);

    std::shared_ptr<Layer> biomeLayer = islandLayer;
    biomeLayer = ZoomLayer::zoom(1000, biomeLayer, 0);
    biomeLayer = std::make_shared<BiomeInitLayer>(200, biomeLayer, levelType);

    biomeLayer = ZoomLayer::zoom(1000, biomeLayer, 2);
    biomeLayer = std::make_shared<RegionHillsLayer>(1000, biomeLayer);

    for (int i = 0; i < zoomLevel; i++) {
        biomeLayer = std::make_shared<ZoomLayer>(1000 + i, biomeLayer);

        if (i == 0)
            biomeLayer = std::make_shared<AddIslandLayer>(3, biomeLayer);

        if (i == 0) {
            // 4J - moved mushroom islands to here. This skips 3 zooms that the
            // old location of the add was, making them about 1/8 of the
            // original size. Adding them at this scale actually lets us place
            // them near enough other land, if we add them at the same scale as
            // java then they have to be too far out to see for the scale of our
            // maps
            biomeLayer = std::shared_ptr<Layer>(
                new AddMushroomIslandLayer(5, biomeLayer));
        }

        if (i == 1) {
            // 4J - now expand mushroom islands up again. This does a simple
            // region grow to add a new mushroom island element when any of the
            // neighbours are also mushroom islands. This helps make the islands
            // into nice compact shapes of the type that are actually likely to
            // be able to make an island out of the sea in a small space. Also
            // helps the shore layer from doing too much damage in shrinking the
            // islands we are making
            biomeLayer = std::shared_ptr<Layer>(
                new GrowMushroomIslandLayer(5, biomeLayer));
            // Note - this reduces the size of mushroom islands by turning their
            // edges into shores. We are doing this at i == 1 rather than i == 0
            // as the original does
            biomeLayer = std::make_shared<ShoreLayer>(1000, biomeLayer);

            biomeLayer = std::make_shared<SwampRiversLayer>(1000, biomeLayer);
        }
    }

    biomeLayer = std::make_shared<SmoothLayer>(1000, biomeLayer);

    biomeLayer = std::shared_ptr<Layer>(
        new RiverMixerLayer(100, biomeLayer, riverLayer));

#if !defined(_CONTENT_PACKAGE)
#if defined(_BIOME_OVERRIDE)
    if (gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_EnableBiomeOverride)) {
        biomeLayer = std::make_shared<BiomeOverrideLayer>(1);
    }
#endif
#endif

    std::shared_ptr<Layer> debugLayer = biomeLayer;

    std::shared_ptr<Layer> zoomedLayer =
        std::make_shared<VoronoiZoom>(10, biomeLayer);

    biomeLayer->init(seed);
    zoomedLayer->init(seed);

    std::vector<std::shared_ptr<Layer>> result(3);
    result[0] = biomeLayer;
    result[1] = zoomedLayer;
    result[2] = debugLayer;
    return result;
}

Layer::Layer(int64_t seedMixup) {
    parent = nullptr;

    // 4jcraft added casts to prevent signed int overflow
    this->seedMixup = seedMixup;
    this->seedMixup *=
        (uint64_t)this->seedMixup * 6364136223846793005l + 1442695040888963407l;
    this->seedMixup = (uint64_t)this->seedMixup + seedMixup;
    this->seedMixup *=
        (uint64_t)this->seedMixup * 6364136223846793005l + 1442695040888963407l;
    this->seedMixup = (uint64_t)this->seedMixup + seedMixup;
    this->seedMixup *=
        (uint64_t)this->seedMixup * 6364136223846793005l + 1442695040888963407l;
    this->seedMixup = (uint64_t)this->seedMixup + seedMixup;
}

void Layer::init(int64_t seed) {
    this->seed = seed;
    if (parent != nullptr) parent->init(seed);
    // 4jcraft added casts to prevent signed int overflow
    this->seed *=
        (uint64_t)this->seed * 6364136223846793005l + 1442695040888963407l;
    this->seed = (uint64_t)this->seed + seedMixup;
    this->seed *=
        (uint64_t)this->seed * 6364136223846793005l + 1442695040888963407l;
    this->seed = (uint64_t)this->seed + seedMixup;
    this->seed *=
        (uint64_t)this->seed * 6364136223846793005l + 1442695040888963407l;
    this->seed = (uint64_t)this->seed + seedMixup;
}

void Layer::initRandom(int64_t x, int64_t y) {
    rval = seed;
    // 4jcraft added casts to prevent signed int overflow
    rval *= (uint64_t)rval * 6364136223846793005l + 1442695040888963407l;
    rval += (uint64_t)x;
    rval *= (uint64_t)rval * 6364136223846793005l + 1442695040888963407l;
    rval += (uint64_t)y;
    rval *= (uint64_t)rval * 6364136223846793005l + 1442695040888963407l;
    rval += (uint64_t)x;
    rval *= (uint64_t)rval * 6364136223846793005l + 1442695040888963407l;
    rval += (uint64_t)y;
}

int Layer::nextRandom(int max) {
    int result = (int)((rval >> 24) % max);

    if (result < 0) result += max;
    // 4jcraft added cast to unsigned
    rval *= (uint64_t)rval * 6364136223846793005l + 1442695040888963407l;
    rval += (uint64_t)seed;
    return result;
}
