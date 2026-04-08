#pragma once

#include <memory>
#include <string>

#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"

class ItemInstance;

class AddEnchantmentRuleDefinition : public GameRuleDefinition {
private:
    int m_enchantmentId;
    int m_enchantmentLevel;

public:
    AddEnchantmentRuleDefinition();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_AddEnchantment;
    }

    virtual void writeAttributes(DataOutputStream*, unsigned int numAttrs);

    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool enchantItem(std::shared_ptr<ItemInstance> item);
};