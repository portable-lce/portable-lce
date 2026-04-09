#include "LevelRuleset.h"

#include "app/common/GameRules/LevelRules/RuleDefinitions/CompoundGameRuleDefinition.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/NamedAreaRuleDefinition.h"
#include "minecraft/locale/StringTable.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"

class AABB;

LevelRuleset::LevelRuleset() { m_stringTable = nullptr; }

LevelRuleset::~LevelRuleset() {
    for (auto it = m_areas.begin(); it != m_areas.end(); ++it) {
        delete *it;
    }
}

void LevelRuleset::getChildren(std::vector<GameRuleDefinition*>* children) {
    CompoundGameRuleDefinition::getChildren(children);
    for (auto it = m_areas.begin(); it != m_areas.end(); it++)
        children->push_back(*it);
}

GameRuleDefinition* LevelRuleset::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_NamedArea) {
        rule = new NamedAreaRuleDefinition();
        m_areas.push_back((NamedAreaRuleDefinition*)rule);
    } else {
        rule = CompoundGameRuleDefinition::addChild(ruleType);
    }
    return rule;
}

void LevelRuleset::loadStringTable(StringTable* table) {
    m_stringTable = table;
}

const char* LevelRuleset::getString(const std::string& key) {
    if (m_stringTable == nullptr) {
        return "";
    } else {
        return m_stringTable->getString(key);
    }
}

AABB* LevelRuleset::getNamedArea(const std::string& areaName) {
    AABB* area = nullptr;
    for (auto it = m_areas.begin(); it != m_areas.end(); ++it) {
        if ((*it)->getName().compare(areaName) == 0) {
            area = (*it)->getArea();
            break;
        }
    }
    return area;
}
