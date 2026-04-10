#pragma once
#include <memory>

#include "DispenseItemBehavior.h"
#include "minecraft/world/item/ItemInstance.h"

class FacingEnum;
class Position;
class BlockSource;
class Level;

class DefaultDispenseItemBehavior : public DispenseItemBehavior {
protected:
    enum eOUTCOME {
        // Item has special behaviour that was executed successfully.
        ACTIVATED_ITEM = 0,

        // Item was dispenced onto the ground as a pickup.
        DISPENCED_ITEM = 1,

        // Execution failed, the item was left unaffected.
        LEFT_ITEM = 2,
    };

public:
    DefaultDispenseItemBehavior() {};
    virtual ~DefaultDispenseItemBehavior() {};
    virtual std::shared_ptr<ItemInstance> dispense(
        BlockSource* source, std::shared_ptr<ItemInstance> dispensed);

protected:
    // 4J-JEV: Added value used to play FAILED sound effect upon reaching spawn
    // limits.
    virtual std::shared_ptr<ItemInstance> execute(
        BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
        eOUTCOME& outcome);

public:
    static void spawnItem(Level* world, std::shared_ptr<ItemInstance> item,
                          int accuracy, FacingEnum* facing, Position* position);

protected:
    virtual void playSound(BlockSource* source, eOUTCOME outcome);
    virtual void playAnimation(BlockSource* source, FacingEnum* facing,
                               eOUTCOME outcome);

private:
    virtual int getLevelEventDataFrom(FacingEnum* facing);
};