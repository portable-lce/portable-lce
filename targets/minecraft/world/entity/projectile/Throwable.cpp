#include "Throwable.h"

#include <math.h>
#include <stdint.h>

#include <numbers>
#include <vector>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/HitResult.h"
#include "minecraft/world/phys/Vec3.h"
#include "nbt/CompoundTag.h"

void Throwable::_throwableInit() {
    xTile = -1;
    yTile = -1;
    zTile = -1;
    lastTile = 0;
    inGround = false;
    shakeTime = 0;
    owner = nullptr;
    life = 0;
    flightTime = 0;
    ownerName = "";
}

Throwable::Throwable(Level* level) : Entity(level) {
    _throwableInit();
    setSize(0.25f, 0.25f);
}

void Throwable::defineSynchedData() {}

bool Throwable::shouldRenderAtSqrDistance(double distance) {
    double size = bb.getSize() * 4;
    size *= 64.0f;
    return distance < size * size;
}

Throwable::Throwable(Level* level, std::shared_ptr<LivingEntity> mob)
    : Entity(level) {
    _throwableInit();
    owner = mob;

    setSize(4 / 16.0f, 4 / 16.0f);

    moveTo(mob->x, mob->y + mob->getHeadHeight(), mob->z, mob->yRot, mob->xRot);

    x -= cos(yRot / 180 * std::numbers::pi) * 0.16f;
    y -= 0.1f;
    z -= sin(yRot / 180 * std::numbers::pi) * 0.16f;
    setPos(x, y, z);
    heightOffset = 0;

    float speed = 0.4f;
    xd = (-sin(yRot / 180 * std::numbers::pi) *
          cos(xRot / 180 * std::numbers::pi)) *
         speed;
    zd = (cos(yRot / 180 * std::numbers::pi) *
          cos(xRot / 180 * std::numbers::pi)) *
         speed;
    yd = (-sin((xRot + getThrowUpAngleOffset()) / 180 * std::numbers::pi)) *
         speed;

    shoot(xd, yd, zd, getThrowPower(), 1);
}

Throwable::Throwable(Level* level, double x, double y, double z)
    : Entity(level) {
    _throwableInit();
    life = 0;

    setSize(4 / 16.0f, 4 / 16.0f);

    setPos(x, y, z);
    heightOffset = 0;
}

float Throwable::getThrowPower() { return 1.5f; }

float Throwable::getThrowUpAngleOffset() { return 0; }

void Throwable::shoot(double xd, double yd, double zd, float pow,
                      float uncertainty) {
    float dist = sqrt(xd * xd + yd * yd + zd * zd);

    xd /= dist;
    yd /= dist;
    zd /= dist;

    xd += (random->nextGaussian()) * 0.0075f * uncertainty;
    yd += (random->nextGaussian()) * 0.0075f * uncertainty;
    zd += (random->nextGaussian()) * 0.0075f * uncertainty;

    xd *= pow;
    yd *= pow;
    zd *= pow;

    this->xd = xd;
    this->yd = yd;
    this->zd = zd;

    float sd = (float)sqrt(xd * xd + zd * zd);

    yRotO = yRot = (float)(atan2(xd, zd) * 180 / std::numbers::pi);
    xRotO = xRot = (float)(atan2(yd, (double)sd) * 180 / std::numbers::pi);
    life = 0;
}

void Throwable::lerpMotion(double xd, double yd, double zd) {
    this->xd = xd;
    this->yd = yd;
    this->zd = zd;
    if (xRotO == 0 && yRotO == 0) {
        float sd = (float)sqrt(xd * xd + zd * zd);
        yRotO = yRot = (float)(atan2(xd, zd) * 180 / std::numbers::pi);
        xRotO = xRot = (float)(atan2(yd, (double)sd) * 180 / std::numbers::pi);
    }
}

