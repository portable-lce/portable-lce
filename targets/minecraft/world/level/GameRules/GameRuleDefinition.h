#pragma once
// using namespace std;
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"

class GameRule;
class LevelRuleset;
class Player;
class WstringLookup;
class Connection;
class DataOutputStream;
class ItemInstance;

class GameRuleDefinition {
private:
    // Owner type defines who this rule applies to
    GameRulesInstance::EGameRulesInstanceType m_ownerType;

protected:
    // These attributes should map to those in the XSD GameRuleType
    std::string m_descriptionId;
    std::string m_promptId;
    int m_4JDataValue;

public:
    GameRuleDefinition();
    virtual ~GameRuleDefinition() {}

    virtual ConsoleGameRules::EGameRuleType getActionType() = 0;

    void setOwnerType(GameRulesInstance::EGameRulesInstanceType ownerType) {
        m_ownerType = ownerType;
    }

    virtual void write(DataOutputStream*);

    virtual void writeAttributes(DataOutputStream* dos,
                                 unsigned int numAttributes);
    virtual void getChildren(std::vector<GameRuleDefinition*>*);

    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    virtual void populateGameRule(
        GameRulesInstance::EGameRulesInstanceType type, GameRule* rule);

    bool getComplete(GameRule* rule);
    void setComplete(GameRule* rule, bool val);

    virtual int getGoal() { return 0; }
    virtual int getProgress(GameRule* rule) { return 0; }

    virtual int getIcon() { return -1; }
    virtual int getAuxValue() { return 0; }

    // Here we should have functions for all the hooks, with a GameRule* as the
    // first parameter
    virtual bool onUseTile(GameRule* rule, int tileId, int x, int y, int z) {
        return false;
    }
    virtual bool onCollectItem(GameRule* rule,
                               std::shared_ptr<ItemInstance> item) {
        return false;
    }
    virtual void postProcessPlayer(std::shared_ptr<Player> player) {}

    std::vector<GameRuleDefinition*>* enumerate();
    std::unordered_map<GameRuleDefinition*, int>* enumerateMap();

    // Static functions
    static GameRulesInstance* generateNewGameRulesInstance(
        GameRulesInstance::EGameRulesInstanceType type, LevelRuleset* rules,
        Connection* connection);
    static std::string generateDescriptionString(
        ConsoleGameRules::EGameRuleType defType, const std::string& description,
        void* data = nullptr, int dataLength = 0);
};
