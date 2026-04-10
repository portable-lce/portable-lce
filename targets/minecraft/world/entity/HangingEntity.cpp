#include "HangingEntity.h"

#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>

#include "minecraft/Direction.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/damageSource/EntityDamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"

void HangingEntity::_init(Level* level) {
    checkInterval = 0;
    dir = 0;
    xTile = yTile = zTile = 0;
    this->heightOffset = 0;
    this->setSize(0.5f, 0.5f);
}

HangingEntity::HangingEntity(Level* level) : Entity(level) { _init(level); }

HangingEntity::HangingEntity(Level* level, int xTile, int yTile, int zTile,
                             int dir)
    : Entity(level) {
    _init(level);
    this->xTile = xTile;
    this->yTile = yTile;
    this->zTile = zTile;
}

void HangingEntity::setDir(int dir) {
    this->dir = dir;
    yRotO = yRot = (float)(dir * 90);

    float w = (float)getWidth();
    float h = (float)getHeight();
    float d = (float)getWidth();

    if (dir == Direction::NORTH || dir == Direction::SOUTH) {
        d = 0.5f;
        yRot = yRotO = (float)(Direction::DIRECTION_OPPOSITE[dir] * 90);
    } else {
        w = 0.5f;
    }

    w /= 32.0f;
    h /= 32.0f;
    d /= 32.0f;

    float x = xTile + 0.5f;
    float y = yTile + 0.5f;
    float z = zTile + 0.5f;

    float fOffs = 0.5f + 1.0f / 16.0f;

    if (dir == Direction::NORTH) z -= fOffs;
    if (dir == Direction::WEST) x -= fOffs;
    if (dir == Direction::SOUTH) z += fOffs;
    if (dir == Direction::EAST) x += fOffs;

    if (dir == Direction::NORTH) x -= offs(getWidth());
    if (dir == Direction::WEST) z += offs(getWidth());
    if (dir == Direction::SOUTH) x += offs(getWidth());
    if (dir == Direction::EAST) z -= offs(getWidth());
    y += offs(getHeight());

    setPos(x, y, z);

    float ss = -(0.5f / 16.0f);

    // 4J Stu - Due to rotations the bb couold be set with a lower bound x/z
    // being higher than the higher bound
    float x0 = x - w - ss;
    float x1 = x + w + ss;
    float y0 = y - h - ss;
    float y1 = y + h + ss;
    float z0 = z - d - ss;
    float z1 = z + d + ss;
    bb = {std::min(x0, x1), std::min(y0, y1), std::min(z0, z1),
          std::max(x0, x1), std::max(y0, y1), std::max(z0, z1)};
}

float HangingEntity::offs(int w) {
    if (w == 32) return 0.5f;
    if (w == 64) return 0.5f;
    return 0.0f;
}

void HangingEntity::tick() {
    xo = x;
    yo = y;
    zo = z;
    if (checkInterval++ == 20 * 5 && !level->isClientSide) {
        checkInterval = 0;
        if (!removed && !survives()) {
            remove();
            dropItem(nullptr);
        }
    }
}

