#include "XboxStructureActionGenerateBox.h"

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelGeneration/ConsoleGenerateStructureAction.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/level/levelgen/structure/StructurePiece.h"

XboxStructureActionGenerateBox::XboxStructureActionGenerateBox() {
    m_x0 = m_y0 = m_z0 = m_x1 = m_y1 = m_z1 = m_edgeTile = m_fillTile = 0;
    m_skipAir = false;
}

void XboxStructureActionGenerateBox::writeAttributes(DataOutputStream* dos,
                                                     unsigned int numAttrs) {
    ConsoleGenerateStructureAction::writeAttributes(dos, numAttrs + 9);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x0);
    dos->writeUTF(toWString(m_x0));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y0);
    dos->writeUTF(toWString(m_y0));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z0);
    dos->writeUTF(toWString(m_z0));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x1);
    dos->writeUTF(toWString(m_x1));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y1);
    dos->writeUTF(toWString(m_y1));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z1);
    dos->writeUTF(toWString(m_z1));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_edgeTile);
    dos->writeUTF(toWString(m_edgeTile));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_fillTile);
    dos->writeUTF(toWString(m_fillTile));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_skipAir);
    dos->writeUTF(toWString(m_skipAir));
}

void XboxStructureActionGenerateBox::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("x0") == 0) {
        int value = fromWString<int>(attributeValue);
        m_x0 = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter x0=%d\n", m_x0);
    } else if (attributeName.compare("y0") == 0) {
        int value = fromWString<int>(attributeValue);
        m_y0 = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter y0=%d\n", m_y0);
    } else if (attributeName.compare("z0") == 0) {
        int value = fromWString<int>(attributeValue);
        m_z0 = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter z0=%d\n", m_z0);
    } else if (attributeName.compare("x1") == 0) {
        int value = fromWString<int>(attributeValue);
        m_x1 = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter x1=%d\n", m_x1);
    } else if (attributeName.compare("y1") == 0) {
        int value = fromWString<int>(attributeValue);
        m_y1 = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter y1=%d\n", m_y1);
    } else if (attributeName.compare("z1") == 0) {
        int value = fromWString<int>(attributeValue);
        m_z1 = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter z1=%d\n", m_z1);
    } else if (attributeName.compare("edgeTile") == 0) {
        int value = fromWString<int>(attributeValue);
        m_edgeTile = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter edgeTile=%d\n",
            m_edgeTile);
    } else if (attributeName.compare("fillTile") == 0) {
        int value = fromWString<int>(attributeValue);
        m_fillTile = value;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter fillTile=%d\n",
            m_fillTile);
    } else if (attributeName.compare("skipAir") == 0) {
        if (attributeValue.compare("true") == 0) m_skipAir = true;
        app.DebugPrintf(
            "XboxStructureActionGenerateBox: Adding parameter skipAir=%s\n",
            m_skipAir ? "true" : "false");
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool XboxStructureActionGenerateBox::generateBoxInLevel(
    StructurePiece* structure, Level* level, BoundingBox* chunkBB) {
    app.DebugPrintf("XboxStructureActionGenerateBox - generating a box\n");
    structure->generateBox(level, chunkBB, m_x0, m_y0, m_z0, m_x1, m_y1, m_z1,
                           m_edgeTile, m_fillTile, m_skipAir);
    return true;
}