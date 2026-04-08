#include "AddItemRuleDefinition.h"

#include <algorithm>

#include "AddEnchantmentRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"

AddItemRuleDefinition::AddItemRuleDefinition() {
    m_itemId = m_quantity = m_auxValue = m_dataTag = 0;
    m_slot = -1;
}

void AddItemRuleDefinition::writeAttributes(DataOutputStream* dos,
                                            unsigned int numAttrs) {
    GameRuleDefinition::writeAttributes(dos, numAttrs + 5);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_itemId);
    dos->writeUTF(toWString(m_itemId));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_quantity);
    dos->writeUTF(toWString(m_quantity));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_auxValue);
    dos->writeUTF(toWString(m_auxValue));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_dataTag);
    dos->writeUTF(toWString(m_dataTag));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_slot);
    dos->writeUTF(toWString(m_slot));
}

void AddItemRuleDefinition::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    GameRuleDefinition::getChildren(children);
    for (auto it = m_enchantments.begin(); it != m_enchantments.end(); it++)
        children->push_back(*it);
}

GameRuleDefinition* AddItemRuleDefinition::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_AddEnchantment) {
        rule = new AddEnchantmentRuleDefinition();
        m_enchantments.push_back((AddEnchantmentRuleDefinition*)rule);
    } else {
    }
    return rule;
}

void AddItemRuleDefinition::addAttribute(const std::string& attributeName,
                                         const std::string& attributeValue) {
    if (attributeName.compare("itemId") == 0) {
        int value = fromWString<int>(attributeValue);
        m_itemId = value;
        // app.DebugPrintf(2,"AddItemRuleDefinition: Adding parameter
        // itemId=%d\n",m_itemId);
    } else if (attributeName.compare("quantity") == 0) {
        int value = fromWString<int>(attributeValue);
        m_quantity = value;
        // app.DebugPrintf(2,"AddItemRuleDefinition: Adding parameter
        // quantity=%d\n",m_quantity);
    } else if (attributeName.compare("auxValue") == 0) {
        int value = fromWString<int>(attributeValue);
        m_auxValue = value;
        // app.DebugPrintf(2,"AddItemRuleDefinition: Adding parameter
        // auxValue=%d\n",m_auxValue);
    } else if (attributeName.compare("dataTag") == 0) {
        int value = fromWString<int>(attributeValue);
        m_dataTag = value;
        // app.DebugPrintf(2,"AddItemRuleDefinition: Adding parameter
        // dataTag=%d\n",m_dataTag);
    } else if (attributeName.compare("slot") == 0) {
        int value = fromWString<int>(attributeValue);
        m_slot = value;
        // app.DebugPrintf(2,"AddItemRuleDefinition: Adding parameter
        // slot=%d\n",m_slot);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool AddItemRuleDefinition::addItemToContainer(
    std::shared_ptr<Container> container, int slotId) {
    bool added = false;
    if (Item::items[m_itemId] != nullptr) {
        int quantity =
            std::min(m_quantity, Item::items[m_itemId]->getMaxStackSize());
        std::shared_ptr<ItemInstance> newItem = std::shared_ptr<ItemInstance>(
            new ItemInstance(m_itemId, quantity, m_auxValue));
        newItem->set4JData(m_dataTag);

        for (auto it = m_enchantments.begin(); it != m_enchantments.end();
             ++it) {
            (*it)->enchantItem(newItem);
        }

        if (m_slot >= 0 && m_slot < container->getContainerSize()) {
            container->setItem(m_slot, newItem);
            added = true;
        } else if (slotId >= 0 && slotId < container->getContainerSize()) {
            container->setItem(slotId, newItem);
            added = true;
        } else if (std::dynamic_pointer_cast<Inventory>(container) != nullptr) {
            added =
                std::dynamic_pointer_cast<Inventory>(container)->add(newItem);
        }
    }
    return added;
}