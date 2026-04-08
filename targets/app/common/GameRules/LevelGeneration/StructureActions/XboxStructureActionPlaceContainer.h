#pragma once

#include <string>
#include <vector>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "XboxStructureActionPlaceBlock.h"

class AddItemRuleDefinition;
class StructurePiece;
class Level;
class BoundingBox;

class XboxStructureActionPlaceContainer : public XboxStructureActionPlaceBlock {
private:
    std::vector<AddItemRuleDefinition*> m_items;

public:
    XboxStructureActionPlaceContainer();
    ~XboxStructureActionPlaceContainer();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_PlaceContainer;
    }

    virtual void getChildren(std::vector<GameRuleDefinition*>* children);
    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);

    // 4J-JEV: Super class handles attr-facing fine.
    // virtual void writeAttributes(DataOutputStream *dos, uint32_t
    // numAttributes);

    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    bool placeContainerInLevel(StructurePiece* structure, Level* level,
                               BoundingBox* chunkBB);
};