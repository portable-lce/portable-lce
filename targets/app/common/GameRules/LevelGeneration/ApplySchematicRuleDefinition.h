#pragma once
#include <stdint.h>

#include <optional>
#include <string>

#include "ConsoleSchematicFile.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/Vec3.h"

class AABB;
class Vec3;
class LevelChunk;
class LevelGenerationOptions;
class GRFObject;

class ApplySchematicRuleDefinition : public GameRuleDefinition {
private:
    LevelGenerationOptions* m_levelGenOptions;
    std::string m_schematicName;
    ConsoleSchematicFile* m_schematic;
    Vec3 m_location;
    std::optional<AABB> m_locationBox;
    ConsoleSchematicFile::ESchematicRotation m_rotation;
    int m_dimension;

    int64_t m_totalBlocksChanged;
    int64_t m_totalBlocksChangedLighting;
    bool m_completed;

    void updateLocationBox();

public:
    ApplySchematicRuleDefinition(LevelGenerationOptions* levelGenOptions);
    ~ApplySchematicRuleDefinition();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_ApplySchematic;
    }

    virtual void writeAttributes(DataOutputStream* dos, unsigned int numAttrs);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    void processSchematic(AABB* chunkBox, LevelChunk* chunk);
    void processSchematicLighting(AABB* chunkBox, LevelChunk* chunk);

    bool checkIntersects(int x0, int y0, int z0, int x1, int y1, int z1);
    int getMinY();

    bool isComplete() { return m_completed; }

    std::string getSchematicName() { return m_schematicName; }

    /** 4J-JEV:
     *  This GameRuleDefinition contains limited game state.
     *	Reset any state to how it should be before a new game.
     */
    void reset();
};
