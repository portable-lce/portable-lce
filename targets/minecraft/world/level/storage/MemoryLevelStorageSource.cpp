#include "MemoryLevelStorageSource.h"

#include "LevelSummary.h"
#include "MemoryLevelStorage.h"

MemoryLevelStorageSource::MemoryLevelStorageSource() {}

std::string MemoryLevelStorageSource::getName() { return "Memory Storage"; }

std::shared_ptr<LevelStorage> MemoryLevelStorageSource::selectLevel(
    const std::string& levelId, bool createPlayerDir) {
        return std::shared_ptr<LevelStorage> () new MemoryLevelStorage());
}

std::vector<LevelSummary*>* MemoryLevelStorageSource::getLevelList() {
    return new std::vector<LevelSummary*>;
}

void MemoryLevelStorageSource::clearAll() {}

LevelData* MemoryLevelStorageSource::getDataTagFor(const std::string& levelId) {
    return nullptr;
}

bool MemoryLevelStorageSource::isNewLevelIdAcceptable(
    const std::string& levelId) {
    return true;
}

void MemoryLevelStorageSource::deleteLevel(const std::string& levelId) {}

void MemoryLevelStorageSource::renameLevel(const std::string& levelId,
                                           const std::string& newLevelName) {}

bool MemoryLevelStorageSource::isConvertible(const std::string& levelId) {
    return false;
}

bool MemoryLevelStorageSource::requiresConversion(const std::string& levelId) {
    return false;
}

bool MemoryLevelStorageSource::convertLevel(const std::string& levelId,
                                            ProgressListener* progress) {
    return false;
}