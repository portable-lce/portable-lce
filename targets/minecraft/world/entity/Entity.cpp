#include "Entity.h"

#include <stdarg.h>
#include <stdlib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include "EntityIO.h"
#include "EntityPos.h"
#include "SyncedEntityData.h"
#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/Direction.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/Pos.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/PlayerList.h"
#include "minecraft/server/level/ServerLevel.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/util/Log.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/ProtectionEnchantment.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/LiquidTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/Vec3.h"
#include "nbt/CompoundTag.h"
#include "nbt/DoubleTag.h"
#include "nbt/FloatTag.h"
#include "nbt/ListTag.h"
#include "util/StringHelpers.h"

thread_local bool Entity::m_tlsUseSmallIds = false;

const std::string Entity::RIDING_TAG = "Riding";
int Entity::entityCounter =
    2048;  // 4J - changed initialiser to 2048, as we are using range 0 - 2047
           // as special unique smaller ids for things that need network tracked

// 4J - added getSmallId & freeSmallId methods
unsigned int Entity::entityIdUsedFlags[2048 / 32] = {0};
unsigned int Entity::entityIdWanderFlags[2048 / 32] = {0};
unsigned int Entity::entityIdRemovingFlags[2048 / 32] = {0};
int Entity::extraWanderIds[EXTRA_WANDER_MAX] = {0};
int Entity::extraWanderTicks = 0;
int Entity::extraWanderCount = 0;

int Entity::getSmallId() {
    unsigned int* puiUsedFlags = entityIdUsedFlags;
    unsigned int* puiRemovedFlags = nullptr;

    // If we are the server (we should be, if we are allocating small Ids), then
    // check with the server if there are any small Ids which are still in the
    // ServerPlayer's vectors of entities to be removed - these are used to
    // gather up a set of entities into one network packet for final
    // notification to the client that the entities are removed. We can't go
    // re-using these small Ids yet, as otherwise we will potentially end up
    // telling the client that the entity has been removed After we have already
    // re-used its Id and created a new entity. This ends up with newly created
    // client-side entities being removed by accident, causing invisible mobs.
    if (m_tlsUseSmallIds) {
        MinecraftServer* server = MinecraftServer::getInstance();
        if (server) {
            // In some attempt to optimise this, flagEntitiesToBeRemoved most of
            // the time shouldn't do anything at all, and in this case it
            // doesn't even memset the entityIdRemovingFlags array, so we
            // shouldn't use it if the return value is false.
            bool removedFound =
                server->flagEntitiesToBeRemoved(entityIdRemovingFlags);
            if (removedFound) {
                // Has set up the entityIdRemovingFlags vector in this case, so
                // we should check against this when allocating new ids
                //				Log::info("getSmallId:
                // Removed entities found\n");
                puiRemovedFlags = entityIdRemovingFlags;
            }
        }
    }

    for (int i = 0; i < (2048 / 32); i++) {
        unsigned int uiFlags = *puiUsedFlags;
        if (uiFlags != 0xffffffff) {
            unsigned int uiMask = 0x80000000;
            for (int j = 0; j < 32; j++) {
                // See comments above - now checking (if required) that these
                // aren't newly removed entities that the clients still haven't
                // been told about, so we don't reuse those ids before we
                // should.
                if (puiRemovedFlags) {
                    if (puiRemovedFlags[i] & uiMask) {
                        //						Log::info("Avoiding
                        // using ID %d (0x%x)\n", i * 32 +
                        // j,puiRemovedFlags[i]);
                        uiMask >>= 1;
                        continue;
                    }
                }
                if ((uiFlags & uiMask) == 0) {
                    uiFlags |= uiMask;
                    *puiUsedFlags = uiFlags;
                    return i * 32 + j;
                }
                uiMask >>= 1;
            }
        }
        puiUsedFlags++;
    }

    Log::info("Out of small entity Ids... possible leak?\n");
    assert(0);
    return -1;
}

void Entity::countFlagsForPIX() {
    int freecount = 0;
    unsigned int* puiUsedFlags = entityIdUsedFlags;
    for (int i = 0; i < (2048 / 32); i++) {
        unsigned int uiFlags = *puiUsedFlags;
        if (uiFlags != 0xffffffff) {
            unsigned int uiMask = 0x80000000;
            for (int j = 0; j < 32; j++) {
                if ((uiFlags & uiMask) == 0) {
                    freecount++;
                }
                uiMask >>= 1;
            }
        }
        puiUsedFlags++;
    }
}

void Entity::resetSmallId() {
    freeSmallId(entityId);
    if (m_tlsUseSmallIds) {
        entityId = getSmallId();
    }
}

void Entity::freeSmallId(int index) {
    if (!m_tlsUseSmallIds)
        return;  // Don't do anything with small ids if this isn't the server
                 // thread
    if (index >= 2048) return;  // Don't do anything if this isn't a short id

    unsigned int i = index / 32;
    unsigned int j = index % 32;
    unsigned int uiMask = ~(0x80000000 >> j);

    entityIdUsedFlags[i] &= uiMask;
    entityIdWanderFlags[i] &= uiMask;
}

void Entity::useSmallIds() { m_tlsUseSmallIds = true; }

// Things also added here to be able to manage the concept of a number of extra
// "wandering" entities - normally path finding entities aren't allowed to
// randomly wander about once they are a certain distance away from any player,
// but we want to be able to (in a controlled fashion) allow some to be able to
// move so that we can determine whether they have been enclosed in some kind of
// farm, and so be able to better determine what shouldn't or shouldn't be
// despawned.

// Let the management system here know whether or not to consider this
// particular entity for some extra wandering
void Entity::considerForExtraWandering(bool enable) {
    if (!m_tlsUseSmallIds)
        return;  // Don't do anything with small ids if this isn't the server
                 // thread
    if (entityId >= 2048) return;  // Don't do anything if this isn't a short id

    unsigned int i = entityId / 32;
    unsigned int j = entityId % 32;
    if (enable) {
        unsigned int uiMask = 0x80000000 >> j;
        entityIdWanderFlags[i] |= uiMask;
    } else {
        unsigned int uiMask = ~(0x80000000 >> j);
        entityIdWanderFlags[i] &= uiMask;
    }
}

// Should this entity do wandering in addition to what the java code would have
// done?
bool Entity::isExtraWanderingEnabled() {
    if (!m_tlsUseSmallIds)
        return false;  // Don't do anything with small ids if this isn't the
                       // server thread
    if (entityId >= 2048)
        return false;  // Don't do anything if this isn't a short id

    for (int i = 0; i < extraWanderCount; i++) {
        if (extraWanderIds[i] == entityId) return true;
    }
    return false;
}

// Returns a quadrant of direction that a given entity should be moved in - this
// is to stop the randomness of the wandering/strolling from just making the
// entity double back on itself and thus making the determination of whether the
// entity has been enclosed take longer than it needs to. This function returns
// a quadrant from 0 to 3 that should be consistent within one period of an
// entity being considered for extra wandering, but should potentially vary
// between tries and between different entities.
int Entity::getWanderingQuadrant() {
    return (entityId + (extraWanderTicks / EXTRA_WANDER_TICKS)) & 3;
}

