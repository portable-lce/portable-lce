#pragma once

#include <vector>

#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRulesInstance.h"

class CompoundGameRuleDefinition : public GameRuleDefinition {
protected:
    std::vector<GameRuleDefinition*> m_children;

protected:
    GameRuleDefinition* m_lastRuleStatusChanged;

public:
    CompoundGameRuleDefinition();
    virtual ~CompoundGameRuleDefinition();

    virtual void getChildren(std::vector<GameRuleDefinition*>* children);
    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);

    virtual void populateGameRule(
        GameRulesInstance::EGameRulesInstanceType type, GameRule* rule);

    virtual bool onUseTile(GameRule* rule, int tileId, int x, int y, int z);
    virtual bool onCollectItem(GameRule* rule,
                               std::shared_ptr<ItemInstance> item);
    virtual void postProcessPlayer(std::shared_ptr<Player> player);
};