void Throwable::tick() {
    xOld = x;
    yOld = y;
    zOld = z;
    Entity::tick();

    if (shakeTime > 0) shakeTime--;

    if (inGround) {
        int tile = level->getTile(xTile, yTile, zTile);
        if (tile == lastTile) {
            life++;
            if (life == 20 * 60) remove();
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
        to = Vec3(res->pos.x, res->pos.y, res->pos.z);
    }

    if (!level->isClientSide) {
        std::shared_ptr<Entity> hitEntity = nullptr;
        AABB grown = bb.expand(xd, yd, zd).grow(1, 1, 1);
        std::vector<std::shared_ptr<Entity> >* objects =
            level->getEntities(shared_from_this(), &grown);
        double nearest = 0;
        std::shared_ptr<LivingEntity> owner = getOwner();
        for (int i = 0; i < objects->size(); i++) {
            std::shared_ptr<Entity> e = objects->at(i);
            if (!e->isPickable() || (e == owner && flightTime < 5)) continue;

            float rr = 0.3f;
            AABB bb = e->bb.grow(rr, rr, rr);
            HitResult* p = bb.clip(from, to);
            if (p != nullptr) {
                double dd = from.distanceTo(p->pos);
                delete p;
                if (dd < nearest || nearest == 0) {
                    hitEntity = e;
                    nearest = dd;
                }
            }
        }

        if (hitEntity != nullptr) {
            if (res != nullptr) delete res;
            res = new HitResult(hitEntity);
        }
    }

    if (res != nullptr) {
        if ((res->type == HitResult::TILE) &&
            (level->getTile(res->x, res->y, res->z) == Tile::portalTile_Id)) {
            handleInsidePortal();
        } else {
            onHit(res);
        }
        delete res;
    }
    x += xd;
    y += yd;
    z += zd;

    float sd = (float)sqrt(xd * xd + zd * zd);
    yRot = (float)(atan2(xd, zd) * 180 / std::numbers::pi);
    xRot = (float)(atan2(yd, (double)sd) * 180 / std::numbers::pi);

    while (xRot - xRotO < -180) xRotO -= 360;
    while (xRot - xRotO >= 180) xRotO += 360;

    while (yRot - yRotO < -180) yRotO -= 360;
    while (yRot - yRotO >= 180) yRotO += 360;

    xRot = xRotO + (xRot - xRotO) * 0.2f;
    yRot = yRotO + (yRot - yRotO) * 0.2f;

    float inertia = 0.99f;
    float gravity = getGravity();

    if (isInWater()) {
        for (int i = 0; i < 4; i++) {
            float s = 1 / 4.0f;
            level->addParticle(eParticleType_bubble, x - xd * s, y - yd * s,
                               z - zd * s, xd, yd, zd);
        }
        inertia = 0.80f;
    }

    xd *= inertia;
    yd *= inertia;
    zd *= inertia;
    yd -= gravity;

    setPos(x, y, z);
}

float Throwable::getGravity() { return 0.03f; }

void Throwable::addAdditonalSaveData(CompoundTag* tag) {
    tag->putShort("xTile", (short)xTile);
    tag->putShort("yTile", (short)yTile);
    tag->putShort("zTile", (short)zTile);
    tag->putByte("inTile", (uint8_t)lastTile);
    tag->putByte("shake", (uint8_t)shakeTime);
    tag->putByte("inGround", (uint8_t)(inGround ? 1 : 0));

    if (ownerName.empty() && (owner != nullptr) &&
        owner->instanceof(eTYPE_PLAYER)) {
        ownerName = owner->getAName();
    }

    tag->putString("ownerName", ownerName.empty() ? "" : ownerName);
}

void Throwable::readAdditionalSaveData(CompoundTag* tag) {
    xTile = tag->getShort("xTile");
    yTile = tag->getShort("yTile");
    zTile = tag->getShort("zTile");
    lastTile = tag->getByte("inTile") & 0xff;
    shakeTime = tag->getByte("shake") & 0xff;
    inGround = tag->getByte("inGround") == 1;
    ownerName = tag->getString("ownerName");
    if (ownerName.empty()) ownerName = "";
}

float Throwable::getShadowHeightOffs() { return 0; }

std::shared_ptr<LivingEntity> Throwable::getOwner() {
    if (owner == nullptr && !ownerName.empty()) {
        owner = level->getPlayerByName(ownerName);
    }
    return owner;
}
