#include "LevelGenerationOptions.h"

#include <limits.h>
#include <wchar.h>

#include <unordered_set>
#include <utility>

#include "minecraft/GameEnums.h"
#include "app/common/DLC/DLCGameRulesHeader.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/common/GameRules/LevelGeneration/ApplySchematicRuleDefinition.h"
#include "app/common/GameRules/LevelGeneration/BiomeOverride.h"
#include "app/common/GameRules/LevelGeneration/ConsoleGenerateStructure.h"
#include "app/common/GameRules/LevelGeneration/ConsoleSchematicFile.h"
#include "app/common/GameRules/LevelGeneration/StartFeature.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "minecraft/locale/StringTable.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "java/File.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/Pos.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/levelgen/structure/BoundingBox.h"
#include "minecraft/world/phys/AABB.h"
#include "platform/fs/fs.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"
#include "strings.h"
#include "util/StringHelpers.h"

JustGrSource::JustGrSource() {
    m_displayName = "Default_DisplayName";
    m_worldName = "Default_WorldName";
    m_defaultSaveName = "Default_DefaultSaveName";
    m_bRequiresTexturePack = false;
    m_requiredTexturePackId = 0;
    m_grfPath = "__NO_GRF_PATH__";
    m_bRequiresBaseSave = false;
}

bool JustGrSource::requiresTexturePack() { return m_bRequiresTexturePack; }
std::uint32_t JustGrSource::getRequiredTexturePackId() {
    return m_requiredTexturePackId;
}
std::string JustGrSource::getDefaultSaveName() { return m_defaultSaveName; }
const char* JustGrSource::getWorldName() { return m_worldName.c_str(); }
const char* JustGrSource::getDisplayName() { return m_displayName.c_str(); }
std::string JustGrSource::getGrfPath() { return m_grfPath; }
bool JustGrSource::requiresBaseSave() { return m_bRequiresBaseSave; };
std::string JustGrSource::getBaseSavePath() { return m_baseSavePath; };

void JustGrSource::setRequiresTexturePack(bool x) {
    m_bRequiresTexturePack = x;
}
void JustGrSource::setRequiredTexturePackId(std::uint32_t x) {
    m_requiredTexturePackId = x;
}
void JustGrSource::setDefaultSaveName(const std::string& x) {
    m_defaultSaveName = x;
}
void JustGrSource::setWorldName(const std::string& x) { m_worldName = x; }
void JustGrSource::setDisplayName(const std::string& x) { m_displayName = x; }
void JustGrSource::setGrfPath(const std::string& x) { m_grfPath = x; }
void JustGrSource::setBaseSavePath(const std::string& x) {
    m_baseSavePath = x;
    m_bRequiresBaseSave = true;
}

bool JustGrSource::ready() { return true; }

LevelGenerationOptions::LevelGenerationOptions(DLCPack* parentPack) {
    m_spawnPos = nullptr;
    m_stringTable = nullptr;

    m_hasLoadedData = false;

    m_seed = 0;
    m_bHasBeenInCreative = true;
    m_useFlatWorld = false;
    m_bHaveMinY = false;
    m_minY = INT_MAX;
    m_bRequiresGameRules = false;

    m_pbBaseSaveData = nullptr;
    m_baseSaveSize = 0;

    m_parentDLCPack = parentPack;
    m_bLoadingData = false;
}

LevelGenerationOptions::~LevelGenerationOptions() {
    clearSchematics();
    if (m_spawnPos != nullptr) delete m_spawnPos;
    for (auto it = m_schematicRules.begin(); it != m_schematicRules.end();
         ++it) {
        delete *it;
    }
    for (auto it = m_structureRules.begin(); it != m_structureRules.end();
         ++it) {
        delete *it;
    }

    for (auto it = m_biomeOverrides.begin(); it != m_biomeOverrides.end();
         ++it) {
        delete *it;
    }

    for (auto it = m_features.begin(); it != m_features.end(); ++it) {
        delete *it;
    }

    if (m_stringTable)
        if (!isTutorial()) delete m_stringTable;

    if (isFromSave()) delete m_pSrc;
}

ConsoleGameRules::EGameRuleType LevelGenerationOptions::getActionType() {
    return ConsoleGameRules::eGameRuleType_LevelGenerationOptions;
}

