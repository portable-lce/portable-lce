#include "ConsoleGenerateStructure.h"

#include <wchar.h>

#include <algorithm>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "app/common/GameRules/LevelGeneration/ConsoleGenerateStructureAction.h"
#include "app/common/GameRules/LevelGeneration/StructureActions/XboxStructureActionGenerateBox.h"
#include "app/common/GameRules/LevelGeneration/StructureActions/XboxStructureActionPlaceBlock.h"
#include "app/common/GameRules/LevelGeneration/StructureActions/XboxStructureActionPlaceContainer.h"
#include "app/common/GameRules/LevelGeneration/StructureActions/XboxStructureActionPlaceSpawner.h"
#include "app/common/GameRules/LevelRules/RuleDefinitions/GameRuleDefinition.h"
#include "app/linux/LinuxGame.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/Direction.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/levelgen/structure/BoundingBox.h"

ConsoleGenerateStructure::ConsoleGenerateStructure() : StructurePiece(0) {
    m_x = m_y = m_z = 0;
    boundingBox = nullptr;
    orientation = Direction::NORTH;
    m_dimension = 0;
}

void ConsoleGenerateStructure::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    GameRuleDefinition::getChildren(children);

    for (auto it = m_actions.begin(); it != m_actions.end(); it++)
        children->push_back(*it);
}

GameRuleDefinition* ConsoleGenerateStructure::addChild(
    ConsoleGameRules::EGameRuleType ruleType) {
    GameRuleDefinition* rule = nullptr;
    if (ruleType == ConsoleGameRules::eGameRuleType_GenerateBox) {
        rule = new XboxStructureActionGenerateBox();
        m_actions.push_back((XboxStructureActionGenerateBox*)rule);
    } else if (ruleType == ConsoleGameRules::eGameRuleType_PlaceBlock) {
        rule = new XboxStructureActionPlaceBlock();
        m_actions.push_back((XboxStructureActionPlaceBlock*)rule);
    } else if (ruleType == ConsoleGameRules::eGameRuleType_PlaceContainer) {
        rule = new XboxStructureActionPlaceContainer();
        m_actions.push_back((XboxStructureActionPlaceContainer*)rule);
    } else if (ruleType == ConsoleGameRules::eGameRuleType_PlaceSpawner) {
        rule = new XboxStructureActionPlaceSpawner();
        m_actions.push_back((XboxStructureActionPlaceSpawner*)rule);
    } else {
#ifndef _CONTENT_PACKAGE
        printf(
            "ConsoleGenerateStructure: Attempted to add invalid child rule - "
            "%d\n",
            ruleType);
#endif
    }
    return rule;
}

void ConsoleGenerateStructure::writeAttributes(DataOutputStream* dos,
                                               unsigned int numAttrs) {
    GameRuleDefinition::writeAttributes(dos, numAttrs + 5);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x);
    dos->writeUTF(toWString(m_x));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y);
    dos->writeUTF(toWString(m_y));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z);
    dos->writeUTF(toWString(m_z));

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_orientation);
    dos->writeUTF(toWString(orientation));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_dimension);
    dos->writeUTF(toWString(m_dimension));
}

void ConsoleGenerateStructure::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("x") == 0) {
        int value = fromWString<int>(attributeValue);
        m_x = value;
        app.DebugPrintf("ConsoleGenerateStructure: Adding parameter x=%d\n",
                        m_x);
    } else if (attributeName.compare("y") == 0) {
        int value = fromWString<int>(attributeValue);
        m_y = value;
        app.DebugPrintf("ConsoleGenerateStructure: Adding parameter y=%d\n",
                        m_y);
    } else if (attributeName.compare("z") == 0) {
        int value = fromWString<int>(attributeValue);
        m_z = value;
        app.DebugPrintf("ConsoleGenerateStructure: Adding parameter z=%d\n",
                        m_z);
    } else if (attributeName.compare("orientation") == 0) {
        int value = fromWString<int>(attributeValue);
        orientation = value;
        app.DebugPrintf(
            "ConsoleGenerateStructure: Adding parameter orientation=%d\n",
            orientation);
    } else if (attributeName.compare("dim") == 0) {
        m_dimension = fromWString<int>(attributeValue);
        if (m_dimension > 1 || m_dimension < -1) m_dimension = 0;
        app.DebugPrintf(
            "ApplySchematicRuleDefinition: Adding parameter dimension=%d\n",
            m_dimension);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

BoundingBox* ConsoleGenerateStructure::getBoundingBox() {
    if (boundingBox == nullptr) {
        // Find the max bounds
        int maxX, maxY, maxZ;
        maxX = maxY = maxZ = 1;
        for (auto it = m_actions.begin(); it != m_actions.end(); ++it) {
            ConsoleGenerateStructureAction* action = *it;
            maxX = std::max(maxX, action->getEndX());
            maxY = std::max(maxY, action->getEndY());
            maxZ = std::max(maxZ, action->getEndZ());
        }

        boundingBox =
            new BoundingBox(m_x, m_y, m_z, m_x + maxX, m_y + maxY, m_z + maxZ);
    }
    return boundingBox;
}

bool ConsoleGenerateStructure::postProcess(Level* level, Random* random,
                                           BoundingBox* chunkBB) {
    if (level->dimension->id != m_dimension) return false;

    for (auto it = m_actions.begin(); it != m_actions.end(); ++it) {
        ConsoleGenerateStructureAction* action = *it;

        switch (action->getActionType()) {
            case ConsoleGameRules::eGameRuleType_GenerateBox: {
                XboxStructureActionGenerateBox* genBox =
                    (XboxStructureActionGenerateBox*)action;
                genBox->generateBoxInLevel(this, level, chunkBB);
            } break;
            case ConsoleGameRules::eGameRuleType_PlaceBlock: {
                XboxStructureActionPlaceBlock* pPlaceBlock =
                    (XboxStructureActionPlaceBlock*)action;
                pPlaceBlock->placeBlockInLevel(this, level, chunkBB);
            } break;
            case ConsoleGameRules::eGameRuleType_PlaceContainer: {
                XboxStructureActionPlaceContainer* pPlaceContainer =
                    (XboxStructureActionPlaceContainer*)action;
                pPlaceContainer->placeContainerInLevel(this, level, chunkBB);
            } break;
            case ConsoleGameRules::eGameRuleType_PlaceSpawner: {
                XboxStructureActionPlaceSpawner* pPlaceSpawner =
                    (XboxStructureActionPlaceSpawner*)action;
                pPlaceSpawner->placeSpawnerInLevel(this, level, chunkBB);
            } break;
            default:
                break;
        };
    }

    return false;
}

bool ConsoleGenerateStructure::checkIntersects(int x0, int y0, int z0, int x1,
                                               int y1, int z1) {
    return getBoundingBox()->intersects(x0, y0, z0, x1, y1, z1);
}

int ConsoleGenerateStructure::getMinY() { return getBoundingBox()->y0; }