// Every EXTRA_WANDER_TICKS ticks, attempt to find EXTRA_WANDER_MAX entity Ids
// from those that have been flagged as ones that should be considered for extra
// wandering
void Entity::tickExtraWandering() {
    extraWanderTicks++;
    // Time to move onto some new entities?

    if ((extraWanderTicks % EXTRA_WANDER_TICKS == 0)) {
        //		printf("Updating extras: ");
        // Start from the next Id after the one that we last found, or zero if
        // we didn't find anything last time
        int entityId = 0;
        if (extraWanderCount) {
            entityId = (extraWanderIds[extraWanderCount - 1] + 1) % 2048;
        }

        extraWanderCount = 0;

        for (int k = 0; (k < 2048) && (extraWanderCount < EXTRA_WANDER_MAX);
             k++) {
            unsigned int i = entityId / 32;
            unsigned int j = entityId % 32;
            unsigned int uiMask = 0x80000000 >> j;

            if (entityIdWanderFlags[i] & uiMask) {
                extraWanderIds[extraWanderCount++] = entityId;
                //				printf("%d, ", entityId);
            }

            entityId = (entityId + 1) % 2048;
        }
        //		printf("\n");
    }
}

// 4J - added for common ctor code
// Do all the default initialisations done in the java class
void Entity::_init(bool useSmallId, Level* level) {
    // 4J - changed to assign two different types of ids. A range from 0-2047 is
    // used for things that we'll be wanting to identify over the network, so we
    // should only need 11 bits rather than 32 to uniquely identify them. The
    // rest of the range is used for anything we don't need to track like this,
    // currently particles. We only ever want to allocate this type of id from
    // the server thread, so using thread local storage to isolate this.
    if (useSmallId && m_tlsUseSmallIds) {
        entityId = getSmallId();
    } else {
        entityId = Entity::entityCounter++;
        if (entityCounter == 0x7ffffff) entityCounter = 2048;
    }

    viewScale = 1.0;

    blocksBuilding = false;
    rider = std::weak_ptr<Entity>();
    riding = nullptr;
    forcedLoading = false;

    // level = nullptr; // Level is assigned to in the original c_tor code
    xo = yo = zo = 0.0;
    x = y = z = 0.0;
    xd = yd = zd = 0.0;
    yRot = xRot = 0.0f;
    yRotO = xRotO = 0.0f;
    bb = AABB(0, 0, 0, 0, 0, 0);  // 4J Was final
    onGround = false;
    horizontalCollision = verticalCollision = false;
    collision = false;
    hurtMarked = false;
    isStuckInWeb = false;

    slide = true;
    removed = false;
    heightOffset = 0 / 16.0f;

    bbWidth = 0.6f;
    bbHeight = 1.8f;

    walkDistO = 0;
    walkDist = 0;
    moveDist = 0.0f;

    fallDistance = 0;

    nextStep = 1;

    xOld = yOld = zOld = 0.0;
    ySlideOffset = 0;
    footSize = 0.0f;
    noPhysics = false;
    pushthrough = 0.0f;

    random = new Random();

    tickCount = 0;
    flameTime = 1;

    onFire = 0;
    wasInWater = false;

    invulnerableTime = 0;

    firstTick = true;

    fireImmune = false;

    // values that need to be sent to clients in SMP
    if (useSmallId) {
        entityData = std::make_shared<SynchedEntityData>();
    } else {
        entityData = nullptr;
    }

    xRideRotA = yRideRotA = 0.0;
    inChunk = false;
    xChunk = yChunk = zChunk = 0;
    xp = yp = zp = 0;
    xRotp = yRotp = 0;
    noCulling = false;

    hasImpulse = false;

    changingDimensionDelay = 0;
    isInsidePortal = false;
    portalTime = 0;
    dimension = 0;
    portalEntranceDir = 0;
    invulnerable = false;
    if (useSmallId) {
        uuid = "ent" + Mth::createInsecureUUID(random);
    }

    // 4J Added
    m_ignoreVerticalCollisions = false;
    m_uiAnimOverrideBitmask = 0L;
    m_ignorePortal = false;
}

Entity::Entity(Level* level,
               bool useSmallId)  // 4J - added useSmallId parameter
{
    _init(useSmallId, level);

    this->level = level;
    // resetPos();
    setPos(0, 0, 0);

    if (level != nullptr) {
        dimension = level->dimension->id;
    }

    if (entityData) {
        entityData->define(DATA_SHARED_FLAGS_ID, (uint8_t)0);
        entityData->define(
            DATA_AIR_SUPPLY_ID,
            TOTAL_AIR_SUPPLY);  // 4J Stu - Brought forward from 1.2.3 to fix
                                // 38654 - Gameplay: Player will take damage
                                // when air bubbles are present if resuming game
                                // from load/autosave underwater.
    }

    // 4J Stu - We cannot call virtual functions in ctors, as at this point the
    // object is of type Entity and not a derived class
    // this->defineSynchedData();
}

Entity::~Entity() {
    freeSmallId(entityId);
    delete random;
}

std::shared_ptr<SynchedEntityData> Entity::getEntityData() {
    return entityData;
}

/*
public bool equals(Object obj) {
if (obj instanceof Entity) {
return ((Entity) obj).entityId == entityId;
}
return false;
}

public int hashCode() {
return entityId;
}
*/

void Entity::resetPos() {
    if (level == nullptr) return;

    std::shared_ptr<Entity> sharedThis = shared_from_this();
    while (true && y > 0) {
        setPos(x, y, z);
        if (level->getCubes(sharedThis, &bb)->empty()) break;
        y += 1;
    }

    xd = yd = zd = 0;
    xRot = 0;
}

void Entity::remove() { removed = true; }

void Entity::setSize(float w, float h) {
    if (w != bbWidth || h != bbHeight) {
        float oldW = bbWidth;

        bbWidth = w;
        bbHeight = h;

        bb.x1 = bb.x0 + bbWidth;
        bb.z1 = bb.z0 + bbWidth;
        bb.y1 = bb.y0 + bbHeight;

        if (bbWidth > oldW && !firstTick && !level->isClientSide) {
            move(oldW - bbWidth, 0, oldW - bbWidth);
        }
    }
}

void Entity::setPos(EntityPos* pos) {
    if (pos->move)
        setPos(pos->x, pos->y, pos->z);
    else
        setPos(x, y, z);

    if (pos->rot)
        setRot(pos->yRot, pos->xRot);
    else
        setRot(yRot, xRot);
}

void Entity::setRot(float yRot, float xRot) {
    /* JAVA:
    this->yRot = yRot % 360.0f;
    this->xRot = xRot % 360.0f;

    C++ Cannot do mod of non-integral type
    */

    while (yRot >= 360.0f) yRot -= 360.0f;
    while (yRot < 0) yRot += 360.0f;
    while (xRot >= 360.0f) xRot -= 360.0f;

    this->yRot = yRot;
    this->xRot = xRot;
}

void Entity::setPos(double x, double y, double z) {
    this->x = x;
    this->y = y;
    this->z = z;
    float w = bbWidth / 2;
    float h = bbHeight;
    bb = {x - w, y - heightOffset + ySlideOffset,     z - w,
          x + w, y - heightOffset + ySlideOffset + h, z + w};
}

void Entity::turn(float xo, float yo) {
    float xRotOld = xRot;
    float yRotOld = yRot;

    yRot += xo * 0.15f;
    xRot -= yo * 0.15f;
    if (xRot < -90) xRot = -90;
    if (xRot > 90) xRot = 90;

    xRotO += xRot - xRotOld;
    yRotO += yRot - yRotOld;
}

void Entity::interpolateTurn(float xo, float yo) {
    yRot += xo * 0.15f;
    xRot -= yo * 0.15f;
    if (xRot < -90) xRot = -90;
    if (xRot > 90) xRot = 90;
}

void Entity::tick() { baseTick(); }

