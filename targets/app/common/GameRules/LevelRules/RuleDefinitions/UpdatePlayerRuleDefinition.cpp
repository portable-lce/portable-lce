#include "UpdatePlayerRuleDefinition.h"

#include <wchar.h>

#include <memory>

#include "app/common/GameRules/LevelRules/RuleDefinitions/AddItemRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/Pos.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/food/FoodData.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "util/StringHelpers.h"

UpdatePlayerRuleDefinition::UpdatePlayerRuleDefinition() {
    m_bUpdateHealth = m_bUpdateFood = m_bUpdateYRot = false;
    ;
    m_health = 0;
    m_food = 0;
    m_spawnPos = nullptr;
    m_yRot = 0.0f;
}

UpdatePlayerRuleDefinition::~UpdatePlayerRuleDefinition() {
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        delete *it;
    }
}

void UpdatePlayerRuleDefinition::writeAttributes(DataOutputStream* dos,
                                                 unsigned int numAttributes) {
    int attrCount = 3;
    if (m_bUpdateHealth) ++attrCount;
    if (m_bUpdateFood) ++attrCount;
    if (m_bUpdateYRot) ++attrCount;
    GameRuleDefinition::writeAttributes(dos, numAttributes + attrCount);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_spawnX);
    dos->writeUTF(toWString(m_spawnPos->x));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_spawnY);
    dos->writeUTF(toWString(m_spawnPos->y));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_spawnZ);
    dos->writeUTF(toWString(m_spawnPos->z));

    if (m_bUpdateYRot) {
        ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_yRot);
        dos->writeUTF(toWString(m_yRot));
    }
    if (m_bUpdateHealth) {
        ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_food);
        dos->writeUTF(toWString(m_health));
    }
    if (m_bUpdateFood) {
        ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_health);
        dos->writeUTF(toWString(m_food));
    }
}

void UpdatePlayerRuleDefinition::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    GameRuleDefinition::getChildren(children);
    for (auto it = m_items.begin(); it != m_items.end(); it++)
        children->push_back(*it);
}

GameRuleDefinition* UpdatePlayerRuleDefinition::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_AddItem) {
        rule = new AddItemRuleDefinition();
        m_items.push_back((AddItemRuleDefinition*)rule);
    } else {
#ifndef _CONTENT_PACKAGE
        printf(
            "UpdatePlayerRuleDefinition: Attempted to add invalid child rule "
            "- %d\n",
            ruleType);
#endif
    }
    return rule;
}

void UpdatePlayerRuleDefinition::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("spawnX") == 0) {
        if (m_spawnPos == nullptr) m_spawnPos = new Pos();
        int value = fromWString<int>(attributeValue);
        m_spawnPos->x = value;
        app.DebugPrintf(
            "UpdatePlayerRuleDefinition: Adding parameter spawnX=%d\n", value);
    } else if (attributeName.compare("spawnY") == 0) {
        if (m_spawnPos == nullptr) m_spawnPos = new Pos();
        int value = fromWString<int>(attributeValue);
        m_spawnPos->y = value;
        app.DebugPrintf(
            "UpdatePlayerRuleDefinition: Adding parameter spawnY=%d\n", value);
    } else if (attributeName.compare("spawnZ") == 0) {
        if (m_spawnPos == nullptr) m_spawnPos = new Pos();
        int value = fromWString<int>(attributeValue);
        m_spawnPos->z = value;
        app.DebugPrintf(
            "UpdatePlayerRuleDefinition: Adding parameter spawnZ=%d\n", value);
    } else if (attributeName.compare("health") == 0) {
        int value = fromWString<int>(attributeValue);
        m_health = value;
        m_bUpdateHealth = true;
        app.DebugPrintf(
            "UpdatePlayerRuleDefinition: Adding parameter health=%d\n", value);
    } else if (attributeName.compare("food") == 0) {
        int value = fromWString<int>(attributeValue);
        m_food = value;
        m_bUpdateFood = true;
        app.DebugPrintf(
            "UpdatePlayerRuleDefinition: Adding parameter health=%d\n", value);
    } else if (attributeName.compare("yRot") == 0) {
        float value = fromWString<float>(attributeValue);
        m_yRot = value;
        m_bUpdateYRot = true;
        app.DebugPrintf(
            "UpdatePlayerRuleDefinition: Adding parameter yRot=%f\n", value);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

void UpdatePlayerRuleDefinition::postProcessPlayer(
    std::shared_ptr<Player> player) {
    if (m_bUpdateHealth) {
        player->lastHealth = m_health;
        player->setHealth(m_health);
    }

    if (m_bUpdateFood) {
        player->getFoodData()->setFoodLevel(m_food);
    }

    double x = player->x;
    double y = player->y;
    double z = player->z;
    float yRot = player->yRot;
    float xRot = player->xRot;
    if (m_spawnPos != nullptr) {
        x = m_spawnPos->x;
        y = m_spawnPos->y;
        z = m_spawnPos->z;
    }

    if (m_bUpdateYRot) {
        yRot = m_yRot;
    }

    if (m_spawnPos != nullptr || m_bUpdateYRot)
        player->absMoveTo(x, y, z, yRot, xRot);

    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        AddItemRuleDefinition* addItem = *it;

        addItem->addItemToContainer(player->inventory, -1);
    }
}