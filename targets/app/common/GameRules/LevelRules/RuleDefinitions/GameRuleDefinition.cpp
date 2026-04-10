#include "minecraft/world/level/GameRules/GameRuleDefinition.h"

#include <assert.h>
#include <wchar.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/common/Game.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/CompleteAllRuleDefinition.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/LevelRuleset.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRule.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"
#include "util/StringHelpers.h"

class Connection;

GameRuleDefinition::GameRuleDefinition() {
    m_descriptionId = "";
    m_promptId = "";
    m_4JDataValue = 0;
}

void GameRuleDefinition::write(DataOutputStream* dos) {
    // Write EGameRuleType.
    ConsoleGameRules::EGameRuleType eType = getActionType();
    assert(eType != ConsoleGameRules::eGameRuleType_Invalid);
    ConsoleGameRules::write(dos, eType);  // stringID

    writeAttributes(dos, 0);

    // 4J-JEV: Get children.
    std::vector<GameRuleDefinition*>* children =
        new std::vector<GameRuleDefinition*>();
    getChildren(children);

    // Write children.
    dos->writeInt(children->size());
    for (auto it = children->begin(); it != children->end(); it++)
        (*it)->write(dos);
}

void GameRuleDefinition::writeAttributes(DataOutputStream* dos,
                                         unsigned int numAttributes) {
    dos->writeInt(numAttributes + 3);

    ConsoleGameRules::write(dos,
                            ConsoleGameRules::eGameRuleAttr_descriptionName);
    dos->writeUTF(m_descriptionId);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_promptName);
    dos->writeUTF(m_promptId);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_dataTag);
    dos->writeUTF(toWString(m_4JDataValue));
}

void GameRuleDefinition::getChildren(
    std::vector<GameRuleDefinition*>* children) {}

GameRuleDefinition* GameRuleDefinition::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
#ifndef _CONTENT_PACKAGE
    printf("GameRuleDefinition: Attempted to add invalid child rule - %d\n",
           ruleType);
#endif
    return nullptr;
}

void GameRuleDefinition::addAttribute(const std::string& attributeName,
                                      const std::string& attributeValue) {
    if (attributeName.compare("descriptionName") == 0) {
        m_descriptionId = attributeValue;
#ifndef _CONTENT_PACKAGE
        printf("GameRuleDefinition: Adding parameter descriptionId=%s\n",
               m_descriptionId.c_str());
#endif
    } else if (attributeName.compare("promptName") == 0) {
        m_promptId = attributeValue;
#ifndef _CONTENT_PACKAGE
        printf("GameRuleDefinition: Adding parameter m_promptId=%s\n",
               m_promptId.c_str());
#endif
    } else if (attributeName.compare("dataTag") == 0) {
        m_4JDataValue = fromWString<int>(attributeValue);
        app.DebugPrintf(
            "GameRuleDefinition: Adding parameter m_4JDataValue=%d\n",
            m_4JDataValue);
    } else {
#ifndef _CONTENT_PACKAGE
        printf("GameRuleDefinition: Attempted to add invalid attribute: %s\n",
               attributeName.c_str());
#endif
    }
}

void GameRuleDefinition::populateGameRule(
    GameRulesInstance::EGameRulesInstanceType type, GameRule* rule) {
    GameRule::ValueType value;
    value.b = false;
    rule->setParameter("bComplete", value);
}

bool GameRuleDefinition::getComplete(GameRule* rule) {
    GameRule::ValueType value;
    value = rule->getParameter("bComplete");
    return value.b;
}

void GameRuleDefinition::setComplete(GameRule* rule, bool val) {
    GameRule::ValueType value;
    value = rule->getParameter("bComplete");
    value.b = val;
    rule->setParameter("bComplete", value);
}

std::vector<GameRuleDefinition*>* GameRuleDefinition::enumerate() {
    // Get Vector.
    std::vector<GameRuleDefinition*>* gRules;
    gRules = new std::vector<GameRuleDefinition*>();
    gRules->push_back(this);
    getChildren(gRules);
    return gRules;
}

std::unordered_map<GameRuleDefinition*, int>*
GameRuleDefinition::enumerateMap() {
    std::unordered_map<GameRuleDefinition*, int>* out =
        new std::unordered_map<GameRuleDefinition*, int>();

    int i = 0;
    std::vector<GameRuleDefinition*>* gRules = enumerate();
    for (auto it = gRules->begin(); it != gRules->end(); it++)
        out->insert(std::pair<GameRuleDefinition*, int>(*it, i++));

    return out;
}

GameRulesInstance* GameRuleDefinition::generateNewGameRulesInstance(
    GameRulesInstance::EGameRulesInstanceType type, LevelRuleset* rules,
    Connection* connection) {
    GameRulesInstance* manager = new GameRulesInstance(rules, connection);

    rules->populateGameRule(type, manager);

    return manager;
}

std::string GameRuleDefinition::generateDescriptionString(
    ConsoleGameRules::EGameRuleType defType, const std::string& description,
    void* data, int dataLength) {
    std::string formatted = description;
    switch (defType) {
        case ConsoleGameRules::eGameRuleType_CompleteAllRule:
            formatted = CompleteAllRuleDefinition::generateDescriptionString(
                description, data, dataLength);
            break;
        default:
            break;
    };
    return formatted;
}