void LevelGenerationOptions::writeAttributes(DataOutputStream* dos,
                                             unsigned int numAttrs) {
    GameRuleDefinition::writeAttributes(dos, numAttrs + 5);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_spawnX);
    dos->writeUTF(toWString(m_spawnPos->x));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_spawnY);
    dos->writeUTF(toWString(m_spawnPos->y));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_spawnZ);
    dos->writeUTF(toWString(m_spawnPos->z));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_seed);
    dos->writeUTF(toWString(m_seed));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_flatworld);
    dos->writeUTF(toWString(m_useFlatWorld));
}

void LevelGenerationOptions::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    GameRuleDefinition::getChildren(children);

    std::vector<ApplySchematicRuleDefinition*> used_schematics;
    for (auto it = m_schematicRules.begin(); it != m_schematicRules.end(); it++)
        if (!(*it)->isComplete()) used_schematics.push_back(*it);

    for (auto it = m_structureRules.begin(); it != m_structureRules.end(); it++)
        children->push_back(*it);
    for (auto it = used_schematics.begin(); it != used_schematics.end(); it++)
        children->push_back(*it);
    for (auto it = m_biomeOverrides.begin(); it != m_biomeOverrides.end(); ++it)
        children->push_back(*it);
    for (auto it = m_features.begin(); it != m_features.end(); ++it)
        children->push_back(*it);
}

GameRuleDefinition* LevelGenerationOptions::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_ApplySchematic) {
        rule = new ApplySchematicRuleDefinition(this);
        m_schematicRules.push_back((ApplySchematicRuleDefinition*)rule);
    } else if (ruleType == ConsoleGameRules::eGameRuleType_GenerateStructure) {
        rule = new ConsoleGenerateStructure();
        m_structureRules.push_back((ConsoleGenerateStructure*)rule);
    } else if (ruleType == ConsoleGameRules::eGameRuleType_BiomeOverride) {
        rule = new BiomeOverride();
        m_biomeOverrides.push_back((BiomeOverride*)rule);
    } else if (ruleType == ConsoleGameRules::eGameRuleType_StartFeature) {
        rule = new StartFeature();
        m_features.push_back((StartFeature*)rule);
    } else {
#if !defined(_CONTENT_PACKAGE)
        printf(
            "LevelGenerationOptions: Attempted to add invalid child rule - "
            "%d\n",
            ruleType);
#endif
    }
    return rule;
}

void LevelGenerationOptions::addAttribute(const std::string& attributeName,
                                          const std::string& attributeValue) {
    if (attributeName.compare("seed") == 0) {
        m_seed = fromWString<int64_t>(attributeValue);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter m_seed=%I64d\n", m_seed);
    } else if (attributeName.compare("spawnX") == 0) {
        if (m_spawnPos == nullptr) m_spawnPos = new Pos();
        int value = fromWString<int>(attributeValue);
        m_spawnPos->x = value;
        app.DebugPrintf("LevelGenerationOptions: Adding parameter spawnX=%d\n",
                        value);
    } else if (attributeName.compare("spawnY") == 0) {
        if (m_spawnPos == nullptr) m_spawnPos = new Pos();
        int value = fromWString<int>(attributeValue);
        m_spawnPos->y = value;
        app.DebugPrintf("LevelGenerationOptions: Adding parameter spawnY=%d\n",
                        value);
    } else if (attributeName.compare("spawnZ") == 0) {
        if (m_spawnPos == nullptr) m_spawnPos = new Pos();
        int value = fromWString<int>(attributeValue);
        m_spawnPos->z = value;
        app.DebugPrintf("LevelGenerationOptions: Adding parameter spawnZ=%d\n",
                        value);
    } else if (attributeName.compare("flatworld") == 0) {
        if (attributeValue.compare("true") == 0) m_useFlatWorld = true;
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter flatworld=%s\n",
            m_useFlatWorld ? "true" : "false");
    } else if (attributeName.compare("saveName") == 0) {
        std::string string(getString(attributeValue));
        if (!string.empty())
            setDefaultSaveName(string);
        else
            setDefaultSaveName(attributeValue);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter saveName=%s\n",
            getDefaultSaveName().c_str());
    } else if (attributeName.compare("worldName") == 0) {
        std::string string(getString(attributeValue));
        if (!string.empty())
            setWorldName(string);
        else
            setWorldName(attributeValue);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter worldName=%s\n",
            getWorldName());
    } else if (attributeName.compare("displayName") == 0) {
        std::string string(getString(attributeValue));
        if (!string.empty())
            setDisplayName(string);
        else
            setDisplayName(attributeValue);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter displayName=%s\n",
            getDisplayName());
    } else if (attributeName.compare("texturePackId") == 0) {
        setRequiredTexturePackId(fromWString<unsigned int>(attributeValue));
        setRequiresTexturePack(true);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter texturePackId=%0x\n",
            getRequiredTexturePackId());
    } else if (attributeName.compare("isTutorial") == 0) {
        if (attributeValue.compare("true") == 0) setSrc(eSrc_tutorial);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter isTutorial=%s\n",
            isTutorial() ? "true" : "false");
    } else if (attributeName.compare("baseSaveName") == 0) {
        setBaseSavePath(attributeValue);
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter baseSaveName=%s\n",
            getBaseSavePath().c_str());
    } else if (attributeName.compare("hasBeenInCreative") == 0) {
        bool value = fromWString<bool>(attributeValue);
        m_bHasBeenInCreative = value;
        app.DebugPrintf(
            "LevelGenerationOptions: Adding parameter gameMode=%d\n",
            m_bHasBeenInCreative);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}
