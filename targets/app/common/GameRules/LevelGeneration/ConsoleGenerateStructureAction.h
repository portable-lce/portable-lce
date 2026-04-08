#pragma once

#include "minecraft/world/level/GameRules/GameRuleDefinition.h"

class ConsoleGenerateStructureAction : public GameRuleDefinition {
public:
    virtual int getEndX() = 0;
    virtual int getEndY() = 0;
    virtual int getEndZ() = 0;
};