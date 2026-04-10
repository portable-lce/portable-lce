#include "ItemDispenseBehaviors.h"

#include <memory>
#include <string>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/core/AbstractProjectileDispenseBehavior.h"
#include "minecraft/core/BlockSource.h"
#include "minecraft/core/DefaultDispenseItemBehavior.h"
#include "minecraft/core/FacingEnum.h"
#include "minecraft/core/Position.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/item/Boat.h"
#include "minecraft/world/entity/item/PrimedTnt.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "minecraft/world/entity/projectile/FireworksRocketEntity.h"
#include "minecraft/world/entity/projectile/SmallFireball.h"
#include "minecraft/world/entity/projectile/Snowball.h"
#include "minecraft/world/entity/projectile/ThrownEgg.h"
#include "minecraft/world/entity/projectile/ThrownExpBottle.h"
#include "minecraft/world/entity/projectile/ThrownPotion.h"
#include "minecraft/world/item/BucketItem.h"
#include "minecraft/world/item/DyePowderItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/item/SpawnEggItem.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/DispenserTile.h"
#include "minecraft/world/level/tile/LevelEvent.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/DispenserTileEntity.h"

/* Arrow */

std::shared_ptr<Projectile> ArrowDispenseBehavior::getProjectile(
    Level* world, Position* position) {
    std::shared_ptr<Arrow> arrow = std::shared_ptr<Arrow>(
        new Arrow(world, position->getX(), position->getY(), position->getZ()));
    arrow->pickup = Arrow::PICKUP_ALLOWED;

    return arrow;
}

/* ThrownEgg */

std::shared_ptr<Projectile> EggDispenseBehavior::getProjectile(
    Level* world, Position* position) {
    return std::make_shared<ThrownEgg>(world, position->getX(),
                                       position->getY(), position->getZ());
}

/* Snowball */

std::shared_ptr<Projectile> SnowballDispenseBehavior::getProjectile(
    Level* world, Position* position) {
    return std::make_shared<Snowball>(world, position->getX(), position->getY(),
                                      position->getZ());
}

/* Exp Bottle */

std::shared_ptr<Projectile> ExpBottleDispenseBehavior::getProjectile(
    Level* world, Position* position) {
    return std::make_shared<ThrownExpBottle>(
        world, position->getX(), position->getY(), position->getZ());
}

float ExpBottleDispenseBehavior::getUncertainty() {
    return AbstractProjectileDispenseBehavior::getUncertainty() * .5f;
}

float ExpBottleDispenseBehavior::getPower() {
    return AbstractProjectileDispenseBehavior::getPower() * 1.25f;
}

/* Thrown Potion */

ThrownPotionDispenseBehavior::ThrownPotionDispenseBehavior(int potionValue) {
    m_potionValue = potionValue;
}

std::shared_ptr<Projectile> ThrownPotionDispenseBehavior::getProjectile(
    Level* world, Position* position) {
    return std::shared_ptr<Projectile>(
        new ThrownPotion(world, position->getX(), position->getY(),
                         position->getZ(), m_potionValue));
}

float ThrownPotionDispenseBehavior::getUncertainty() {
    return AbstractProjectileDispenseBehavior::getUncertainty() * .5f;
}

float ThrownPotionDispenseBehavior::getPower() {
    return AbstractProjectileDispenseBehavior::getPower() * 1.25f;
}

/* Potion */

std::shared_ptr<ItemInstance> PotionDispenseBehavior::dispense(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed) {
    if (PotionItem::isThrowable(dispensed->getAuxValue())) {
        return ThrownPotionDispenseBehavior(dispensed->getAuxValue())
            .dispense(source, dispensed);
    } else {
        return DefaultDispenseItemBehavior::dispense(source, dispensed);
    }
}

/* SpawnEggItem */

