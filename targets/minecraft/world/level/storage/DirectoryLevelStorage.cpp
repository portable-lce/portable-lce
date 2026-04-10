#include "DirectoryLevelStorage.h"

#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <format>
#include <memory>
#include <utility>

#include "LevelData.h"
#include "java/File.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/InputOutputStream/FileOutputStream.h"
#include "java/System.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/chunk/storage/OldChunkStorage.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/dimension/HellDimension.h"
#include "minecraft/world/level/dimension/TheEndDimension.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileInputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOutputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/LevelStorage.h"
#include "minecraft/world/level/storage/PlayerIO.h"
#include "nbt/CompoundTag.h"
#include "nbt/DoubleTag.h"
#include "nbt/ListTag.h"
#include "nbt/NbtIo.h"
#include "platform/input/input.h"
#include "platform/storage/storage.h"
#include "util/StringHelpers.h"

const std::string DirectoryLevelStorage::sc_szPlayerDir("players/");

_MapDataMappings::_MapDataMappings() {
    memset(xuids, 0, sizeof(PlayerUID) * MAXIMUM_MAP_SAVE_DATA);
    memset(dimensions, 0, sizeof(uint8_t) * (MAXIMUM_MAP_SAVE_DATA / 4));
}

int _MapDataMappings::getDimension(int id) {
    int offset = (2 * (id % 4));
    int val = (dimensions[id >> 2] & (3 << offset)) >> offset;

    int returnVal = 0;

    switch (val) {
        case 0:
            returnVal = 0;  // Overworld
            break;
        case 1:
            returnVal = -1;  // Nether
            break;
        case 2:
            returnVal = 1;  // End
            break;
        default:
#if !defined(_CONTENT_PACKAGE)
            printf("Read invalid dimension from MapDataMapping\n");
            assert(0);
#endif
            break;
    }
    return returnVal;
}

void _MapDataMappings::setMapping(int id, PlayerUID xuid, int dimension) {
    xuids[id] = xuid;

    int offset = (2 * (id % 4));

    // Reset it first
    dimensions[id >> 2] &= ~(2 << offset);
    switch (dimension) {
        case 0:  // Overworld
            // dimensions[id>>2] &= ~( 2 << offset );
            break;
        case -1:  // Nether
            dimensions[id >> 2] |= (1 << offset);
            break;
        case 1:  // End
            dimensions[id >> 2] |= (2 << offset);
            break;
        default:
#if !defined(_CONTENT_PACKAGE)
            printf(
                "Trinyg to set a MapDataMapping for an invalid dimension.\n");
            assert(0);
#endif
            break;
    }
}

// Old version the only used 1 bit for dimension indexing
_MapDataMappings_old::_MapDataMappings_old() {
    memset(xuids, 0, sizeof(PlayerUID) * MAXIMUM_MAP_SAVE_DATA);
    memset(dimensions, 0, sizeof(uint8_t) * (MAXIMUM_MAP_SAVE_DATA / 8));
}

int _MapDataMappings_old::getDimension(int id) {
    return dimensions[id >> 3] & (128 >> (id % 8)) ? -1 : 0;
}

void _MapDataMappings_old::setMapping(int id, PlayerUID xuid, int dimension) {
    xuids[id] = xuid;
    if (dimension == 0) {
        dimensions[id >> 3] &= ~(128 >> (id % 8));
    } else {
        dimensions[id >> 3] |= (128 >> (id % 8));
    }
}

#if defined(_LARGE_WORLDS)
void DirectoryLevelStorage::PlayerMappings::addMapping(int id, int centreX,
                                                       int centreZ,
                                                       int dimension,
                                                       int scale) {
    int64_t index = (((int64_t)(centreZ & 0x1FFFFFFF)) << 34) |
                    (((int64_t)(centreX & 0x1FFFFFFF)) << 5) |
                    ((scale & 0x7) << 2) | (dimension & 0x3);
    m_mappings[index] = id;
    // Log::info("Adding mapping: %d - (%d,%d)/%d/%d [%I64d -
    // 0x%016llx]\n", id, centreX, centreZ, dimension, scale, index, index);
}

