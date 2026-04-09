#pragma once

#include <memory>
#include <string>
#include <vector>

#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "platform/PlatformTypes.h"

class PlayerIO;
class Dimension;
class ChunkStorage;
class LevelData;
class Player;
class File;
class ConsoleSaveFile;

class LevelStorage {
public:
    static const std::string NETHER_FOLDER;
    static const std::string ENDER_FOLDER;

    virtual ~LevelStorage() {}
    virtual LevelData* prepareLevel() = 0;
    virtual void checkSession() = 0;
    virtual ChunkStorage* createChunkStorage(Dimension* dimension) = 0;
    virtual void saveLevelData(
        LevelData* levelData,
        std::vector<std::shared_ptr<Player> >* players) = 0;
    virtual void saveLevelData(LevelData* levelData) = 0;
    virtual PlayerIO* getPlayerIO() = 0;
    virtual void closeAll() = 0;
    virtual ConsoleSavePath getDataFile(const std::string& id) = 0;
    virtual std::string getLevelId() = 0;

public:
    virtual ConsoleSaveFile* getSaveFile() { return nullptr; }
    virtual void flushSaveFile(bool autosave) {}

    // 4J Added
    virtual int getAuxValueForMap(PlayerUID xuid, int dimension, int centreXC,
                                  int centreZC, int scale) {
        return 0;
    }
};
