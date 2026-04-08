#include "GameRuleManager.h"

#include <assert.h>
#include <string.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "app/common/DLC/DLCGameRulesFile.h"
#include "app/common/DLC/DLCGameRulesHeader.h"
#include "app/common/DLC/DLCLocalisationFile.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/GameRules/LevelGeneration/ConsoleSchematicFile.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "app/common/GameRules/LevelGeneration/LevelGenerators.h"
#include "app/common/GameRules/LevelRules/LevelRules.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/LevelRuleset.h"
#include "minecraft/locale/StringTable.h"
#include "app/linux/LinuxGame.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "java/File.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "strings.h"

const char* GameRuleManager::wchTagNameA[] = {
    "",                   // eGameRuleType_Root
    "MapOptions",         // eGameRuleType_LevelGenerationOptions
    "ApplySchematic",     // eGameRuleType_ApplySchematic
    "GenerateStructure",  // eGameRuleType_GenerateStructure
    "GenerateBox",        // eGameRuleType_GenerateBox
    "PlaceBlock",         // eGameRuleType_PlaceBlock
    "PlaceContainer",     // eGameRuleType_PlaceContainer
    "PlaceSpawner",       // eGameRuleType_PlaceSpawner
    "BiomeOverride",      // eGameRuleType_BiomeOverride
    "StartFeature",       // eGameRuleType_StartFeature
    "AddItem",            // eGameRuleType_AddItem
    "AddEnchantment",     // eGameRuleType_AddEnchantment
    "LevelRules",         // eGameRuleType_LevelRules
    "NamedArea",          // eGameRuleType_NamedArea
    "UseTile",            // eGameRuleType_UseTileRule
    "CollectItem",        // eGameRuleType_CollectItemRule
    "CompleteAll",        // eGameRuleType_CompleteAllRule
    "UpdatePlayer",       // eGameRuleType_UpdatePlayerRule
};

const char* GameRuleManager::wchAttrNameA[] = {
    "descriptionName",   // eGameRuleAttr_descriptionName
    "promptName",        // eGameRuleAttr_promptName
    "dataTag",           // eGameRuleAttr_dataTag
    "enchantmentId",     // eGameRuleAttr_enchantmentId
    "enchantmentLevel",  // eGameRuleAttr_enchantmentLevel
    "itemId",            // eGameRuleAttr_itemId
    "quantity",          // eGameRuleAttr_quantity
    "auxValue",          // eGameRuleAttr_auxValue
    "slot",              // eGameRuleAttr_slot
    "name",              // eGameRuleAttr_name
    "food",              // eGameRuleAttr_food
    "health",            // eGameRuleAttr_health
    "tileId",            // eGameRuleAttr_tileId
    "useCoords",         // eGameRuleAttr_useCoords
    "seed",              // eGameRuleAttr_seed
    "flatworld",         // eGameRuleAttr_flatworld
    "filename",          // eGameRuleAttr_filename
    "rot",               // eGameRuleAttr_rot
    "data",              // eGameRuleAttr_data
    "block",             // eGameRuleAttr_block
    "entity",            // eGameRuleAttr_entity
    "facing",            // eGameRuleAttr_facing
    "edgeTile",          // eGameRuleAttr_edgeTile
    "fillTile",          // eGameRuleAttr_fillTile
    "skipAir",           // eGameRuleAttr_skipAir
    "x",                 // eGameRuleAttr_x
    "x0",                // eGameRuleAttr_x0
    "x1",                // eGameRuleAttr_x1
    "y",                 // eGameRuleAttr_y
    "y0",                // eGameRuleAttr_y0
    "y1",                // eGameRuleAttr_y1
    "z",                 // eGameRuleAttr_z
    "z0",                // eGameRuleAttr_z0
    "z1",                // eGameRuleAttr_z1
    "chunkX",            // eGameRuleAttr_chunkX
    "chunkZ",            // eGameRuleAttr_chunkZ
    "yRot",              // eGameRuleAttr_yRot
    "spawnX",            // eGameRuleAttr_spawnX
    "spawnY",            // eGameRuleAttr_spawnY
    "spawnZ",            // eGameRuleAttr_spawnZ
    "orientation",
    "dimension",
    "topTileId",  // eGameRuleAttr_topTileId
    "biomeId",    // eGameRuleAttr_biomeId
    "feature",    // eGameRuleAttr_feature
};