bool DirectoryLevelStorage::PlayerMappings::getMapping(int& id, int centreX,
                                                       int centreZ,
                                                       int dimension,
                                                       int scale) {
    // int64_t zMasked = centreZ & 0x1FFFFFFF;
    // int64_t xMasked = centreX & 0x1FFFFFFF;
    // int64_t zShifted = zMasked << 34;
    // int64_t xShifted = xMasked << 5;
    //  Log::info("xShifted = %d (0x%016x), zShifted = %I64d
    //  (0x%016llx)\n", xShifted, xShifted, zShifted, zShifted);
    int64_t index = (((int64_t)(centreZ & 0x1FFFFFFF)) << 34) |
                    (((int64_t)(centreX & 0x1FFFFFFF)) << 5) |
                    ((scale & 0x7) << 2) | (dimension & 0x3);
    auto it = m_mappings.find(index);
    if (it != m_mappings.end()) {
        id = it->second;
        // Log::info("Found mapping: %d - (%d,%d)/%d/%d [%I64d -
        // 0x%016llx]\n", id, centreX, centreZ, dimension, scale, index, index);
        return true;
    } else {
        // Log::info("Failed to find mapping: (%d,%d)/%d/%d [%I64d -
        // 0x%016llx]\n", centreX, centreZ, dimension, scale, index, index);
        return false;
    }
}

void DirectoryLevelStorage::PlayerMappings::writeMappings(
    DataOutputStream* dos) {
    dos->writeInt(m_mappings.size());
    for (auto it = m_mappings.begin(); it != m_mappings.end(); ++it) {
        Log::info("    -- %lld (0x%016llx) = %d\n",
                  static_cast<long long>(it->first),
                  static_cast<unsigned long long>(it->first), it->second);
        dos->writeLong(it->first);
        dos->writeInt(it->second);
    }
}

void DirectoryLevelStorage::PlayerMappings::readMappings(DataInputStream* dis) {
    int count = dis->readInt();
    for (unsigned int i = 0; i < count; ++i) {
        int64_t index = dis->readLong();
        int id = dis->readInt();
        m_mappings[index] = id;
        Log::info("    -- %lld (0x%016llx) = %d\n",
                  static_cast<long long>(index),
                  static_cast<unsigned long long>(index), id);
    }
}
#endif

DirectoryLevelStorage::DirectoryLevelStorage(ConsoleSaveFile* saveFile,
                                             const File dir,
                                             const std::string& levelId,
                                             bool createPlayerDir)
    : sessionId(System::currentTimeMillis()),
      dir(""),
      playerDir(sc_szPlayerDir),
      dataDir(std::string("data/")),
      levelId(levelId) {
    m_saveFile = saveFile;
    m_bHasLoadedMapDataMappings = false;

#if defined(_LARGE_WORLDS)
    m_usedMappings = std::vector<uint8_t>(MAXIMUM_MAP_SAVE_DATA / 8);
#endif
}

DirectoryLevelStorage::~DirectoryLevelStorage() {
    delete m_saveFile;

    for (auto it = m_cachedSaveData.begin(); it != m_cachedSaveData.end();
         ++it) {
        delete it->second;
    }
}

void DirectoryLevelStorage::initiateSession() {
    // 4J Jev, removed try/catch.

    File dataFile = File(dir, std::string("session.lock"));
    FileOutputStream fos = FileOutputStream(dataFile);
    DataOutputStream dos = DataOutputStream(&fos);
    dos.writeLong(sessionId);
    dos.close();
}

File DirectoryLevelStorage::getFolder() { return dir; }

void DirectoryLevelStorage::checkSession() {
    // 4J-PB - Not in the Xbox game

    /*
    File dataFile = File( dir, string("session.lock"));
    FileInputStream fis = FileInputStream(dataFile);
    DataInputStream dis = DataInputStream(&fis);
    dis.close();
    */
}

