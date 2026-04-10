#include "XboxStructureActionPlaceContainer.h"

#include <wchar.h>

#include <memory>

#include "app/common/Game.h"
#include "app/common/GameRules/LevelGeneration/StructureActions/XboxStructureActionPlaceBlock.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/AddItemRuleDefinition.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/levelgen/structure/BoundingBox.h"
#include "minecraft/world/level/levelgen/structure/StructurePiece.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "util/StringHelpers.h"

XboxStructureActionPlaceContainer::XboxStructureActionPlaceContainer() {
    m_tile = Tile::chest_Id;
}

XboxStructureActionPlaceContainer::~XboxStructureActionPlaceContainer() {
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        delete *it;
    }
}

// 4J-JEV: Super class handles attr-facing fine.
// void XboxStructureActionPlaceContainer::writeAttributes(DataOutputStream
// *dos, uint32_t numAttrs)

void XboxStructureActionPlaceContainer::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    XboxStructureActionPlaceBlock::getChildren(children);
    for (auto it = m_items.begin(); it != m_items.end(); it++)
        children->push_back(*it);
}

GameRuleDefinition* XboxStructureActionPlaceContainer::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_AddItem) {
        rule = new AddItemRuleDefinition();
        m_items.push_back((AddItemRuleDefinition*)rule);
    } else {
#ifndef _CONTENT_PACKAGE
        printf(
            "XboxStructureActionPlaceContainer: Attempted to add invalid "
            "child rule - %d\n",
            ruleType);
#endif
    }
    return rule;
}

void XboxStructureActionPlaceContainer::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("facing") == 0) {
        int value = fromWString<int>(attributeValue);
        m_data = value;
        app.DebugPrintf(
            "XboxStructureActionPlaceContainer: Adding parameter facing=%d\n",
            m_data);
    } else {
        XboxStructureActionPlaceBlock::addAttribute(attributeName,
                                                    attributeValue);
    }
}

bool XboxStructureActionPlaceContainer::placeContainerInLevel(
    StructurePiece* structure, Level* level, BoundingBox* chunkBB) {
    int worldX = structure->getWorldX(m_x, m_z);
    int worldY = structure->getWorldY(m_y);
    int worldZ = structure->getWorldZ(m_x, m_z);

    if (chunkBB->isInside(worldX, worldY, worldZ)) {
        if (level->getTileEntity(worldX, worldY, worldZ) != nullptr) {
            // Remove the current tile entity
            level->removeTileEntity(worldX, worldY, worldZ);
            level->setTileAndData(worldX, worldY, worldZ, 0, 0,
                                  Tile::UPDATE_ALL);
        }

        level->setTileAndData(worldX, worldY, worldZ, m_tile, 0,
                              Tile::UPDATE_ALL);
        std::shared_ptr<Container> container =
            std::dynamic_pointer_cast<Container>(
                level->getTileEntity(worldX, worldY, worldZ));

        app.DebugPrintf(
            "XboxStructureActionPlaceContainer - placing a container at "
            "(%d,%d,%d)\n",
            worldX, worldY, worldZ);
        if (container != nullptr) {
            level->setData(worldX, worldY, worldZ, m_data,
                           Tile::UPDATE_CLIENTS);
            // Add items
            int slotId = 0;
            for (auto it = m_items.begin();
                 it != m_items.end() &&
                 (slotId < container->getContainerSize());
                 ++it, ++slotId) {
                AddItemRuleDefinition* addItem = *it;

                addItem->addItemToContainer(container, slotId);
            }
        }
        return true;
    }
    return false;
}