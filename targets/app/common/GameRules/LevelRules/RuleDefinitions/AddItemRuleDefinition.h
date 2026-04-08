#pragma once

#include <memory>
#include <string>
#include <vector>

#include "GameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"

class Container;
class AddEnchantmentRuleDefinition;

class AddItemRuleDefinition : public GameRuleDefinition {
private:
    int m_itemId;
    int m_quantity;
    int m_auxValue;
    int m_dataTag;
    int m_slot;
    std::vector<AddEnchantmentRuleDefinition*> m_enchantments;

public:
    AddItemRuleDefinition();

    virtual void writeAttributes(DataOutputStream*, unsigned int numAttributes);
    virtual void getChildren(std::vector<GameRuleDefinition*>* children);

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_AddItem;
    }

    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool addItemToContainer(std::shared_ptr<Container> container, int slotId);
};