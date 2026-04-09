#include "StructureFeatureSavedData.h"

#include <string>

#include "minecraft/world/level/saveddata/SavedData.h"
#include "nbt/CompoundTag.h"
#include "util/StringHelpers.h"

std::string StructureFeatureSavedData::TAG_FEATURES = "Features";

StructureFeatureSavedData::StructureFeatureSavedData(const std::string& idName)
    : SavedData(idName) {
    this->pieceTags = new CompoundTag(TAG_FEATURES);
}

StructureFeatureSavedData::~StructureFeatureSavedData() { delete pieceTags; }

void StructureFeatureSavedData::load(CompoundTag* tag) {
    this->pieceTags = tag->getCompound(TAG_FEATURES);
}

void StructureFeatureSavedData::save(CompoundTag* tag) {
    tag->put(TAG_FEATURES, pieceTags->copy());
}

CompoundTag* StructureFeatureSavedData::getFeatureTag(int chunkX, int chunkZ) {
    return pieceTags->getCompound(createFeatureTagId(chunkX, chunkZ));
}

void StructureFeatureSavedData::putFeatureTag(CompoundTag* tag, int chunkX,
                                              int chunkZ) {
    std::string name = createFeatureTagId(chunkX, chunkZ);
    tag->setName(name);
    pieceTags->put(name, tag);
}

std::string StructureFeatureSavedData::createFeatureTagId(int chunkX,
                                                          int chunkZ) {
    return "[" + toWString<int>(chunkX) + "," + toWString<int>(chunkZ) + "]";
}

CompoundTag* StructureFeatureSavedData::getFullTag() { return pieceTags; }