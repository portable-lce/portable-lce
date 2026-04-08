#include "XboxStructureActionPlaceBlock.h"

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelGeneration/ConsoleGenerateStructureAction.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/level/levelgen/structure/StructurePiece.h"

XboxStructureActionPlaceBlock::XboxStructureActionPlaceBlock() {
    m_x = m_y = m_z = m_tile = m_data = 0;
}

void XboxStructureActionPlaceBlock::writeAttributes(DataOutputStream* dos,
                                                    unsigned int numAttrs) {
    ConsoleGenerateStructureAction::writeAttributes(dos, numAttrs + 5);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x);
    dos->writeUTF(toWString(m_x));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y);
    dos->writeUTF(toWString(m_y));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z);
    dos->writeUTF(toWString(m_z));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_data);
    dos->writeUTF(toWString(m_data));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_block);
    dos->writeUTF(toWString(m_tile));
}

void XboxStructureActionPlaceBlock::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("x") == 0) {
        int value = fromWString<int>(attributeValue);
        m_x = value;
        app.DebugPrintf(
            "XboxStructureActionPlaceBlock: Adding parameter x=%d\n", m_x);
    } else if (attributeName.compare("y") == 0) {
        int value = fromWString<int>(attributeValue);
        m_y = value;
        app.DebugPrintf(
            "XboxStructureActionPlaceBlock: Adding parameter y=%d\n", m_y);
    } else if (attributeName.compare("z") == 0) {
        int value = fromWString<int>(attributeValue);
        m_z = value;
        app.DebugPrintf(
            "XboxStructureActionPlaceBlock: Adding parameter z=%d\n", m_z);
    } else if (attributeName.compare("block") == 0) {
        int value = fromWString<int>(attributeValue);
        m_tile = value;
        app.DebugPrintf(
            "XboxStructureActionPlaceBlock: Adding parameter block=%d\n",
            m_tile);
    } else if (attributeName.compare("data") == 0) {
        int value = fromWString<int>(attributeValue);
        m_data = value;
        app.DebugPrintf(
            "XboxStructureActionPlaceBlock: Adding parameter data=%d\n",
            m_data);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool XboxStructureActionPlaceBlock::placeBlockInLevel(StructurePiece* structure,
                                                      Level* level,
                                                      BoundingBox* chunkBB) {
    app.DebugPrintf("XboxStructureActionPlaceBlock - placing a block\n");
    structure->placeBlock(level, m_tile, m_data, m_x, m_y, m_z, chunkBB);
    return true;
}