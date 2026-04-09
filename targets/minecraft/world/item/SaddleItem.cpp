#include "SaddleItem.h"

#include <memory>

#include "java/Class.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/Pig.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"

SaddleItem::SaddleItem(int id) : Item(id) { maxStackSize = 1; }

bool SaddleItem::interactEnemy(std::shared_ptr<ItemInstance> itemInstance,
                               std::shared_ptr<Player> player,
                               std::shared_ptr<LivingEntity> mob) {
    if ((mob != nullptr) && mob->instanceof (eTYPE_PIG)) {
        std::shared_ptr<Pig> pig = std::dynamic_pointer_cast<Pig>(mob);
        if (!pig->hasSaddle() && !pig->isBaby()) {
            pig->setSaddle(true);
            itemInstance->count--;
        }
        return true;
    }
    return false;
}

bool SaddleItem::hurtEnemy(std::shared_ptr<ItemInstance> itemInstance,
                           std::shared_ptr<LivingEntity> mob,
                           std::shared_ptr<LivingEntity> attacker) {
    interactEnemy(itemInstance, nullptr, mob);
    return true;
}