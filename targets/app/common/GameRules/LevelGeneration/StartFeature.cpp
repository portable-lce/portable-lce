#include "StartFeature.h"

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"

StartFeature::StartFeature() {
    m_chunkX = 0;
    m_chunkZ = 0;
    m_orientation = 0;
    m_feature = StructureFeature::eFeature_Temples;
}

void StartFeature::writeAttributes(DataOutputStream* dos,
                                   unsigned int numAttrs) {
    GameRuleDefinition::writeAttributes(dos, numAttrs + 4);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_chunkX);
    dos->writeUTF(toWString(m_chunkX));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_chunkZ);
    dos->writeUTF(toWString(m_chunkZ));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_feature);
    dos->writeUTF(toWString((int)m_feature));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_orientation);
    dos->writeUTF(toWString(m_orientation));
}

void StartFeature::addAttribute(const std::string& attributeName,
                                const std::string& attributeValue) {
    if (attributeName.compare("chunkX") == 0) {
        int value = fromWString<int>(attributeValue);
        m_chunkX = value;
        app.DebugPrintf("StartFeature: Adding parameter chunkX=%d\n", m_chunkX);
    } else if (attributeName.compare("chunkZ") == 0) {
        int value = fromWString<int>(attributeValue);
        m_chunkZ = value;
        app.DebugPrintf("StartFeature: Adding parameter chunkZ=%d\n", m_chunkZ);
    } else if (attributeName.compare("orientation") == 0) {
        int value = fromWString<int>(attributeValue);
        m_orientation = value;
        app.DebugPrintf("StartFeature: Adding parameter orientation=%d\n",
                        m_orientation);
    } else if (attributeName.compare("feature") == 0) {
        int value = fromWString<int>(attributeValue);
        m_feature = (StructureFeature::EFeatureTypes)value;
        app.DebugPrintf("StartFeature: Adding parameter feature=%d\n",
                        m_feature);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool StartFeature::isFeatureChunk(int chunkX, int chunkZ,
                                  StructureFeature::EFeatureTypes feature,
                                  int* orientation) {
    if (orientation != nullptr) *orientation = m_orientation;
    return chunkX == m_chunkX && chunkZ == m_chunkZ && feature == m_feature;
}