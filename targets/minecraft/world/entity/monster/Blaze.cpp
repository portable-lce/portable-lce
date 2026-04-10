#include "Blaze.h"

#include <stdint.h>

#include <cmath>
#include <memory>
#include <numbers>

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/monster/Monster.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/projectile/SmallFireball.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/LevelEvent.h"
#include "minecraft/world/phys/AABB.h"

Blaze::Blaze(Level* level) : Monster(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());

    fireImmune = true;
    xpReward = XP_REWARD_LARGE;

    // 4J Default inits
    allowedHeightOffset = 0.5f;
    nextHeightOffsetChangeTick = 0;
    attackCounter = 0;
}

void Blaze::registerAttributes() {
    Monster::registerAttributes();
    getAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)->setBaseValue(6);
}

void Blaze::defineSynchedData() {
    Monster::defineSynchedData();

    entityData->define(DATA_FLAGS_ID, (uint8_t)0);
}

int Blaze::getAmbientSound() { return eSoundType_MOB_BLAZE_BREATHE; }

int Blaze::getHurtSound() { return eSoundType_MOB_BLAZE_HURT; }

int Blaze::getDeathSound() { return eSoundType_MOB_BLAZE_DEATH; }

int Blaze::getLightColor(float a) {
    return SharedConstants::FULLBRIGHT_LIGHTVALUE;
}

float Blaze::getBrightness(float a) { return 1.0f; }

void Blaze::aiStep() {
    if (!level->isClientSide) {
        if (isInWaterOrRain()) {
            hurt(DamageSource::drown, 1);
        }

        nextHeightOffsetChangeTick--;
        if (nextHeightOffsetChangeTick <= 0) {
            nextHeightOffsetChangeTick = SharedConstants::TICKS_PER_SECOND * 5;
            allowedHeightOffset = .5f + (float)random->nextGaussian() * 3;
        }

        if (getAttackTarget() != nullptr &&
            (getAttackTarget()->y + getAttackTarget()->getHeadHeight()) >
                (y + getHeadHeight() + allowedHeightOffset)) {
            yd = yd + (.3f - yd) * .3f;
        }
    }

    if (random->nextInt(24) == 0) {
        level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, eSoundType_FIRE_FIRE,
                         1 + random->nextFloat(),
                         random->nextFloat() * 0.7f + 0.3f);
    }

    // slow falling, like chicken
    if (!onGround && yd < 0) {
        yd *= 0.6;
    }

    for (int i = 0; i < 2; i++) {
        level->addParticle(eParticleType_largesmoke,
                           x + (random->nextDouble() - 0.5) * bbWidth,
                           y + random->nextDouble() * bbHeight,
                           z + (random->nextDouble() - 0.5) * bbWidth, 0, 0, 0);
    }

    Monster::aiStep();
}

void Blaze::checkHurtTarget(std::shared_ptr<Entity> target, float d) {
    if (attackTime <= 0 && d < 2.0f && target->bb.y1 > bb.y0 &&
        target->bb.y0 < bb.y1) {
        attackTime = 20;
        doHurtTarget(target);
    } else if (d < 30) {
        double xd = target->x - x;
        double yd = (target->bb.y0 + target->bbHeight / 2) - (y + bbHeight / 2);
        double zd = target->z - z;

        if (attackTime == 0) {
            attackCounter++;
            if (attackCounter == 1) {
                attackTime = SharedConstants::TICKS_PER_SECOND * 3;
                setCharged(true);
            } else if (attackCounter <= 4) {
                attackTime = SharedConstants::TICKS_PER_SECOND / 3;
            } else {
                attackTime = SharedConstants::TICKS_PER_SECOND * 5;
                attackCounter = 0;
                setCharged(false);
            }

            if (attackCounter > 1) {
                float sqd = sqrt(d) * .5f;

                level->levelEvent(nullptr, LevelEvent::SOUND_BLAZE_FIREBALL,
                                  (int)x, (int)y, (int)z, 0);
                //                    level.playSound(this,
                //                    "mob.ghast.fireball", getSoundVolume(),
                //                    (random.nextFloat() - random.nextFloat())
                //                    * 0.2f + 1.0f);
                for (int i = 0; i < 1; i++) {
                    std::shared_ptr<SmallFireball> ie =
                        std::make_shared<SmallFireball>(
                            level,
                            std::dynamic_pointer_cast<Mob>(shared_from_this()),
                            xd + random->nextGaussian() * sqd, yd,
                            zd + random->nextGaussian() * sqd);
                    //                        Vec3 v = getViewVector(1);
                    //                        ie.x = x + v.x * 1.5;
                    ie->y = y + bbHeight / 2 + 0.5f;
                    //                        ie.z = z + v.z * 1.5;
                    level->addEntity(ie);
                }
            }
        }
        yRot = (float)(atan2(zd, xd) * 180 / std::numbers::pi) - 90;

        holdGround = true;
    }
}

void Blaze::causeFallDamage(float distance) {}

int Blaze::getDeathLoot() { return Item::blazeRod_Id; }

bool Blaze::isOnFire() { return isCharged(); }

void Blaze::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) {
    if (wasKilledByPlayer) {
        int count = random->nextInt(2 + playerBonusLevel);
        for (int i = 0; i < count; i++) {
            spawnAtLocation(Item::blazeRod_Id, 1);
        }
        // 4J-PB - added to the XBLA version due to our limited amount of
        // glowstone in the Nether - drop 0-2 glowstone dust
        count = random->nextInt(3 + playerBonusLevel);
        for (int i = 0; i < count; i++) {
            spawnAtLocation(Item::yellowDust_Id, 1);
        }
    }
}

bool Blaze::isCharged() {
    return (entityData->getByte(DATA_FLAGS_ID) & 0x1) != 0;
}

void Blaze::setCharged(bool value) {
    uint8_t flags = entityData->getByte(DATA_FLAGS_ID);
    if (value) {
        flags |= 0x1;
    } else {
        flags &= ~0x1;
    }
    entityData->set(DATA_FLAGS_ID, flags);
}

bool Blaze::isDarkEnoughToSpawn() { return true; }
