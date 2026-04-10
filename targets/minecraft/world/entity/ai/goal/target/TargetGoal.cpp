#include "TargetGoal.h"

#include <string>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/OwnableEntity.h"
#include "minecraft/world/entity/PathfinderMob.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/navigation/PathNavigation.h"
#include "minecraft/world/entity/ai/sensing/Sensing.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/pathfinder/Node.h"
#include "minecraft/world/level/pathfinder/Path.h"

void TargetGoal::_init(PathfinderMob* mob, bool mustSee, bool mustReach) {
    reachCache = EmptyReachCache;
    reachCacheTime = 0;
    unseenTicks = 0;

    this->mob = mob;
    this->mustSee = mustSee;
    this->mustReach = mustReach;
}

TargetGoal::TargetGoal(PathfinderMob* mob, bool mustSee) {
    _init(mob, mustSee, false);
}

TargetGoal::TargetGoal(PathfinderMob* mob, bool mustSee, bool mustReach) {
    _init(mob, mustSee, mustReach);
}

bool TargetGoal::canContinueToUse() {
    std::shared_ptr<LivingEntity> target = mob->getTarget();
    if (target == nullptr) return false;
    if (!target->isAlive()) return false;

    double within = getFollowDistance();
    if (mob->distanceToSqr(target) > within * within) return false;
    if (mustSee) {
        if (mob->getSensing()->canSee(target)) {
            unseenTicks = 0;
        } else {
            if (++unseenTicks > UnseenMemoryTicks) return false;
        }
    }
    return true;
}

double TargetGoal::getFollowDistance() {
    AttributeInstance* followRange =
        mob->getAttribute(SharedMonsterAttributes::FOLLOW_RANGE);
    return followRange == nullptr ? 16 : followRange->getValue();
}

void TargetGoal::start() {
    reachCache = EmptyReachCache;
    reachCacheTime = 0;
    unseenTicks = 0;
}

void TargetGoal::stop() { mob->setTarget(nullptr); }

bool TargetGoal::canAttack(std::shared_ptr<LivingEntity> target,
                           bool allowInvulnerable) {
    if (target == nullptr) return false;
    if (target == mob->shared_from_this()) return false;
    if (!target->isAlive()) return false;
    if (!mob->canAttackType(target->GetType())) return false;

    OwnableEntity* ownableMob = dynamic_cast<OwnableEntity*>(mob);
    if (ownableMob != nullptr && !ownableMob->getOwnerUUID().empty()) {
        std::shared_ptr<OwnableEntity> ownableTarget =
            std::dynamic_pointer_cast<OwnableEntity>(target);
        if (ownableTarget != nullptr &&
            ownableMob->getOwnerUUID().compare(ownableTarget->getOwnerUUID()) ==
                0) {
            // We're attacking something owned by the same person...
            return false;
        }

        if (target == ownableMob->getOwner()) {
            // We're attacking our owner
            return false;
        }
    } else if (target->instanceof(eTYPE_PLAYER)) {
        if (!allowInvulnerable &&
            (std::dynamic_pointer_cast<Player>(target))->abilities.invulnerable)
            return false;
    }

    if (!mob->isWithinRestriction(Mth::floor(target->x), Mth::floor(target->y),
                                  Mth::floor(target->z)))
        return false;

    if (mustSee && !mob->getSensing()->canSee(target)) return false;

    if (mustReach) {
        if (--reachCacheTime <= 0) reachCache = EmptyReachCache;
        if (reachCache == EmptyReachCache)
            reachCache = canReach(target) ? CanReachCache : CantReachCache;
        if (reachCache == CantReachCache) return false;
    }

    return true;
}

bool TargetGoal::canReach(std::shared_ptr<LivingEntity> target) {
    reachCacheTime = 10 + mob->getRandom()->nextInt(5);
    Path* path = mob->getNavigation()->createPath(target);
    if (path == nullptr) return false;
    Node* last = path->last();
    if (last == nullptr) {
        delete path;
        return false;
    }
    int xx = last->x - Mth::floor(target->x);
    int zz = last->z - Mth::floor(target->z);
    delete path;
    return xx * xx + zz * zz <= 1.5 * 1.5;
}
