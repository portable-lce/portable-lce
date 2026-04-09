#pragma once

#include <memory>
#include <string>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"

class Pos;
class UseTileRuleDefinition;
class ItemInstance;

class CollectItemRuleDefinition : public GameRuleDefinition {
private:
    // These values should map directly to the xsd definition for this Rule
    int m_itemId;
    unsigned char m_auxValue;
    int m_quantity;

public:
    CollectItemRuleDefinition();
    ~CollectItemRuleDefinition();

    ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_CollectItemRule;
    }

    virtual void writeAttributes(DataOutputStream*, unsigned int numAttributes);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    virtual int getGoal();
    virtual int getProgress(GameRule* rule);

    virtual int getIcon() { return m_itemId; }
    virtual int getAuxValue() { return m_auxValue; }

    void populateGameRule(GameRulesInstance::EGameRulesInstanceType type,
                          GameRule* rule);

    bool onCollectItem(GameRule* rule, std::shared_ptr<ItemInstance> item);

    static std::string generateXml(std::shared_ptr<ItemInstance> item);

private:
    // static std::string generateXml(CollectItemRuleDefinition *ruleDef);
};