#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "minecraft/world/level/saveddata/SavedData.h"
#include "platform/PlatformTypes.h"

class ConsoleSaveFile;
class LevelStorage;

class SavedDataStorage {
private:
    LevelStorage* levelStorage;

    typedef std::unordered_map<std::string, std::shared_ptr<SavedData> >
        cacheMapType;
    cacheMapType cache;

    std::vector<std::shared_ptr<SavedData> > savedDatas;

    typedef std::unordered_map<std::string, short> uaiMapType;
    uaiMapType usedAuxIds;

public:
    SavedDataStorage(LevelStorage*);
    std::shared_ptr<SavedData> get(const std::type_info& clazz,
                                   const std::string& id);
    void set(const std::string& id, std::shared_ptr<SavedData> data);
    void save();

private:
    void save(std::shared_ptr<SavedData> data);
    void loadAuxValues();

public:
    int getFreeAuxValueFor(const std::string& id);

    // 4J Added
    int getAuxValueForMap(PlayerUID xuid, int dimension, int centreXC,
                          int centreZC, int scale);
};
