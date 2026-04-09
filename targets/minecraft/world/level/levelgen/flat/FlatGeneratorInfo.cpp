#include "FlatGeneratorInfo.h"

#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/levelgen/flat/FlatLayerInfo.h"
#include "minecraft/world/level/tile/Tile.h"
#include "util/StringHelpers.h"

const std::string FlatGeneratorInfo::STRUCTURE_VILLAGE = "village";
const std::string FlatGeneratorInfo::STRUCTURE_BIOME_SPECIFIC = "biome_1";
const std::string FlatGeneratorInfo::STRUCTURE_STRONGHOLD = "stronghold";
const std::string FlatGeneratorInfo::STRUCTURE_MINESHAFT = "mineshaft";
const std::string FlatGeneratorInfo::STRUCTURE_BIOME_DECORATION = "decoration";
const std::string FlatGeneratorInfo::STRUCTURE_LAKE = "lake";
const std::string FlatGeneratorInfo::STRUCTURE_LAVA_LAKE = "lava_lake";
const std::string FlatGeneratorInfo::STRUCTURE_DUNGEON = "dungeon";

FlatGeneratorInfo::FlatGeneratorInfo() { biome = 0; }

FlatGeneratorInfo::~FlatGeneratorInfo() {
    for (auto it = layers.begin(); it != layers.end(); ++it) {
        delete *it;
    }
}

int FlatGeneratorInfo::getBiome() { return biome; }

void FlatGeneratorInfo::setBiome(int biome) { this->biome = biome; }

std::unordered_map<std::string, std::unordered_map<std::string, std::string> >*
FlatGeneratorInfo::getStructures() {
    return &structures;
}

std::vector<FlatLayerInfo*>* FlatGeneratorInfo::getLayers() { return &layers; }

void FlatGeneratorInfo::updateLayers() {
    int y = 0;

    for (auto it = layers.begin(); it != layers.end(); ++it) {
        FlatLayerInfo* layer = *it;
        layer->setStart(y);
        y += layer->getHeight();
    }
}

std::string FlatGeneratorInfo::toString() { return ""; }

FlatLayerInfo* FlatGeneratorInfo::getLayerFromString(const std::string& input,
                                                     int yOffset) {
    return nullptr;
}

std::vector<FlatLayerInfo*>* FlatGeneratorInfo::getLayersFromString(
    const std::string& input) {
    if (input.empty()) return nullptr;

    std::vector<FlatLayerInfo*>* result = new std::vector<FlatLayerInfo*>();
    std::vector<std::string> depths = stringSplit(input, ',');

    int yOffset = 0;

    for (auto it = depths.begin(); it != depths.end(); ++it) {
        FlatLayerInfo* layer = getLayerFromString(*it, yOffset);
        if (layer == nullptr) return nullptr;
        result->push_back(layer);
        yOffset += layer->getHeight();
    }

    return result;
}

FlatGeneratorInfo* FlatGeneratorInfo::fromValue(const std::string& input) {
    return getDefault();
}

FlatGeneratorInfo* FlatGeneratorInfo::getDefault() {
    FlatGeneratorInfo* result = new FlatGeneratorInfo();

    result->setBiome(Biome::plains->id);
    result->getLayers()->push_back(new FlatLayerInfo(1, Tile::unbreakable_Id));
    result->getLayers()->push_back(new FlatLayerInfo(2, Tile::dirt_Id));
    result->getLayers()->push_back(new FlatLayerInfo(1, Tile::grass_Id));
    result->updateLayers();
    (*(result->getStructures()))[STRUCTURE_VILLAGE] =
        std::unordered_map<std::string, std::string>();

    return result;
}