GameRuleManager::GameRuleManager() {
    m_currentGameRuleDefinitions = nullptr;
    m_currentLevelGenerationOptions = nullptr;
}

void GameRuleManager::loadGameRules(DLCPack* pack) {
    StringTable* strings = nullptr;

    if (pack->doesPackContainFile(DLCManager::e_DLCType_LocalisationData,
                                  "languages.loc")) {
        DLCLocalisationFile* localisationFile =
            (DLCLocalisationFile*)pack->getFile(
                DLCManager::e_DLCType_LocalisationData, "languages.loc");
        strings = localisationFile->getStringTable();
    }

    int gameRulesCount =
        pack->getDLCItemsCount(DLCManager::e_DLCType_GameRulesHeader);
    for (int i = 0; i < gameRulesCount; ++i) {
        DLCGameRulesHeader* dlcHeader = (DLCGameRulesHeader*)pack->getFile(
            DLCManager::e_DLCType_GameRulesHeader, i);
        std::uint32_t dSize;
        uint8_t* dData = dlcHeader->getData(dSize);

        LevelGenerationOptions* createdLevelGenerationOptions =
            new LevelGenerationOptions(pack);
        //	= loadGameRules(dData, dSize); //, strings);

        createdLevelGenerationOptions->setGrSource(dlcHeader);
        createdLevelGenerationOptions->setSrc(
            LevelGenerationOptions::eSrc_fromDLC);

        readRuleFile(createdLevelGenerationOptions, dData, dSize, strings);

        dlcHeader->lgo = createdLevelGenerationOptions;
    }

    gameRulesCount = pack->getDLCItemsCount(DLCManager::e_DLCType_GameRules);
    for (int i = 0; i < gameRulesCount; ++i) {
        DLCGameRulesFile* dlcFile = (DLCGameRulesFile*)pack->getFile(
            DLCManager::e_DLCType_GameRules, i);

        std::uint32_t dSize;
        uint8_t* dData = dlcFile->getData(dSize);

        LevelGenerationOptions* createdLevelGenerationOptions =
            new LevelGenerationOptions(pack);
        //	= loadGameRules(dData, dSize); //, strings);

        createdLevelGenerationOptions->setGrSource(new JustGrSource());
        createdLevelGenerationOptions->setSrc(
            LevelGenerationOptions::eSrc_tutorial);

        readRuleFile(createdLevelGenerationOptions, dData, dSize, strings);

        createdLevelGenerationOptions->setLoadedData();
    }
}

LevelGenerationOptions* GameRuleManager::loadGameRules(uint8_t* dIn,
                                                       unsigned int dSize) {
    LevelGenerationOptions* lgo = new LevelGenerationOptions();
    lgo->setGrSource(new JustGrSource());
    lgo->setSrc(LevelGenerationOptions::eSrc_fromSave);
    loadGameRules(lgo, dIn, dSize);
    lgo->setLoadedData();
    return lgo;
}