bool HangingEntity::survives() {
    if (level->getCubes(shared_from_this(), &bb)->size() != 0)  // isEmpty())
    {
        return false;
    } else {
        int ws = std::max(1, getWidth() / 16);
        int hs = std::max(1, getHeight() / 16);

        int xt = xTile;
        int yt = yTile;
        int zt = zTile;
        if (dir == Direction::NORTH) xt = Mth::floor(x - getWidth() / 32.0f);
        if (dir == Direction::WEST) zt = Mth::floor(z - getWidth() / 32.0f);
        if (dir == Direction::SOUTH) xt = Mth::floor(x - getWidth() / 32.0f);
        if (dir == Direction::EAST) zt = Mth::floor(z - getWidth() / 32.0f);
        yt = Mth::floor(y - getHeight() / 32.0f);

        for (int ss = 0; ss < ws; ss++) {
            for (int yy = 0; yy < hs; yy++) {
                Material* m;
                if (dir == Direction::NORTH || dir == Direction::SOUTH) {
                    m = level->getMaterial(xt + ss, yt + yy, zTile);
                } else {
                    m = level->getMaterial(xTile, yt + yy, zt + ss);
                }
                if (!m->isSolid()) {
                    return false;
                }
            }

            std::vector<std::shared_ptr<Entity> >* entities =
                level->getEntities(shared_from_this(), &bb);

            if (entities != nullptr && entities->size() > 0) {
                auto itEnd = entities->end();
                for (auto it = entities->begin(); it != itEnd; it++) {
                    std::shared_ptr<Entity> e = (*it);
                    if (e->instanceof(eTYPE_HANGING_ENTITY)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool HangingEntity::isPickable() { return true; }

bool HangingEntity::skipAttackInteraction(std::shared_ptr<Entity> source) {
    if (source->GetType() == eTYPE_PLAYER) {
        return hurt(DamageSource::playerAttack(
                        std::dynamic_pointer_cast<Player>(source)),
                    0);
    }
    return false;
}

bool HangingEntity::hurt(DamageSource* source, float damage) {
    if (isInvulnerable()) return false;
    if (!removed && !level->isClientSide) {
        if (dynamic_cast<EntityDamageSource*>(source) != nullptr) {
            std::shared_ptr<Entity> sourceEntity = source->getDirectEntity();

            if ((sourceEntity != nullptr) &&
                sourceEntity->instanceof(eTYPE_PLAYER) &&
                !std::dynamic_pointer_cast<Player>(sourceEntity)
                     ->isAllowedToHurtEntity(shared_from_this())) {
                return false;
            }
        }

        remove();
        markHurt();

        std::shared_ptr<Player> player = nullptr;
        std::shared_ptr<Entity> e = source->getEntity();
        if ((e != nullptr) &&
            e->instanceof(
                eTYPE_PLAYER))  // check if it's serverplayer or player
        {
            player = std::dynamic_pointer_cast<Player>(e);
        }

        if (player != nullptr && player->abilities.instabuild) {
            return true;
        }

        dropItem(nullptr);
    }
    return true;
}

// 4J - added noEntityCubes parameter
void HangingEntity::move(double xa, double ya, double za, bool noEntityCubes) {
    if (!level->isClientSide && !removed && (xa * xa + ya * ya + za * za) > 0) {
        remove();
        dropItem(nullptr);
    }
}

void HangingEntity::push(double xa, double ya, double za) {
    if (!level->isClientSide && !removed && (xa * xa + ya * ya + za * za) > 0) {
        remove();
        dropItem(nullptr);
    }
}

void HangingEntity::addAdditonalSaveData(CompoundTag* tag) {
    tag->putByte("Direction", (uint8_t)dir);
    tag->putInt("TileX", xTile);
    tag->putInt("TileY", yTile);
    tag->putInt("TileZ", zTile);

    // Back compat
    switch (dir) {
        case Direction::NORTH:
            tag->putByte("Dir", (uint8_t)0);
            break;
        case Direction::WEST:
            tag->putByte("Dir", (uint8_t)1);
            break;
        case Direction::SOUTH:
            tag->putByte("Dir", (uint8_t)2);
            break;
        case Direction::EAST:
            tag->putByte("Dir", (uint8_t)3);
            break;
    }
}

void HangingEntity::readAdditionalSaveData(CompoundTag* tag) {
    if (tag->contains("Direction")) {
        dir = tag->getByte("Direction");
    } else {
        switch (tag->getByte("Dir")) {
            case 0:
                dir = Direction::NORTH;
                break;
            case 1:
                dir = Direction::WEST;
                break;
            case 2:
                dir = Direction::SOUTH;
                break;
            case 3:
                dir = Direction::EAST;
                break;
        }
    }
    xTile = tag->getInt("TileX");
    yTile = tag->getInt("TileY");
    zTile = tag->getInt("TileZ");
    setDir(dir);
}

bool HangingEntity::repositionEntityAfterLoad() { return false; }