void Entity::baseTick() {
    // 4J Stu - Not needed
    // util.Timer.push("entityBaseTick");

    if (riding != nullptr && riding->removed) {
        riding = nullptr;
    }

    walkDistO = walkDist;
    xo = x;
    yo = y;
    zo = z;
    xRotO = xRot;
    yRotO = yRot;

    if (!level->isClientSide)  // 4J Stu - Don't need this && level instanceof
                               // ServerLevel)
    {
        if (!m_ignorePortal)  // 4J Added
        {
            MinecraftServer* server =
                dynamic_cast<ServerLevel*>(level)->getServer();
            int waitTime = getPortalWaitTime();

            if (isInsidePortal) {
                if (server->isNetherEnabled()) {
                    if (riding == nullptr) {
                        if (portalTime++ >= waitTime) {
                            portalTime = waitTime;
                            changingDimensionDelay =
                                getDimensionChangingDelay();

                            int targetDimension;

                            if (level->dimension->id == -1) {
                                targetDimension = 0;
                            } else {
                                targetDimension = -1;
                            }

                            changeDimension(targetDimension);
                        }
                    }
                    isInsidePortal = false;
                }
            } else {
                if (portalTime > 0) portalTime -= 4;
                if (portalTime < 0) portalTime = 0;
            }
            if (changingDimensionDelay > 0) changingDimensionDelay--;
        }
    }

    if (isSprinting() && !isInWater() && canCreateParticles()) {
        int xt = Mth::floor(x);
        int yt = Mth::floor(y - 0.2f - heightOffset);
        int zt = Mth::floor(z);
        int t = level->getTile(xt, yt, zt);
        int d = level->getData(xt, yt, zt);
        if (t > 0) {
            level->addParticle(PARTICLE_TILECRACK(t, d),
                               x + (random->nextFloat() - 0.5) * bbWidth,
                               bb.y0 + 0.1,
                               z + (random->nextFloat() - 0.5) * bbWidth,
                               -xd * 4, 1.5, -zd * 4);
        }
    }

    updateInWaterState();

    if (level->isClientSide) {
        onFire = 0;
    } else {
        if (onFire > 0) {
            if (fireImmune) {
                onFire -= 4;
                if (onFire < 0) onFire = 0;
            } else {
                if (onFire % 20 == 0) {
                    hurt(DamageSource::onFire, 1);
                }
                onFire--;
            }
        }
    }

    if (isInLava()) {
        lavaHurt();
        fallDistance *= .5f;
    }

    if (y < -64) {
        outOfWorld();
    }

    if (!level->isClientSide) {
        setSharedFlag(FLAG_ONFIRE, onFire > 0);
    }

    firstTick = false;

    // 4J Stu - Unused
    // util.Timer.pop();
}

int Entity::getPortalWaitTime() { return 0; }

void Entity::lavaHurt() {
    if (fireImmune) {
    } else {
        hurt(DamageSource::lava, 4);
        setOnFire(15);
    }
}

void Entity::setOnFire(int numberOfSeconds) {
    int newValue = numberOfSeconds * SharedConstants::TICKS_PER_SECOND;
    newValue = ProtectionEnchantment::getFireAfterDampener(shared_from_this(),
                                                           newValue);
    if (onFire < newValue) {
        onFire = newValue;
    }
}

void Entity::clearFire() { onFire = 0; }

void Entity::outOfWorld() { remove(); }

bool Entity::isFree(float xa, float ya, float za, float grow) {
    AABB box = bb.grow(grow, grow, grow).move(xa, ya, za);
    std::vector<AABB>* aABBs = level->getCubes(shared_from_this(), &box);
    if (!aABBs->empty()) return false;
    if (level->containsAnyLiquid(&box)) return false;
    return true;
}

bool Entity::isFree(double xa, double ya, double za) {
    AABB box = bb.move(xa, ya, za);
    std::vector<AABB>* aABBs = level->getCubes(shared_from_this(), &box);
    if (!aABBs->empty()) return false;
    if (level->containsAnyLiquid(&box)) return false;
    return true;
}

