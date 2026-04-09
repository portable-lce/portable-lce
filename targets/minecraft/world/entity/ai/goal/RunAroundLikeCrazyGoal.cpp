#include "RunAroundLikeCrazyGoal.h"

#include <memory>
#include <optional>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityEvent.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/PathfinderMob.h"
#include "minecraft/world/entity/ai/control/Control.h"
#include "minecraft/world/entity/ai/navigation/PathNavigation.h"
#include "minecraft/world/entity/ai/util/RandomPos.h"
#include "minecraft/world/entity/animal/EntityHorse.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/phys/Vec3.h"

RunAroundLikeCrazyGoal::RunAroundLikeCrazyGoal(EntityHorse* mob,
                                               double speedModifier) {
    horse = mob;
    this->speedModifier = speedModifier;
    setRequiredControlFlags(Control::MoveControlFlag);
}

bool RunAroundLikeCrazyGoal::canUse() {
    if (horse->isTamed() || horse->rider.lock() == nullptr) return false;
    auto pos = RandomPos::getPos(
        std::dynamic_pointer_cast<PathfinderMob>(horse->shared_from_this()), 5,
        4);
    if (!pos.has_value()) return false;
    posX = pos->x;
    posY = pos->y;
    posZ = pos->z;
    return true;
}

void RunAroundLikeCrazyGoal::start() {
    horse->getNavigation()->moveTo(posX, posY, posZ, speedModifier);
}

bool RunAroundLikeCrazyGoal::canContinueToUse() {
    return !horse->getNavigation()->isDone() && horse->rider.lock() != nullptr;
}

void RunAroundLikeCrazyGoal::tick() {
    if (horse->getRandom()->nextInt(50) == 0) {
        if (horse->rider.lock()->instanceof (eTYPE_PLAYER)) {
            int temper = horse->getTemper();
            int maxTemper = horse->getMaxTemper();
            if (maxTemper > 0 &&
                horse->getRandom()->nextInt(maxTemper) < temper) {
                horse->tameWithName(
                    std::dynamic_pointer_cast<Player>(horse->rider.lock()));
                horse->level->broadcastEntityEvent(
                    horse->shared_from_this(), EntityEvent::TAMING_SUCCEEDED);
                return;
            }
            horse->modifyTemper(5);
        }

        horse->rider.lock()->ride(nullptr);
        horse->rider = std::weak_ptr<LivingEntity>();
        horse->makeMad();
        horse->level->broadcastEntityEvent(horse->shared_from_this(),
                                           EntityEvent::TAMING_FAILED);
    }
}
