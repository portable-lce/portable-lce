#include "minecraft/IGameServices.h"
#include "PigZombie.h"

#include <string>
#include <vector>

#include "java/Random.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/world/Difficulty.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/attributes/AttributeModifier.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/monster/Zombie.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"

AttributeModifier* PigZombie::SPEED_MODIFIER_ATTACKING =
    (new AttributeModifier(eModifierId_MOB_PIG_ATTACKSPEED, 0.45,
                           AttributeModifier::OPERATION_ADDITION))
        ->setSerialize(false);

void PigZombie::_init() {
    registerAttributes();

    angerTime = 0;
    playAngrySoundIn = 0;
    lastAttackTarget = nullptr;
}

PigZombie::PigZombie(Level* level) : Zombie(level) {
    _init();

    fireImmune = true;
}

void PigZombie::registerAttributes() {
    Zombie::registerAttributes();

    getAttribute(SPAWN_REINFORCEMENTS_CHANCE)->setBaseValue(0);
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.5f);
    getAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)->setBaseValue(5);
}

bool PigZombie::useNewAi() { return false; }

void PigZombie::tick() {
    if (lastAttackTarget != attackTarget && !level->isClientSide) {
        AttributeInstance* speed =
            getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
        speed->removeModifier(SPEED_MODIFIER_ATTACKING);

        if (attackTarget != nullptr) {
            speed->addModifier(
                new AttributeModifier(*SPEED_MODIFIER_ATTACKING));
        }
    }
    lastAttackTarget = attackTarget;

    if (playAngrySoundIn > 0) {
        if (--playAngrySoundIn == 0) {
            playSound(
                eSoundType_MOB_ZOMBIEPIG_ZPIGANGRY, getSoundVolume() * 2,
                ((random->nextFloat() - random->nextFloat()) * 0.2f + 1.0f) *
                    1.8f);
        }
    }
    Zombie::tick();
}

bool PigZombie::canSpawn() {
    return level->difficulty > Difficulty::PEACEFUL &&
           level->isUnobstructed(&bb) &&
           level->getCubes(shared_from_this(), &bb)->empty() &&
           !level->containsAnyLiquid(&bb);
}

void PigZombie::addAdditonalSaveData(CompoundTag* tag) {
    Zombie::addAdditonalSaveData(tag);
    tag->putShort("Anger", (short)angerTime);
}

void PigZombie::readAdditionalSaveData(CompoundTag* tag) {
    Zombie::readAdditionalSaveData(tag);
    angerTime = tag->getShort("Anger");
}

std::shared_ptr<Entity> PigZombie::findAttackTarget() {
#ifndef _FINAL_BUILD
#ifdef _DEBUG_MENUS_ENABLED
    if (gameServices().debugMobsDontAttack()) {
        return std::shared_ptr<Player>();
    }
#endif
#endif

    if (angerTime == 0) return nullptr;
    return Zombie::findAttackTarget();
}

bool PigZombie::hurt(DamageSource* source, float dmg) {
    std::shared_ptr<Entity> sourceEntity = source->getEntity();
    if (sourceEntity != nullptr && sourceEntity->instanceof(eTYPE_PLAYER)) {
        AABB grown = bb.grow(32, 32, 32);
        std::vector<std::shared_ptr<Entity> >* nearby =
            level->getEntities(shared_from_this(), &grown);
        auto itEnd = nearby->end();
        for (auto it = nearby->begin(); it != itEnd; it++) {
            std::shared_ptr<Entity> e = *it;  // nearby->at(i);
            if (e->instanceof(eTYPE_PIGZOMBIE)) {
                std::shared_ptr<PigZombie> pigZombie =
                    std::dynamic_pointer_cast<PigZombie>(e);
                pigZombie->alert(sourceEntity);
            }
        }
        alert(sourceEntity);
    }
    return Zombie::hurt(source, dmg);
}

void PigZombie::alert(std::shared_ptr<Entity> target) {
    attackTarget = target;
    angerTime = 20 * 20 + random->nextInt(20 * 20);
    playAngrySoundIn = random->nextInt(20 * 2);
}

int PigZombie::getAmbientSound() { return eSoundType_MOB_ZOMBIEPIG_AMBIENT; }

int PigZombie::getHurtSound() { return eSoundType_MOB_ZOMBIEPIG_HURT; }

int PigZombie::getDeathSound() { return eSoundType_MOB_ZOMBIEPIG_DEATH; }

void PigZombie::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) {
    int count = random->nextInt(2 + playerBonusLevel);
    for (int i = 0; i < count; i++) {
        spawnAtLocation(Item::rotten_flesh_Id, 1);
    }
    count = random->nextInt(2 + playerBonusLevel);
    for (int i = 0; i < count; i++) {
        spawnAtLocation(Item::goldNugget_Id, 1);
    }
}

bool PigZombie::mobInteract(std::shared_ptr<Player> player) { return false; }

void PigZombie::dropRareDeathLoot(int rareLootLevel) {
    spawnAtLocation(Item::goldIngot_Id, 1);
}

int PigZombie::getDeathLoot() { return Item::rotten_flesh_Id; }

void PigZombie::populateDefaultEquipmentSlots() {
    setEquippedSlot(SLOT_WEAPON, std::shared_ptr<ItemInstance>(
                                     new ItemInstance(Item::sword_gold)));
}

MobGroupData* PigZombie::finalizeMobSpawn(
    MobGroupData* groupData, int extraData /*= 0*/)  // 4J Added extraData param
{
    Zombie::finalizeMobSpawn(groupData);
    setVillager(false);
    return groupData;
}
