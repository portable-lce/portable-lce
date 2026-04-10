#include "OldChunkStorage.h"

#include <assert.h>
#include <stdio.h>

#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "java/File.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/InputOutputStream/FileInputStream.h"
#include "java/InputOutputStream/FileOutputStream.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityIO.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/TickNextTickData.h"
#include "minecraft/world/level/TilePos.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "nbt/NbtIo.h"
#include "platform/input/input.h"

thread_local OldChunkStorage::ThreadStorage* OldChunkStorage::m_tlsStorage =
    nullptr;
OldChunkStorage::ThreadStorage* OldChunkStorage::m_defaultThreadStorage =
    nullptr;

OldChunkStorage::ThreadStorage::ThreadStorage() {
    blockData = std::vector<uint8_t>(Level::CHUNK_TILE_COUNT);
    dataData = std::vector<uint8_t>(Level::HALF_CHUNK_TILE_COUNT);
    skyLightData = std::vector<uint8_t>(Level::HALF_CHUNK_TILE_COUNT);
    blockLightData = std::vector<uint8_t>(Level::HALF_CHUNK_TILE_COUNT);
}

OldChunkStorage::ThreadStorage::~ThreadStorage() {}

void OldChunkStorage::CreateNewThreadStorage() {
    ThreadStorage* tls = new ThreadStorage();

    if (m_defaultThreadStorage == nullptr) {
        m_defaultThreadStorage = tls;
    }

    m_tlsStorage = tls;
}

void OldChunkStorage::UseDefaultThreadStorage() {
    m_tlsStorage = m_defaultThreadStorage;
}

void OldChunkStorage::ReleaseThreadStorage() {
    if (m_tlsStorage != m_defaultThreadStorage) {
        delete m_tlsStorage;
    }
}

OldChunkStorage::OldChunkStorage(File dir, bool create) {
    this->dir = dir;
    this->create = create;
}

