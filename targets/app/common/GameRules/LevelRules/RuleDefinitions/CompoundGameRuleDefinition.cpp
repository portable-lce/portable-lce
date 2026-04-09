#include "CompoundGameRuleDefinition.h"

#include <wchar.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "app/common/GameRules/LevelRules/RuleDefinitions/CollectItemRuleDefinition.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/CompleteAllRuleDefinition.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/UpdatePlayerRuleDefinition.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/UseTileRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRule.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"
#include "util/StringHelpers.h"

CompoundGameRuleDefinition::CompoundGameRuleDefinition() {
    m_lastRuleStatusChanged = nullptr;
}

CompoundGameRuleDefinition::~CompoundGameRuleDefinition() {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        delete (*it);
    }
}

void CompoundGameRuleDefinition::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    GameRuleDefinition::getChildren(children);
    for (auto it = m_children.begin(); it != m_children.end(); it++)
        children->push_back(*it);
}

GameRuleDefinition* CompoundGameRuleDefinition::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_CompleteAllRule) {
        rule = new CompleteAllRuleDefinition();
    } else if (ruleType == ConsoleGameRules::eGameRuleType_CollectItemRule) {
        rule = new CollectItemRuleDefinition();
    } else if (ruleType == ConsoleGameRules::eGameRuleType_UseTileRule) {
        rule = new UseTileRuleDefinition();
    } else if (ruleType == ConsoleGameRules::eGameRuleType_UpdatePlayerRule) {
        rule = new UpdatePlayerRuleDefinition();
    } else {
#ifndef _CONTENT_PACKAGE
        printf(
            "CompoundGameRuleDefinition: Attempted to add invalid child rule "
            "- %d\n",
            ruleType);
#endif
    }
    if (rule != nullptr) m_children.push_back(rule);
    return rule;
}

void CompoundGameRuleDefinition::populateGameRule(
    GameRulesInstance::EGameRulesInstanceType type, GameRule* rule) {
    GameRule* newRule = nullptr;
    int i = 0;
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        newRule = new GameRule(*it, rule->getConnection());
        (*it)->populateGameRule(type, newRule);

        GameRule::ValueType value;
        value.gr = newRule;
        value.isPointer = true;

        // Somehow add the newRule to the current rule
        rule->setParameter("rule" + toWString<int>(i), value);
        ++i;
    }
    GameRuleDefinition::populateGameRule(type, rule);
}

bool CompoundGameRuleDefinition::onUseTile(GameRule* rule, int tileId, int x,
                                           int y, int z) {
    bool statusChanged = false;
    for (auto it = rule->m_parameters.begin(); it != rule->m_parameters.end();
         ++it) {
        if (it->second.isPointer) {
            bool changed = it->second.gr->getGameRuleDefinition()->onUseTile(
                it->second.gr, tileId, x, y, z);
            if (!statusChanged && changed) {
                m_lastRuleStatusChanged =
                    it->second.gr->getGameRuleDefinition();
                statusChanged = true;
            }
        }
    }
    return statusChanged;
}

bool CompoundGameRuleDefinition::onCollectItem(
    GameRule* rule, std::shared_ptr<ItemInstance> item) {
    bool statusChanged = false;
    for (auto it = rule->m_parameters.begin(); it != rule->m_parameters.end();
         ++it) {
        if (it->second.isPointer) {
            bool changed =
                it->second.gr->getGameRuleDefinition()->onCollectItem(
                    it->second.gr, item);
            if (!statusChanged && changed) {
                m_lastRuleStatusChanged =
                    it->second.gr->getGameRuleDefinition();
                statusChanged = true;
            }
        }
    }
    return statusChanged;
}

void CompoundGameRuleDefinition::postProcessPlayer(
    std::shared_ptr<Player> player) {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        (*it)->postProcessPlayer(player);
    }
}