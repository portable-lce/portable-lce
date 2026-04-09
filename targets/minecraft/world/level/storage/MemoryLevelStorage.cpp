#include "MemoryLevelStorage.h"

#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileIO.h"
#include "nbt/NbtIo.h"

MemoryLevelStorage::MemoryLevelStorage() {}

LevelData* MemoryLevelStorage::prepareLevel() { return nullptr; }

void MemoryLevelStorage::checkSession() {}

ChunkStorage* MemoryLevelStorage::createChunkStorage(Dimension* dimension) {
    return new MemoryChunkStorage();
}

void MemoryLevelStorage::saveLevelData(
    LevelData* levelData, std::vector<std::shared_ptr<Player> >* players) {}

void MemoryLevelStorage::saveLevelData(LevelData* levelData) {}

PlayerIO* MemoryLevelStorage::getPlayerIO() { return this; }

void MemoryLevelStorage::closeAll() {}

void MemoryLevelStorage::save(std::shared_ptr<Player> player) {}

bool MemoryLevelStorage::load(std::shared_ptr<Player> player) { return false; }

CompoundTag* MemoryLevelStorage::loadPlayerDataTag(
    const std::string& playerName) {
    return nullptr;
}

ConsoleSavePath MemoryLevelStorage::getDataFile(const std::string& id) {
    return ConsoleSaveFile(std::string(""));
}