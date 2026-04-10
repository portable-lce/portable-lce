#include "WitherSkull.h"

#include <stdint.h>

#include <algorithm>

#include "minecraft/SharedConstants.h"
#include "minecraft/world/Difficulty.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/projectile/Fireball.h"
#include "minecraft/world/level/GameRules.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/HitResult.h"

WitherSkull::WitherSkull(Level* level) : Fireball(level) {
    defineSynchedData();

    setSize(5 / 16.0f, 5 / 16.0f);
}

WitherSkull::WitherSkull(Level* level, std::shared_ptr<LivingEntity> mob,
                         double xa, double ya, double za)
    : Fireball(level, mob, xa, ya, za) {
    defineSynchedData();

    setSize(5 / 16.0f, 5 / 16.0f);
}

float WitherSkull::getInertia() {
    return isDangerous() ? 0.73f : Fireball::getInertia();
}

WitherSkull::WitherSkull(Level* level, double x, double y, double z, double xa,
                         double ya, double za)
    : Fireball(level, x, y, z, xa, ya, za) {
    defineSynchedData();

    setSize(5 / 16.0f, 5 / 16.0f);
}

bool WitherSkull::isOnFire() { return false; }

float WitherSkull::getTileExplosionResistance(Explosion* explosion,
                                              Level* level, int x, int y, int z,
                                              Tile* tile) {
    float result =
        Fireball::getTileExplosionResistance(explosion, level, x, y, z, tile);

    if (isDangerous() && tile != Tile::unbreakable &&
        tile != Tile::endPortalTile && tile != Tile::endPortalFrameTile) {
        result = std::min(0.8f, result);
    }

    return result;
}

void WitherSkull::onHit(HitResult* res) {
    if (!level->isClientSide) {
        if (res->entity != nullptr) {
            if (owner != nullptr) {
                DamageSource* damageSource = DamageSource::mobAttack(owner);
                if (res->entity->hurt(damageSource, 8)) {
                    if (!res->entity->isAlive()) {
                        owner->heal(5);
                    }
                }
                delete damageSource;
            } else {
                res->entity->hurt(DamageSource::magic, 5);
            }
            if (res->entity->instanceof(eTYPE_LIVINGENTITY)) {
                int witherSeconds = 0;
                if (level->difficulty <= Difficulty::EASY) {
                    // Nothing
                } else if (level->difficulty == Difficulty::NORMAL) {
                    witherSeconds = 10;
                } else if (level->difficulty == Difficulty::HARD) {
                    witherSeconds = 40;
                }
                if (witherSeconds > 0) {
                    std::dynamic_pointer_cast<LivingEntity>(res->entity)
                        ->addEffect(new MobEffectInstance(
                            MobEffect::wither->id,
                            SharedConstants::TICKS_PER_SECOND * witherSeconds,
                            1));
                }
            }
        }
        level->explode(
            shared_from_this(), x, y, z, 1, false,
            level->getGameRules()->getBoolean(GameRules::RULE_MOBGRIEFING));
        remove();
    }
}

bool WitherSkull::isPickable() { return false; }

bool WitherSkull::hurt(DamageSource* source, float damage) { return false; }

void WitherSkull::defineSynchedData() {
    entityData->define(DATA_DANGEROUS, (uint8_t)0);
}

bool WitherSkull::isDangerous() {
    return entityData->getByte(DATA_DANGEROUS) == 1;
}

void WitherSkull::setDangerous(bool value) {
    entityData->set(DATA_DANGEROUS, value ? (uint8_t)1 : (uint8_t)0);
}

bool WitherSkull::shouldBurn() { return false; }
