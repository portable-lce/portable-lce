#include "DirectoryLevelStorageSource.h"

#include <memory>
#include <vector>

#include "DirectoryLevelStorage.h"
#include "LevelData.h"
#include "java/File.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileInputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOriginal.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOutputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"

DirectoryLevelStorageSource::DirectoryLevelStorageSource(const File dir)
    : baseDir(dir) {
    // if (!dir.exists()) dir.mkdirs(); // 4J Removed
    // this->baseDir = dir;
}

std::string DirectoryLevelStorageSource::getName() { return "Old Format"; }

std::vector<LevelSummary*>* DirectoryLevelStorageSource::getLevelList() {
    // 4J Stu - We don't use directory list with the Xbox save locations
    std::vector<LevelSummary*>* levels = new std::vector<LevelSummary*>;
    return levels;
}

void DirectoryLevelStorageSource::clearAll() {}

LevelData* DirectoryLevelStorageSource::getDataTagFor(
    ConsoleSaveFile* saveFile, const std::string& levelId) {
    // File dataFile(dir, "level.dat");
    ConsoleSavePath dataFile = ConsoleSavePath(std::string("level.dat"));
    if (saveFile->doesFileExist(dataFile)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(saveFile, dataFile);
        CompoundTag* root = NbtIo::readCompressed(&fis);
        CompoundTag* tag = root->getCompound("Data");
        LevelData* ret = new LevelData(tag);
        delete root;
        return ret;
    }

    return nullptr;
}

void DirectoryLevelStorageSource::renameLevel(const std::string& levelId,
                                              const std::string& newLevelName) {
    ConsoleSaveFileOriginal tempSave(levelId);

    // File dataFile = File(dir, "level.dat");
    ConsoleSavePath dataFile = ConsoleSavePath(std::string("level.dat"));
    if (tempSave.doesFileExist(dataFile)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(&tempSave, dataFile);
        CompoundTag* root = NbtIo::readCompressed(&fis);
        CompoundTag* tag = root->getCompound("Data");
        tag->putString("LevelName", newLevelName);

        ConsoleSaveFileOutputStream fos =
            ConsoleSaveFileOutputStream(&tempSave, dataFile);
        NbtIo::writeCompressed(root, &fos);
    }
}

bool DirectoryLevelStorageSource::isNewLevelIdAcceptable(
    const std::string& levelId) {
    // 4J Jev, removed try/catch.

    File levelFolder = File(baseDir, levelId);
    if (levelFolder.exists()) {
        return false;
    }

    levelFolder.mkdir();

    return true;
}

void DirectoryLevelStorageSource::deleteLevel(const std::string& levelId) {
    File dir = File(baseDir, levelId);
    if (!dir.exists()) return;

    deleteRecursive(dir.listFiles());
    dir._delete();
}

void DirectoryLevelStorageSource::deleteRecursive(std::vector<File*>* files) {
    auto itEnd = files->end();
    for (auto it = files->begin(); it != itEnd; it++) {
        File* file = *it;
        if (file->isDirectory()) {
            deleteRecursive(file->listFiles());
        }
        file->_delete();
    }
}

std::shared_ptr<LevelStorage> DirectoryLevelStorageSource::selectLevel(
    ConsoleSaveFile* saveFile, const std::string& levelId,
    bool createPlayerDir) {
    return std::shared_ptr<LevelStorage>(
        new DirectoryLevelStorage(saveFile, baseDir, levelId, createPlayerDir));
}

bool DirectoryLevelStorageSource::isConvertible(ConsoleSaveFile* saveFile,
                                                const std::string& levelId) {
    return false;
}

bool DirectoryLevelStorageSource::requiresConversion(
    ConsoleSaveFile* saveFile, const std::string& levelId) {
    return false;
}

bool DirectoryLevelStorageSource::convertLevel(ConsoleSaveFile* saveFile,
                                               const std::string& levelId,
                                               ProgressListener* progress) {
    return false;
}