std::shared_ptr<ItemInstance> SpawnEggDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    FacingEnum* facing = DispenserTile::getFacing(source->getData());

    // Spawn entity in the middle of the block in front of the dispenser
    double spawnX = source->getX() + facing->getStepX();
    double spawnY = source->getBlockY() + .2f;  // Above pressure plates
    double spawnZ = source->getZ() + facing->getStepZ();

    int iResult = 0;
    std::shared_ptr<Entity> entity =
        SpawnEggItem::spawnMobAt(source->getWorld(), dispensed->getAuxValue(),
                                 spawnX, spawnY, spawnZ, &iResult);

    // 4J-JEV: Added in-case spawn limit is encountered.
    if (entity == nullptr) {
        outcome = LEFT_ITEM;
        return dispensed;
    }

    if (entity->instanceof(eTYPE_MOB) && dispensed->hasCustomHoverName()) {
        std::dynamic_pointer_cast<Mob>(entity)->setCustomName(
            dispensed->getHoverName());
    }

    outcome = ACTIVATED_ITEM;

    dispensed->remove(1);
    return dispensed;
}

/* Fireworks*/

std::shared_ptr<ItemInstance> FireworksDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    Level* world = source->getWorld();
    if (world->countInstanceOf(eTYPE_PROJECTILE, false) >=
        Level::MAX_DISPENSABLE_PROJECTILES) {
        outcome = LEFT_ITEM;
        return dispensed;
    }

    FacingEnum* facing = DispenserTile::getFacing(source->getData());

    double spawnX = source->getX() + facing->getStepX();
    double spawnY = source->getBlockY() + .2f;
    double spawnZ = source->getZ() + facing->getStepZ();

    std::shared_ptr<FireworksRocketEntity> firework =
        std::make_shared<FireworksRocketEntity>(world, spawnX, spawnY, spawnZ,
                                                dispensed);
    source->getWorld()->addEntity(firework);

    outcome = ACTIVATED_ITEM;

    dispensed->remove(1);
    return dispensed;
}

void FireworksDispenseBehavior::playSound(BlockSource* source,
                                          eOUTCOME outcome) {
    // 4J-JEV: This is exactly the same as the default at the moment.
    // source->getWorld()->levelEvent(LevelEvent::SOUND_CLICK,
    // source->getBlockX(), source->getBlockY(), source->getBlockZ(), 0);

    DefaultDispenseItemBehavior::playSound(source, outcome);
}

/* Fireballs */

std::shared_ptr<ItemInstance> FireballDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    Level* world = source->getWorld();
    if (world->countInstanceOf(eTYPE_SMALL_FIREBALL, true) >=
        Level::MAX_DISPENSABLE_FIREBALLS) {
        outcome = LEFT_ITEM;
        return dispensed;
    }

    FacingEnum* facing = DispenserTile::getFacing(source->getData());

    Position* position = DispenserTile::getDispensePosition(source);
    double spawnX = position->getX() + facing->getStepX() * .3f;
    double spawnY = position->getY() + facing->getStepX() * .3f;
    double spawnZ = position->getZ() + facing->getStepZ() * .3f;

    delete position;

    Random* random = world->random;

    double dirX = random->nextGaussian() * .05 + facing->getStepX();
    double dirY = random->nextGaussian() * .05 + facing->getStepY();
    double dirZ = random->nextGaussian() * .05 + facing->getStepZ();

    world->addEntity(std::shared_ptr<SmallFireball>(
        new SmallFireball(world, spawnX, spawnY, spawnZ, dirX, dirY, dirZ)));

    outcome = ACTIVATED_ITEM;

    dispensed->remove(1);
    return dispensed;
}

void FireballDispenseBehavior::playSound(BlockSource* source,
                                         eOUTCOME outcome) {
    if (outcome == ACTIVATED_ITEM) {
        source->getWorld()->levelEvent(LevelEvent::SOUND_BLAZE_FIREBALL,
                                       source->getBlockX(), source->getBlockY(),
                                       source->getBlockZ(), 0);
    } else {
        DefaultDispenseItemBehavior::playSound(source, outcome);
    }
}

/* Boats */

BoatDispenseBehavior::BoatDispenseBehavior() : DefaultDispenseItemBehavior() {
    defaultDispenseItemBehavior = new DefaultDispenseItemBehavior();
}

BoatDispenseBehavior::~BoatDispenseBehavior() {
    delete defaultDispenseItemBehavior;
}

