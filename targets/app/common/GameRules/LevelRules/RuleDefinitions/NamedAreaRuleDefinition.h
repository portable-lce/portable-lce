#pragma once

#include <string>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/phys/AABB.h"

class NamedAreaRuleDefinition : public GameRuleDefinition {
private:
    std::string m_name;
    AABB m_area;

public:
    NamedAreaRuleDefinition();

    virtual void writeAttributes(DataOutputStream* dos,
                                 unsigned int numAttributes);

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_NamedArea;
    }

    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    AABB* getArea() { return &m_area; }
    std::string getName() { return m_name; }
};