// 4jcraft: better schematic caching
void LevelGenerationOptions::processSchematics(LevelChunk* chunk) {
    AABB chunkBox(chunk->x * 16, 0, chunk->z * 16, chunk->x * 16 + 16,
                  Level::maxBuildHeight, chunk->z * 16 + 16);

    ChunkRuleCacheKey key;
    key.chunkX = chunk->x;
    key.chunkZ = chunk->z;
    key.dimension = chunk->level->dimension->id;

    auto cacheIt = m_chunkRuleCache.find(key);
    if (cacheIt == m_chunkRuleCache.end()) {
        // if no cache hit, show em the goods
        ChunkRuleCacheEntry entry;
        for (auto it = m_schematicRules.begin(); it != m_schematicRules.end();
             ++it) {
            ApplySchematicRuleDefinition* rule = *it;
            if (rule->checkIntersects(chunkBox.x0, chunkBox.y0, chunkBox.z0,
                                      chunkBox.x1, chunkBox.y1, chunkBox.z1)) {
                entry.schematicRules.push_back(rule);
            }
        }

        int cx = (chunk->x << 4);
        int cz = (chunk->z << 4);
        for (auto it = m_structureRules.begin(); it != m_structureRules.end();
             ++it) {
            ConsoleGenerateStructure* structureStart = *it;
            if (structureStart->getBoundingBox()->intersects(cx, cz, cx + 15,
                                                             cz + 15)) {
                entry.structureRules.push_back(structureStart);
            }
        }

        cacheIt = m_chunkRuleCache
                      .insert(std::pair<ChunkRuleCacheKey, ChunkRuleCacheEntry>(
                          key, entry))
                      .first;
    } else if (cacheIt->second.structureRules.empty() &&
               !m_structureRules.empty()) {
        int cx = (chunk->x << 4);
        int cz = (chunk->z << 4);
        for (auto it = m_structureRules.begin(); it != m_structureRules.end();
             ++it) {
            ConsoleGenerateStructure* structureStart = *it;
            if (structureStart->getBoundingBox()->intersects(cx, cz, cx + 15,
                                                             cz + 15)) {
                cacheIt->second.structureRules.push_back(structureStart);
            }
        }
    }

    for (auto it = cacheIt->second.schematicRules.begin();
         it != cacheIt->second.schematicRules.end(); ++it) {
        (*it)->processSchematic(&chunkBox, chunk);
    }

    int cx = (chunk->x << 4);
    int cz = (chunk->z << 4);

    for (auto it = cacheIt->second.structureRules.begin();
         it != cacheIt->second.structureRules.end(); ++it) {
        ConsoleGenerateStructure* structureStart = *it;
        BoundingBox* bb = new BoundingBox(cx, cz, cx + 15, cz + 15);
        structureStart->postProcess(chunk->level, nullptr, bb);
        delete bb;
    }
}

