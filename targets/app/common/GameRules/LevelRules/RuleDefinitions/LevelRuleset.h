#pragma once

#include <string>
#include <vector>

#include "CompoundGameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"

class NamedAreaRuleDefinition;
class AABB;
class StringTable;

class LevelRuleset : public CompoundGameRuleDefinition {
private:
    std::vector<NamedAreaRuleDefinition*> m_areas;
    StringTable* m_stringTable;

public:
    LevelRuleset();
    ~LevelRuleset();

    virtual void getChildren(std::vector<GameRuleDefinition*>* children);
    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_LevelRules;
    }

    void loadStringTable(StringTable* table);
    const char* getString(const std::string& key);

    AABB* getNamedArea(const std::string& areaName);

    StringTable* getStringTable() { return m_stringTable; }
};
