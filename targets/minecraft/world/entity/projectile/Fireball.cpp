#include "Fireball.h"

#include <stdint.h>
#include <stdlib.h>

#include <cmath>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/HitResult.h"
#include "minecraft/world/phys/Vec3.h"
#include "nbt/CompoundTag.h"
#include "nbt/DoubleTag.h"
#include "nbt/ListTag.h"

// 4J - added common ctor code.
void Fireball::_init() {
    xTile = -1;
    yTile = -1;
    zTile = -1;
    lastTile = 0;
    inGround = false;
    flightTime = 0;

    life = 0;
    owner = nullptr;
    xPower = 0.0;
    yPower = 0.0;
    zPower = 0.0;
}

Fireball::Fireball(Level* level) : Entity(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();

    _init();

    setSize(16 / 16.0f, 16 / 16.0f);
}

void Fireball::defineSynchedData() {}

bool Fireball::shouldRenderAtSqrDistance(double distance) {
    double size = bb.getSize() * 4;
    size *= 64.0f;
    return distance < size * size;
}

Fireball::Fireball(Level* level, double x, double y, double z, double xa,
                   double ya, double za)
    : Entity(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();

    _init();

    setSize(16 / 16.0f, 16 / 16.0f);

    moveTo(x, y, z, yRot, xRot);
    setPos(x, y, z);

    double dd = sqrt(xa * xa + ya * ya + za * za);

    // Fix for #69150 - [CRASH] TU8: Code: Gameplay: Nether portal mechanics can
    // become permanently broken, causing a hard lock upon usage. IF xa, ya and
    // za are 0 then dd is 0 and the xa/dd etc return NAN
    if (dd == 0.0) {
        xPower = 0.0;
        yPower = 0.0;
        zPower = 0.0;
    } else {
        xPower = xa / dd * 0.10;
        yPower = ya / dd * 0.10;
        zPower = za / dd * 0.10;
    }
}

Fireball::Fireball(Level* level, std::shared_ptr<LivingEntity> mob, double xa,
                   double ya, double za)
    : Entity(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();

    _init();

    owner = mob;

    setSize(16 / 16.0f, 16 / 16.0f);

    moveTo(mob->x, mob->y, mob->z, mob->yRot, mob->xRot);
    setPos(x, y, z);
    heightOffset = 0;

    xd = yd = zd = 0.0;

    xa += random->nextGaussian() * 0.4;
    ya += random->nextGaussian() * 0.4;
    za += random->nextGaussian() * 0.4;
    double dd = sqrt(xa * xa + ya * ya + za * za);

    // Fix for #69150 - [CRASH] TU8: Code: Gameplay: Nether portal mechanics can
    // become permanently broken, causing a hard lock upon usage. IF xa, ya and
    // za are 0 then dd is 0 and the xa/dd etc return NAN
    if (dd == 0.0) {
        xPower = 0.0;
        yPower = 0.0;
        zPower = 0.0;
    } else {
        xPower = xa / dd * 0.10;
        yPower = ya / dd * 0.10;
        zPower = za / dd * 0.10;
    }
}