void LevelGenerationOptions::processSchematicsLighting(LevelChunk* chunk) {
    AABB chunkBox(chunk->x * 16, 0, chunk->z * 16, chunk->x * 16 + 16,
                  Level::maxBuildHeight, chunk->z * 16 + 16);

    ChunkRuleCacheKey key;
    key.chunkX = chunk->x;
    key.chunkZ = chunk->z;
    key.dimension = chunk->level->dimension->id;

    auto cacheIt = m_chunkRuleCache.find(key);
    if (cacheIt == m_chunkRuleCache.end()) {
        // lighting shouldn't affect structure rules...
        ChunkRuleCacheEntry entry;
        for (auto it = m_schematicRules.begin(); it != m_schematicRules.end();
             ++it) {
            ApplySchematicRuleDefinition* rule = *it;
            if (rule->checkIntersects(chunkBox.x0, chunkBox.y0, chunkBox.z0,
                                      chunkBox.x1, chunkBox.y1, chunkBox.z1)) {
                entry.schematicRules.push_back(rule);
            }
        }
        // structureRules is initially empty because it will be populated by
        // processSchematics later onn

        cacheIt = m_chunkRuleCache
                      .insert(std::pair<ChunkRuleCacheKey, ChunkRuleCacheEntry>(
                          key, entry))
                      .first;
    }

    for (auto it = cacheIt->second.schematicRules.begin();
         it != cacheIt->second.schematicRules.end(); ++it) {
        (*it)->processSchematicLighting(&chunkBox, chunk);
    }
}

bool LevelGenerationOptions::checkIntersects(int x0, int y0, int z0, int x1,
                                             int y1, int z1) {
    // As an optimisation, we can quickly discard things below a certain y which
    // makes most ore checks faster due to a) ores generally being below
    // ground/sea level and b) tutorial world additions generally being above
    // ground/sea level
    if (!m_bHaveMinY) {
        for (auto it = m_schematicRules.begin(); it != m_schematicRules.end();
             ++it) {
            ApplySchematicRuleDefinition* rule = *it;
            int minY = rule->getMinY();
            if (minY < m_minY) m_minY = minY;
        }

        for (auto it = m_structureRules.begin(); it != m_structureRules.end();
             it++) {
            ConsoleGenerateStructure* structureStart = *it;
            int minY = structureStart->getMinY();
            if (minY < m_minY) m_minY = minY;
        }

        m_bHaveMinY = true;
    }

    // 4J Stu - We DO NOT intersect if our upper bound is below the lower bound
    // for all schematics
    if (y1 < m_minY) return false;

    bool intersects = false;
    for (auto it = m_schematicRules.begin(); it != m_schematicRules.end();
         ++it) {
        ApplySchematicRuleDefinition* rule = *it;
        intersects = rule->checkIntersects(x0, y0, z0, x1, y1, z1);
        if (intersects) break;
    }

    if (!intersects) {
        for (auto it = m_structureRules.begin(); it != m_structureRules.end();
             it++) {
            ConsoleGenerateStructure* structureStart = *it;
            intersects =
                structureStart->checkIntersects(x0, y0, z0, x1, y1, z1);
            if (intersects) break;
        }
    }

    return intersects;
}

void LevelGenerationOptions::clearSchematics() {
    for (auto it = m_schematics.begin(); it != m_schematics.end(); ++it) {
        delete it->second;
    }
    m_schematics.clear();
    clearChunkRuleCache();
}

void LevelGenerationOptions::clearChunkRuleCache() { m_chunkRuleCache.clear(); }

ConsoleSchematicFile* LevelGenerationOptions::loadSchematicFile(
    const std::string& filename, std::uint8_t* pbData,
    unsigned int dataLength) {
    // If we have already loaded this, just return
    auto it = m_schematics.find(filename);
    if (it != m_schematics.end()) {
#if !defined(_CONTENT_PACKAGE)
        printf("We have already loaded schematic file %s\n",
                filename.c_str());
#endif
        it->second->incrementRefCount();
        return it->second;
    }

    ConsoleSchematicFile* schematic = nullptr;
    // 4jcraft: we use a constructor to reduce copies.
    std::vector<uint8_t> data(pbData, pbData + dataLength);
    ByteArrayInputStream bais(std::move(data));
    DataInputStream dis(&bais);
    schematic = new ConsoleSchematicFile();
    schematic->load(&dis);
    m_schematics[filename] = schematic;
    bais.reset();
    return schematic;
}

