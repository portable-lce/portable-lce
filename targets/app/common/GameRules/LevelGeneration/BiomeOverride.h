#pragma once
// using namespace std;

#include <cstdint>
#include <string>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"

class BiomeOverride : public GameRuleDefinition {
private:
    std::uint8_t m_topTile;
    std::uint8_t m_tile;
    int m_biomeId;

public:
    BiomeOverride();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_BiomeOverride;
    }

    virtual void writeAttributes(DataOutputStream* dos, unsigned int numAttrs);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool isBiome(int id);
    void getTileValues(std::uint8_t& tile, std::uint8_t& topTile);
};
