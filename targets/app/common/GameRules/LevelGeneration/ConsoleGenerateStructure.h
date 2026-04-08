#pragma once
#include <string>
#include <vector>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/levelgen/structure/StructureFeatureIO.h"
#include "minecraft/world/level/levelgen/structure/StructurePiece.h"

class Level;
class Random;
class BoundingBox;
class ConsoleGenerateStructureAction;
class XboxStructureActionPlaceContainer;
class GRFObject;

class ConsoleGenerateStructure : public GameRuleDefinition,
                                 public StructurePiece {
private:
    int m_x, m_y, m_z;
    std::vector<ConsoleGenerateStructureAction*> m_actions;
    int m_dimension;

public:
    ConsoleGenerateStructure();

    virtual ConsoleGameRules::EGameRuleType getActionType() {
        return ConsoleGameRules::eGameRuleType_GenerateStructure;
    }

    virtual void getChildren(std::vector<GameRuleDefinition*>* children);
    virtual GameRuleDefinition* addChild(
        ConsoleGameRules::EGameRuleType ruleType);

    virtual void writeAttributes(DataOutputStream* dos, unsigned int numAttrs);
    virtual void addAttribute(const std::string& attributeName,
                              const std::string& attributeValue);

    // StructurePiece
    virtual BoundingBox* getBoundingBox();
    virtual bool postProcess(Level* level, Random* random,
                             BoundingBox* chunkBB);

    void createContainer(XboxStructureActionPlaceContainer* action,
                         Level* level, BoundingBox* chunkBB);

    bool checkIntersects(int x0, int y0, int z0, int x1, int y1, int z1);

    virtual int getMinY();

    EStructurePiece GetType() { return (EStructurePiece)0; }
    void addAdditonalSaveData(CompoundTag* tag) {}
    void readAdditonalSaveData(CompoundTag* tag) {}
};