void Entity::move(double xa, double ya, double za,
                  bool noEntityCubes)  // 4J - added noEntityCubes parameter
{
    if (noPhysics) {
        bb = bb.move(xa, ya, za);
        x = (bb.x0 + bb.x1) / 2.0f;
        y = bb.y0 + heightOffset - ySlideOffset;
        z = (bb.z0 + bb.z1) / 2.0f;
        return;
    }

    ySlideOffset *= 0.4f;

    double xo = x;
    double yo = y;
    double zo = z;

    if (isStuckInWeb) {
        isStuckInWeb = false;

        xa *= 0.25f;
        ya *= 0.05f;
        za *= 0.25f;
        xd = 0.0f;
        yd = 0.0f;
        zd = 0.0f;
    }

    double xaOrg = xa;
    double yaOrg = ya;
    double zaOrg = za;

    AABB bbOrg = bb;

    bool isPlayerSneaking = onGround && isSneaking() && instanceof
        (eTYPE_PLAYER);

    if (isPlayerSneaking) {
        double d = 0.05;

        AABB translated_bb = bb.move(xa, -1.0, 0.0);
        while (xa != 0 &&
               level->getCubes(shared_from_this(), &translated_bb)->empty()) {
            if (xa < d && xa >= -d)
                xa = 0;
            else if (xa > 0)
                xa -= d;
            else
                xa += d;
            xaOrg = xa;
        }

        translated_bb = bb.move(0, -1.0, za);
        while (za != 0 &&
               level->getCubes(shared_from_this(), &translated_bb)->empty()) {
            if (za < d && za >= -d)
                za = 0;
            else if (za > 0)
                za -= d;
            else
                za += d;
            zaOrg = za;
        }

        translated_bb = bb.move(xa, -1.0, za);
        while (xa != 0 && za != 0 &&
               level->getCubes(shared_from_this(), &translated_bb)->empty()) {
            if (xa < d && xa >= -d)
                xa = 0;
            else if (xa > 0)
                xa -= d;
            else
                xa += d;
            if (za < d && za >= -d)
                za = 0;
            else if (za > 0)
                za -= d;
            else
                za += d;
            xaOrg = xa;
            zaOrg = za;
        }
    }

    AABB expanded = bb.expand(xa, ya, za);
    std::vector<AABB>* aABBs =
        level->getCubes(shared_from_this(), &expanded, noEntityCubes, true);

    // LAND FIRST, then x and z
    auto itEndAABB = aABBs->end();

    // 4J Stu - Particles (and possibly other entities) don't have xChunk and
    // zChunk set, so calculate the chunk instead
    int xc = Mth::floor(x / 16);
    int zc = Mth::floor(z / 16);
    if (!level->isClientSide || level->reallyHasChunk(xc, zc)) {
        // 4J Stu - It's horrible that the client is doing any movement at all!
        // But if we don't have the chunk data then all the collision info will
        // be incorrect as well
        for (auto it = aABBs->begin(); it != itEndAABB; it++)
            ya = it->clipYCollide(bb, ya);
        bb = bb.move(0, ya, 0);
    }

    if (!slide && yaOrg != ya) {
        xa = ya = za = 0;
    }

    bool og = onGround || (yaOrg != ya && yaOrg < 0);

    itEndAABB = aABBs->end();
    for (auto it = aABBs->begin(); it != itEndAABB; it++)
        xa = it->clipXCollide(bb, xa);

    bb = bb.move(xa, 0, 0);

    if (!slide && xaOrg != xa) {
        xa = ya = za = 0;
    }

    itEndAABB = aABBs->end();
    for (auto it = aABBs->begin(); it != itEndAABB; it++)
        za = it->clipZCollide(bb, za);
    bb = bb.move(0, 0, za);

    if (!slide && zaOrg != za) {
        xa = ya = za = 0;
    }

    if (footSize > 0 && og && (isPlayerSneaking || ySlideOffset < 0.05f) &&
        ((xaOrg != xa) || (zaOrg != za))) {
        double xaN = xa;
        double yaN = ya;
        double zaN = za;

        xa = xaOrg;
        ya = footSize;
        za = zaOrg;

        AABB normal = bb;
        bb = bbOrg;

        // 4J - added extra expand, as if we don't move up by footSize by
        // hitting a block above us, then overall we could be trying to move as
        // much as footSize downwards, so we'd better include cubes under our
        // feet in this list of things we might possibly collide with
        AABB expanded = bb.expand(xa, ya, za).expand(0, -ya, 0);
        aABBs = level->getCubes(shared_from_this(), &expanded, false, true);

        // LAND FIRST, then x and z
        itEndAABB = aABBs->end();

        if (!level->isClientSide || level->reallyHasChunk(xc, zc)) {
            // 4J Stu - It's horrible that the client is doing any movement at
            // all! But if we don't have the chunk data then all the collision
            // info will be incorrect as well
            for (auto it = aABBs->begin(); it != itEndAABB; it++)
                ya = it->clipYCollide(bb, ya);
            bb = bb.move(0, ya, 0);
        }

        if (!slide && yaOrg != ya) {
            xa = ya = za = 0;
        }

        itEndAABB = aABBs->end();
        for (auto it = aABBs->begin(); it != itEndAABB; it++)
            xa = it->clipXCollide(bb, xa);
        bb = bb.move(xa, 0, 0);

        if (!slide && xaOrg != xa) {
            xa = ya = za = 0;
        }

        itEndAABB = aABBs->end();
        for (auto it = aABBs->begin(); it != itEndAABB; it++)
            za = it->clipZCollide(bb, za);
        bb = bb.move(0, 0, za);

        if (!slide && zaOrg != za) {
            xa = ya = za = 0;
        }

        if (!slide && yaOrg != ya) {
            xa = ya = za = 0;
        } else {
            ya = -footSize;
            // LAND FIRST, then x and z
            itEndAABB = aABBs->end();
            for (auto it = aABBs->begin(); it != itEndAABB; it++)
                ya = it->clipYCollide(bb, ya);
            bb = bb.move(0, ya, 0);
        }

        if (xaN * xaN + zaN * zaN >= xa * xa + za * za) {
            xa = xaN;
            ya = yaN;
            za = zaN;
            bb = normal;
        }
    }

    x = (bb.x0 + bb.x1) / 2.0f;
    y = bb.y0 + heightOffset - ySlideOffset;
    z = (bb.z0 + bb.z1) / 2.0f;

    horizontalCollision = (xaOrg != xa) || (zaOrg != za);
    verticalCollision = !m_ignoreVerticalCollisions && (yaOrg != ya);
    onGround = !m_ignoreVerticalCollisions && yaOrg != ya && yaOrg < 0;
    collision = horizontalCollision || verticalCollision;
    checkFallDamage(ya, onGround);

    if (xaOrg != xa) xd = 0;
    if (yaOrg != ya) yd = 0;
    if (zaOrg != za) zd = 0;

    double xm = x - xo;
    double ym = y - yo;
    double zm = z - zo;

    if (makeStepSound() && !isPlayerSneaking && riding == nullptr) {
        int xt = Mth::floor(x);
        int yt = Mth::floor(y - 0.2f - heightOffset);
        int zt = Mth::floor(z);
        int t = level->getTile(xt, yt, zt);
        if (t == 0) {
            int renderShape = level->getTileRenderShape(xt, yt - 1, zt);
            if (renderShape == Tile::SHAPE_FENCE ||
                renderShape == Tile::SHAPE_WALL ||
                renderShape == Tile::SHAPE_FENCE_GATE) {
                t = level->getTile(xt, yt - 1, zt);
            }
        }
        if (t != Tile::ladder_Id) {
            ym = 0;
        }

        walkDist += Mth::sqrt(xm * xm + zm * zm) * 0.6;
        moveDist += Mth::sqrt(xm * xm + ym * ym + zm * zm) * 0.6;

        if (moveDist > nextStep && t > 0) {
            nextStep = (int)moveDist + 1;
            if (isInWater()) {
                float speed =
                    Mth::sqrt(xd * xd * 0.2f + yd * yd + zd * zd * 0.2f) *
                    0.35f;
                if (speed > 1) speed = 1;
                playSound(
                    eSoundType_LIQUID_SWIM, speed,
                    1 + (random->nextFloat() - random->nextFloat()) * 0.4f);
            }
            playStepSound(xt, yt, zt, t);
            Tile::tiles[t]->stepOn(level, xt, yt, zt, shared_from_this());
        }
    }

    checkInsideTiles();

    bool water = isInWaterOrRain();
    AABB shrunk = bb.shrink(0.001, 0.001, 0.001);
    if (level->containsFireTile(&shrunk)) {
        burn(1);
        if (!water) {
            onFire++;
            if (onFire == 0) setOnFire(8);
        }
    } else {
        if (onFire <= 0) {
            onFire = -flameTime;
        }
    }

    if (water && onFire > 0) {
        playSound(eSoundType_RANDOM_FIZZ, 0.7f,
                  1.6f + (random->nextFloat() - random->nextFloat()) * 0.4f);
        onFire = -flameTime;
    }
}

void Entity::checkInsideTiles() {
    int x0 = Mth::floor(bb.x0 + 0.001);
    int y0 = Mth::floor(bb.y0 + 0.001);
    int z0 = Mth::floor(bb.z0 + 0.001);
    int x1 = Mth::floor(bb.x1 - 0.001);
    int y1 = Mth::floor(bb.y1 - 0.001);
    int z1 = Mth::floor(bb.z1 - 0.001);

    if (level->hasChunksAt(x0, y0, z0, x1, y1, z1)) {
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++) {
                    int t = level->getTile(x, y, z);
                    if (t > 0) {
                        Tile::tiles[t]->entityInside(level, x, y, z,
                                                     shared_from_this());
                    }
                }
    }
}

void Entity::playStepSound(int xt, int yt, int zt, int t) {
    const Tile::SoundType* soundType = Tile::tiles[t]->soundType;

    if (GetType() == eTYPE_PLAYER) {
        // should we turn off step sounds?
        unsigned int uiAnimOverrideBitmask =
            getAnimOverrideBitmask();  // this is masked for custom anim off,
                                       // and force anim

        if ((uiAnimOverrideBitmask & (1 << HumanoidModel::eAnim_NoLegAnim)) !=
            0) {
            return;
        }
    }
    if (level->getTile(xt, yt + 1, zt) == Tile::topSnow_Id) {
        soundType = Tile::topSnow->soundType;
        playSound(soundType->getStepSound(), soundType->getVolume() * 0.15f,
                  soundType->getPitch());
    } else if (!Tile::tiles[t]->material->isLiquid()) {
        playSound(soundType->getStepSound(), soundType->getVolume() * 0.15f,
                  soundType->getPitch());
    }
}

void Entity::playSound(int iSound, float volume, float pitch) {
    level->playEntitySound(shared_from_this(), iSound, volume, pitch);
}

bool Entity::makeStepSound() { return true; }

void Entity::checkFallDamage(double ya, bool onGround) {
    if (onGround) {
        if (fallDistance > 0) {
            causeFallDamage(fallDistance);
            fallDistance = 0;
        }
    } else {
        if (ya < 0) fallDistance -= (float)ya;
    }
}