std::shared_ptr<ItemInstance> BoatDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    FacingEnum* facing = DispenserTile::getFacing(source->getData());
    Level* world = source->getWorld();

    // Spawn the boat 'just' outside the dispenser, it overlaps 2 'pixels' now.
    double spawnX = source->getX() + facing->getStepX() * (1 + 2.0f / 16);
    double spawnY = source->getY() + facing->getStepY() * (1 + 2.0f / 16);
    double spawnZ = source->getZ() + facing->getStepZ() * (1 + 2.0f / 16);

    int frontX = source->getBlockX() + facing->getStepX();
    int frontY = source->getBlockY() + facing->getStepY();
    int frontZ = source->getBlockZ() + facing->getStepZ();
    Material* inFront = world->getMaterial(frontX, frontY, frontZ);

    double yOffset;

    // 4J: If we're at limit, just dispense item (instead of adding boat)
    if (world->countInstanceOf(eTYPE_BOAT, true) >= Level::MAX_XBOX_BOATS) {
        return defaultDispenseItemBehavior->dispense(source, dispensed);
    }

    if (Material::water == inFront) {
        yOffset = 1;
    } else if (Material::air == inFront &&
               Material::water ==
                   world->getMaterial(frontX, frontY - 1, frontZ)) {
        yOffset = 0;
    } else {
        return defaultDispenseItemBehavior->dispense(source, dispensed);
    }

    outcome = ACTIVATED_ITEM;

    std::shared_ptr<Boat> boat = std::shared_ptr<Boat>(
        new Boat(world, spawnX, spawnY + yOffset, spawnZ));
    world->addEntity(boat);

    dispensed->remove(1);
    return dispensed;
}

void BoatDispenseBehavior::playSound(BlockSource* source, eOUTCOME outcome) {
    // 4J-JEV: This is exactly the same as the default at the moment.
    // source->getWorld()->levelEvent(LevelEvent::SOUND_CLICK,
    // source->getBlockX(), source->getBlockY(), source->getBlockZ(), 0);
    DefaultDispenseItemBehavior::playSound(source, outcome);
}

/* FilledBucket */

std::shared_ptr<ItemInstance> FilledBucketDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    BucketItem* bucket = (BucketItem*)dispensed->getItem();
    int sourceX = source->getBlockX();
    int sourceY = source->getBlockY();
    int sourceZ = source->getBlockZ();

    FacingEnum* facing = DispenserTile::getFacing(source->getData());
    if (bucket->emptyBucket(source->getWorld(), sourceX + facing->getStepX(),
                            sourceY + facing->getStepY(),
                            sourceZ + facing->getStepZ())) {
        dispensed->id = Item::bucket_empty->id;
        dispensed->count = 1;

        outcome = ACTIVATED_ITEM;
        return dispensed;
    }

    return DefaultDispenseItemBehavior::dispense(source, dispensed);
}

/* EmptyBucket */

std::shared_ptr<ItemInstance> EmptyBucketDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    FacingEnum* facing = DispenserTile::getFacing(source->getData());
    Level* world = source->getWorld();

    int targetX = source->getBlockX() + facing->getStepX();
    int targetY = source->getBlockY() + facing->getStepY();
    int targetZ = source->getBlockZ() + facing->getStepZ();

    Material* material = world->getMaterial(targetX, targetY, targetZ);
    int dataValue = world->getData(targetX, targetY, targetZ);

    Item* targetType;
    if (Material::water == material && dataValue == 0) {
        targetType = Item::bucket_water;
    } else if (Material::lava == material && dataValue == 0) {
        targetType = Item::bucket_lava;
    } else {
        return DefaultDispenseItemBehavior::execute(source, dispensed, outcome);
    }

    world->removeTile(targetX, targetY, targetZ);
    if (--dispensed->count == 0) {
        dispensed->id = targetType->id;
        dispensed->count = 1;
    } else if (std::dynamic_pointer_cast<DispenserTileEntity>(
                   source->getEntity())
                   ->addItem(std::shared_ptr<ItemInstance>(
                       new ItemInstance(targetType))) < 0) {
        DefaultDispenseItemBehavior::dispense(
            source, std::make_shared<ItemInstance>(targetType));
    }

    outcome = ACTIVATED_ITEM;
    return dispensed;
}

