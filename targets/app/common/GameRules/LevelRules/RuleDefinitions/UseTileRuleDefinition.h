#pragma once
// using namespace std;

#include <string>

#include "GameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/Pos.h"

class UseTileRuleDefinition : public GameRuleDefinition {
private:
    // These values should map directly to the xsd definition for this Rule
    int m_tileId;
    bool m_useCoords;
    Pos m_coordinates;

public:
    UseTileRuleDefinition();

    ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_UseTileRule;
    }

    virtual void writeAttributes(DataOutputStream* dos,
                                 unsigned int numAttributes);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    virtual bool onUseTile(GameRule* rule, int tileId, int x, int y, int z);
};