AABB* Entity::getCollideBox() { return nullptr; }

void Entity::burn(int dmg) {
    if (!fireImmune) {
        hurt(DamageSource::inFire, dmg);
    }
}

bool Entity::isFireImmune() { return fireImmune; }

void Entity::causeFallDamage(float distance) {
    if (rider.lock() != nullptr) rider.lock()->causeFallDamage(distance);
}

bool Entity::isInWaterOrRain() {
    return wasInWater ||
           (level->isRainingAt(Mth::floor(x), Mth::floor(y), Mth::floor(z)) ||
            level->isRainingAt(Mth::floor(x), Mth::floor(y + bbHeight),
                               Mth::floor(z)));
}

bool Entity::isInWater() { return wasInWater; }

bool Entity::updateInWaterState() {
    AABB shrunk = bb.grow(0, -0.4, 0).shrink(0.001, 0.001, 0.001);
    if (level->checkAndHandleWater(&shrunk, Material::water,
                                   shared_from_this())) {
        if (!wasInWater && !firstTick && canCreateParticles()) {
            float speed =
                Mth::sqrt(xd * xd * 0.2f + yd * yd + zd * zd * 0.2f) * 0.2f;
            if (speed > 1) speed = 1;
            playSound(eSoundType_RANDOM_SPLASH, speed,
                      1 + (random->nextFloat() - random->nextFloat()) * 0.4f);
            float yt = (float)Mth::floor(bb.y0);
            for (int i = 0; i < 1 + bbWidth * 20; i++) {
                float xo = (random->nextFloat() * 2 - 1) * bbWidth;
                float zo = (random->nextFloat() * 2 - 1) * bbWidth;
                level->addParticle(eParticleType_bubble, x + xo, yt + 1, z + zo,
                                   xd, yd - random->nextFloat() * 0.2f, zd);
            }
            for (int i = 0; i < 1 + bbWidth * 20; i++) {
                float xo = (random->nextFloat() * 2 - 1) * bbWidth;
                float zo = (random->nextFloat() * 2 - 1) * bbWidth;
                level->addParticle(eParticleType_splash, x + xo, yt + 1, z + zo,
                                   xd, yd, zd);
            }
        }
        fallDistance = 0;
        wasInWater = true;
        onFire = 0;
    } else {
        wasInWater = false;
    }
    return wasInWater;
}

bool Entity::isUnderLiquid(Material* material) {
    double yp = y + getHeadHeight();
    int xt = Mth::floor(x);
    int yt = Mth::floor(
        yp);  // 4J - this used to be a nested pair of floors for some reason
    int zt = Mth::floor(z);
    int t = level->getTile(xt, yt, zt);
    if (t != 0 && Tile::tiles[t]->material == material) {
        float hh = LiquidTile::getHeight(level->getData(xt, yt, zt)) - 1 / 9.0f;
        float h = yt + 1 - hh;
        return yp < h;
    }
    return false;
}

float Entity::getHeadHeight() { return 0; }

bool Entity::isInLava() {
    AABB mat_bounds = bb.grow(-0.1, -0.4, -0.1);
    return level->containsMaterial(&mat_bounds, Material::lava);
}

void Entity::moveRelative(float xa, float za, float speed) {
    float dist = xa * xa + za * za;
    if (dist < 0.01f * 0.01f) return;

    dist = sqrt(dist);
    if (dist < 1) dist = 1;
    dist = speed / dist;
    xa *= dist;
    za *= dist;

    float sinVar = sinf(yRot * std::numbers::pi / 180);
    float cosVar = cosf(yRot * std::numbers::pi / 180);

    xd += xa * cosVar - za * sinVar;
    zd += za * cosVar + xa * sinVar;
}

// 4J - change brought forward from 1.8.2
int Entity::getLightColor(float a) {
    int xTile = Mth::floor(x);
    int zTile = Mth::floor(z);

    if (level->hasChunkAt(xTile, 0, zTile)) {
        double hh = (bb.y1 - bb.y0) * 0.66;
        int yTile = Mth::floor(y - heightOffset + hh);
        return level->getLightColor(xTile, yTile, zTile, 0);
    }
    return 0;
}

// 4J - changes brought forward from 1.8.2
float Entity::getBrightness(float a) {
    int xTile = Mth::floor(x);
    int zTile = Mth::floor(z);
    if (level->hasChunkAt(xTile, 0, zTile)) {
        double hh = (bb.y1 - bb.y0) * 0.66;
        int yTile = Mth::floor(y - heightOffset + hh);
        return level->getBrightness(xTile, yTile, zTile);
    }
    return 0;
}

void Entity::setLevel(Level* level) { this->level = level; }

void Entity::absMoveTo(double x, double y, double z, float yRot, float xRot) {
    xo = this->x = x;
    yo = this->y = y;
    zo = this->z = z;
    yRotO = this->yRot = yRot;
    xRotO = this->xRot = xRot;
    ySlideOffset = 0;

    double yRotDiff = yRotO - yRot;
    if (yRotDiff < -180) yRotO += 360;
    if (yRotDiff >= 180) yRotO -= 360;
    setPos(this->x, this->y, this->z);
    setRot(yRot, xRot);
}

void Entity::moveTo(double x, double y, double z, float yRot, float xRot) {
    xOld = xo = this->x = x;
    yOld = yo = this->y = y + heightOffset;
    zOld = zo = this->z = z;
    this->yRot = yRot;
    this->xRot = xRot;
    setPos(this->x, this->y, this->z);
}

float Entity::distanceTo(std::shared_ptr<Entity> e) {
    float xd = (float)(x - e->x);
    float yd = (float)(y - e->y);
    float zd = (float)(z - e->z);
    return sqrt(xd * xd + yd * yd + zd * zd);
}

double Entity::distanceToSqr(double x2, double y2, double z2) {
    double xd = (x - x2);
    double yd = (y - y2);
    double zd = (z - z2);
    return xd * xd + yd * yd + zd * zd;
}

double Entity::distanceTo(double x2, double y2, double z2) {
    double xd = (x - x2);
    double yd = (y - y2);
    double zd = (z - z2);
    return sqrt(xd * xd + yd * yd + zd * zd);
}

double Entity::distanceToSqr(std::shared_ptr<Entity> e) {
    double xd = x - e->x;
    double yd = y - e->y;
    double zd = z - e->z;
    return xd * xd + yd * yd + zd * zd;
}

void Entity::playerTouch(std::shared_ptr<Player> player) {}

void Entity::push(std::shared_ptr<Entity> e) {
    if (e->rider.lock().get() == this || e->riding.get() == this) return;

    const double dx = e->x - x;
    const double dz = e->z - z;

    constexpr double min_displacement = 0.1;
    const double max_displacement = std::max(std::fabs(dx), std::fabs(dz));

    if (max_displacement >= min_displacement) {
        constexpr double dampening = 0.05;
        const double dist = std::sqrt(dx * dx + dz * dz);

        const double nx = dx / dist;
        const double nz = dz / dist;

        const double force =
            std::min(1.0 / dist, 1.0) * dampening * (1.0 - pushthrough);

        push(-nx * force, 0.0, -nz * force);
        e->push(nx * force, 0, nz * force);
    }
}

void Entity::push(double xa, double ya, double za) {
    move(xa, ya, za);
    hasImpulse = true;
}

void Entity::markHurt() { hurtMarked = true; }

bool Entity::hurt(DamageSource* source, float damage) {
    if (isInvulnerable()) return false;
    markHurt();
    return false;
}

