#include "SavedDataStorage.h"

#include <assert.h>

#include <algorithm>
#include <utility>

#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/entity/ai/village/Villages.h"
#include "minecraft/world/level/levelgen/structure/StructureFeatureSavedData.h"
#include "minecraft/world/level/saveddata/MapItemSavedData.h"
#include "minecraft/world/level/saveddata/SavedData.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileInputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOutputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/DirectoryLevelStorage.h"
#include "minecraft/world/level/storage/LevelStorage.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"
#include "nbt/ShortTag.h"
#include "nbt/Tag.h"

SavedDataStorage::SavedDataStorage(LevelStorage* levelStorage) {
    /*
    cache = new unordered_map<string, shared_ptr<SavedData> >;
    savedDatas = new vector<shared_ptr<SavedData> >;
    usedAuxIds = new unordered_map<string, short*>;
    */

    this->levelStorage = levelStorage;
    loadAuxValues();
}

std::shared_ptr<SavedData> SavedDataStorage::get(const std::type_info& clazz,
                                                 const std::string& id) {
    auto it = cache.find(id);
    if (it != cache.end()) return (*it).second;

    std::shared_ptr<SavedData> data = nullptr;
    if (levelStorage != nullptr) {
        // File file = levelStorage->getDataFile(id);
        ConsoleSavePath file = levelStorage->getDataFile(id);
        if (!file.getName().empty() &&
            levelStorage->getSaveFile()->doesFileExist(file)) {
            // mob = std::dynamic_pointer_cast<Mob>(Mob::_class->newInstance(
            // level
            // ));
            // data = clazz.getConstructor(String.class).newInstance(id);

            if (clazz == typeid(MapItemSavedData)) {
                data = std::dynamic_pointer_cast<SavedData>(
                    std::shared_ptr<MapItemSavedData>(
                        new MapItemSavedData(id)));
            } else if (clazz == typeid(Villages)) {
                data = std::dynamic_pointer_cast<SavedData>(
                    std::make_shared<Villages>(id));
            } else if (clazz == typeid(StructureFeatureSavedData)) {
                data = std::dynamic_pointer_cast<SavedData>(
                    std::shared_ptr<StructureFeatureSavedData>(
                        new StructureFeatureSavedData(id)));
            } else {
                // Handling of new SavedData class required
                assert(0);
            }

            ConsoleSaveFileInputStream fis =
                ConsoleSaveFileInputStream(levelStorage->getSaveFile(), file);
            CompoundTag* root = NbtIo::readCompressed(&fis);
            fis.close();

            data->load(root->getCompound("data"));
        }
    }

    if (data != nullptr) {
        cache.insert(
            std::unordered_map<std::string,
                               std::shared_ptr<SavedData> >::value_type(id,
                                                                        data));
        savedDatas.push_back(data);
    }
    return data;
}

void SavedDataStorage::set(const std::string& id,
                           std::shared_ptr<SavedData> data) {
    if (data == nullptr) {
        // TODO 4J Stu - throw new RuntimeException("Can't set null data");
        assert(false);
    }
    auto it = cache.find(id);
    if (it != cache.end()) {
        auto it2 = find(savedDatas.begin(), savedDatas.end(), it->second);
        if (it2 != savedDatas.end()) {
            savedDatas.erase(it2);
        }
        cache.erase(it);
    }
    cache.insert(cacheMapType::value_type(id, data));
    savedDatas.push_back(data);
}

void SavedDataStorage::save() {
    auto itEnd = savedDatas.end();
    for (auto it = savedDatas.begin(); it != itEnd; it++) {
        std::shared_ptr<SavedData> data = *it;  // savedDatas->at(i);
        if (data->isDirty()) {
            save(data);
            data->setDirty(false);
        }
    }
}

void SavedDataStorage::save(std::shared_ptr<SavedData> data) {
    if (levelStorage == nullptr) return;
    // File file = levelStorage->getDataFile(data->id);
    ConsoleSavePath file = levelStorage->getDataFile(data->id);
    if (!file.getName().empty()) {
        CompoundTag* dataTag = new CompoundTag();
        data->save(dataTag);

        CompoundTag* tag = new CompoundTag();
        tag->putCompound("data", dataTag);

        ConsoleSaveFileOutputStream fos =
            ConsoleSaveFileOutputStream(levelStorage->getSaveFile(), file);
        NbtIo::writeCompressed(tag, &fos);
        fos.close();

        delete tag;
    }
}

void SavedDataStorage::loadAuxValues() {
    usedAuxIds.clear();

    if (levelStorage == nullptr) return;
    // File file = levelStorage->getDataFile("idcounts");
    ConsoleSavePath file = levelStorage->getDataFile("idcounts");
    if (!file.getName().empty() &&
        levelStorage->getSaveFile()->doesFileExist(file)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(levelStorage->getSaveFile(), file);
        DataInputStream dis = DataInputStream(&fis);
        CompoundTag* tags = NbtIo::read(&dis);
        dis.close();

        Tag* tag;
        std::vector<Tag*> allTags = tags->getAllTags();
        auto itEnd = allTags.end();
        for (auto it = allTags.begin(); it != itEnd; it++) {
            tag = *it;

            if (dynamic_cast<ShortTag*>(tag) != nullptr) {
                ShortTag* sTag = (ShortTag*)tag;
                std::string id = sTag->getName();
                short val = sTag->data;
                usedAuxIds.insert(uaiMapType::value_type(id, val));
            }
        }
    }
}

int SavedDataStorage::getFreeAuxValueFor(const std::string& id) {
    auto it = usedAuxIds.find(id);
    short val = 0;
    if (it != usedAuxIds.end()) {
        val = (*it).second;
        val++;
    }

    usedAuxIds[id] = val;
    if (levelStorage == nullptr) return val;
    // File file = levelStorage->getDataFile("idcounts");
    ConsoleSavePath file = levelStorage->getDataFile("idcounts");
    if (!file.getName().empty()) {
        CompoundTag* tag = new CompoundTag();

        // TODO 4J Stu - This was iterating over the keySet in Java, so
        // potentially we are looking at more items?
        auto itEndAuxIds = usedAuxIds.end();
        for (uaiMapType::iterator it2 = usedAuxIds.begin(); it2 != itEndAuxIds;
             it2++) {
            short value = it2->second;
            tag->putShort((char*)it2->first.c_str(), value);
        }

        ConsoleSaveFileOutputStream fos =
            ConsoleSaveFileOutputStream(levelStorage->getSaveFile(), file);
        DataOutputStream dos = DataOutputStream(&fos);
        NbtIo::write(tag, &dos);
        dos.close();
    }
    return val;
}

// 4J Added
int SavedDataStorage::getAuxValueForMap(PlayerUID xuid, int dimension,
                                        int centreXC, int centreZC, int scale) {
    if (levelStorage == nullptr) {
        switch (dimension) {
            case -1:
                return MAP_NETHER_DEFAULT_INDEX;
            case 1:
                return MAP_END_DEFAULT_INDEX;
            case 0:
            default:
                return MAP_OVERWORLD_DEFAULT_INDEX;
        }
    } else {
        return levelStorage->getAuxValueForMap(xuid, dimension, centreXC,
                                               centreZC, scale);
    }
}
