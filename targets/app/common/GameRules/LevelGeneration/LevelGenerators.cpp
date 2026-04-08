#include "LevelGenerators.h"

#include <algorithm>

#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"

LevelGenerators::LevelGenerators() {}

void LevelGenerators::addLevelGenerator(const std::string& displayName,
                                        LevelGenerationOptions* generator) {
    if (!displayName.empty()) generator->setDisplayName(displayName);
    m_levelGenerators.push_back(generator);
}

void LevelGenerators::removeLevelGenerator(LevelGenerationOptions* generator) {
    std::vector<LevelGenerationOptions*>::iterator it;
    while ((it = find(m_levelGenerators.begin(), m_levelGenerators.end(),
                      generator)) != m_levelGenerators.end()) {
        m_levelGenerators.erase(it);
    }
}