// 4J-JEV: Reverse of saveGameRules.
void GameRuleManager::loadGameRules(LevelGenerationOptions* lgo, uint8_t* dIn,
                                    unsigned int dSize) {
    app.DebugPrintf("GameRuleManager::LoadingGameRules:\n");

    std::vector<uint8_t> inputBuf(dIn, dIn + dSize);
    ByteArrayInputStream bais(inputBuf);
    DataInputStream dis(&bais);

    // Read file header.

    // dis.readInt(); // File Size

    short version = dis.readShort();
    assert(0x1 == version);
    app.DebugPrintf("\tversion=%d.\n", version);

    for (int i = 0; i < 8; i++) dis.readByte();

    std::uint8_t compression_type = dis.readByte();

    app.DebugPrintf("\tcompressionType=%d.\n", compression_type);

    unsigned int compr_len, decomp_len;
    compr_len = dis.readInt();
    decomp_len = dis.readInt();

    app.DebugPrintf("\tcompr_len=%d.\n\tdecomp_len=%d.\n", compr_len,
                    decomp_len);

    // Decompress File Body

    std::vector<uint8_t> content(decomp_len);
    std::vector<uint8_t> compr_content(compr_len);
    dis.read(compr_content);

    Compression::getCompression()->SetDecompressionType(
        (Compression::ECompressionTypes)compression_type);
    unsigned int contentSize = decomp_len;
    Compression::getCompression()->DecompressLZXRLE(
        content.data(), &contentSize, compr_content.data(),
        compr_content.size());
    content.resize(contentSize);
    Compression::getCompression()->SetDecompressionType(
        SAVE_FILE_PLATFORM_LOCAL);

    dis.close();
    bais.close();

    ByteArrayInputStream bais2(content);
    DataInputStream dis2(&bais2);

    // Read StringTable.
    unsigned int bStringTableSize = dis2.readInt();
    std::vector<uint8_t> bStringTable(bStringTableSize);
    dis2.read(bStringTable);
    StringTable* strings =
        new StringTable(bStringTable.data(), bStringTable.size());

    // Read RuleFile.
    std::vector<uint8_t> bRuleFile(content.size() - bStringTable.size());
    dis2.read(bRuleFile);

    // 4J-JEV: I don't believe that the path-name is ever used.
    // DLCGameRulesFile *dlcgr = new DLCGameRulesFile("__PLACEHOLDER__");
    // dlcgr->addData(bRuleFile.data(),bRuleFile.size());

    if (readRuleFile(lgo, bRuleFile.data(), bRuleFile.size(), strings)) {
        // Set current gen options and ruleset.
        // createdLevelGenerationOptions->setFromSaveGame(true);
        lgo->setSrc(LevelGenerationOptions::eSrc_fromSave);
        setLevelGenerationOptions(lgo);
        // m_currentGameRuleDefinitions = lgo->getRequiredGameRules();
    } else {
        delete lgo;
    }

    // Close and return.
    dis2.close();
    bais2.close();

    return;
}

// 4J-JEV: Reverse of loadGameRules.
void GameRuleManager::saveGameRules(uint8_t** dOut, unsigned int* dSize) {
    if (m_currentGameRuleDefinitions == nullptr &&
        m_currentLevelGenerationOptions == nullptr) {
        app.DebugPrintf("GameRuleManager:: Nothing here to save.");
        *dOut = nullptr;
        *dSize = 0;
        return;
    }

    app.DebugPrintf("GameRuleManager::saveGameRules:\n");

    // Initialise output stream.
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);

    // Write header.

    // VERSION NUMBER
    dos.writeShort(0x1);  // version_number

    // Write 8 bytes of empty space in case we need them later.
    // Mainly useful for the ones we save embedded in game saves.
    for (unsigned int i = 0; i < 8; i++) dos.writeByte(0x0);

    dos.writeByte(APPROPRIATE_COMPRESSION_TYPE);  // m_compressionType

    // -- START COMPRESSED -- //
    ByteArrayOutputStream compr_baos;
    DataOutputStream compr_dos(&compr_baos);

    if (m_currentGameRuleDefinitions == nullptr) {
        compr_dos.writeInt(0);  // numStrings for StringTable
        compr_dos.writeInt(version_number);
        compr_dos.writeByte(
            Compression::eCompressionType_None);  // compression type
        for (int i = 0; i < 2; i++) compr_dos.writeByte(0x0);  // Padding.
        compr_dos.writeInt(0);  // StringLookup.size()
        compr_dos.writeInt(0);  // SchematicFiles.size()
        compr_dos.writeInt(0);  // XmlObjects.size()
    } else {
        StringTable* st = m_currentGameRuleDefinitions->getStringTable();

        if (st == nullptr) {
            app.DebugPrintf(
                "GameRuleManager::saveGameRules: StringTable == nullptr!");
        } else {
            // Write string table.
            uint8_t* stbaPtr = nullptr;
            unsigned int stbaSize = 0;
            m_currentGameRuleDefinitions->getStringTable()->getData(&stbaPtr,
                                                                    &stbaSize);
            std::vector<uint8_t> stba(stbaPtr, stbaPtr + stbaSize);
            compr_dos.writeInt(stba.size());
            compr_dos.write(stba);

            // Write game rule file to second
            // buffer and generate string lookup.
            writeRuleFile(&compr_dos);
        }
    }

    // Compress compr_dos and write to dos.
    std::vector<uint8_t> compr_ba(compr_baos.buf.size());
    unsigned int compr_ba_size = compr_ba.size();
    Compression::getCompression()->CompressLZXRLE(
        compr_ba.data(), &compr_ba_size, compr_baos.buf.data(),
        compr_baos.buf.size());
    compr_ba.resize(compr_ba_size);

    app.DebugPrintf("\tcompr_ba.size()=%d.\n\tcompr_baos.buf.size()=%d.\n",
                    compr_ba.size(), compr_baos.buf.size());

    dos.writeInt(compr_ba.size());  // Write length
    dos.writeInt(compr_baos.buf.size());
    dos.write(compr_ba);

    compr_dos.close();
    compr_baos.close();
    // -- END COMPRESSED -- //

    // return
    *dSize = baos.buf.size();
    *dOut = new uint8_t[baos.buf.size()];
    memcpy(*dOut, baos.buf.data(), baos.buf.size());

    dos.close();
    baos.close();
}

