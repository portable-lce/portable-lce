#pragma once
// using namespace std;

#include <string>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "minecraft/world/level/levelgen/structure/StructureFeature.h"

class StartFeature : public GameRuleDefinition {
private:
    int m_chunkX, m_chunkZ, m_orientation;
    StructureFeature::EFeatureTypes m_feature;

public:
    StartFeature();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_StartFeature;
    }

    virtual void writeAttributes(DataOutputStream* dos, unsigned int numAttrs);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool isFeatureChunk(int chunkX, int chunkZ,
                        StructureFeature::EFeatureTypes feature,
                        int* orientation);
};