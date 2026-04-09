#include "NearestAttackableTargetGoal.h"

#include <algorithm>
#include <vector>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntitySelector.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/PathfinderMob.h"
#include "minecraft/world/entity/ai/goal/target/TargetGoal.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/phys/AABB.h"

SubselectEntitySelector::SubselectEntitySelector(
    NearestAttackableTargetGoal* parent, EntitySelector* subselector) {
    m_parent = parent;
    m_subselector = subselector;
}

SubselectEntitySelector::~SubselectEntitySelector() { delete m_subselector; }

bool SubselectEntitySelector::matches(std::shared_ptr<Entity> entity) const {
    if (!entity->instanceof (eTYPE_LIVINGENTITY)) return false;
    if (m_subselector != nullptr && !m_subselector->matches(entity))
        return false;
    return m_parent->canAttack(std::dynamic_pointer_cast<LivingEntity>(entity),
                               false);
}

NearestAttackableTargetGoal::DistComp::DistComp(Entity* source) {
    this->source = source;
}

bool NearestAttackableTargetGoal::DistComp::operator()(
    std::shared_ptr<Entity> e1, std::shared_ptr<Entity> e2) {
    // Should return true if e1 comes before e2 in the sorted list
    double distSqr1 = source->distanceToSqr(e1);
    double distSqr2 = source->distanceToSqr(e2);
    if (distSqr1 < distSqr2) return true;
    if (distSqr1 > distSqr2) return false;
    return true;
}

NearestAttackableTargetGoal::NearestAttackableTargetGoal(
    PathfinderMob* mob, const std::type_info& targetType, int randomInterval,
    bool mustSee, bool mustReach /*= false*/,
    EntitySelector* entitySelector /* =nullptr */)
    : TargetGoal(mob, mustSee, mustReach), targetType(targetType) {
    this->randomInterval = randomInterval;
    this->distComp = new DistComp(mob);
    setRequiredControlFlags(TargetGoal::TargetFlag);

    this->selector = new SubselectEntitySelector(this, entitySelector);
}

NearestAttackableTargetGoal::~NearestAttackableTargetGoal() {
    delete distComp;
    delete selector;
}

bool NearestAttackableTargetGoal::canUse() {
    if (randomInterval > 0 && mob->getRandom()->nextInt(randomInterval) != 0)
        return false;
    double within = getFollowDistance();

    AABB mob_bb = mob->bb.grow(within, 4, within);
    std::vector<std::shared_ptr<Entity> >* entities =
        mob->level->getEntitiesOfClass(targetType, &mob_bb, selector);

    bool result = false;
    if (entities != nullptr && !entities->empty()) {
        std::sort(entities->begin(), entities->end(), *distComp);
        target = std::weak_ptr<LivingEntity>(
            std::dynamic_pointer_cast<LivingEntity>(entities->at(0)));
        result = true;
    }

    delete entities;
    return result;
}

void NearestAttackableTargetGoal::start() {
    mob->setTarget(target.lock());
    TargetGoal::start();
}
