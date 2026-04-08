#include "CollectItemRuleDefinition.h"

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/GameRules/GameRule.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/network/Connection.h"
#include "minecraft/network/packet/UpdateGameRuleProgressPacket.h"
#include "minecraft/world/item/ItemInstance.h"

CollectItemRuleDefinition::CollectItemRuleDefinition() {
    m_itemId = 0;
    m_auxValue = 0;
    m_quantity = 0;
}

CollectItemRuleDefinition::~CollectItemRuleDefinition() {}

void CollectItemRuleDefinition::writeAttributes(DataOutputStream* dos,
                                                unsigned int numAttributes) {
    GameRuleDefinition::writeAttributes(dos, numAttributes + 3);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_itemId);
    dos->writeUTF(toWString(m_itemId));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_auxValue);
    dos->writeUTF(toWString(m_auxValue));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_quantity);
    dos->writeUTF(toWString(m_quantity));
}

void CollectItemRuleDefinition::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("itemId") == 0) {
        m_itemId = fromWString<int>(attributeValue);
        app.DebugPrintf("CollectItemRule: Adding parameter itemId=%d\n",
                        m_itemId);
    } else if (attributeName.compare("auxValue") == 0) {
        m_auxValue = fromWString<int>(attributeValue);
        app.DebugPrintf("CollectItemRule: Adding parameter m_auxValue=%d\n",
                        m_auxValue);
    } else if (attributeName.compare("quantity") == 0) {
        m_quantity = fromWString<int>(attributeValue);
        app.DebugPrintf("CollectItemRule: Adding parameter m_quantity=%d\n",
                        m_quantity);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

int CollectItemRuleDefinition::getGoal() { return m_quantity; }

int CollectItemRuleDefinition::getProgress(GameRule* rule) {
    GameRule::ValueType value = rule->getParameter("iQuantity");
    return value.i;
}

void CollectItemRuleDefinition::populateGameRule(
    GameRulesInstance::EGameRulesInstanceType type, GameRule* rule) {
    GameRule::ValueType value;
    value.i = 0;
    rule->setParameter("iQuantity", value);

    GameRuleDefinition::populateGameRule(type, rule);
}

bool CollectItemRuleDefinition::onCollectItem(
    GameRule* rule, std::shared_ptr<ItemInstance> item) {
    bool statusChanged = false;
    if (item != nullptr && item->id == m_itemId &&
        item->getAuxValue() == m_auxValue &&
        item->get4JData() == m_4JDataValue) {
        if (!getComplete(rule)) {
            GameRule::ValueType value = rule->getParameter("iQuantity");
            int quantityCollected = (value.i += item->count);
            rule->setParameter("iQuantity", value);

            statusChanged = true;

            if (quantityCollected >= m_quantity) {
                setComplete(rule, true);
                app.DebugPrintf(
                    "Completed CollectItemRule with info - itemId:%d, "
                    "auxValue:%d, quantity:%d, dataTag:%d\n",
                    m_itemId, m_auxValue, m_quantity, m_4JDataValue);

                if (rule->getConnection() != nullptr) {
                    rule->getConnection()->send(
                        std::shared_ptr<UpdateGameRuleProgressPacket>(
                            new UpdateGameRuleProgressPacket(
                                getActionType(), this->m_descriptionId,
                                m_itemId, m_auxValue, this->m_4JDataValue,
                                nullptr, 0)));
                }
            }
        }
    }
    return statusChanged;
}

std::string CollectItemRuleDefinition::generateXml(
    std::shared_ptr<ItemInstance> item) {
    // 4J Stu - This should be kept in sync with the GameRulesDefinition.xsd
    std::string xml = "";
    if (item != nullptr) {
        xml = "<CollectItemRule itemId=\"" + toWString<int>(item->id) +
              "\" quantity=\"SET\" descriptionName=\"OPTIONAL\" "
              "promptName=\"OPTIONAL\"";
        if (item->getAuxValue() != 0)
            xml +=
                " auxValue=\"" + toWString<int>(item->getAuxValue()) + "\"";
        if (item->get4JData() != 0)
            xml += " dataTag=\"" + toWString<int>(item->get4JData()) + "\"";
        xml += "/>\n";
    }
    return xml;
}