// 4J-JEV: Reverse of readRuleFile.
void GameRuleManager::writeRuleFile(DataOutputStream* dos) {
    // Write Header
    dos->writeShort(version_number);                       // Version number.
    dos->writeByte(Compression::eCompressionType_None);    // compression type
    for (int i = 0; i < 8; i++) dos->writeBoolean(false);  // Padding.

    // Write string lookup.
    int numStrings = static_cast<int>(ConsoleGameRules::eGameRuleType_Count) +
                     static_cast<int>(ConsoleGameRules::eGameRuleAttr_Count);
    dos->writeInt(numStrings);
    for (int i = 0; i < ConsoleGameRules::eGameRuleType_Count; i++)
        dos->writeUTF(wchTagNameA[i]);
    for (int i = 0; i < ConsoleGameRules::eGameRuleAttr_Count; i++)
        dos->writeUTF(wchAttrNameA[i]);

    // Write schematic files.
    std::unordered_map<std::string, ConsoleSchematicFile*>* files;
    files = getLevelGenerationOptions()->getUnfinishedSchematicFiles();
    dos->writeInt(files->size());
    for (auto it = files->begin(); it != files->end(); it++) {
        std::string filename = it->first;
        ConsoleSchematicFile* file = it->second;

        ByteArrayOutputStream fileBaos;
        DataOutputStream fileDos(&fileBaos);
        file->save(&fileDos);

        dos->writeUTF(filename);
        // dos->writeInt(file->m_data.size());
        dos->writeInt(fileBaos.buf.size());
        dos->write((std::vector<uint8_t>)fileBaos.buf);

        fileDos.close();
        fileBaos.close();
    }

    // Write xml objects.
    dos->writeInt(2);  // numChildren
    m_currentLevelGenerationOptions->write(dos);
    m_currentGameRuleDefinitions->write(dos);
}

