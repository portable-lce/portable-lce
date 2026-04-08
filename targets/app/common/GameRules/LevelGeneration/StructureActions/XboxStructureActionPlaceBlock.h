#pragma once
#include <string>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelGeneration/ConsoleGenerateStructureAction.h"

class StructurePiece;
class Level;
class BoundingBox;

class XboxStructureActionPlaceBlock : public ConsoleGenerateStructureAction {
protected:
    int m_x, m_y, m_z, m_tile, m_data;

public:
    XboxStructureActionPlaceBlock();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_PlaceBlock;
    }

    virtual int getEndX() { return m_x; }
    virtual int getEndY() { return m_y; }
    virtual int getEndZ() { return m_z; }

    virtual void writeAttributes(DataOutputStream* dos, unsigned int numAttrs);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool placeBlockInLevel(StructurePiece* structure, Level* level,
                           BoundingBox* chunkBB);
};