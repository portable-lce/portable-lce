#include "Witch.h"

#include <memory>
#include <vector>

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/EntityEvent.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/attributes/AttributeModifier.h"
#include "minecraft/world/entity/ai/goal/FloatGoal.h"
#include "minecraft/world/entity/ai/goal/GoalSelector.h"
#include "minecraft/world/entity/ai/goal/LookAtPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/RandomLookAroundGoal.h"
#include "minecraft/world/entity/ai/goal/RandomStrollGoal.h"
#include "minecraft/world/entity/ai/goal/RangedAttackGoal.h"
#include "minecraft/world/entity/ai/goal/target/HurtByTargetGoal.h"
#include "minecraft/world/entity/ai/goal/target/NearestAttackableTargetGoal.h"
#include "minecraft/world/entity/monster/Monster.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/ThrownPotion.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/item/alchemy/PotionBrewing.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/phys/AABB.h"

AttributeModifier* Witch::SPEED_MODIFIER_DRINKING =
    (new AttributeModifier(eModifierId_MOB_WITCH_DRINKSPEED, -0.25f,
                           AttributeModifier::OPERATION_ADDITION))
        ->setSerialize(false);

const int Witch::DEATH_LOOT[Witch::DEATH_LOOT_COUNT] = {
    Item::yellowDust_Id, Item::sugar_Id,       Item::redStone_Id,
    Item::spiderEye_Id,  Item::glassBottle_Id, Item::gunpowder_Id,
    Item::stick_Id,      Item::stick_Id,
};

Witch::Witch(Level* level) : Monster(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());

    usingTime = 0;

    goalSelector.addGoal(1, new FloatGoal(this));
    goalSelector.addGoal(
        2, new RangedAttackGoal(this, this, 1.0,
                                SharedConstants::TICKS_PER_SECOND * 3, 10));
    goalSelector.addGoal(2, new RandomStrollGoal(this, 1.0));
    goalSelector.addGoal(3, new LookAtPlayerGoal(this, typeid(Player), 8));
    goalSelector.addGoal(3, new RandomLookAroundGoal(this));

    targetSelector.addGoal(1, new HurtByTargetGoal(this, false));
    targetSelector.addGoal(
        2, new NearestAttackableTargetGoal(this, typeid(Player), 0, true));
}

void Witch::defineSynchedData() {
    Monster::defineSynchedData();

    getEntityData()->define(DATA_USING_ITEM, (uint8_t)0);
}

int Witch::getAmbientSound() {
    return eSoundType_MOB_WITCH_IDLE;  //"mob.witch.idle";
}

int Witch::getHurtSound() {
    return eSoundType_MOB_WITCH_HURT;  //"mob.witch.hurt";
}

int Witch::getDeathSound() {
    return eSoundType_MOB_WITCH_DEATH;  //"mob.witch.death";
}

void Witch::setUsingItem(bool isUsing) {
    getEntityData()->set(DATA_USING_ITEM, isUsing ? (uint8_t)1 : (uint8_t)0);
}

bool Witch::isUsingItem() {
    return getEntityData()->getByte(DATA_USING_ITEM) == 1;
}

void Witch::registerAttributes() {
    Monster::registerAttributes();

    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(26);
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.25f);
}

bool Witch::useNewAi() { return true; }