ChunkStorage* DirectoryLevelStorage::createChunkStorage(Dimension* dimension) {
    // 4J Jev, removed try/catch.

    if (dynamic_cast<HellDimension*>(dimension) != nullptr) {
        File dir2 = File(dir, LevelStorage::NETHER_FOLDER);
        // dir2.mkdirs(); // 4J Removed
        return new OldChunkStorage(dir2, true);
    }
    if (dynamic_cast<TheEndDimension*>(dimension) != nullptr) {
        File dir2 = File(dir, LevelStorage::ENDER_FOLDER);
        // dir2.mkdirs(); // 4J Removed
        return new OldChunkStorage(dir2, true);
    }

    return new OldChunkStorage(dir, true);
}

LevelData* DirectoryLevelStorage::prepareLevel() {
    // 4J Stu Added
#if defined(_LARGE_WORLDS)
    ConsoleSavePath mapFile = getDataFile("largeMapDataMappings");
#else
    ConsoleSavePath mapFile = getDataFile("mapDataMappings");
#endif
    if (!m_bHasLoadedMapDataMappings && !mapFile.getName().empty() &&
        getSaveFile()->doesFileExist(mapFile)) {
        unsigned int NumberOfBytesRead;
        FileEntry* fileEntry = getSaveFile()->createFile(mapFile);

        {
            getSaveFile()->setFilePointer(fileEntry, 0,
                                          SaveFileSeekOrigin::Begin);

#if defined(_LARGE_WORLDS)
            std::vector<uint8_t> data(fileEntry->getFileSize());
            getSaveFile()->readFile(fileEntry, data.data(),
                                    fileEntry->getFileSize(),
                                    &NumberOfBytesRead);
            assert(NumberOfBytesRead == fileEntry->getFileSize());

            ByteArrayInputStream bais(data);
            DataInputStream dis(&bais);
            int count = dis.readInt();
            Log::info("Loading %d mappings\n", count);
            for (unsigned int i = 0; i < count; ++i) {
                PlayerUID playerUid = dis.readPlayerUID();
                Log::info("  -- %llu\n",
                          static_cast<unsigned long long>(playerUid));
                m_playerMappings[playerUid].readMappings(&dis);
            }
            dis.readFully(m_usedMappings);
#else

            if (getSaveFile()->getSaveVersion() <
                END_DIMENSION_MAP_MAPPINGS_SAVE_VERSION) {
                MapDataMappings_old oldMapDataMappings;
                getSaveFile()->readFile(
                    fileEntry,
                    &oldMapDataMappings,          // data buffer
                    sizeof(MapDataMappings_old),  // number of bytes to read
                    &NumberOfBytesRead            // number of bytes read
                );
                assert(NumberOfBytesRead == sizeof(MapDataMappings_old));

                for (unsigned int i = 0; i < MAXIMUM_MAP_SAVE_DATA; ++i) {
                    m_saveableMapDataMappings.setMapping(
                        i, oldMapDataMappings.xuids[i],
                        oldMapDataMappings.getDimension(i));
                }
            } else {
                getSaveFile()->readFile(
                    fileEntry,
                    &m_saveableMapDataMappings,  // data buffer
                    sizeof(MapDataMappings),     // number of bytes to read
                    &NumberOfBytesRead           // number of bytes read
                );
                assert(NumberOfBytesRead == sizeof(MapDataMappings));
            }

            memcpy(&m_mapDataMappings, &m_saveableMapDataMappings,
                   sizeof(MapDataMappings));
#endif

            // Write out our changes now
            if (getSaveFile()->getSaveVersion() <
                END_DIMENSION_MAP_MAPPINGS_SAVE_VERSION)
                saveMapIdLookup();
        }

        m_bHasLoadedMapDataMappings = true;
    }

    // 4J Jev, removed try/catch

    ConsoleSavePath dataFile = ConsoleSavePath(std::string("level.dat"));

    if (m_saveFile->doesFileExist(dataFile)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(m_saveFile, dataFile);
        CompoundTag* root = NbtIo::readCompressed(&fis);
        CompoundTag* tag = root->getCompound("Data");
        LevelData* ret = new LevelData(tag);
        delete root;
        return ret;
    }

    return nullptr;
}

