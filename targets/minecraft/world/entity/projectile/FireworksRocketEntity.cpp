#include "FireworksRocketEntity.h"

#include <math.h>

#include <numbers>
#include <string>

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityEvent.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/item/FireworksItem.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "nbt/CompoundTag.h"

FireworksRocketEntity::FireworksRocketEntity(Level* level) : Entity(level) {
    defineSynchedData();

    life = 0;
    lifetime = 0;
    setSize(0.25f, 0.25f);
}

void FireworksRocketEntity::defineSynchedData() {
    entityData->defineNULL(DATA_ID_FIREWORKS_ITEM, nullptr);
}

bool FireworksRocketEntity::shouldRenderAtSqrDistance(double distance) {
    return distance < 64 * 64;
}

FireworksRocketEntity::FireworksRocketEntity(
    Level* level, double x, double y, double z,
    std::shared_ptr<ItemInstance> sourceItem)
    : Entity(level) {
    defineSynchedData();

    life = 0;

    setSize(0.25f, 0.25f);

    setPos(x, y, z);
    heightOffset = 0;

    int flightCount = 1;
    if (sourceItem != nullptr && sourceItem->hasTag()) {
        entityData->set(DATA_ID_FIREWORKS_ITEM, sourceItem);

        CompoundTag* tag = sourceItem->getTag();
        CompoundTag* compound = tag->getCompound(FireworksItem::TAG_FIREWORKS);
        if (compound != nullptr) {
            flightCount += compound->getByte(FireworksItem::TAG_FLIGHT);
        }
    }
    xd = random->nextGaussian() * .001;
    zd = random->nextGaussian() * .001;
    yd = 0.05;

    lifetime = (SharedConstants::TICKS_PER_SECOND / 2) * flightCount +
               random->nextInt(6) + random->nextInt(7);
}

void FireworksRocketEntity::lerpMotion(double xd, double yd, double zd) {
    xd = xd;
    yd = yd;
    zd = zd;
    if (xRotO == 0 && yRotO == 0) {
        double sd = Mth::sqrt(xd * xd + zd * zd);
        yRotO = yRot = (float)(atan2(xd, zd) * 180 / std::numbers::pi);
        xRotO = xRot = (float)(atan2(yd, sd) * 180 / std::numbers::pi);
    }
}

void FireworksRocketEntity::tick() {
    xOld = x;
    yOld = y;
    zOld = z;
    Entity::tick();

    xd *= 1.15;
    zd *= 1.15;
    yd += .04;
    move(xd, yd, zd);

    double sd = Mth::sqrt(xd * xd + zd * zd);
    yRot = (float)(atan2(xd, zd) * 180 / std::numbers::pi);
    xRot = (float)(atan2(yd, sd) * 180 / std::numbers::pi);

    while (xRot - xRotO < -180) xRotO -= 360;
    while (xRot - xRotO >= 180) xRotO += 360;

    while (yRot - yRotO < -180) yRotO -= 360;
    while (yRot - yRotO >= 180) yRotO += 360;

    xRot = xRotO + (xRot - xRotO) * 0.2f;
    yRot = yRotO + (yRot - yRotO) * 0.2f;

    if (!level->isClientSide) {
        if (life == 0) {
            level->playEntitySound(shared_from_this(),
                                   eSoundType_FIREWORKS_LAUNCH, 3, 1);
        }
    }

    life++;
    if (level->isClientSide && (life % 2) < 2) {
        level->addParticle(eParticleType_fireworksspark, x, y - .3, z,
                           random->nextGaussian() * .05, -yd * .5,
                           random->nextGaussian() * .05);
    }
    if (!level->isClientSide && life > lifetime) {
        level->broadcastEntityEvent(shared_from_this(),
                                    EntityEvent::FIREWORKS_EXPLODE);
        remove();
    }
}

void FireworksRocketEntity::handleEntityEvent(uint8_t eventId) {
    if (eventId == EntityEvent::FIREWORKS_EXPLODE && level->isClientSide) {
        std::shared_ptr<ItemInstance> sourceItem =
            entityData->getItemInstance(DATA_ID_FIREWORKS_ITEM);
        CompoundTag* tag = nullptr;
        if (sourceItem != nullptr && sourceItem->hasTag()) {
            tag =
                sourceItem->getTag()->getCompound(FireworksItem::TAG_FIREWORKS);
        }
        level->createFireworks(x, y, z, xd, yd, zd, tag);
    }
    Entity::handleEntityEvent(eventId);
}

void FireworksRocketEntity::addAdditonalSaveData(CompoundTag* tag) {
    tag->putInt("Life", life);
    tag->putInt("LifeTime", lifetime);
    std::shared_ptr<ItemInstance> itemInstance =
        entityData->getItemInstance(DATA_ID_FIREWORKS_ITEM);
    if (itemInstance != nullptr) {
        CompoundTag* itemTag = new CompoundTag();
        itemInstance->save(itemTag);
        tag->putCompound("FireworksItem", itemTag);
    }
}

void FireworksRocketEntity::readAdditionalSaveData(CompoundTag* tag) {
    life = tag->getInt("Life");
    lifetime = tag->getInt("LifeTime");

    CompoundTag* itemTag = tag->getCompound("FireworksItem");
    if (itemTag != nullptr) {
        std::shared_ptr<ItemInstance> fromTag = ItemInstance::fromTag(itemTag);
        if (fromTag != nullptr) {
            entityData->set(DATA_ID_FIREWORKS_ITEM, fromTag);
        }
    }
}

float FireworksRocketEntity::getShadowHeightOffs() { return 0; }

float FireworksRocketEntity::getBrightness(float a) {
    return Entity::getBrightness(a);
}

int FireworksRocketEntity::getLightColor(float a) {
    return Entity::getLightColor(a);
}

bool FireworksRocketEntity::isAttackable() { return false; }