// https://cplusplus.com/forum/general/144043/
void to_base36(int value, char* buf) {
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[64];
    int i = 0;
    unsigned int uval =
        (value < 0) ? -(unsigned int)value : (unsigned int)value;

    if (uval == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (uval > 0) {
        tmp[i++] = digits[uval % 36];
        uval /= 36;
    }
    if (value < 0) tmp[i++] = '-';

    for (int j = 0; j < i; ++j) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

File OldChunkStorage::getFile(int x, int z) {
    inline constexpr int MAX_PATH_SIZE = 256;
    
    char name[MAX_PATH_SIZE];
    char path1[MAX_PATH_SIZE];
    char path2[MAX_PATH_SIZE];

    char xRadix36[64];
    char zRadix36[64];

    to_base36(x, xRadix36);
    to_base36(z, zRadix36);
    snprintf(name, MAX_PATH_SIZE, "c.%s.%s.dat", xRadix36, zRadix36);
    to_base36(x & 63, path1);
    to_base36(z & 63, path2);

    // sprintf(file,"%s\\%s",dir,path1);
    File file(dir, std::string(path1));
    if (!file.exists()) {
        if (create)
            file.mkdir();
        else {
            return File("");
        }
    }

    // strcat(file,"\\");
    // strcat(file,path2);
    file = File(file, std::string(path2));
    if (!file.exists()) {
        if (create)
            file.mkdir();
        else {
            return File("");
        }
    }

    // strcat(file,"\\");
    // strcat(file,name);
    // sprintf(file,"%s\\%s",file,name);
    file = File(file, std::string(name));
    if (!file.exists()) {
        if (!create) {
            return File("");
        }
    }
    return file;
}

LevelChunk* OldChunkStorage::load(Level* level, int x, int z) {
    File file = getFile(x, z);
    if (!file.getPath().empty() && file.exists()) {
        // 4J - removed try/catch
        //		try {
        //                System.out.println("Loading chunk "+x+", "+z);
        FileInputStream fis = FileInputStream(file);
        CompoundTag* tag = NbtIo::readCompressed(&fis);
        if (!tag->contains("Level")) {
            char buf[256];
            sprintf(buf,
                    "Chunk file at %d, %d is missing level data, skipping\n", x,
                    z);
            Log::info("%s", buf);
            return nullptr;
        }
        if (!tag->getCompound("Level")->contains("Blocks")) {
            char buf[256];
            sprintf(buf,
                    "Chunk file at %d, %d is missing block data, skipping\n", x,
                    z);
            Log::info("%s", buf);
            return nullptr;
        }
        LevelChunk* levelChunk =
            OldChunkStorage::load(level, tag->getCompound("Level"));
        if (!levelChunk->isAt(x, z)) {
            char buf[256];
            sprintf(buf,
                    "Chunk fileat %d, %d is in the wrong location; relocating. "
                    "Expected %d, %d, got %d, %d\n",
                    x, z, x, z, levelChunk->x, levelChunk->z);
            Log::info("%s", buf);
            tag->putInt("xPos", x);
            tag->putInt("zPos", z);
            levelChunk =
                OldChunkStorage::load(level, tag->getCompound("Level"));
        }

        return levelChunk;
        //		} catch (Exception e) {
        //			e.printStackTrace();
        //		}
    }
    return nullptr;
}

void OldChunkStorage::save(Level* level, LevelChunk* levelChunk) {
    level->checkSession();
    File file = getFile(levelChunk->x, levelChunk->z);
    if (file.exists()) {
        LevelData* levelData = level->getLevelData();
        levelData->setSizeOnDisk(levelData->getSizeOnDisk() - file.length());
    }

    // 4J - removed try/catch
    //    try {
    // char tmpFileName[MAX_PATH_SIZE];
    // sprintf(tmpFileName,"%s\\%s",dir,"tmp_chunk.dat");
    File tmpFile(dir, "tmp_chunk.dat");
    //            System.out.println("Saving chunk "+levelChunk.x+",
    //            "+levelChunk.z);

    FileOutputStream fos = FileOutputStream(tmpFile);
    CompoundTag* tag = new CompoundTag();
    CompoundTag* levelData = new CompoundTag();
    tag->put("Level", levelData);
    OldChunkStorage::save(levelChunk, level, levelData);
    NbtIo::writeCompressed(tag, &fos);
    fos.close();

    if (file.exists()) {
        // DeleteFile(file);
        file._delete();
    }
    // MoveFile(tmpFile,file);
    tmpFile.renameTo(file);

    LevelData* levelInfo = level->getLevelData();
    levelInfo->setSizeOnDisk(levelInfo->getSizeOnDisk() + file.length());
    //    } catch (Exception e) {
    //        e.printStackTrace();
    //    }
}

bool OldChunkStorage::saveEntities(LevelChunk* lc, Level* level,
                                   CompoundTag* tag) {
    // If we saved and it had no entities, and nothing has been added since skip
    // this one
    if (!lc->lastSaveHadEntities) return false;

    lc->lastSaveHadEntities = false;
    ListTag<CompoundTag>* entityTags = new ListTag<CompoundTag>();

    {
        std::lock_guard<std::recursive_mutex> lock(lc->m_csEntities);
        for (int i = 0; i < lc->ENTITY_BLOCKS_LENGTH; i++) {
            auto itEnd = lc->entityBlocks[i]->end();
            for (std::vector<std::shared_ptr<Entity> >::iterator it =
                     lc->entityBlocks[i]->begin();
                 it != itEnd; it++) {
                std::shared_ptr<Entity> e = *it;
                lc->lastSaveHadEntities = true;
                CompoundTag* teTag = new CompoundTag();
                if (e->save(teTag)) {
                    entityTags->add(teTag);
                }
            }
        }
    }

    tag->put("Entities", entityTags);

    return lc->lastSaveHadEntities;
}

void OldChunkStorage::save(LevelChunk* lc, Level* level,
                           DataOutputStream* dos) {
    dos->writeShort(SAVE_FILE_VERSION_NUMBER);
    dos->writeInt(lc->x);
    dos->writeInt(lc->z);
    dos->writeLong(level->getGameTime());
    dos->writeLong(lc->inhabitedTime);

    lc->writeCompressedBlockData(dos);

    lc->writeCompressedDataData(dos);

    lc->writeCompressedSkyLightData(dos);
    lc->writeCompressedBlockLightData(dos);

    dos->write(lc->heightmap);
    dos->writeShort(lc->terrainPopulated);
    dos->write(lc->getBiomes());

    CompoundTag* tag = new CompoundTag();
#if !defined(SPLIT_SAVES)
    saveEntities(lc, level, tag);
#endif

    ListTag<CompoundTag>* tileEntityTags = new ListTag<CompoundTag>();

    auto itEnd = lc->tileEntities.end();
    for (std::unordered_map<TilePos, std::shared_ptr<TileEntity>,
                            TilePosKeyHash, TilePosKeyEq>::iterator it =
             lc->tileEntities.begin();
         it != itEnd; it++) {
        std::shared_ptr<TileEntity> te = it->second;
        CompoundTag* teTag = new CompoundTag();
        te->save(teTag);
        tileEntityTags->add(teTag);
    }
    tag->put("TileEntities", tileEntityTags);

    std::vector<TickNextTickData>* ticksInChunk =
        level->fetchTicksInChunk(lc, false);
    if (ticksInChunk != nullptr) {
        int64_t levelTime = level->getGameTime();

        ListTag<CompoundTag>* tickTags = new ListTag<CompoundTag>();
        for (int i = 0; i < ticksInChunk->size(); i++) {
            TickNextTickData td = ticksInChunk->at(i);
            CompoundTag* teTag = new CompoundTag();
            teTag->putInt("i", td.tileId);
            teTag->putInt("x", td.x);
            teTag->putInt("y", td.y);
            teTag->putInt("z", td.z);
            teTag->putInt("t", (int)(td.m_delay - levelTime));

            tickTags->add(teTag);
        }
        tag->put("TileTicks", tickTags);
    }
    delete ticksInChunk;

    NbtIo::write(tag, dos);
    delete tag;
}

void OldChunkStorage::save(LevelChunk* lc, Level* level, CompoundTag* tag) {
    level->checkSession();
    tag->putInt("xPos", lc->x);
    tag->putInt("zPos", lc->z);
    tag->putLong("LastUpdate", level->getGameTime());
    tag->putLong("InhabitedTime", lc->inhabitedTime);
    // 4J - changes here for new storage. Now have static storage for getting
    // lighting data for block, data, and sky & block lighting. This wasn't
    // required in the original version as we could just reference the
    // information in the level itself, but with our new storage system the full
    // data doesn't normally exist & so getSkyLightData/getBlockLightData etc.
    // need somewhere to output this data. Making this static so that we aren't
    // dynamically allocating memory in the server thread when writing chunks as
    // this causes serious stalling on the main thread. Will be fine so long as
    // we only actually create tags for once chunk at a time.

    // 4J Stu - As we now save on multiple threads, the static data has been
    // moved to TLS
    ThreadStorage* tls = m_tlsStorage;

    // static std::vector<uint8_t> blockData = std::vector<uint8_t>(32768);
    lc->getBlockData(tls->blockData);
    tag->putByteArray("Blocks", tls->blockData);

    // static std::vector<uint8_t> dataData = std::vector<uint8_t>(16384);
    lc->getDataData(tls->dataData);
    tag->putByteArray("Data", tls->dataData);

    // static std::vector<uint8_t> skyLightData = std::vector<uint8_t>(16384);
    // static std::vector<uint8_t> blockLightData = std::vector<uint8_t>(16384);
    lc->getSkyLightData(tls->skyLightData);
    lc->getBlockLightData(tls->blockLightData);
    tag->putByteArray("SkyLight", tls->skyLightData);
    tag->putByteArray("BlockLight", tls->blockLightData);

    tag->putByteArray("HeightMap", lc->heightmap);
    tag->putShort(
        "TerrainPopulatedFlags",
        lc->terrainPopulated);  // 4J - changed from "TerrainPopulated" to
                                // "TerrainPopulatedFlags" as now stores a
                                // bitfield, java stores a bool
    std::vector<uint8_t> biomeData = lc->getBiomes();
    tag->putByteArray("Biomes", biomeData);

#if !defined(SPLIT_SAVES)
    saveEntities(lc, level, tag);
#endif

    ListTag<CompoundTag>* tileEntityTags = new ListTag<CompoundTag>();

    auto itEnd = lc->tileEntities.end();
    for (std::unordered_map<TilePos, std::shared_ptr<TileEntity>,
                            TilePosKeyHash, TilePosKeyEq>::iterator it =
             lc->tileEntities.begin();
         it != itEnd; it++) {
        std::shared_ptr<TileEntity> te = it->second;
        CompoundTag* teTag = new CompoundTag();
        te->save(teTag);
        tileEntityTags->add(teTag);
    }
    tag->put("TileEntities", tileEntityTags);

    std::vector<TickNextTickData>* ticksInChunk =
        level->fetchTicksInChunk(lc, false);
    if (ticksInChunk != nullptr) {
        int64_t levelTime = level->getGameTime();

        ListTag<CompoundTag>* tickTags = new ListTag<CompoundTag>();
        for (int i = 0; i < ticksInChunk->size(); i++) {
            TickNextTickData td = ticksInChunk->at(i);
            CompoundTag* teTag = new CompoundTag();
            teTag->putInt("i", td.tileId);
            teTag->putInt("x", td.x);
            teTag->putInt("y", td.y);
            teTag->putInt("z", td.z);
            teTag->putInt("t", (int)(td.m_delay - levelTime));
            teTag->putInt("p", td.priorityTilt);

            tickTags->add(teTag);
        }
        tag->put("TileTicks", tickTags);
    }
    delete ticksInChunk;
}

void OldChunkStorage::loadEntities(LevelChunk* lc, Level* level,
                                   CompoundTag* tag) {
    ListTag<CompoundTag>* entityTags =
        (ListTag<CompoundTag>*)tag->getList("Entities");
    if (entityTags != nullptr) {
        for (int i = 0; i < entityTags->size(); i++) {
            CompoundTag* teTag = entityTags->get(i);
            std::shared_ptr<Entity> te = EntityIO::loadStatic(teTag, level);
            lc->lastSaveHadEntities = true;
            if (te != nullptr) {
                lc->addEntity(te);
            }
        }
    }

    ListTag<CompoundTag>* tileEntityTags =
        (ListTag<CompoundTag>*)tag->getList("TileEntities");
    if (tileEntityTags != nullptr) {
        for (int i = 0; i < tileEntityTags->size(); i++) {
            CompoundTag* teTag = tileEntityTags->get(i);
            std::shared_ptr<TileEntity> te = TileEntity::loadStatic(teTag);
            if (te != nullptr) {
                lc->addTileEntity(te);
            }
        }
    }
}

LevelChunk* OldChunkStorage::load(Level* level, DataInputStream* dis) {
    short version = dis->readShort();
    int x = dis->readInt();
    int z = dis->readInt();
    int time = dis->readLong();

    LevelChunk* levelChunk = new LevelChunk(level, x, z);

    if (version >= SAVE_FILE_VERSION_CHUNK_INHABITED_TIME) {
        levelChunk->inhabitedTime = dis->readLong();
    }

    levelChunk->readCompressedBlockData(dis);
    levelChunk->readCompressedDataData(dis);
    levelChunk->readCompressedSkyLightData(dis);
    levelChunk->readCompressedBlockLightData(dis);

    dis->readFully(levelChunk->heightmap);

    levelChunk->terrainPopulated = dis->readShort();
    // If all neighbours have been post-processed, then we should have done the
    // post-post-processing now. Check that this is set as if it isn't then we
    // won't be able to send network data for chunks, and we won't ever try and
    // set it again as all the directional flags are now already set - should
    // only be an issue for old maps before this flag was added.
    if ((levelChunk->terrainPopulated &
         LevelChunk::sTerrainPopulatedAllNeighbours) ==
        LevelChunk::sTerrainPopulatedAllNeighbours) {
        levelChunk->terrainPopulated |= LevelChunk::sTerrainPostPostProcessed;
    }

#if !defined(_CONTENT_PACKAGE)
    if (gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_EnableBiomeOverride)) {
        // Read the biome data from the stream, but don't use it
        std::vector<uint8_t> dummyBiomes(levelChunk->biomes.size());
        dis->readFully(dummyBiomes);
    } else
#endif
    {
        dis->readFully(levelChunk->biomes);
    }

    CompoundTag* tag = NbtIo::read(dis);

    loadEntities(levelChunk, level, tag);

    if (tag->contains("TileTicks")) {
        ListTag<CompoundTag>* tileTicks =
            (ListTag<CompoundTag>*)tag->getList("TileTicks");

        if (tileTicks != nullptr) {
            for (int i = 0; i < tileTicks->size(); i++) {
                CompoundTag* teTag = tileTicks->get(i);

                level->forceAddTileTick(teTag->getInt("x"), teTag->getInt("y"),
                                        teTag->getInt("z"), teTag->getInt("i"),
                                        teTag->getInt("t"), teTag->getInt("p"));
            }
        }
    }

    delete tag;

    return levelChunk;
}

LevelChunk* OldChunkStorage::load(Level* level, CompoundTag* tag) {
    int x = tag->getInt("xPos");
    int z = tag->getInt("zPos");

    LevelChunk* levelChunk = new LevelChunk(level, x, z);
    // 4J - the original code uses the data in the tag directly, but this is now
    // just used as a source when creating the compressed data, so we need to
    // free up the data in the tag once we are done
    {
        auto blocks = tag->getByteArray("Blocks");
        levelChunk->setBlockData(blocks);
    }
    //	levelChunk->blocks = tag->getByteArray("Blocks");

    // 4J - the original code uses the data in the tag directly, but this is now
    // just used as a source when creating the compressed data, so we need to
    // free up the data in the tag once we are done
    {
        auto data = tag->getByteArray("Data");
        levelChunk->setDataData(data);
    }

    // 4J - changed to use our new methods for accessing lighting
    {
        auto skyLight = tag->getByteArray("SkyLight");
        levelChunk->setSkyLightData(skyLight);
    }
    {
        auto blockLight = tag->getByteArray("BlockLight");
        levelChunk->setBlockLightData(blockLight);
    }

    // In the original code (commented out below) constructing DataLayers from
    // these arrays uses the data directly and so it doesn't need deleted. The
    // new setSkyLightData/setBlockLightData take a copy of the data so we need
    // to delete the local one now

    //	levelChunk->skyLight = new DataLayer(tag->getByteArray("SkyLight"),
    // level->depthBits); 	levelChunk->blockLight = new
    // DataLayer(tag->getByteArray("BlockLight"), level->depthBits);

    levelChunk->heightmap = tag->getByteArray("HeightMap");
    // 4J - TerrainPopulated was a bool (java), then changed to be a byte
    // bitfield, then replaced with TerrainPopulatedShort to store a wider
    // bitfield
    if (tag->get("TerrainPopulated")) {
        // Java bool type or byte bitfield
        levelChunk->terrainPopulated = tag->getByte("TerrainPopulated");
        if (levelChunk->terrainPopulated >= 1)
            levelChunk->terrainPopulated =
                LevelChunk::sTerrainPopulatedAllNeighbours |
                LevelChunk::sTerrainPostPostProcessed;  // Convert from old bool
                                                        // type to new bitfield
    } else {
        // New style short
        levelChunk->terrainPopulated = tag->getShort("TerrainPopulatedFlags");
        // If all neighbours have been post-processed, then we should have done
        // the post-post-processing now. Check that this is set as if it isn't
        // then we won't be able to send network data for chunks, and we won't
        // ever try and set it again as all the directional flags are now
        // already set - should only be an issue for old maps before this flag
        // was added.
        if ((levelChunk->terrainPopulated &
             LevelChunk::sTerrainPopulatedAllNeighbours) ==
            LevelChunk::sTerrainPopulatedAllNeighbours) {
            levelChunk->terrainPopulated |=
                LevelChunk::sTerrainPostPostProcessed;
        }
    }

    // 4J removed - we shouldn't need this any more

#if !defined(_CONTENT_PACKAGE)
    if (gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_EnableBiomeOverride)) {
        // Do nothing
    } else
#endif
    {
        if (tag->contains("Biomes")) {
            auto biomes = tag->getByteArray("Biomes");
            levelChunk->setBiomes(biomes);
        }
    }

    loadEntities(levelChunk, level, tag);

    if (tag->contains("TileTicks")) {
        ListTag<CompoundTag>* tileTicks =
            (ListTag<CompoundTag>*)tag->getList("TileTicks");

        if (tileTicks != nullptr) {
            for (int i = 0; i < tileTicks->size(); i++) {
                CompoundTag* teTag = tileTicks->get(i);

                level->forceAddTileTick(teTag->getInt("x"), teTag->getInt("y"),
                                        teTag->getInt("z"), teTag->getInt("i"),
                                        teTag->getInt("t"), teTag->getInt("p"));
            }
        }
    }

    return levelChunk;
}

void OldChunkStorage::tick() {}

void OldChunkStorage::flush() {}

void OldChunkStorage::saveEntities(Level* level, LevelChunk* levelChunk) {}