void DirectoryLevelStorage::saveLevelData(
    LevelData* levelData, std::vector<std::shared_ptr<Player> >* players) {
    // 4J Jev, removed try/catch

    CompoundTag* dataTag = levelData->createTag(players);

    CompoundTag* root = new CompoundTag();
    root->put("Data", dataTag);

    ConsoleSavePath currentFile = ConsoleSavePath(std::string("level.dat"));

    ConsoleSaveFileOutputStream fos =
        ConsoleSaveFileOutputStream(m_saveFile, currentFile);
    NbtIo::writeCompressed(root, &fos);

    delete root;
}

void DirectoryLevelStorage::saveLevelData(LevelData* levelData) {
    // 4J Jev, removed try/catch

    CompoundTag* dataTag = levelData->createTag();

    CompoundTag* root = new CompoundTag();
    root->put("Data", dataTag);

    ConsoleSavePath currentFile = ConsoleSavePath(std::string("level.dat"));

    ConsoleSaveFileOutputStream fos =
        ConsoleSaveFileOutputStream(m_saveFile, currentFile);
    NbtIo::writeCompressed(root, &fos);

    delete root;
}

void DirectoryLevelStorage::save(std::shared_ptr<Player> player) {
    // 4J Jev, removed try/catch.
    PlayerUID playerXuid = player->getXuid();
    if (playerXuid != INVALID_XUID && !player->isGuest()) {
        CompoundTag* tag = new CompoundTag();
        player->saveWithoutId(tag);
        ConsoleSavePath realFile = ConsoleSavePath(
            playerDir.getName() + toWString(player->getXuid()) + ".dat");
        // If saves are disabled (e.g. because we are writing the save buffer to
        // disk) then cache this player data
        if (PlatformStorage.GetSaveDisabled()) {
            ByteArrayOutputStream* bos = new ByteArrayOutputStream();
            NbtIo::writeCompressed(tag, bos);

            auto it = m_cachedSaveData.find(realFile.getName());
            if (it != m_cachedSaveData.end()) {
                delete it->second;
            }
            m_cachedSaveData[realFile.getName()] = bos;
            Log::info("Cached saving of file %s due to saves being disabled\n",
                      realFile.getName().c_str());
        } else {
            ConsoleSaveFileOutputStream fos =
                ConsoleSaveFileOutputStream(m_saveFile, realFile);
            NbtIo::writeCompressed(tag, &fos);
        }
        delete tag;
    } else if (playerXuid != INVALID_XUID) {
        Log::info("Not saving player as their XUID is a guest\n");
        dontSaveMapMappingForPlayer(playerXuid);
    }
}

// 4J Changed return val to bool to check if new player or loaded player
CompoundTag* DirectoryLevelStorage::load(std::shared_ptr<Player> player) {
    CompoundTag* tag = loadPlayerDataTag(player->getXuid());
    if (tag != nullptr) {
        player->load(tag);
    }
    return tag;
}

CompoundTag* DirectoryLevelStorage::loadPlayerDataTag(PlayerUID xuid) {
    // 4J Jev, removed try/catch.
    ConsoleSavePath realFile =
        ConsoleSavePath(playerDir.getName() + toWString(xuid) + ".dat");
    auto it = m_cachedSaveData.find(realFile.getName());
    if (it != m_cachedSaveData.end()) {
        ByteArrayOutputStream* bos = it->second;
        ByteArrayInputStream bis(bos->buf, 0, bos->size());
        CompoundTag* tag = NbtIo::readCompressed(&bis);
        bis.reset();
        Log::info("Loaded player data from cached file %s\n",
                  realFile.getName().c_str());
        return tag;
    } else if (m_saveFile->doesFileExist(realFile)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(m_saveFile, realFile);
        return NbtIo::readCompressed(&fis);
    }
    return nullptr;
}

