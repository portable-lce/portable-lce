#include "SnowMan.h"

#include <memory>

#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/goal/GoalSelector.h"
#include "minecraft/world/entity/ai/goal/LookAtPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/RandomLookAroundGoal.h"
#include "minecraft/world/entity/ai/goal/RandomStrollGoal.h"
#include "minecraft/world/entity/ai/goal/RangedAttackGoal.h"
#include "minecraft/world/entity/ai/goal/target/NearestAttackableTargetGoal.h"
#include "minecraft/world/entity/ai/navigation/PathNavigation.h"
#include "minecraft/world/entity/animal/Golem.h"
#include "minecraft/world/entity/monster/Enemy.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/Snowball.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/tile/Tile.h"

SnowMan::SnowMan(Level* level) : Golem(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());

    this->setSize(0.4f, 1.8f);

    getNavigation()->setAvoidWater(true);
    goalSelector.addGoal(
        1, new RangedAttackGoal(this, this, 1.25,
                                SharedConstants::TICKS_PER_SECOND * 1, 10));
    goalSelector.addGoal(2, new RandomStrollGoal(this, 1.0));
    goalSelector.addGoal(3, new LookAtPlayerGoal(this, typeid(Player), 6));
    goalSelector.addGoal(4, new RandomLookAroundGoal(this));

    targetSelector.addGoal(
        1, new NearestAttackableTargetGoal(this, typeid(Mob), 0, true, false,
                                           Enemy::ENEMY_SELECTOR));
}

bool SnowMan::useNewAi() { return true; }

void SnowMan::registerAttributes() {
    Golem::registerAttributes();

    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(4);
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.2f);
}

void SnowMan::aiStep() {
    Golem::aiStep();

    if (isInWaterOrRain()) hurt(DamageSource::drown, 1);

    {
        int xx = Mth::floor(x);
        int zz = Mth::floor(z);
        if (level->getBiome(xx, zz)->getTemperature() > 1) {
            hurt(DamageSource::onFire, 1);
        }
    }

    for (int i = 0; i < 4; i++) {
        int xx = Mth::floor(x + (i % 2 * 2 - 1) * 0.25f);
        int yy = Mth::floor(y);
        int zz = Mth::floor(z + ((i / 2) % 2 * 2 - 1) * 0.25f);
        if (level->getTile(xx, yy, zz) == 0) {
            if (level->getBiome(xx, zz)->getTemperature() < 0.8f) {
                if (Tile::topSnow->mayPlace(level, xx, yy, zz)) {
                    level->setTileAndUpdate(xx, yy, zz, Tile::topSnow_Id);
                }
            }
        }
    }
}

int SnowMan::getDeathLoot() { return Item::snowBall_Id; }

void SnowMan::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) {
    // drop some feathers
    int count = random->nextInt(16);
    for (int i = 0; i < count; i++) {
        spawnAtLocation(Item::snowBall_Id, 1);
    }
}

void SnowMan::performRangedAttack(std::shared_ptr<LivingEntity> target,
                                  float power) {
    std::shared_ptr<Snowball> snowball = std::make_shared<Snowball>(
        level, std::dynamic_pointer_cast<LivingEntity>(shared_from_this()));
    double xd = target->x - x;
    double yd = (target->y + target->getHeadHeight() - 1.1f) - snowball->y;
    double zd = target->z - z;
    float yo = Mth::sqrt(xd * xd + zd * zd) * 0.2f;
    snowball->shoot(xd, yd + yo, zd, 1.60f, 12);

    playSound(eSoundType_RANDOM_BOW, 1.0f,
              1 / (getRandom()->nextFloat() * 0.4f + 0.8f));
    level->addEntity(snowball);
}