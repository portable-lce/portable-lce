#pragma once
// using namespace std;

// #pragma message("LevelGenerationOptions.h ")

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/common/DLC/DLCPack.h"
#include "app/common/GameRules/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "minecraft/locale/StringTable.h"
#include "minecraft/world/level/levelgen/structure/StructureFeature.h"

class ApplySchematicRuleDefinition;
class LevelChunk;
class ConsoleGenerateStructure;
class ConsoleSchematicFile;
class LevelRuleset;
class BiomeOverride;
class StartFeature;
class DLCPack;
class Pos;
class StringTable;

class GrSource {
public:
    // 4J-JEV:
    // Moved all this here; I didn't like that all this header information
    // was being mixed in with all the game information as they have
    // completely different lifespans.

    virtual ~GrSource() {}
    virtual bool requiresTexturePack() = 0;
    virtual std::uint32_t getRequiredTexturePackId() = 0;
    virtual std::string getDefaultSaveName() = 0;
    virtual const char* getWorldName() = 0;
    virtual const char* getDisplayName() = 0;
    virtual std::string getGrfPath() = 0;
    virtual bool requiresBaseSave() = 0;
    virtual std::string getBaseSavePath() = 0;

    virtual void setRequiresTexturePack(bool) = 0;
    virtual void setRequiredTexturePackId(std::uint32_t) = 0;
    virtual void setDefaultSaveName(const std::string&) = 0;
    virtual void setWorldName(const std::string&) = 0;
    virtual void setDisplayName(const std::string&) = 0;
    virtual void setGrfPath(const std::string&) = 0;
    virtual void setBaseSavePath(const std::string&) = 0;

    virtual bool ready() = 0;

    // virtual void getGrfData(std::uint8_t *&pData, unsigned int &pSize)=0;
};

class JustGrSource : public GrSource {
protected:
    std::string m_worldName;
    std::string m_displayName;
    std::string m_defaultSaveName;
    bool m_bRequiresTexturePack;
    std::uint32_t m_requiredTexturePackId;
    std::string m_grfPath;
    std::string m_baseSavePath;
    bool m_bRequiresBaseSave;

public:
    virtual bool requiresTexturePack();
    virtual std::uint32_t getRequiredTexturePackId();
    virtual std::string getDefaultSaveName();
    virtual const char* getWorldName();
    virtual const char* getDisplayName();
    virtual std::string getGrfPath();
    virtual bool requiresBaseSave();
    virtual std::string getBaseSavePath();

    virtual void setRequiresTexturePack(bool x);
    virtual void setRequiredTexturePackId(std::uint32_t x);
    virtual void setDefaultSaveName(const std::string& x);
    virtual void setWorldName(const std::string& x);
    virtual void setDisplayName(const std::string& x);
    virtual void setGrfPath(const std::string& x);
    virtual void setBaseSavePath(const std::string& x);

    virtual bool ready();

    JustGrSource();
};

class LevelGenerationOptions : public GameRuleDefinition {
public:
    enum eSrc {
        eSrc_none,

        eSrc_fromSave,  // Neither content or header is persistent.

        eSrc_fromDLC,  // Header is persistent, content should be deleted to
                       // conserve space.

        eSrc_tutorial,  // Both header and content is persistent, content cannot
                        // be reloaded.

        eSrc_MAX
    };

private:
    struct ChunkRuleCacheKey {
        int chunkX;
        int chunkZ;
        int dimension;

        bool operator==(const ChunkRuleCacheKey& other) const {
            return chunkX == other.chunkX && chunkZ == other.chunkZ &&
                   dimension == other.dimension;
        }
    };