// 4J Added function
void DirectoryLevelStorage::clearOldPlayerFiles() {
    if (PlatformStorage.GetSaveDisabled()) return;

    std::vector<FileEntry*>* playerFiles =
        m_saveFile->getFilesWithPrefix(playerDir.getName());

    if (playerFiles != nullptr) {
#if !defined(_FINAL_BUILD)
        if (gameServices().debugSettingsOn() &&
            gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
                (1L << eDebugSetting_DistributableSave)) {
            for (unsigned int i = 0; i < playerFiles->size(); ++i) {
                FileEntry* file = playerFiles->at(i);
                std::string xuidStr = replaceAll(
                    replaceAll(file->data.filename, playerDir.getName(), ""),
                    ".dat", "");
                PlayerUID xuid = fromWString<PlayerUID>(xuidStr);
                deleteMapFilesForPlayer(xuid);
                m_saveFile->deleteFile(playerFiles->at(i));
            }
        } else
#endif
            if (playerFiles->size() > MAX_PLAYER_DATA_SAVES) {
            sort(playerFiles->begin(), playerFiles->end(),
                 FileEntry::newestFirst);

            for (unsigned int i = MAX_PLAYER_DATA_SAVES;
                 i < playerFiles->size(); ++i) {
                FileEntry* file = playerFiles->at(i);
                std::string xuidStr = replaceAll(
                    replaceAll(file->data.filename, playerDir.getName(), ""),
                    ".dat", "");
                PlayerUID xuid = fromWString<PlayerUID>(xuidStr);
                deleteMapFilesForPlayer(xuid);
                m_saveFile->deleteFile(playerFiles->at(i));
            }
        }

        delete playerFiles;
    }
}

PlayerIO* DirectoryLevelStorage::getPlayerIO() { return this; }

void DirectoryLevelStorage::closeAll() {}

ConsoleSavePath DirectoryLevelStorage::getDataFile(const std::string& id) {
    return ConsoleSavePath(dataDir.getName() + id + ".dat");
}

std::string DirectoryLevelStorage::getLevelId() { return levelId; }

void DirectoryLevelStorage::flushSaveFile(bool autosave) {
#if !defined(_CONTENT_PACKAGE)
    if (gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_DistributableSave)) {
        // Delete gamerules files if it exists
        ConsoleSavePath gameRulesFiles(GAME_RULE_SAVENAME);
        if (m_saveFile->doesFileExist(gameRulesFiles)) {
            FileEntry* fe = m_saveFile->createFile(gameRulesFiles);
            m_saveFile->deleteFile(fe);
        }
    }
#endif
    m_saveFile->Flush(autosave);
}

// 4J Added
void DirectoryLevelStorage::resetNetherPlayerPositions() {
    if (gameServices().getResetNether()) {
        std::vector<FileEntry*>* playerFiles =
            m_saveFile->getFilesWithPrefix(playerDir.getName());

        if (playerFiles != nullptr) {
            for (auto it = playerFiles->begin(); it != playerFiles->end();
                 ++it) {
                FileEntry* realFile = *it;
                ConsoleSaveFileInputStream fis =
                    ConsoleSaveFileInputStream(m_saveFile, realFile);
                CompoundTag* tag = NbtIo::readCompressed(&fis);
                if (tag != nullptr) {
                    // If the player is in the nether, set their y position
                    // above the top of the nether This will force the player to
                    // be spawned in a valid position in the overworld when they
                    // are loaded
                    if (tag->contains("Dimension") &&
                        tag->getInt("Dimension") ==
                            LevelData::DIMENSION_NETHER &&
                        tag->contains("Pos")) {
                        ListTag<DoubleTag>* pos =
                            (ListTag<DoubleTag>*)tag->getList("Pos");
                        pos->get(1)->data = DBL_MAX;

                        ConsoleSaveFileOutputStream fos =
                            ConsoleSaveFileOutputStream(m_saveFile, realFile);
                        NbtIo::writeCompressed(tag, &fos);
                    }
                    delete tag;
                }
            }
            delete playerFiles;
        }
    }
}