void Fireball::tick() {
    // 4J-PB - Moved forward from 1.2.3
    // if (!level->isClientSide && (owner == nullptr || owner->removed))
    if (!level->isClientSide) {
        if ((owner != nullptr && owner->removed) ||
            !level->hasChunkAt((int)x, (int)y, (int)z)) {
            Log::info(
                "Fireball removed - owner is null or removed is true for "
                "owner\n");
            remove();
            return;
        } else {
            // 4J-PB - TU9 bug fix - fireballs can hit the edge of the world,
            // and stay there
            int minXZ = -(level->dimension->getXZSize() * 16) / 2;
            int maxXZ = (level->dimension->getXZSize() * 16) / 2 - 1;

            if ((x <= minXZ) || (x >= maxXZ) || (z <= minXZ) || (z >= maxXZ)) {
                remove();
                Log::info("Fireball removed - end of world\n");
                return;
            }
        }
    }

    Entity::tick();

    // Log::info("Fireball x %d, y %d, z%d\n",(int)x,(int)y,(int)z);

    if (shouldBurn()) setOnFire(1);

    if (inGround) {
        int tile = level->getTile(xTile, yTile, zTile);
        if (tile == lastTile) {
            life++;
            if (life == SharedConstants::TICKS_PER_SECOND * 30) {
                remove();
                Log::info("Fireball removed - life is 20*60\n");
            }
            return;
        } else {
            inGround = false;

            xd *= random->nextFloat() * 0.2f;
            yd *= random->nextFloat() * 0.2f;
            zd *= random->nextFloat() * 0.2f;
            life = 0;
            flightTime = 0;
        }
    } else {
        flightTime++;
    }

    Vec3 from(x, y, z);
    Vec3 to(x + xd, y + yd, z + zd);
    HitResult* res = level->clip(&from, &to);

    from = Vec3(x, y, z);
    to = Vec3(x + xd, y + yd, z + zd);
    if (res != nullptr) {
        to = Vec3{res->pos.x, res->pos.y, res->pos.z};
    }
    std::shared_ptr<Entity> hitEntity = nullptr;
    AABB grown = bb.expand(xd, yd, zd).grow(1, 1, 1);
    std::vector<std::shared_ptr<Entity> >* objects =
        level->getEntities(shared_from_this(), &grown);
    double nearest = 0;
    auto itEnd = objects->end();
    for (auto it = objects->begin(); it != itEnd; it++) {
        std::shared_ptr<Entity> e = *it;  // objects->at(i);
        if (!e->isPickable() || (e->is(owner)))
            continue;  // 4J Stu - Never collide with the owner (Enderdragon) //
                       // && flightTime < 25)) continue;

        float rr = 0.3f;
        AABB bb = e->bb.grow(rr, rr, rr);
        HitResult* p = bb.clip(from, to);
        if (p != nullptr) {
            double dd = from.distanceTo(p->pos);
            if (dd < nearest || nearest == 0) {
                hitEntity = e;
                nearest = dd;
            }
            delete p;
        }
    }

    if (hitEntity != nullptr) {
        delete res;
        res = new HitResult(hitEntity);
    }

    if (res != nullptr) {
        onHit(res);
    }
    delete res;
    x += xd;
    y += yd;
    z += zd;

    double sd = sqrt(xd * xd + zd * zd);
    yRot = (float)(atan2(zd, xd) * 180 / std::numbers::pi) + 90;
    xRot = (float)(atan2(sd, yd) * 180 / std::numbers::pi) - 90;

    while (xRot - xRotO < -180) xRotO -= 360;
    while (xRot - xRotO >= 180) xRotO += 360;

    while (yRot - yRotO < -180) yRotO -= 360;
    while (yRot - yRotO >= 180) yRotO += 360;

    xRot = xRotO + (xRot - xRotO) * 0.2f;
    yRot = yRotO + (yRot - yRotO) * 0.2f;

    float inertia = getInertia();
    if (isInWater()) {
        for (int i = 0; i < 4; i++) {
            float s = 1 / 4.0f;
            level->addParticle(eParticleType_bubble, x - xd * s, y - yd * s,
                               z - zd * s, xd, yd, zd);
        }
        inertia = 0.80f;
    }

    xd += xPower;
    yd += yPower;
    zd += zPower;
    xd *= inertia;
    yd *= inertia;
    zd *= inertia;

    // 4J-PB - bug fix for the fireballs in a saved game - they are saved with
    // no/very small velocity, so end up hanging around in the air
    if (!level->isClientSide) {
        if ((abs(xd) < 0.002) && (abs(yd) < 0.002) && (abs(zd) < 0.002)) {
            xd = 0.0;
            zd = 0.0;
            yd = 0.0;
            Log::info("Removing a fireball with zero velocity\n");
            remove();
        }
    }

    level->addParticle(getTrailParticleType(), x, y + 0.5f, z, 0, 0.01, 0);

    setPos(x, y, z);
}

float Fireball::getInertia() { return 0.95f; }

void Fireball::addAdditonalSaveData(CompoundTag* tag) {
    tag->putShort("xTile", (short)xTile);
    tag->putShort("yTile", (short)yTile);
    tag->putShort("zTile", (short)zTile);
    tag->putByte("inTile", (uint8_t)lastTile);
    tag->putByte("inGround", (uint8_t)(inGround ? 1 : 0));
    tag->put("direction", newDoubleList(3, xd, yd, zd));
}

void Fireball::readAdditionalSaveData(CompoundTag* tag) {
    xTile = tag->getShort("xTile");
    yTile = tag->getShort("yTile");
    zTile = tag->getShort("zTile");
    lastTile = tag->getByte("inTile") & 0xff;
    inGround = tag->getByte("inGround") == 1;

    // Load the stored direction and apply it to the fireball
    //   if it has no stored direction, remove it.
    if (tag->contains("direction")) {
        ListTag<DoubleTag>* listTag =
            (ListTag<DoubleTag>*)tag->getList("direction");
        xd = ((DoubleTag*)listTag->get(0))->data;
        yd = ((DoubleTag*)listTag->get(1))->data;
        zd = ((DoubleTag*)listTag->get(2))->data;
    } else {
        remove();
    }
}

bool Fireball::isPickable() { return true; }

float Fireball::getPickRadius() { return 1; }

bool Fireball::hurt(DamageSource* source, float damage) {
    if (isInvulnerable()) return false;
    markHurt();

    if (source->getEntity() != nullptr) {
        auto lookAngle = source->getEntity()->getLookAngle();
        if (lookAngle.has_value()) {
            xd = lookAngle->x;
            yd = lookAngle->y;
            zd = lookAngle->z;
            xPower = xd * 0.1;
            yPower = yd * 0.1;
            zPower = zd * 0.1;
        }

        if (source->getEntity()->instanceof (eTYPE_LIVINGENTITY)) {
            owner =
                std::dynamic_pointer_cast<LivingEntity>(source->getEntity());
        }
        return true;
    }
    return false;
}

float Fireball::getShadowHeightOffs() { return 0; }

float Fireball::getBrightness(float a) { return 1.0f; }

int Fireball::getLightColor(float a) { return 15 << 20 | 15 << 4; }

ePARTICLE_TYPE Fireball::getTrailParticleType() { return eParticleType_smoke; }

bool Fireball::shouldBurn() { return true; }
