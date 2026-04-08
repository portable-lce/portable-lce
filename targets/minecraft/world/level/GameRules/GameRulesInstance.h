#pragma once
// using namespace std;
#include <vector>

#include "minecraft/world/level/GameRules/GameRule.h"

class GameRuleDefinition;

// The game rule manager belongs to a player/server or other object, and
// maintains their current state for each of the rules that apply to them
class GameRulesInstance : public GameRule {
public:
    // These types are used by the GameRuleDefinition to know which rules to add
    // to this GameRulesInstance
    enum EGameRulesInstanceType {
        eGameRulesInstanceType_ServerPlayer,
        eGameRulesInstanceType_Server,
        eGameRulesInstanceType_Count
    };

public:
    GameRulesInstance(GameRuleDefinition* definition, Connection* connection)
        : GameRule(definition, connection) {}
    // Functions for all the hooks should go here
};