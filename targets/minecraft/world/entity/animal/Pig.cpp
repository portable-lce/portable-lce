#include "Pig.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "java/Random.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/goal/BreedGoal.h"
#include "minecraft/world/entity/ai/goal/ControlledByPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/FloatGoal.h"
#include "minecraft/world/entity/ai/goal/FollowParentGoal.h"
#include "minecraft/world/entity/ai/goal/GoalSelector.h"
#include "minecraft/world/entity/ai/goal/LookAtPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/PanicGoal.h"
#include "minecraft/world/entity/ai/goal/RandomLookAroundGoal.h"
#include "minecraft/world/entity/ai/goal/RandomStrollGoal.h"
#include "minecraft/world/entity/ai/goal/TemptGoal.h"
#include "minecraft/world/entity/ai/navigation/PathNavigation.h"
#include "minecraft/world/entity/animal/Animal.h"
#include "minecraft/world/entity/monster/PigZombie.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "nbt/CompoundTag.h"

Pig::Pig(Level* level) : Animal(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());

    setSize(0.9f, 0.9f);

    getNavigation()->setAvoidWater(true);
    goalSelector.addGoal(0, new FloatGoal(this));
    goalSelector.addGoal(1, new PanicGoal(this, 1.25));
    goalSelector.addGoal(
        2, controlGoal = new ControlledByPlayerGoal(this, 0.3f, 0.25f));
    goalSelector.addGoal(3, new BreedGoal(this, 1.0));
    goalSelector.addGoal(
        4, new TemptGoal(this, 1.2, Item::carrotOnAStick_Id, false));
    goalSelector.addGoal(4, new TemptGoal(this, 1.2, Item::carrots_Id, false));
    goalSelector.addGoal(5, new FollowParentGoal(this, 1.1));
    goalSelector.addGoal(6, new RandomStrollGoal(this, 1.0));
    goalSelector.addGoal(7, new LookAtPlayerGoal(this, typeid(Player), 6));
    goalSelector.addGoal(8, new RandomLookAroundGoal(this));
}

bool Pig::useNewAi() { return true; }

void Pig::registerAttributes() {
    Animal::registerAttributes();

    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(10);
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.25f);
}

void Pig::newServerAiStep() { Animal::newServerAiStep(); }

bool Pig::canBeControlledByRider() {
    std::shared_ptr<ItemInstance> item =
        std::dynamic_pointer_cast<Player>(rider.lock())->getCarriedItem();

    return item != nullptr && item->id == Item::carrotOnAStick_Id;
}

void Pig::defineSynchedData() {
    Animal::defineSynchedData();
    entityData->define(DATA_SADDLE_ID, (uint8_t)0);
}

void Pig::addAdditonalSaveData(CompoundTag* tag) {
    Animal::addAdditonalSaveData(tag);
    tag->putBoolean("Saddle", hasSaddle());
}

void Pig::readAdditionalSaveData(CompoundTag* tag) {
    Animal::readAdditionalSaveData(tag);
    setSaddle(tag->getBoolean("Saddle"));
}

int Pig::getAmbientSound() { return eSoundType_MOB_PIG_AMBIENT; }

int Pig::getHurtSound() { return eSoundType_MOB_PIG_AMBIENT; }

int Pig::getDeathSound() { return eSoundType_MOB_PIG_DEATH; }

void Pig::playStepSound(int xt, int yt, int zt, int t) {
    playSound(eSoundType_MOB_PIG_STEP, 0.15f, 1);
}

bool Pig::mobInteract(std::shared_ptr<Player> player) {
    if (!Animal::mobInteract(player)) {
        if (hasSaddle() && !level->isClientSide &&
            (rider.lock() == nullptr || rider.lock() == player)) {
            // 4J HEG - Fixed issue with player not being able to dismount pig
            // (issue #4479)
            player->ride(rider.lock() == player ? nullptr : shared_from_this());
            return true;
        }
        return false;
    }
    return true;
}

int Pig::getDeathLoot() {
    if (this->isOnFire()) return Item::porkChop_cooked->id;
    return Item::porkChop_raw_Id;
}

void Pig::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) {
    int count = random->nextInt(3) + 1 + random->nextInt(1 + playerBonusLevel);

    for (int i = 0; i < count; i++) {
        if (isOnFire()) {
            spawnAtLocation(Item::porkChop_cooked_Id, 1);
        } else {
            spawnAtLocation(Item::porkChop_raw_Id, 1);
        }
    }
    if (hasSaddle()) spawnAtLocation(Item::saddle_Id, 1);
}

bool Pig::hasSaddle() { return (entityData->getByte(DATA_SADDLE_ID) & 1) != 0; }

void Pig::setSaddle(bool value) {
    if (value) {
        entityData->set(DATA_SADDLE_ID, (uint8_t)1);
    } else {
        entityData->set(DATA_SADDLE_ID, (uint8_t)0);
    }
}

void Pig::thunderHit(const LightningBolt* lightningBolt) {
    if (level->isClientSide) return;
    std::shared_ptr<PigZombie> pz = std::make_shared<PigZombie>(level);
    pz->moveTo(x, y, z, yRot, xRot);
    level->addEntity(pz);
    remove();
}

void Pig::causeFallDamage(float distance) {
    Animal::causeFallDamage(distance);
    if ((distance > 5) && rider.lock() != nullptr && rider.lock()->instanceof
        (eTYPE_PLAYER)) {
        (std::dynamic_pointer_cast<Player>(rider.lock()))
            ->awardStat(GenericStats::flyPig(), GenericStats::param_flyPig());
    }
}

std::shared_ptr<AgableMob> Pig::getBreedOffspring(
    std::shared_ptr<AgableMob> target) {
    // 4J - added limit to number of animals that can be bred
    if (level->canCreateMore(GetType(), Level::eSpawnType_Breed)) {
        return std::make_shared<Pig>(level);
    } else {
        return nullptr;
    }
}

bool Pig::isFood(std::shared_ptr<ItemInstance> itemInstance) {
    return itemInstance != nullptr && itemInstance->id == Item::carrots_Id;
}

ControlledByPlayerGoal* Pig::getControlGoal() { return controlGoal; }
