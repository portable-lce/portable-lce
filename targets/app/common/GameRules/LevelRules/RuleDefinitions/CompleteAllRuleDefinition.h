#pragma once

#include <string>

#include "CompoundGameRuleDefinition.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"

class GameRule;

class CompleteAllRuleDefinition : public CompoundGameRuleDefinition {
private:
    typedef struct _packetData {
        int goal;
        int progress;
    } PacketData;

public:
    ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_CompleteAllRule;
    }

    virtual void getChildren(std::vector<GameRuleDefinition*>* children);

    virtual bool onUseTile(GameRule* rule, int tileId, int x, int y, int z);
    virtual bool onCollectItem(GameRule* rule,
                               std::shared_ptr<ItemInstance> item);

    static std::string generateDescriptionString(const std::string& description,
                                                 void* data, int dataLength);

private:
    void updateStatus(GameRule* rule);
};