void Witch::aiStep() {
    if (!level->isClientSide) {
        if (isUsingItem()) {
            if (usingTime-- <= 0) {
                setUsingItem(false);
                std::shared_ptr<ItemInstance> item = getCarriedItem();
                setEquippedSlot(SLOT_WEAPON, nullptr);

                if (item != nullptr && item->id == Item::potion_Id) {
                    std::vector<MobEffectInstance*>* effects =
                        Item::potion->getMobEffects(item);
                    if (effects != nullptr) {
                        for (auto it = effects->begin(); it != effects->end();
                             ++it) {
                            addEffect(new MobEffectInstance(*it));
                        }
                    }
                    delete effects;
                }

                getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)
                    ->removeModifier(SPEED_MODIFIER_DRINKING);
            }
        } else {
            int potion = -1;

            if (random->nextFloat() < 0.15f && isOnFire() &&
                !hasEffect(MobEffect::fireResistance)) {
                potion = PotionBrewing::POTION_ID_FIRE_RESISTANCE;
            } else if (random->nextFloat() < 0.05f &&
                       getHealth() < getMaxHealth()) {
                potion = PotionBrewing::POTION_ID_HEAL;
            } else if (random->nextFloat() < 0.25f && getTarget() != nullptr &&
                       !hasEffect(MobEffect::movementSpeed) &&
                       getTarget()->distanceToSqr(shared_from_this()) >
                           11 * 11) {
                potion = PotionBrewing::POTION_ID_SWIFTNESS;
            } else if (random->nextFloat() < 0.25f && getTarget() != nullptr &&
                       !hasEffect(MobEffect::movementSpeed) &&
                       getTarget()->distanceToSqr(shared_from_this()) >
                           11 * 11) {
                potion = PotionBrewing::POTION_ID_SWIFTNESS;
            }

            if (potion > -1) {
                setEquippedSlot(SLOT_WEAPON,
                                std::shared_ptr<ItemInstance>(
                                    new ItemInstance(Item::potion, 1, potion)));
                usingTime = getCarriedItem()->getUseDuration();
                setUsingItem(true);
                AttributeInstance* speed =
                    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
                speed->removeModifier(SPEED_MODIFIER_DRINKING);
                speed->addModifier(
                    new AttributeModifier(*SPEED_MODIFIER_DRINKING));
            }
        }

        if (random->nextFloat() < 0.00075f) {
            level->broadcastEntityEvent(shared_from_this(),
                                        EntityEvent::WITCH_HAT_MAGIC);
        }
    }

    Monster::aiStep();
}

void Witch::handleEntityEvent(uint8_t id) {
    if (id == EntityEvent::WITCH_HAT_MAGIC) {
        for (int i = 0; i < random->nextInt(35) + 10; i++) {
            level->addParticle(eParticleType_witchMagic,
                               x + random->nextGaussian() * .13f,
                               bb.y1 + 0.5f + random->nextGaussian() * .13f,
                               z + random->nextGaussian() * .13f, 0, 0, 0);
        }
    } else {
        Monster::handleEntityEvent(id);
    }
}

float Witch::getDamageAfterMagicAbsorb(DamageSource* damageSource,
                                       float damage) {
    damage = Monster::getDamageAfterMagicAbsorb(damageSource, damage);

    if (damageSource->getEntity() == shared_from_this()) damage = 0;
    if (damageSource->isMagic()) damage *= 0.15;

    return damage;
}

void Witch::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) {
    int passes = random->nextInt(3) + 1;
    for (int pass = 0; pass < passes; pass++) {
        int count = random->nextInt(3);
        int type = DEATH_LOOT[random->nextInt(DEATH_LOOT_COUNT)];
        if (playerBonusLevel > 0)
            count += random->nextInt(playerBonusLevel + 1);

        for (int i = 0; i < count; i++) {
            spawnAtLocation(type, 1);
        }
    }
}

void Witch::performRangedAttack(std::shared_ptr<LivingEntity> target,
                                float power) {
    if (isUsingItem()) return;

    std::shared_ptr<ThrownPotion> potion = std::make_shared<ThrownPotion>(
        level, std::dynamic_pointer_cast<LivingEntity>(shared_from_this()),
        PotionBrewing::POTION_ID_SPLASH_DAMAGE);
    potion->xRot -= -20;
    double xd = (target->x + target->xd) - x;
    double yd = (target->y + target->getHeadHeight() - 1.1f) - y;
    double zd = (target->z + target->zd) - z;
    float dist = Mth::sqrt(xd * xd + zd * zd);

    if (dist >= 8 && !target->hasEffect(MobEffect::movementSlowdown)) {
        potion->setPotionValue(PotionBrewing::POTION_ID_SPLASH_SLOWNESS);
    } else if (target->getHealth() >= 8 &&
               !target->hasEffect(MobEffect::poison)) {
        potion->setPotionValue(PotionBrewing::POTION_ID_SPLASH_POISON);
    } else if (dist <= 3 && !target->hasEffect(MobEffect::weakness) &&
               random->nextFloat() < 0.25f) {
        potion->setPotionValue(PotionBrewing::POTION_ID_SPLASH_WEAKNESS);
    }

    potion->shoot(xd, yd + dist * 0.2f, zd, 0.75f, 8);

    level->addEntity(potion);
}
