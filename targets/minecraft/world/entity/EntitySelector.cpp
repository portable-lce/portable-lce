#include "EntitySelector.h"

#include "java/Class.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"

class ItemInstance;

const EntitySelector* EntitySelector::ENTITY_STILL_ALIVE =
    new AliveEntitySelector();
const EntitySelector* EntitySelector::CONTAINER_ENTITY_SELECTOR =
    new ContainerEntitySelector();

bool AliveEntitySelector::matches(std::shared_ptr<Entity> entity) const {
    return entity->isAlive();
}

bool ContainerEntitySelector::matches(std::shared_ptr<Entity> entity) const {
    return (std::dynamic_pointer_cast<Container>(entity) != nullptr) &&
           entity->isAlive();
}

MobCanWearArmourEntitySelector::MobCanWearArmourEntitySelector(
    std::shared_ptr<ItemInstance> item) {
    this->item = item;
}

bool MobCanWearArmourEntitySelector::matches(
    std::shared_ptr<Entity> entity) const {
    if (!entity->isAlive()) return false;
    if (!entity->instanceof (eTYPE_LIVINGENTITY)) return false;

    std::shared_ptr<LivingEntity> mob =
        std::dynamic_pointer_cast<LivingEntity>(entity);

    if (mob->getCarried(Mob::getEquipmentSlotForItem(item)) != nullptr)
        return false;

    if (mob->instanceof (eTYPE_MOB)) {
        return std::dynamic_pointer_cast<Mob>(mob)->canPickUpLoot();
    } else if (mob->instanceof (eTYPE_PLAYER)) {
        return true;
    }

    return false;
}