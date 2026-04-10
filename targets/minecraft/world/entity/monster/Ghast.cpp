#include "Ghast.h"

#include <math.h>
#include <stdint.h>

#include <numbers>
#include <string>
#include <vector>

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/world/Difficulty.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/FlyingMob.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/monster/Enemy.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/LargeFireball.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/LevelEvent.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/Vec3.h"
#include "nbt/CompoundTag.h"

void Ghast::_init() {
    explosionPower = 1;
    floatDuration = 0;
    target = nullptr;
    retargetTime = 0;
    oCharge = 0;
    charge = 0;

    xTarget = 0.0f;
    yTarget = 0.0f;
    zTarget = 0.0f;
}

Ghast::Ghast(Level* level) : FlyingMob(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());

    _init();

    setSize(4, 4);
    fireImmune = true;
    xpReward = Enemy::XP_REWARD_MEDIUM;
}

bool Ghast::isCharging() { return entityData->getByte(DATA_IS_CHARGING) != 0; }

bool Ghast::hurt(DamageSource* source, float dmg) {
    if (isInvulnerable()) return false;
    if (source->getMsgId() == ChatPacket::e_ChatDeathFireball) {
        if ((source->getEntity() != nullptr) &&
            source->getEntity()->instanceof(eTYPE_PLAYER)) {
            // reflected fireball, kill the ghast
            FlyingMob::hurt(source, 1000);
            std::dynamic_pointer_cast<Player>(source->getEntity())
                ->awardStat(GenericStats::ghast(), GenericStats::param_ghast());
            return true;
        }
    }

    return FlyingMob::hurt(source, dmg);
}

void Ghast::defineSynchedData() {
    FlyingMob::defineSynchedData();

    entityData->define(DATA_IS_CHARGING, (uint8_t)0);
}

void Ghast::registerAttributes() {
    FlyingMob::registerAttributes();

    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(10);
}

void Ghast::serverAiStep() {
    if (!level->isClientSide && level->difficulty == Difficulty::PEACEFUL)
        remove();
    checkDespawn();

    oCharge = charge;
    double xd = xTarget - x;
    double yd = yTarget - y;
    double zd = zTarget - z;

    double dd = xd * xd + yd * yd + zd * zd;

    if (dd < 1 * 1 || dd > 60 * 60) {
        xTarget = x + (random->nextFloat() * 2 - 1) * 16;
        yTarget = y + (random->nextFloat() * 2 - 1) * 16;
        zTarget = z + (random->nextFloat() * 2 - 1) * 16;
    }

    if (floatDuration-- <= 0) {
        floatDuration += random->nextInt(5) + 2;

        dd = sqrt(dd);

        if (canReach(xTarget, yTarget, zTarget, dd)) {
            this->xd += xd / dd * 0.1;
            this->yd += yd / dd * 0.1;
            this->zd += zd / dd * 0.1;
        } else {
            xTarget = x;
            yTarget = y;
            zTarget = z;
        }
    }

    if (target != nullptr && target->removed) target = nullptr;
    if (target == nullptr || retargetTime-- <= 0) {
        target = level->getNearestAttackablePlayer(shared_from_this(), 100);
        if (target != nullptr) {
            retargetTime = 20;
        }
    }

    double maxDist = 64.0f;
    if (target != nullptr &&
        target->distanceToSqr(shared_from_this()) < maxDist * maxDist) {
        double xdd = target->x - x;
        double ydd =
            (target->bb.y0 + target->bbHeight / 2) - (y + bbHeight / 2);
        double zdd = target->z - z;
        yBodyRot = yRot = -(float)atan2(xdd, zdd) * 180 / std::numbers::pi;

        if (canSee(target)) {
            if (charge == 10) {
                // 4J - change brought forward from 1.2.3
                level->levelEvent(nullptr, LevelEvent::SOUND_GHAST_WARNING,
                                  (int)x, (int)y, (int)z, 0);
            }
            charge++;
            if (charge == 20) {
                // 4J - change brought forward from 1.2.3
                level->levelEvent(nullptr, LevelEvent::SOUND_GHAST_FIREBALL,
                                  (int)x, (int)y, (int)z, 0);
                std::shared_ptr<LargeFireball> ie =
                    std::make_shared<LargeFireball>(
                        level,
                        std::dynamic_pointer_cast<Mob>(shared_from_this()), xdd,
                        ydd, zdd);
                ie->explosionPower = explosionPower;
                double d = 4;
                Vec3 v = getViewVector(1);
                ie->x = x + v.x * d;
                ie->y = y + bbHeight / 2 + 0.5f;
                ie->z = z + v.z * d;
                level->addEntity(ie);
                charge = -40;
            }
        } else {
            if (charge > 0) charge--;
        }
    } else {
        yBodyRot = yRot =
            -(float)atan2(this->xd, this->zd) * 180 / std::numbers::pi;
        if (charge > 0) charge--;
    }

    if (!level->isClientSide) {
        uint8_t old = entityData->getByte(DATA_IS_CHARGING);
        uint8_t current = (uint8_t)(charge > 10 ? 1 : 0);
        if (old != current) {
            entityData->set(DATA_IS_CHARGING, current);
        }
    }
}

bool Ghast::canReach(double xt, double yt, double zt, double dist) {
    double xd = (xTarget - x) / dist;
    double yd = (yTarget - y) / dist;
    double zd = (zTarget - z) / dist;
    AABB probe = bb;

    for (int d = 1; d < dist; d++) {
        probe = probe.move(xd, yd, zd);
        if (!level->getCubes(shared_from_this(), &probe)->empty()) return false;
    }

    return true;
}

int Ghast::getAmbientSound() { return eSoundType_MOB_GHAST_MOAN; }

int Ghast::getHurtSound() { return eSoundType_MOB_GHAST_SCREAM; }

int Ghast::getDeathSound() { return eSoundType_MOB_GHAST_DEATH; }

int Ghast::getDeathLoot() { return Item::gunpowder_Id; }

void Ghast::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) {
    int count = random->nextInt(2) + random->nextInt(1 + playerBonusLevel);
    for (int i = 0; i < count; i++) {
        spawnAtLocation(Item::ghastTear_Id, 1);
    }
    count = random->nextInt(3) + random->nextInt(1 + playerBonusLevel);
    for (int i = 0; i < count; i++) {
        spawnAtLocation(Item::gunpowder_Id, 1);
    }
}

float Ghast::getSoundVolume() {
    return 0.4f;  // 10; 4J-PB - changing due to customer demands
}

bool Ghast::canSpawn() {
    return (random->nextInt(20) == 0 && FlyingMob::canSpawn() &&
            level->difficulty > Difficulty::PEACEFUL);
}

int Ghast::getMaxSpawnClusterSize() { return 1; }
void Ghast::addAdditonalSaveData(CompoundTag* tag) {
    FlyingMob::addAdditonalSaveData(tag);
    tag->putInt("ExplosionPower", explosionPower);
}

void Ghast::readAdditionalSaveData(CompoundTag* tag) {
    FlyingMob::readAdditionalSaveData(tag);
    if (tag->contains("ExplosionPower"))
        explosionPower = tag->getInt("ExplosionPower");
}