int DirectoryLevelStorage::getAuxValueForMap(PlayerUID xuid, int dimension,
                                             int centreXC, int centreZC,
                                             int scale) {
    int mapId = -1;
    bool foundMapping = false;

#if defined(_LARGE_WORLDS)
    auto it = m_playerMappings.find(xuid);
    if (it != m_playerMappings.end()) {
        foundMapping =
            it->second.getMapping(mapId, centreXC, centreZC, dimension, scale);
    }

    if (!foundMapping) {
        for (unsigned int i = 0; i < m_usedMappings.size(); ++i) {
            if (m_usedMappings[i] < 0xFF) {
                unsigned int offset = 0;
                for (; offset < 8; ++offset) {
                    if (!(m_usedMappings[i] & (1 << offset))) {
                        break;
                    }
                }
                mapId = (i * 8) + offset;
                m_playerMappings[xuid].addMapping(mapId, centreXC, centreZC,
                                                  dimension, scale);
                m_usedMappings[i] |= (1 << offset);
                break;
            }
        }
    }
#else
    for (unsigned int i = 0; i < MAXIMUM_MAP_SAVE_DATA; ++i) {
        if (m_mapDataMappings.xuids[i] == xuid &&
            m_mapDataMappings.getDimension(i) == dimension) {
            foundMapping = true;
            mapId = i;
            break;
        }
        if (mapId < 0 && m_mapDataMappings.xuids[i] == INVALID_XUID) {
            mapId = i;
        }
    }
    if (!foundMapping && mapId >= 0 && mapId < MAXIMUM_MAP_SAVE_DATA) {
        m_mapDataMappings.setMapping(mapId, xuid, dimension);
        m_saveableMapDataMappings.setMapping(mapId, xuid, dimension);

        // If we had an old map file for a mapping that is no longer valid,
        // delete it
        std::string id = std::string("map_") + toWString(mapId);
        ConsoleSavePath file = getDataFile(id);

        if (m_saveFile->doesFileExist(file)) {
            auto it = find(m_mapFilesToDelete.begin(), m_mapFilesToDelete.end(),
                           mapId);
            if (it != m_mapFilesToDelete.end()) m_mapFilesToDelete.erase(it);

            m_saveFile->deleteFile(m_saveFile->createFile(file));
        }
    }
#endif
    return mapId;
}

void DirectoryLevelStorage::saveMapIdLookup() {
    if (PlatformStorage.GetSaveDisabled()) return;

#if defined(_LARGE_WORLDS)
    ConsoleSavePath file = getDataFile("largeMapDataMappings");
#else
    ConsoleSavePath file = getDataFile("mapDataMappings");
#endif

    if (!file.getName().empty()) {
        unsigned int NumberOfBytesWritten;
        FileEntry* fileEntry = m_saveFile->createFile(file);
        m_saveFile->setFilePointer(fileEntry, 0, SaveFileSeekOrigin::Begin);

#if defined(_LARGE_WORLDS)
        ByteArrayOutputStream baos;
        DataOutputStream dos(&baos);
        dos.writeInt(m_playerMappings.size());
        Log::info("Saving %zu mappings\n", m_playerMappings.size());
        for (auto it = m_playerMappings.begin(); it != m_playerMappings.end();
             ++it) {
            Log::info("  -- %llu\n",
                      static_cast<unsigned long long>(it->first));
            dos.writePlayerUID(it->first);
            it->second.writeMappings(&dos);
        }
        dos.write(m_usedMappings);
        m_saveFile->writeFile(fileEntry,
                              baos.buf.data(),       // data buffer
                              baos.size(),           // number of bytes to write
                              &NumberOfBytesWritten  // number of bytes written
        );
#else
        m_saveFile->writeFile(
            fileEntry,
            &m_saveableMapDataMappings,  // data buffer
            sizeof(MapDataMappings),     // number of bytes to write
            &NumberOfBytesWritten        // number of bytes written
        );
        assert(NumberOfBytesWritten == sizeof(MapDataMappings));
#endif
    }
}