bool GameRuleManager::readRuleFile(
    LevelGenerationOptions* lgo, uint8_t* dIn, unsigned int dSize,
    StringTable* strings)  //(DLCGameRulesFile *dlcFile, StringTable *strings)
{
    bool levelGenAdded = false;
    bool gameRulesAdded = false;
    LevelGenerationOptions* levelGenerator =
        lgo;  // new LevelGenerationOptions();
    LevelRuleset* gameRules = new LevelRuleset();

    // std::uint32_t dataLength = 0;
    // std::uint8_t *data = dlcFile->getData(dataLength);
    // std::vector<uint8_t> data(pbData,dwLen);

    std::vector<uint8_t> data(dIn, dIn + dSize);
    ByteArrayInputStream bais(data);
    DataInputStream dis(&bais);

    // Read File.

    // version_number
    int64_t version = dis.readShort();
    unsigned char compressionType = 0;
    if (version == 0) {
        for (int i = 0; i < 14; i++) dis.readByte();  // Read padding.
    } else {
        compressionType = dis.readByte();

        // Read the spare bytes we inserted for future use
        for (int i = 0; i < 8; ++i) dis.readBoolean();
    }

    ByteArrayInputStream* contentBais = nullptr;
    DataInputStream* contentDis = nullptr;

    if (compressionType == Compression::eCompressionType_None) {
        // No compression
        // No need to read buffer size, as we can read the stream as it is;
        app.DebugPrintf("De-compressing game rules with: None\n");
        contentDis = &dis;
    } else {
        unsigned int uncompressedSize = dis.readInt();
        unsigned int compressedSize = dis.readInt();
        std::vector<uint8_t> compressedBuffer(compressedSize);
        dis.read(compressedBuffer);

        std::vector<uint8_t> decompressedBuffer =
            std::vector<uint8_t>(uncompressedSize);
        unsigned int decompressedSize = uncompressedSize;

        switch (compressionType) {
            case Compression::eCompressionType_None:
                memcpy(decompressedBuffer.data(), compressedBuffer.data(),
                       uncompressedSize);
                break;

            case Compression::eCompressionType_RLE:
                app.DebugPrintf("De-compressing game rules with: RLE\n");
                Compression::getCompression()->Decompress(
                    decompressedBuffer.data(), &decompressedSize,
                    compressedBuffer.data(), compressedSize);
                decompressedBuffer.resize(decompressedSize);
                break;

            default:
                app.DebugPrintf("De-compressing game rules.");
#if !defined(_CONTENT_PACKAGE)
                assert(compressionType == APPROPRIATE_COMPRESSION_TYPE);
#endif
                // 4J-JEV: DecompressLZXRLE uses the correct platform specific
                // compression type. (need to assert that the data is compressed
                // with it though).
                Compression::getCompression()->DecompressLZXRLE(
                    decompressedBuffer.data(), &decompressedSize,
                    compressedBuffer.data(), compressedSize);
                decompressedBuffer.resize(decompressedSize);
                break;
                /* 4J-JEV:
                        Each platform has only 1 method of compression,
                   'compression.h' file deals with it.

                                case Compression::eCompressionType_LZXRLE:
                                        app.DebugPrintf("De-compressing game
                   rules with: LZX+RLE\n");
                                        Compression::getCompression()->DecompressLZXRLE(
                   decompressedBuffer.data(), &uncompressedSize,
                   compressedBuffer.data(), compressedSize); break; default:
                                        app.DebugPrintf("Invalid compression
                   type %d found\n", compressionType);
                                        assert(0);

                   [] decompressedBuffer.data(); dis.close(); bais.reset();

                                        if(!gameRulesAdded) delete gameRules;
                                        return false;
                                        */
        };

        contentBais = new ByteArrayInputStream(decompressedBuffer);
        contentDis = new DataInputStream(contentBais);
    }

    // string lookup.
    unsigned int numStrings = contentDis->readInt();
    std::vector<std::string> tagsAndAtts;
    for (unsigned int i = 0; i < numStrings; i++)
        tagsAndAtts.push_back(contentDis->readUTF());

    std::unordered_map<int, ConsoleGameRules::EGameRuleType> tagIdMap;
    for (int type = (int)ConsoleGameRules::eGameRuleType_Root;
         type < (int)ConsoleGameRules::eGameRuleType_Count; ++type) {
        for (unsigned int i = 0; i < numStrings; ++i) {
            if (tagsAndAtts[i].compare(wchTagNameA[type]) == 0) {
                tagIdMap.insert(
                    std::unordered_map<int, ConsoleGameRules::EGameRuleType>::
                        value_type(i, (ConsoleGameRules::EGameRuleType)type));
                break;
            }
        }
    }

    // 4J-JEV: TODO: As yet unused.
    /*
    std::unordered_map<int, ConsoleGameRules::EGameRuleAttr> attrIdMap;
    for(int attr = (int)ConsoleGameRules::eGameRuleAttr_descriptionName; attr <
    (int)ConsoleGameRules::eGameRuleAttr_Count; ++attr)
    {
            for (unsigned int i = 0; i < numStrings; i++)
            {
                    if (tagsAndAtts[i].compare(wchAttrNameA[attr]) == 0)
                    {
                            tagIdMap.insert( std::unordered_map<int,
    ConsoleGameRules::EGameRuleAttr>::value_type(i ,
    (ConsoleGameRules::EGameRuleAttr)attr) ); break;
                    }
            }
    }*/

    // subfile
    unsigned int numFiles = contentDis->readInt();
    for (unsigned int i = 0; i < numFiles; i++) {
        std::string sFilename = contentDis->readUTF();
        int length = contentDis->readInt();
        std::vector<uint8_t> ba(length);

        contentDis->read(ba);

        levelGenerator->loadSchematicFile(sFilename, ba.data(), ba.size());
    }

    LEVEL_GEN_ID lgoID = LEVEL_GEN_ID_NULL;

    // xml objects
    unsigned int numObjects = contentDis->readInt();
    for (unsigned int i = 0; i < numObjects; ++i) {
        int tagId = contentDis->readInt();
        ConsoleGameRules::EGameRuleType tagVal =
            ConsoleGameRules::eGameRuleType_Invalid;
        auto it = tagIdMap.find(tagId);
        if (it != tagIdMap.end()) tagVal = it->second;

        GameRuleDefinition* rule = nullptr;

        if (tagVal == ConsoleGameRules::eGameRuleType_LevelGenerationOptions) {
            rule = levelGenerator;
            levelGenAdded = true;
            // m_levelGenerators.addLevelGenerator("",levelGenerator);
            lgoID = addLevelGenerationOptions(levelGenerator);
            levelGenerator->loadStringTable(strings);
        } else if (tagVal == ConsoleGameRules::eGameRuleType_LevelRules) {
            rule = gameRules;
            gameRulesAdded = true;
            m_levelRules.addLevelRule("", gameRules);
            levelGenerator->setRequiredGameRules(gameRules);
            gameRules->loadStringTable(strings);
        }

        readAttributes(contentDis, &tagsAndAtts, rule);
        readChildren(contentDis, &tagsAndAtts, &tagIdMap, rule);
    }

    if (compressionType != 0) {
        // Not default
        contentDis->close();
        if (contentBais != nullptr) delete contentBais;
        delete contentDis;
    }

    dis.close();
    bais.reset();

    // if(!levelGenAdded) { delete levelGenerator; levelGenerator = nullptr; }
    if (!gameRulesAdded) delete gameRules;

    return true;
    // return levelGenerator;
}

