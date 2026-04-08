#include "AddEnchantmentRuleDefinition.h"

#include <algorithm>
#include <vector>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/item/EnchantedBookItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentCategory.h"
#include "minecraft/world/item/enchantment/EnchantmentInstance.h"

AddEnchantmentRuleDefinition::AddEnchantmentRuleDefinition() {
    m_enchantmentId = m_enchantmentLevel = 0;
}

void AddEnchantmentRuleDefinition::writeAttributes(DataOutputStream* dos,
                                                   unsigned int numAttributes) {
    GameRuleDefinition::writeAttributes(dos, numAttributes + 2);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_enchantmentId);
    dos->writeUTF(toWString(m_enchantmentId));

    ConsoleGameRules::write(dos,
                            ConsoleGameRules::eGameRuleAttr_enchantmentLevel);
    dos->writeUTF(toWString(m_enchantmentLevel));
}

void AddEnchantmentRuleDefinition::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("enchantmentId") == 0) {
        int value = fromWString<int>(attributeValue);
        if (value < 0) value = 0;
        if (value >= 256) value = 255;
        m_enchantmentId = value;
        app.DebugPrintf(
            "AddEnchantmentRuleDefinition: Adding parameter enchantmentId=%d\n",
            m_enchantmentId);
    } else if (attributeName.compare("enchantmentLevel") == 0) {
        int value = fromWString<int>(attributeValue);
        if (value < 0) value = 0;
        m_enchantmentLevel = value;
        app.DebugPrintf(
            "AddEnchantmentRuleDefinition: Adding parameter "
            "enchantmentLevel=%d\n",
            m_enchantmentLevel);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool AddEnchantmentRuleDefinition::enchantItem(
    std::shared_ptr<ItemInstance> item) {
    bool enchanted = false;
    if (item != nullptr) {
        // 4J-JEV: Ripped code from enchantmenthelpers
        // Maybe we want to add an addEnchantment method to EnchantmentHelpers
        if (item->id == Item::enchantedBook_Id) {
            Item::enchantedBook->addEnchantment(
                item,
                new EnchantmentInstance(m_enchantmentId, m_enchantmentLevel));
        } else if (item->isEnchantable()) {
            Enchantment* e = Enchantment::enchantments[m_enchantmentId];

            if (e != nullptr && e->category->canEnchant(item->getItem())) {
                int level = std::min(e->getMaxLevel(), m_enchantmentLevel);
                item->enchant(e, m_enchantmentLevel);
                enchanted = true;
            }
        }
    }
    return enchanted;
}