/* Flint and Steel */

std::shared_ptr<ItemInstance> FlintAndSteelDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    outcome = ACTIVATED_ITEM;

    FacingEnum* facing = DispenserTile::getFacing(source->getData());
    Level* world = source->getWorld();

    int targetX = source->getBlockX() + facing->getStepX();
    int targetY = source->getBlockY() + facing->getStepY();
    int targetZ = source->getBlockZ() + facing->getStepZ();

    if (world->isEmptyTile(targetX, targetY, targetZ)) {
        world->setTileAndUpdate(targetX, targetY, targetZ, Tile::fire_Id);

        if (dispensed->hurt(1, world->random)) {
            dispensed->count = 0;
        }
    } else if (world->getTile(targetX, targetY, targetZ) == Tile::tnt_Id) {
        Tile::tnt->destroy(world, targetX, targetY, targetZ, 1);
        world->removeTile(targetX, targetY, targetZ);
    } else {
        outcome = LEFT_ITEM;
    }

    return dispensed;
}

void FlintAndSteelDispenseBehavior::playSound(BlockSource* source,
                                              eOUTCOME outcome) {
    if (outcome == ACTIVATED_ITEM) {
        source->getWorld()->levelEvent(LevelEvent::SOUND_CLICK,
                                       source->getBlockX(), source->getBlockY(),
                                       source->getBlockZ(), 0);
    } else {
        source->getWorld()->levelEvent(LevelEvent::SOUND_CLICK_FAIL,
                                       source->getBlockX(), source->getBlockY(),
                                       source->getBlockZ(), 0);
    }
}

/* Dye */

std::shared_ptr<ItemInstance> DyeDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    if (dispensed->getAuxValue() == DyePowderItem::WHITE) {
        FacingEnum* facing = DispenserTile::getFacing(source->getData());
        Level* world = source->getWorld();

        int targetX = source->getBlockX() + facing->getStepX();
        int targetY = source->getBlockY() + facing->getStepY();
        int targetZ = source->getBlockZ() + facing->getStepZ();

        if (DyePowderItem::growCrop(dispensed, world, targetX, targetY, targetZ,
                                    false)) {
            if (!world->isClientSide)
                world->levelEvent(LevelEvent::PARTICLES_PLANT_GROWTH, targetX,
                                  targetY, targetZ, 0);
            outcome = ACTIVATED_ITEM;
        } else {
            outcome = LEFT_ITEM;
        }

        return dispensed;
    } else {
        return DefaultDispenseItemBehavior::execute(source, dispensed, outcome);
    }
}

void DyeDispenseBehavior::playSound(BlockSource* source, eOUTCOME outcome) {
    if (outcome == ACTIVATED_ITEM) {
        source->getWorld()->levelEvent(LevelEvent::SOUND_CLICK,
                                       source->getBlockX(), source->getBlockY(),
                                       source->getBlockZ(), 0);
    } else {
        source->getWorld()->levelEvent(LevelEvent::SOUND_CLICK_FAIL,
                                       source->getBlockX(), source->getBlockY(),
                                       source->getBlockZ(), 0);
    }
}

/* TNT */

std::shared_ptr<ItemInstance> TntDispenseBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    FacingEnum* facing = DispenserTile::getFacing(source->getData());
    Level* world = source->getWorld();

    if (world->newPrimedTntAllowed() &&
        gameServices().getGameHostOption(eGameHostOption_TNT)) {
        int targetX = source->getBlockX() + facing->getStepX();
        int targetY = source->getBlockY() + facing->getStepY();
        int targetZ = source->getBlockZ() + facing->getStepZ();

        std::shared_ptr<PrimedTnt> tnt = std::shared_ptr<PrimedTnt>(
            new PrimedTnt(world, targetX + 0.5f, targetY + 0.5f, targetZ + 0.5f,
                          nullptr));
        world->addEntity(tnt);

        outcome = ACTIVATED_ITEM;

        dispensed->count--;
    } else {
        outcome = LEFT_ITEM;
    }
    return dispensed;
}