LevelGenerationOptions* GameRuleManager::readHeader(DLCGameRulesHeader* grh) {
    LevelGenerationOptions* out = new LevelGenerationOptions();

    out->setSrc(LevelGenerationOptions::eSrc_fromDLC);
    out->setGrSource(grh);
    addLevelGenerationOptions(out);

    return out;
}

void GameRuleManager::readAttributes(DataInputStream* dis,
                                     std::vector<std::string>* tagsAndAtts,
                                     GameRuleDefinition* rule) {
    int numAttrs = dis->readInt();
    for (unsigned int att = 0; att < static_cast<unsigned int>(numAttrs);
         ++att) {
        int attID = dis->readInt();
        std::string value = dis->readUTF();

        if (rule != nullptr) rule->addAttribute(tagsAndAtts->at(attID), value);
    }
}

void GameRuleManager::readChildren(
    DataInputStream* dis, std::vector<std::string>* tagsAndAtts,
    std::unordered_map<int, ConsoleGameRules::EGameRuleType>* tagIdMap,
    GameRuleDefinition* rule) {
    int numChildren = dis->readInt();
    for (unsigned int child = 0; child < static_cast<unsigned int>(numChildren);
         ++child) {
        int tagId = dis->readInt();
        ConsoleGameRules::EGameRuleType tagVal =
            ConsoleGameRules::eGameRuleType_Invalid;
        auto it = tagIdMap->find(tagId);
        if (it != tagIdMap->end()) tagVal = it->second;

        GameRuleDefinition* childRule = nullptr;
        if (rule != nullptr) childRule = rule->addChild(tagVal);

        readAttributes(dis, tagsAndAtts, childRule);
        readChildren(dis, tagsAndAtts, tagIdMap, childRule);
    }
}

void GameRuleManager::processSchematics(LevelChunk* levelChunk) {
    if (getLevelGenerationOptions() != nullptr) {
        LevelGenerationOptions* levelGenOptions = getLevelGenerationOptions();
        levelGenOptions->processSchematics(levelChunk);
    }
}

void GameRuleManager::processSchematicsLighting(LevelChunk* levelChunk) {
    if (getLevelGenerationOptions() != nullptr) {
        LevelGenerationOptions* levelGenOptions = getLevelGenerationOptions();
        levelGenOptions->processSchematicsLighting(levelChunk);
    }
}

