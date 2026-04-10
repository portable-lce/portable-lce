#include "UseTileRuleDefinition.h"

#include "app/common/Game.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "util/StringHelpers.h"

UseTileRuleDefinition::UseTileRuleDefinition() {
    m_tileId = -1;
    m_useCoords = false;
}

void UseTileRuleDefinition::writeAttributes(DataOutputStream* dos,
                                            unsigned int numAttributes) {
    GameRuleDefinition::writeAttributes(dos, numAttributes + 5);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_tileId);
    dos->writeUTF(toWString(m_tileId));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_useCoords);
    dos->writeUTF(toWString(m_useCoords));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x);
    dos->writeUTF(toWString(m_coordinates.x));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y);
    dos->writeUTF(toWString(m_coordinates.y));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z);
    dos->writeUTF(toWString(m_coordinates.z));
}

void UseTileRuleDefinition::addAttribute(const std::string& attributeName,
                                         const std::string& attributeValue) {
    if (attributeName.compare("tileId") == 0) {
        m_tileId = fromWString<int>(attributeValue);
        app.DebugPrintf("UseTileRule: Adding parameter tileId=%d\n", m_tileId);
    } else if (attributeName.compare("useCoords") == 0) {
        m_useCoords = fromWString<bool>(attributeValue);
        app.DebugPrintf("UseTileRule: Adding parameter useCoords=%s\n",
                        m_useCoords ? "true" : "false");
    } else if (attributeName.compare("x") == 0) {
        m_coordinates.x = fromWString<int>(attributeValue);
        app.DebugPrintf("UseTileRule: Adding parameter x=%d\n",
                        m_coordinates.x);
    } else if (attributeName.compare("y") == 0) {
        m_coordinates.y = fromWString<int>(attributeValue);
        app.DebugPrintf("UseTileRule: Adding parameter y=%d\n",
                        m_coordinates.y);
    } else if (attributeName.compare("z") == 0) {
        m_coordinates.z = fromWString<int>(attributeValue);
        app.DebugPrintf("UseTileRule: Adding parameter z=%d\n",
                        m_coordinates.z);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool UseTileRuleDefinition::onUseTile(GameRule* rule, int tileId, int x, int y,
                                      int z) {
    bool statusChanged = false;
    if (m_tileId == tileId) {
        if (!m_useCoords || (m_coordinates.x == x && m_coordinates.y == y &&
                             m_coordinates.z == z)) {
            if (!getComplete(rule)) {
                statusChanged = true;
                setComplete(rule, true);
                app.DebugPrintf(
                    "Completed UseTileRule with info - t:%d, coords:%s, x:%d, "
                    "y:%d, z:%d\n",
                    m_tileId, m_useCoords ? "true" : "false", m_coordinates.x,
                    m_coordinates.y, m_coordinates.z);

                // Send a packet or some other announcement here
            }
        }
    }
    return statusChanged;
}