ConsoleSchematicFile* LevelGenerationOptions::getSchematicFile(
    const std::string& filename) {
    ConsoleSchematicFile* schematic = nullptr;
    // If we have already loaded this, just return
    auto it = m_schematics.find(filename);
    if (it != m_schematics.end()) {
        schematic = it->second;
    }
    return schematic;
}

void LevelGenerationOptions::releaseSchematicFile(
    const std::string& filename) {
    // 4J Stu - We don't want to delete them when done, but probably want to
    // keep a set of active schematics for the current world
    // auto it = m_schematics.find(filename);
    // if(it != m_schematics.end())
    //{
    //	ConsoleSchematicFile *schematic = it->second;
    //	schematic->decrementRefCount();
    //	if(schematic->shouldDelete())
    //	{
    //		delete schematic;
    //		m_schematics.erase(it);
    //	}
    //}
}

void LevelGenerationOptions::loadStringTable(StringTable* table) {
    m_stringTable = table;
}

const char* LevelGenerationOptions::getString(const std::string& key) {
    if (m_stringTable == nullptr) {
        return "";
    } else {
        return m_stringTable->getString(key);
    }
}

void LevelGenerationOptions::getBiomeOverride(int biomeId, std::uint8_t& tile,
                                              std::uint8_t& topTile) {
    for (auto it = m_biomeOverrides.begin(); it != m_biomeOverrides.end();
         ++it) {
        BiomeOverride* bo = *it;
        if (bo->isBiome(biomeId)) {
            bo->getTileValues(tile, topTile);
            break;
        }
    }
}

bool LevelGenerationOptions::isFeatureChunk(
    int chunkX, int chunkZ, StructureFeature::EFeatureTypes feature,
    int* orientation) {
    bool isFeature = false;

    for (auto it = m_features.begin(); it != m_features.end(); ++it) {
        StartFeature* sf = *it;
        if (sf->isFeatureChunk(chunkX, chunkZ, feature, orientation)) {
            isFeature = true;
            break;
        }
    }
    return isFeature;
}

std::unordered_map<std::string, ConsoleSchematicFile*>*
LevelGenerationOptions::getUnfinishedSchematicFiles() {
    // Clean schematic rules.
    std::unordered_set<std::string> usedFiles =
        std::unordered_set<std::string>();
    for (auto it = m_schematicRules.begin(); it != m_schematicRules.end(); it++)
        if (!(*it)->isComplete()) usedFiles.insert((*it)->getSchematicName());

    // Clean schematic files.
    std::unordered_map<std::string, ConsoleSchematicFile*>* out =
        new std::unordered_map<std::string, ConsoleSchematicFile*>();
    for (auto it = usedFiles.begin(); it != usedFiles.end(); it++)
        out->insert(std::pair<std::string, ConsoleSchematicFile*>(
            *it, getSchematicFile(*it)));

    return out;
}

void LevelGenerationOptions::loadBaseSaveData() {
    int mountIndex = -1;
    if (m_parentDLCPack != nullptr)
        mountIndex = m_parentDLCPack->GetDLCMountIndex();

    if (mountIndex > -1) {
        if (PlatformStorage.MountInstalledDLC(
                PlatformProfile.GetPrimaryPad(), mountIndex,
                [this](int pad, std::uint32_t err, std::uint32_t lic) {
                    return onPackMounted(pad, err, lic);
                },
                "WPACK") != ERROR_IO_PENDING) {
            // corrupt DLC
            setLoadedData();
            app.DebugPrintf("Failed to mount LGO DLC %d for pad %d\n",
                            mountIndex, PlatformProfile.GetPrimaryPad());
        } else {
            m_bLoadingData = true;
            app.DebugPrintf("Attempted to mount DLC data for LGO %d\n",
                            mountIndex);
        }
    } else {
        setLoadedData();
        app.SetAction(PlatformProfile.GetPrimaryPad(),
                      eAppAction_ReloadTexturePack);
    }
}

