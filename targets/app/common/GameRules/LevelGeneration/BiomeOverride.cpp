#include "BiomeOverride.h"

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"

BiomeOverride::BiomeOverride() {
    m_tile = 0;
    m_topTile = 0;
    m_biomeId = 0;
}

void BiomeOverride::writeAttributes(DataOutputStream* dos,
                                    unsigned int numAttrs) {
    GameRuleDefinition::writeAttributes(dos, numAttrs + 3);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_biomeId);
    dos->writeUTF(toWString(m_biomeId));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_tileId);
    dos->writeUTF(toWString(m_tile));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_topTileId);
    dos->writeUTF(toWString(m_topTile));
}

void BiomeOverride::addAttribute(const std::string& attributeName,
                                 const std::string& attributeValue) {
    if (attributeName.compare("tileId") == 0) {
        int value = fromWString<int>(attributeValue);
        m_tile = value;
        app.DebugPrintf("BiomeOverride: Adding parameter tileId=%d\n", m_tile);
    } else if (attributeName.compare("topTileId") == 0) {
        int value = fromWString<int>(attributeValue);
        m_topTile = value;
        app.DebugPrintf("BiomeOverride: Adding parameter topTileId=%d\n",
                        m_topTile);
    } else if (attributeName.compare("biomeId") == 0) {
        int value = fromWString<int>(attributeValue);
        m_biomeId = value;
        app.DebugPrintf("BiomeOverride: Adding parameter biomeId=%d\n",
                        m_biomeId);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool BiomeOverride::isBiome(int id) { return m_biomeId == id; }

void BiomeOverride::getTileValues(std::uint8_t& tile, std::uint8_t& topTile) {
    if (m_tile != 0) tile = m_tile;
    if (m_topTile != 0) topTile = m_topTile;
}