bool Entity::intersects(double x0, double y0, double z0, double x1, double y1,
                        double z1) {
    return bb.intersects(x0, y0, z0, x1, y1, z1);
}

bool Entity::isPickable() { return false; }

bool Entity::isPushable() { return false; }

bool Entity::isShootable() { return false; }

void Entity::awardKillScore(std::shared_ptr<Entity> victim, int score) {}

bool Entity::shouldRender(Vec3* c) {
    double xd = x - c->x;
    double yd = y - c->y;
    double zd = z - c->z;
    double distance = xd * xd + yd * yd + zd * zd;
    return shouldRenderAtSqrDistance(distance);
}

bool Entity::shouldRenderAtSqrDistance(double distance) {
    double size = bb.getSize();
    size *= 64.0f * viewScale;
    return distance < size * size;
}

bool Entity::isCreativeModeAllowed() { return false; }

bool Entity::saveAsMount(CompoundTag* entityTag) {
    std::string id = getEncodeId();
    if (removed || id.empty()) {
        return false;
    }
    // TODO Is this fine to be casting to a non-const char pointer?
    entityTag->putString("id", id);
    saveWithoutId(entityTag);
    return true;
}

bool Entity::save(CompoundTag* entityTag) {
    std::string id = getEncodeId();
    if (removed || id.empty() || (rider.lock() != nullptr)) {
        return false;
    }
    // TODO Is this fine to be casting to a non-const char pointer?
    entityTag->putString("id", id);
    saveWithoutId(entityTag);
    return true;
}

void Entity::saveWithoutId(CompoundTag* entityTag) {
    entityTag->put("Pos", newDoubleList(3, x, y + ySlideOffset, z));
    entityTag->put("Motion", newDoubleList(3, xd, yd, zd));
    entityTag->put("Rotation", newFloatList(2, yRot, xRot));

    entityTag->putFloat("FallDistance", fallDistance);
    entityTag->putShort("Fire", (short)onFire);
    entityTag->putShort("Air", (short)getAirSupply());
    entityTag->putBoolean("OnGround", onGround);
    entityTag->putInt("Dimension", dimension);
    entityTag->putBoolean("Invulnerable", invulnerable);
    entityTag->putInt("PortalCooldown", changingDimensionDelay);

    entityTag->putString("UUID", uuid);

    addAdditonalSaveData(entityTag);

    if (riding != nullptr) {
        CompoundTag* ridingTag = new CompoundTag(RIDING_TAG);
        if (riding->saveAsMount(ridingTag)) {
            entityTag->put("Riding", ridingTag);
        }
    }
}

void Entity::load(CompoundTag* tag) {
    ListTag<DoubleTag>* pos = (ListTag<DoubleTag>*)tag->getList("Pos");
    ListTag<DoubleTag>* motion = (ListTag<DoubleTag>*)tag->getList("Motion");
    ListTag<FloatTag>* rotation = (ListTag<FloatTag>*)tag->getList("Rotation");

    xd = motion->get(0)->data;
    yd = motion->get(1)->data;
    zd = motion->get(2)->data;

    if (abs(xd) > 10.0) {
        xd = 0;
    }
    if (abs(yd) > 10.0) {
        yd = 0;
    }
    if (abs(zd) > 10.0) {
        zd = 0;
    }

    xo = xOld = x = pos->get(0)->data;
    yo = yOld = y = pos->get(1)->data;
    zo = zOld = z = pos->get(2)->data;

    yRotO = yRot = rotation->get(0)->data;
    xRotO = xRot = rotation->get(1)->data;

    fallDistance = tag->getFloat("FallDistance");
    onFire = tag->getShort("Fire");
    setAirSupply(tag->getShort("Air"));
    onGround = tag->getBoolean("OnGround");
    dimension = tag->getInt("Dimension");
    invulnerable = tag->getBoolean("Invulnerable");
    changingDimensionDelay = tag->getInt("PortalCooldown");

    if (tag->contains("UUID")) {
        uuid = tag->getString("UUID");
    }

    setPos(x, y, z);
    setRot(yRot, xRot);

    readAdditionalSaveData(tag);

    // set position again because bb size may have changed
    if (repositionEntityAfterLoad()) setPos(x, y, z);
}

bool Entity::repositionEntityAfterLoad() { return true; }

const std::string Entity::getEncodeId() {
    return EntityIO::getEncodeId(shared_from_this());
}

/**
 * Called after load() has finished and the entity has been added to the
 * world
 */
void Entity::onLoadedFromSave() {}

ListTag<DoubleTag>* Entity::newDoubleList(unsigned int number,
                                          double firstValue, ...) {
    ListTag<DoubleTag>* res = new ListTag<DoubleTag>();

    // Add the first parameter to the ListTag
    res->add(new DoubleTag("", firstValue));

    va_list vl;
    va_start(vl, firstValue);

    double val;

    for (unsigned int i = 1; i < number; i++) {
        val = va_arg(vl, double);
        res->add(new DoubleTag("", val));
    }

    va_end(vl);

    return res;
}

ListTag<FloatTag>* Entity::newFloatList(unsigned int number, float firstValue,
                                        float secondValue) {
    ListTag<FloatTag>* res = new ListTag<FloatTag>();

    // Add the first parameter to the ListTag
    res->add(new FloatTag("", firstValue));

    // TODO - 4J Stu For some reason the va_list wasn't working correctly here
    // We only make a list of two floats so just overriding and not using
    // va_list
    res->add(new FloatTag("", secondValue));

    /*
    va_list vl;
    va_start(vl,firstValue);

    float val;

    for (unsigned int i = 1; i < number; i++)
    {
    val = va_arg(vl,float);
    res->add(new FloatTag(val));
    }
    va_end(vl);
    */
    return res;
}

float Entity::getShadowHeightOffs() { return bbHeight / 2; }

std::shared_ptr<ItemEntity> Entity::spawnAtLocation(int resource, int count) {
    return spawnAtLocation(resource, count, 0);
}

std::shared_ptr<ItemEntity> Entity::spawnAtLocation(int resource, int count,
                                                    float yOffs) {
    return spawnAtLocation(std::make_shared<ItemInstance>(resource, count, 0),
                           yOffs);
}

std::shared_ptr<ItemEntity> Entity::spawnAtLocation(
    std::shared_ptr<ItemInstance> itemInstance, float yOffs) {
    if (itemInstance->count == 0) {
        return nullptr;
    }
    std::shared_ptr<ItemEntity> ie = std::shared_ptr<ItemEntity>(
        new ItemEntity(level, x, y + yOffs, z, itemInstance));
    ie->throwTime = 10;
    level->addEntity(ie);
    return ie;
}

bool Entity::isAlive() { return !removed; }

bool Entity::isInWall() {
    for (int i = 0; i < 8; i++) {
        float xo = ((i >> 0) % 2 - 0.5f) * bbWidth * 0.8f;
        float yo = ((i >> 1) % 2 - 0.5f) * 0.1f;
        float zo = ((i >> 2) % 2 - 0.5f) * bbWidth * 0.8f;
        int xt = Mth::floor(x + xo);
        int yt = Mth::floor(y + getHeadHeight() + yo);
        int zt = Mth::floor(z + zo);
        if (level->isSolidBlockingTile(xt, yt, zt)) {
            return true;
        }
    }
    return false;
}

bool Entity::interact(std::shared_ptr<Player> player) { return false; }

AABB* Entity::getCollideAgainstBox(std::shared_ptr<Entity> entity) {
    return nullptr;
}