int LevelGenerationOptions::onPackMounted(int iPad, uint32_t dwErr,
                                          uint32_t dwLicenceMask) {
    LevelGenerationOptions* lgo = this;
    lgo->m_bLoadingData = false;
    if (dwErr != ERROR_SUCCESS) {
        // corrupt DLC
        app.DebugPrintf("Failed to mount LGO DLC for pad %d: %d\n", iPad,
                        dwErr);
    } else {
        app.DebugPrintf("Mounted DLC for LGO, attempting to load data\n");
        uint32_t dwFilesProcessed = 0;
        int gameRulesCount = lgo->m_parentDLCPack->getDLCItemsCount(
            DLCManager::e_DLCType_GameRulesHeader);
        for (int i = 0; i < gameRulesCount; ++i) {
            DLCGameRulesHeader* dlcFile =
                (DLCGameRulesHeader*)lgo->m_parentDLCPack->getFile(
                    DLCManager::e_DLCType_GameRulesHeader, i);

            if (!dlcFile->getGrfPath().empty()) {
                File grf(app.getFilePath(lgo->m_parentDLCPack->GetPackID(),
                                         dlcFile->getGrfPath(), true,
                                         "WPACK:"));
                if (grf.exists()) {
                    uint32_t dwFileSize = grf.length();
                    if (dwFileSize > 0) {
                        uint8_t* pbData = (uint8_t*)new uint8_t[dwFileSize];
                        auto readResult = PlatformFilesystem.readFile(
                            grf.getPath(), pbData, dwFileSize);
                        if (readResult.status !=
                            IPlatformFilesystem::ReadStatus::Ok) {
                            app.FatalLoadError();
                        }

                        // 4J-PB - is it possible that we can get here after a
                        // read fail and it's not an error?
                        dlcFile->setGrfData(pbData, dwFileSize,
                                            lgo->m_stringTable);

                        delete[] pbData;

                        app.m_gameRules.setLevelGenerationOptions(dlcFile->lgo);
                    }
                }
            }
        }
        if (lgo->requiresBaseSave() && !lgo->getBaseSavePath().empty()) {
            File save(app.getFilePath(lgo->m_parentDLCPack->GetPackID(),
                                      lgo->getBaseSavePath(), true, "WPACK:"));
            if (save.exists()) {
                std::size_t dwFileSize =
                    PlatformFilesystem.fileSize(save.getPath());
                if (dwFileSize > 0) {
                    uint8_t* pbData = (uint8_t*)new uint8_t[dwFileSize];
                    auto readResult = PlatformFilesystem.readFile(
                        save.getPath(), pbData, dwFileSize);
                    if (readResult.status != IPlatformFilesystem::ReadStatus::Ok) {
                        app.FatalLoadError();
                    }

                    // 4J-PB - is it possible that we can get here after a read
                    // fail and it's not an error?
                    lgo->setBaseSaveData(pbData, dwFileSize);
                }
            }
        }
        uint32_t result = PlatformStorage.UnmountInstalledDLC("WPACK");
    }

    lgo->setLoadedData();

    return 0;
}

void LevelGenerationOptions::reset_start() {
    clearChunkRuleCache();
    for (auto it = m_schematicRules.begin(); it != m_schematicRules.end();
         ++it) {  // what in the flip in the fuck
        (*it)->reset();
    }
}

void LevelGenerationOptions::reset_finish() {
    clearChunkRuleCache();
    // if (m_spawnPos)				{ delete m_spawnPos; m_spawnPos
    // = nullptr; } if (m_stringTable)			{ delete m_stringTable;
    // m_stringTable = nullptr; }

    if (isFromDLC()) {
        m_hasLoadedData = false;
    }
}

GrSource* LevelGenerationOptions::info() { return m_pSrc; }
void LevelGenerationOptions::setSrc(eSrc src) { m_src = src; }
LevelGenerationOptions::eSrc LevelGenerationOptions::getSrc() { return m_src; }

bool LevelGenerationOptions::isTutorial() { return getSrc() == eSrc_tutorial; }
bool LevelGenerationOptions::isFromSave() { return getSrc() == eSrc_fromSave; }
bool LevelGenerationOptions::isFromDLC() { return getSrc() == eSrc_fromDLC; }

