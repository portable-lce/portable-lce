#pragma once
// using namespace std;

#include <string>
#include <vector>

#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"

class AddItemRuleDefinition;
class Pos;

class UpdatePlayerRuleDefinition : public GameRuleDefinition {
private:
    std::vector<AddItemRuleDefinition*> m_items;

    bool m_bUpdateHealth, m_bUpdateFood, m_bUpdateYRot, m_bUpdateInventory;
    int m_health;
    int m_food;
    Pos* m_spawnPos;
    float m_yRot;

public:
    UpdatePlayerRuleDefinition();
    ~UpdatePlayerRuleDefinition();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_UpdatePlayerRule;
    }

    virtual void getChildren(std::vector<GameRuleDefinition*>* children);
    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);

    virtual void writeAttributes(DataOutputStream* dos,
                                 unsigned int numAttributes);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    virtual void postProcessPlayer(std::shared_ptr<Player> player);
};