void Entity::rideTick() {
    if (riding->removed) {
        riding = nullptr;
        return;
    }
    xd = yd = zd = 0;
    tick();

    if (riding == nullptr) return;

    // Sets riders old&new position to it's mount's old&new position (plus the
    // ride y-seperatation).
    riding->positionRider();

    yRideRotA += (riding->yRot - riding->yRotO);
    xRideRotA += (riding->xRot - riding->xRotO);

    // Wrap rotation angles.
    while (yRideRotA >= 180) yRideRotA -= 360;
    while (yRideRotA < -180) yRideRotA += 360;
    while (xRideRotA >= 180) xRideRotA -= 360;
    while (xRideRotA < -180) xRideRotA += 360;

    double yra = yRideRotA * 0.5;
    double xra = xRideRotA * 0.5;

    // Cap rotation speed.
    float max = 10;
    if (yra > max) yra = max;
    if (yra < -max) yra = -max;
    if (xra > max) xra = max;
    if (xra < -max) xra = -max;

    yRideRotA -= yra;
    xRideRotA -= xra;

    // jeb: This caused the crosshair to "drift" while riding horses. For now
    // I've just disabled it,
    //      because I can't figure out what it's needed for. Riding boats and
    //      minecarts seem unaffected...
    // yRot += yra;
    // xRot += xra;
}

void Entity::positionRider() {
    std::shared_ptr<Entity> lockedRider = rider.lock();
    if (lockedRider == nullptr) {
        return;
    }
    lockedRider->setPos(x, y + getRideHeight() + lockedRider->getRidingHeight(),
                        z);
}

double Entity::getRidingHeight() { return heightOffset; }

double Entity::getRideHeight() { return bbHeight * .75; }

void Entity::ride(std::shared_ptr<Entity> e) {
    xRideRotA = 0;
    yRideRotA = 0;

    if (e == nullptr) {
        if (riding != nullptr) {
            // 4J Stu - Position should already be updated before the
            // SetEntityLinkPacket comes in
            if (!level->isClientSide)
                moveTo(riding->x, riding->bb.y0 + riding->bbHeight, riding->z,
                       yRot, xRot);
            riding->rider = std::weak_ptr<Entity>();
        }
        riding = nullptr;
        return;
    }
    if (riding != nullptr) {
        riding->rider = std::weak_ptr<Entity>();
    }
    riding = e;
    e->rider = shared_from_this();
}

void Entity::lerpTo(double x, double y, double z, float yRot, float xRot,
                    int steps) {
    setPos(x, y, z);
    setRot(yRot, xRot);

    // 4J - don't know what this special y collision is specifically for, but
    // its definitely bad news for arrows as they are actually Meant to
    // intersect the geometry they land in slightly.
    if (GetType() != eTYPE_ARROW) {
        AABB shrunk = bb.shrink(1 / 32.0, 0.0, 1 / 32.0);
        std::vector<AABB>* collisions =
            level->getCubes(shared_from_this(), &shrunk);
        if (!collisions->empty()) {
            double yTop = 0;
            auto itEnd = collisions->end();
            for (auto it = collisions->begin(); it != itEnd; it++) {
                if (it->y1 > yTop) yTop = it->y1;
            }

            y += yTop - bb.y0;
            setPos(x, y, z);
        }
    }
}

float Entity::getPickRadius() { return 0.1f; }

std::optional<Vec3> Entity::getLookAngle() { return std::nullopt; }

void Entity::handleInsidePortal() {
    if (changingDimensionDelay > 0) {
        changingDimensionDelay = getDimensionChangingDelay();
        return;
    }

    double xd = xo - x;
    double zd = zo - z;

    if (!level->isClientSide && !isInsidePortal) {
        portalEntranceDir = Direction::getDirection(xd, zd);
    }

    isInsidePortal = true;
}

int Entity::getDimensionChangingDelay() {
    return SharedConstants::TICKS_PER_SECOND * 45;
}

void Entity::lerpMotion(double xd, double yd, double zd) {
    this->xd = xd;
    this->yd = yd;
    this->zd = zd;
}

void Entity::handleEntityEvent(uint8_t eventId) {}

void Entity::animateHurt() {}

std::vector<std::shared_ptr<ItemInstance>>
Entity::getEquipmentSlots()  // ItemInstance[]
{
    return std::vector<std::shared_ptr<ItemInstance>>();  // Default ctor
                                                          // creates nullptr
                                                          // internal array
}

// 4J Stu - Brought forward change from 1.3 to fix #64688 - Customer
// Encountered: TU7: Content: Art: Aura of enchanted item is not displayed for
// other players in online game
void Entity::setEquippedSlot(int slot, std::shared_ptr<ItemInstance> item) {}

bool Entity::isOnFire() {
    return !fireImmune && (onFire > 0 || getSharedFlag(FLAG_ONFIRE));
}

bool Entity::isRiding() { return riding != nullptr; }

bool Entity::isSneaking() { return getSharedFlag(FLAG_SNEAKING); }

void Entity::setSneaking(bool value) { setSharedFlag(FLAG_SNEAKING, value); }

bool Entity::isIdle() { return getSharedFlag(FLAG_IDLEANIM); }

void Entity::setIsIdle(bool value) { setSharedFlag(FLAG_IDLEANIM, value); }

bool Entity::isSprinting() { return getSharedFlag(FLAG_SPRINTING); }

void Entity::setSprinting(bool value) { setSharedFlag(FLAG_SPRINTING, value); }

bool Entity::isInvisible() { return getSharedFlag(FLAG_INVISIBLE); }

bool Entity::isInvisibleTo(std::shared_ptr<Player> plr) {
    return isInvisible();
}

void Entity::setInvisible(bool value) { setSharedFlag(FLAG_INVISIBLE, value); }

bool Entity::isWeakened() { return getSharedFlag(FLAG_EFFECT_WEAKENED); }

void Entity::setWeakened(bool value) {
    setSharedFlag(FLAG_EFFECT_WEAKENED, value);
}

bool Entity::isUsingItemFlag() { return getSharedFlag(FLAG_USING_ITEM); }

void Entity::setUsingItemFlag(bool value) {
    setSharedFlag(FLAG_USING_ITEM, value);
}

bool Entity::getSharedFlag(int flag) {
    if (entityData) {
        return (entityData->getByte(DATA_SHARED_FLAGS_ID) & (1 << flag)) != 0;
    } else {
        return false;
    }
}

void Entity::setSharedFlag(int flag, bool value) {
    if (entityData) {
        uint8_t currentValue = entityData->getByte(DATA_SHARED_FLAGS_ID);
        if (value) {
            entityData->set(DATA_SHARED_FLAGS_ID,
                            (uint8_t)(currentValue | (1 << flag)));
        } else {
            entityData->set(DATA_SHARED_FLAGS_ID,
                            (uint8_t)(currentValue & ~(1 << flag)));
        }
    }
}

// 4J Stu - Brought forward from 1.2.3 to fix 38654 - Gameplay: Player will take
// damage when air bubbles are present if resuming game from load/autosave
// underwater.
int Entity::getAirSupply() { return entityData->getShort(DATA_AIR_SUPPLY_ID); }

// 4J Stu - Brought forward from 1.2.3 to fix 38654 - Gameplay: Player will take
// damage when air bubbles are present if resuming game from load/autosave
// underwater.
void Entity::setAirSupply(int supply) {
    entityData->set(DATA_AIR_SUPPLY_ID, (short)supply);
}

void Entity::thunderHit(const LightningBolt* lightningBolt) {
    burn(5);
    onFire++;
    if (onFire == 0) setOnFire(8);
}

void Entity::killed(std::shared_ptr<LivingEntity> mob) {}