bool LevelGenerationOptions::requiresTexturePack() {
    return info()->requiresTexturePack();
}
std::uint32_t LevelGenerationOptions::getRequiredTexturePackId() {
    return info()->getRequiredTexturePackId();
}
std::string LevelGenerationOptions::getDefaultSaveName() {
    switch (getSrc()) {
        case eSrc_fromSave:
            return getString(info()->getDefaultSaveName());
        case eSrc_fromDLC:
            return getString(info()->getDefaultSaveName());
        case eSrc_tutorial:
            return app.GetString(IDS_TUTORIALSAVENAME);
        default:
            break;
    }
    return "";
}
const char* LevelGenerationOptions::getWorldName() {
    switch (getSrc()) {
        case eSrc_fromSave:
            return getString(info()->getWorldName());
        case eSrc_fromDLC:
            return getString(info()->getWorldName());
        case eSrc_tutorial:
            return app.GetString(IDS_PLAY_TUTORIAL);
        default:
            break;
    }
    return "";
}
const char* LevelGenerationOptions::getDisplayName() {
    switch (getSrc()) {
        case eSrc_fromSave:
            return getString(info()->getDisplayName());
        case eSrc_fromDLC:
            return getString(info()->getDisplayName());
        case eSrc_tutorial:
            return "";
        default:
            break;
    }
    return "";
}
std::string LevelGenerationOptions::getGrfPath() {
    return info()->getGrfPath();
}
bool LevelGenerationOptions::requiresBaseSave() {
    return info()->requiresBaseSave();
}
std::string LevelGenerationOptions::getBaseSavePath() {
    return info()->getBaseSavePath();
}

void LevelGenerationOptions::setGrSource(GrSource* grs) { m_pSrc = grs; }

void LevelGenerationOptions::setRequiresTexturePack(bool x) {
    info()->setRequiresTexturePack(x);
}
void LevelGenerationOptions::setRequiredTexturePackId(std::uint32_t x) {
    info()->setRequiredTexturePackId(x);
}
void LevelGenerationOptions::setDefaultSaveName(const std::string& x) {
    info()->setDefaultSaveName(x);
}
void LevelGenerationOptions::setWorldName(const std::string& x) {
    info()->setWorldName(x);
}
void LevelGenerationOptions::setDisplayName(const std::string& x) {
    info()->setDisplayName(x);
}
void LevelGenerationOptions::setGrfPath(const std::string& x) {
    info()->setGrfPath(x);
}
void LevelGenerationOptions::setBaseSavePath(const std::string& x) {
    info()->setBaseSavePath(x);
}

bool LevelGenerationOptions::ready() { return info()->ready(); }

void LevelGenerationOptions::setBaseSaveData(std::uint8_t* pbData,
                                             unsigned int dataSize) {
    m_pbBaseSaveData = pbData;
    m_baseSaveSize = dataSize;
}
std::uint8_t* LevelGenerationOptions::getBaseSaveData(unsigned int& size) {
    size = m_baseSaveSize;
    return m_pbBaseSaveData;
}
bool LevelGenerationOptions::hasBaseSaveData() {
    return m_baseSaveSize > 0 && m_pbBaseSaveData != nullptr;
}
void LevelGenerationOptions::deleteBaseSaveData() {
    delete[] m_pbBaseSaveData;
    m_pbBaseSaveData = nullptr;
    m_baseSaveSize = 0;
}

bool LevelGenerationOptions::hasLoadedData() { return m_hasLoadedData; }
void LevelGenerationOptions::setLoadedData() { m_hasLoadedData = true; }

int64_t LevelGenerationOptions::getLevelSeed() { return m_seed; }
int LevelGenerationOptions::getLevelHasBeenInCreative() {
    return m_bHasBeenInCreative;
}
Pos* LevelGenerationOptions::getSpawnPos() { return m_spawnPos; }
bool LevelGenerationOptions::getuseFlatWorld() { return m_useFlatWorld; }

bool LevelGenerationOptions::requiresGameRules() {
    return m_bRequiresGameRules;
}
void LevelGenerationOptions::setRequiredGameRules(LevelRuleset* rules) {
    m_requiredGameRules = rules;
    m_bRequiresGameRules = true;
}
LevelRuleset* LevelGenerationOptions::getRequiredGameRules() {
    return m_requiredGameRules;
}