void GameRuleManager::loadDefaultGameRules() {
#if !defined(__linux__)
#if defined(_WINDOWS64)
    File packedTutorialFile("Windows64Media\\Tutorial\\Tutorial.pck");
    if (!packedTutorialFile.exists())
        packedTutorialFile = File("Windows64\\Tutorial\\Tutorial.pck");
#else
    File packedTutorialFile("Tutorial\\Tutorial.pck");
#endif
    if (loadGameRulesPack(&packedTutorialFile)) {
        m_levelGenerators.getLevelGenerators()->at(0)->setWorldName(
            app.GetString(IDS_PLAY_TUTORIAL));
        // m_levelGenerators.getLevelGenerators()->at(0)->setDefaultSaveName("Tutorial");
        m_levelGenerators.getLevelGenerators()->at(0)->setDefaultSaveName(
            app.GetString(IDS_TUTORIALSAVENAME));
    }
#else
    std::string fpTutorial = "Tutorial.pck";
    if (app.getArchiveFileSize(fpTutorial) >= 0) {
        DLCPack* pack = new DLCPack("", 0xffffffff);
        uint32_t dwFilesProcessed = 0;
        if (app.m_dlcManager.readDLCDataFile(dwFilesProcessed, fpTutorial, pack,
                                             true)) {
            app.m_dlcManager.addPack(pack);
            m_levelGenerators.getLevelGenerators()->at(0)->setWorldName(
                app.GetString(IDS_PLAY_TUTORIAL));
            m_levelGenerators.getLevelGenerators()->at(0)->setDefaultSaveName(
                app.GetString(IDS_TUTORIALSAVENAME));
        } else
            delete pack;
    }
#endif
}

bool GameRuleManager::loadGameRulesPack(File* path) {
    bool success = false;
    if (path->exists()) {
        DLCPack* pack = new DLCPack("", 0xffffffff);
        unsigned int dwFilesProcessed = 0;
        if (app.m_dlcManager.readDLCDataFile(dwFilesProcessed, path->getPath(),
                                             pack)) {
            app.m_dlcManager.addPack(pack);
            success = true;
        } else {
            delete pack;
        }
    }
    return success;
}

void GameRuleManager::setLevelGenerationOptions(
    LevelGenerationOptions* levelGen) {
    unloadCurrentGameRules();

    m_currentGameRuleDefinitions = nullptr;
    m_currentLevelGenerationOptions = levelGen;

    if (m_currentLevelGenerationOptions != nullptr &&
        m_currentLevelGenerationOptions->requiresGameRules()) {
        m_currentGameRuleDefinitions =
            m_currentLevelGenerationOptions->getRequiredGameRules();
    }

    if (m_currentLevelGenerationOptions != nullptr)
        m_currentLevelGenerationOptions->reset_start();
}

const char* GameRuleManager::GetGameRulesString(const std::string& key) {
    if (m_currentGameRuleDefinitions != nullptr && !key.empty()) {
        return m_currentGameRuleDefinitions->getString(key);
    } else {
        return "";
    }
}

LEVEL_GEN_ID GameRuleManager::addLevelGenerationOptions(
    LevelGenerationOptions* lgo) {
    std::vector<LevelGenerationOptions*>* lgs =
        m_levelGenerators.getLevelGenerators();

    for (int i = 0; i < lgs->size(); i++)
        if (lgs->at(i) == lgo) return i;

    lgs->push_back(lgo);
    return lgs->size() - 1;
}

void GameRuleManager::unloadCurrentGameRules() {
    if (m_currentLevelGenerationOptions != nullptr) {
        if (m_currentGameRuleDefinitions != nullptr &&
            m_currentLevelGenerationOptions->isFromSave())
            m_levelRules.removeLevelRule(m_currentGameRuleDefinitions);

        if (m_currentLevelGenerationOptions->isFromSave()) {
            m_levelGenerators.removeLevelGenerator(
                m_currentLevelGenerationOptions);

            delete m_currentLevelGenerationOptions;
        } else if (m_currentLevelGenerationOptions->isFromDLC()) {
            m_currentLevelGenerationOptions->reset_finish();
        }
    }

    m_currentGameRuleDefinitions = nullptr;
    m_currentLevelGenerationOptions = nullptr;
}
