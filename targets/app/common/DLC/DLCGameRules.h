#pragma once

#include "DLCFile.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"

class DLCGameRules : public DLCFile {
public:
    DLCGameRules(DLCManager::EDLCType type, const std::string& path)
        : DLCFile(type, path) {}
};