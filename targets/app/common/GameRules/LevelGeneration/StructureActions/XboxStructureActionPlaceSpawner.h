#pragma once

#include <string>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "XboxStructureActionPlaceBlock.h"

class StructurePiece;
class Level;
class BoundingBox;
class GRFObject;

class XboxStructureActionPlaceSpawner : public XboxStructureActionPlaceBlock {
private:
    std::string m_entityId;

public:
    XboxStructureActionPlaceSpawner();
    ~XboxStructureActionPlaceSpawner();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_PlaceSpawner;
    }

    virtual void writeAttributes(DataOutputStream* dos, unsigned int numAttrs);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool placeSpawnerInLevel(StructurePiece* structure, Level* level,
                             BoundingBox* chunkBB);
};