    struct ChunkRuleCacheKeyHash {
        std::size_t operator()(const ChunkRuleCacheKey& key) const {
            std::size_t h1 = std::hash<int>()(key.chunkX);
            std::size_t h2 = std::hash<int>()(key.chunkZ);
            std::size_t h3 = std::hash<int>()(key.dimension);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    struct ChunkRuleCacheEntry {
        std::vector<ApplySchematicRuleDefinition*> schematicRules;
        std::vector<ConsoleGenerateStructure*> structureRules;
    };

    eSrc m_src;

    GrSource* m_pSrc;
    GrSource* info();

    bool m_hasLoadedData;

    std::uint8_t* m_pbBaseSaveData;
    unsigned int m_baseSaveSize;

public:
    void setSrc(eSrc src);
    eSrc getSrc();

    bool isTutorial();
    bool isFromSave();
    bool isFromDLC();

    bool requiresTexturePack();
    std::uint32_t getRequiredTexturePackId();
    std::string getDefaultSaveName();
    const char* getWorldName();
    const char* getDisplayName();
    std::string getGrfPath();
    bool requiresBaseSave();
    std::string getBaseSavePath();

    void setGrSource(GrSource* grs);

    void setRequiresTexturePack(bool x);
    void setRequiredTexturePackId(std::uint32_t x);
    void setDefaultSaveName(const std::string& x);
    void setWorldName(const std::string& x);
    void setDisplayName(const std::string& x);
    void setGrfPath(const std::string& x);
    void setBaseSavePath(const std::string& x);

    bool ready();

    void setBaseSaveData(std::uint8_t* pbData, unsigned int dataSize);
    std::uint8_t* getBaseSaveData(unsigned int& size);
    bool hasBaseSaveData();
    void deleteBaseSaveData();

    bool hasLoadedData();
    void setLoadedData();

private:
    // This should match the "MapOptionsRule" definition in the XML schema
    int64_t m_seed;
    bool m_useFlatWorld;
    Pos* m_spawnPos;
    int m_bHasBeenInCreative;
    std::vector<ApplySchematicRuleDefinition*> m_schematicRules;
    std::vector<ConsoleGenerateStructure*> m_structureRules;
    bool m_bHaveMinY;
    int m_minY;
    std::unordered_map<std::string, ConsoleSchematicFile*> m_schematics;
    std::unordered_map<ChunkRuleCacheKey, ChunkRuleCacheEntry, ChunkRuleCacheKeyHash>
        m_chunkRuleCache;
    std::vector<BiomeOverride*> m_biomeOverrides;
    std::vector<StartFeature*> m_features;

    bool m_bRequiresGameRules;
    LevelRuleset* m_requiredGameRules;

    StringTable* m_stringTable;

    DLCPack* m_parentDLCPack;
    bool m_bLoadingData;

public:
    LevelGenerationOptions(DLCPack* parentPack = nullptr);
    ~LevelGenerationOptions();

    virtual ConsoleGameRules::EGameRuleType getActionType();

    virtual void writeAttributes(DataOutputStream* dos,
                                 unsigned int numAttributes);
    virtual void getChildren(std::vector<GameRuleDefinition*>* children);
    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    int64_t getLevelSeed();
    int getLevelHasBeenInCreative();
    Pos* getSpawnPos();
    bool getuseFlatWorld();

    void processSchematics(LevelChunk* chunk);
    void processSchematicsLighting(LevelChunk* chunk);
    void clearChunkRuleCache();

    bool checkIntersects(int x0, int y0, int z0, int x1, int y1, int z1);

private:
    void clearSchematics();

public:
    ConsoleSchematicFile* loadSchematicFile(const std::string& filename,
                                            std::uint8_t* pbData,
                                            unsigned int dataLength);

public:
    ConsoleSchematicFile* getSchematicFile(const std::string& filename);
    void releaseSchematicFile(const std::string& filename);

    bool requiresGameRules();
    void setRequiredGameRules(LevelRuleset* rules);
    LevelRuleset* getRequiredGameRules();

    void getBiomeOverride(int biomeId, std::uint8_t& tile,
                          std::uint8_t& topTile);
    bool isFeatureChunk(int chunkX, int chunkZ,
                        StructureFeature::EFeatureTypes feature,
                        int* orientation = nullptr);

    void loadStringTable(StringTable* table);
    const char* getString(const std::string& key);

    std::unordered_map<std::string, ConsoleSchematicFile*>*
    getUnfinishedSchematicFiles();

    void loadBaseSaveData();
    int onPackMounted(int iPad, uint32_t dwErr, uint32_t dwLicenceMask);

    // 4J-JEV:
    // ApplySchematicRules contain limited state
    // which needs to be reset BEFORE a new game starts.
    void reset_start();

    // 4J-JEV:
    // This file contains state that needs to be deleted
    // or reset once a game has finished.
    void reset_finish();
};
