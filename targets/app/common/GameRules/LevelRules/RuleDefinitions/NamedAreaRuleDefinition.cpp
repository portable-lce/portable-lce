#include "NamedAreaRuleDefinition.h"

#include <wchar.h>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"

NamedAreaRuleDefinition::NamedAreaRuleDefinition() {
    m_name = "";
    m_area = AABB(0, 0, 0, 0, 0, 0);
}

void NamedAreaRuleDefinition::writeAttributes(DataOutputStream* dos,
                                              unsigned int numAttributes) {
    GameRuleDefinition::writeAttributes(dos, numAttributes + 7);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_name);
    dos->writeUTF(m_name);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x0);
    dos->writeUTF(toWString(m_area.x0));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y0);
    dos->writeUTF(toWString(m_area.y0));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z0);
    dos->writeUTF(toWString(m_area.z0));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x1);
    dos->writeUTF(toWString(m_area.x1));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y1);
    dos->writeUTF(toWString(m_area.y1));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z1);
    dos->writeUTF(toWString(m_area.z1));
}

void NamedAreaRuleDefinition::addAttribute(const std::string& attributeName,
                                           const std::string& attributeValue) {
    if (attributeName.compare("name") == 0) {
        m_name = attributeValue;
#ifndef _CONTENT_PACKAGE
        printf("NamedAreaRuleDefinition: Adding parameter name=%s\n",
                m_name.c_str());
#endif
    } else if (attributeName.compare("x0") == 0) {
        m_area.x0 = fromWString<int>(attributeValue);
        app.DebugPrintf("NamedAreaRuleDefinition: Adding parameter x0=%f\n",
                        m_area.x0);
    } else if (attributeName.compare("y0") == 0) {
        m_area.y0 = fromWString<int>(attributeValue);
        if (m_area.y0 < 0) m_area.y0 = 0;
        app.DebugPrintf("NamedAreaRuleDefinition: Adding parameter y0=%f\n",
                        m_area.y0);
    } else if (attributeName.compare("z0") == 0) {
        m_area.z0 = fromWString<int>(attributeValue);
        app.DebugPrintf("NamedAreaRuleDefinition: Adding parameter z0=%f\n",
                        m_area.z0);
    } else if (attributeName.compare("x1") == 0) {
        m_area.x1 = fromWString<int>(attributeValue);
        app.DebugPrintf("NamedAreaRuleDefinition: Adding parameter x1=%f\n",
                        m_area.x1);
    } else if (attributeName.compare("y1") == 0) {
        m_area.y1 = fromWString<int>(attributeValue);
        if (m_area.y1 < 0) m_area.y1 = 0;
        app.DebugPrintf("NamedAreaRuleDefinition: Adding parameter y1=%f\n",
                        m_area.y1);
    } else if (attributeName.compare("z1") == 0) {
        m_area.z1 = fromWString<int>(attributeValue);
        app.DebugPrintf("NamedAreaRuleDefinition: Adding parameter z1=%f\n",
                        m_area.z1);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}