void DirectoryLevelStorage::dontSaveMapMappingForPlayer(PlayerUID xuid) {
#if defined(_LARGE_WORLDS)
    auto it = m_playerMappings.find(xuid);
    if (it != m_playerMappings.end()) {
        for (auto itMap = it->second.m_mappings.begin();
             itMap != it->second.m_mappings.end(); ++itMap) {
            int index = itMap->second / 8;
            int offset = itMap->second % 8;
            m_usedMappings[index] &= ~(1 << offset);
        }
        m_playerMappings.erase(it);
    }
#else
    for (unsigned int i = 0; i < MAXIMUM_MAP_SAVE_DATA; ++i) {
        if (m_saveableMapDataMappings.xuids[i] == xuid) {
            m_saveableMapDataMappings.setMapping(i, INVALID_XUID, 0);
        }
    }
#endif
}

void DirectoryLevelStorage::deleteMapFilesForPlayer(
    std::shared_ptr<Player> player) {
    PlayerUID playerXuid = player->getXuid();
    if (playerXuid != INVALID_XUID) deleteMapFilesForPlayer(playerXuid);
}

void DirectoryLevelStorage::deleteMapFilesForPlayer(PlayerUID xuid) {
#if defined(_LARGE_WORLDS)
    auto it = m_playerMappings.find(xuid);
    if (it != m_playerMappings.end()) {
        for (auto itMap = it->second.m_mappings.begin();
             itMap != it->second.m_mappings.end(); ++itMap) {
            std::string id = std::string("map_") + toWString(itMap->second);
            ConsoleSavePath file = getDataFile(id);

            if (m_saveFile->doesFileExist(file)) {
                // If we can't actually delete this file, store the name so we
                // can delete it later
                if (PlatformStorage.GetSaveDisabled())
                    m_mapFilesToDelete.push_back(itMap->second);
                else
                    m_saveFile->deleteFile(m_saveFile->createFile(file));
            }

            int index = itMap->second / 8;
            int offset = itMap->second % 8;
            m_usedMappings[index] &= ~(1 << offset);
        }
        m_playerMappings.erase(it);
    }
#else
    bool changed = false;
    for (unsigned int i = 0; i < MAXIMUM_MAP_SAVE_DATA; ++i) {
        if (m_mapDataMappings.xuids[i] == xuid) {
            changed = true;

            std::string id = std::string("map_") + toWString(i);
            ConsoleSavePath file = getDataFile(id);

            if (m_saveFile->doesFileExist(file)) {
                // If we can't actually delete this file, store the name so we
                // can delete it later
                if (PlatformStorage.GetSaveDisabled())
                    m_mapFilesToDelete.push_back(i);
                else
                    m_saveFile->deleteFile(m_saveFile->createFile(file));
            }
            m_mapDataMappings.setMapping(i, INVALID_XUID, 0);
            m_saveableMapDataMappings.setMapping(i, INVALID_XUID, 0);
            break;
        }
    }
#endif
}

void DirectoryLevelStorage::saveAllCachedData() {
    if (PlatformStorage.GetSaveDisabled()) return;

    // Save any files that were saved while saving was disabled
    for (auto it = m_cachedSaveData.begin(); it != m_cachedSaveData.end();
         ++it) {
        ByteArrayOutputStream* bos = it->second;

        ConsoleSavePath realFile = ConsoleSavePath(it->first);
        ConsoleSaveFileOutputStream fos =
            ConsoleSaveFileOutputStream(m_saveFile, realFile);

        Log::info("Actually writing cached file %s\n", it->first.c_str());
        fos.write(bos->buf, 0, bos->size());
        delete bos;
    }
    m_cachedSaveData.clear();

    for (auto it = m_mapFilesToDelete.begin(); it != m_mapFilesToDelete.end();
         ++it) {
        std::string id = std::string("map_") + toWString(*it);
        ConsoleSavePath file = getDataFile(id);
        if (m_saveFile->doesFileExist(file)) {
            m_saveFile->deleteFile(m_saveFile->createFile(file));
        }
    }
    m_mapFilesToDelete.clear();
}