bool Entity::checkInTile(double x, double y, double z) {
    int xTile = Mth::floor(x);
    int yTile = Mth::floor(y);
    int zTile = Mth::floor(z);

    double xd = x - (xTile);
    double yd = y - (yTile);
    double zd = z - (zTile);

    auto* cubes = level->getTileCubes(&bb);
    if ((cubes && !cubes->empty()) ||
        level->isFullAABBTile(xTile, yTile, zTile)) {
        bool west = !level->isFullAABBTile(xTile - 1, yTile, zTile);
        bool east = !level->isFullAABBTile(xTile + 1, yTile, zTile);
        bool down = !level->isFullAABBTile(xTile, yTile - 1, zTile);
        bool up = !level->isFullAABBTile(xTile, yTile + 1, zTile);
        bool north = !level->isFullAABBTile(xTile, yTile, zTile - 1);
        bool south = !level->isFullAABBTile(xTile, yTile, zTile + 1);

        int dir = 3;
        double closest = 9999;
        if (west && xd < closest) {
            closest = xd;
            dir = 0;
        }
        if (east && 1 - xd < closest) {
            closest = 1 - xd;
            dir = 1;
        }
        if (up && 1 - yd < closest) {
            closest = 1 - yd;
            dir = 3;
        }
        if (north && zd < closest) {
            closest = zd;
            dir = 4;
        }
        if (south && 1 - zd < closest) {
            closest = 1 - zd;
            dir = 5;
        }

        float speed = random->nextFloat() * 0.2f + 0.1f;
        if (dir == 0) this->xd = -speed;
        if (dir == 1) this->xd = +speed;

        if (dir == 2) this->yd = -speed;
        if (dir == 3) this->yd = +speed;

        if (dir == 4) this->zd = -speed;
        if (dir == 5) this->zd = +speed;

        return true;
    }
    return false;
}

void Entity::makeStuckInWeb() {
    isStuckInWeb = true;
    fallDistance = 0;
}

std::string Entity::getAName() {
#ifdef _DEBUG
    std::string id = EntityIO::getEncodeId(shared_from_this());
    if (id.empty()) id = "generic";
    return "entity." + id + toWString(entityId);
#else
    return "";
#endif
}

std::vector<std::shared_ptr<Entity>>* Entity::getSubEntities() {
    return nullptr;
}

bool Entity::is(std::shared_ptr<Entity> other) {
    return shared_from_this() == other;
}

float Entity::getYHeadRot() { return 0; }

void Entity::setYHeadRot(float yHeadRot) {}

bool Entity::isAttackable() { return true; }

bool Entity::skipAttackInteraction(std::shared_ptr<Entity> source) {
    return false;
}

bool Entity::isInvulnerable() { return invulnerable; }

void Entity::copyPosition(std::shared_ptr<Entity> target) {
    moveTo(target->x, target->y, target->z, target->yRot, target->xRot);
}

void Entity::restoreFrom(std::shared_ptr<Entity> oldEntity, bool teleporting) {
    CompoundTag* tag = new CompoundTag();
    oldEntity->saveWithoutId(tag);
    load(tag);
    delete tag;
    changingDimensionDelay = oldEntity->changingDimensionDelay;
    portalEntranceDir = oldEntity->portalEntranceDir;
}

void Entity::changeDimension(int i) {
    if (level->isClientSide || removed) return;

    MinecraftServer* server = MinecraftServer::getInstance();
    int lastDimension = dimension;
    ServerLevel* oldLevel = server->getLevel(lastDimension);
    ServerLevel* newLevel = server->getLevel(i);

    if (lastDimension == 1 && i == 1) {
        newLevel = server->getLevel(0);
    }

    // 4J: Restrictions on what can go through
    {
        // 4J: Some things should just be destroyed when they hit a portal
        if (instanceof (eTYPE_FALLINGTILE)) {
            removed = true;
            return;
        }

        // 4J: Check server level entity limit (arrows, item entities,
        // experience orbs, etc)
        if (newLevel->atEntityLimit(shared_from_this())) return;

        // 4J: Check level limit on living entities, minecarts and boats
        if (! instanceof
            (eTYPE_PLAYER) &&
                !newLevel->canCreateMore(GetType(), Level::eSpawnType_Portal))
            return;
    }

    // 4J: Definitely sending, set dimension now
    dimension = newLevel->dimension->id;

    level->removeEntity(shared_from_this());
    removed = false;

    server->getPlayers()->repositionAcrossDimension(
        shared_from_this(), lastDimension, oldLevel, newLevel);
    std::shared_ptr<Entity> newEntity = EntityIO::newEntity(
        EntityIO::getEncodeId(shared_from_this()), newLevel);

    if (newEntity != nullptr) {
        newEntity->restoreFrom(shared_from_this(), true);

        if (lastDimension == 1 && i == 1) {
            Pos* spawnPos = newLevel->getSharedSpawnPos();
            spawnPos->y = level->getTopSolidBlock(spawnPos->x, spawnPos->z);
            newEntity->moveTo(spawnPos->x, spawnPos->y, spawnPos->z,
                              newEntity->yRot, newEntity->xRot);
            delete spawnPos;
        }

        newLevel->addEntity(newEntity);
    }

    removed = true;

    oldLevel->resetEmptyTime();
    newLevel->resetEmptyTime();
}

float Entity::getTileExplosionResistance(Explosion* explosion, Level* level,
                                         int x, int y, int z, Tile* tile) {
    return tile->getExplosionResistance(shared_from_this());
}

bool Entity::shouldTileExplode(Explosion* explosion, Level* level, int x, int y,
                               int z, int id, float power) {
    return true;
}

int Entity::getMaxFallDistance() { return 3; }

int Entity::getPortalEntranceDir() { return portalEntranceDir; }

bool Entity::isIgnoringTileTriggers() { return false; }

bool Entity::displayFireAnimation() { return isOnFire(); }

void Entity::setUUID(const std::string& UUID) { uuid = UUID; }

std::string Entity::getUUID() { return uuid; }

bool Entity::isPushedByWater() { return true; }

std::string Entity::getDisplayName() { return getAName(); }

// 4J: Added to retrieve name that should be sent in ChatPackets (important on
// Xbox One for players)
std::string Entity::getNetworkName() { return getDisplayName(); }

void Entity::setAnimOverrideBitmask(unsigned int uiBitmask) {
    m_uiAnimOverrideBitmask = uiBitmask;
    Log::info("!!! Setting anim override bitmask to %d\n", uiBitmask);
}
unsigned int Entity::getAnimOverrideBitmask() {
    if (gameServices().getGameSettings(eGameSetting_CustomSkinAnim) == 0) {
        // We have a force animation for some skins (claptrap)
        // 4J-PB - treat all the eAnim_Disable flags as a force anim
        unsigned int uiIgnoreUserCustomSkinAnimSettingMask =
            (1 << HumanoidModel::eAnim_ForceAnim) |
            (1 << HumanoidModel::eAnim_DisableRenderArm0) |
            (1 << HumanoidModel::eAnim_DisableRenderArm1) |
            (1 << HumanoidModel::eAnim_DisableRenderTorso) |
            (1 << HumanoidModel::eAnim_DisableRenderLeg0) |
            (1 << HumanoidModel::eAnim_DisableRenderLeg1) |
            (1 << HumanoidModel::eAnim_DisableRenderHair);

        if ((m_uiAnimOverrideBitmask &
             HumanoidModel::m_staticBitmaskIgnorePlayerCustomAnimSetting) !=
            0) {
            return m_uiAnimOverrideBitmask;
        }
        return 0;
    }

    return m_uiAnimOverrideBitmask;
}
