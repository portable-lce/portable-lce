#pragma once

#include <memory>

#include "Mob.h"
#include "minecraft/Pos.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/ai/goal/MoveTowardsRestrictionGoal.h"

class Level;
class Path;
class AttributeModifier;

class PathfinderMob : public Mob {
public:
    static AttributeModifier* SPEED_MODIFIER_FLEEING;

private:
    static const int MAX_TURN = 30;

public:
    PathfinderMob(Level* level);
    virtual ~PathfinderMob();

private:
    Path* path;

protected:
    std::shared_ptr<Entity> attackTarget;
    bool holdGround;
    int fleeTime;

private:
    Pos restrictCenter;
    float restrictRadius;
    MoveTowardsRestrictionGoal leashRestrictionGoal;
    bool addedLeashRestrictionGoal;

protected:
    virtual bool shouldHoldGround();
    virtual void serverAiStep();
    virtual void findRandomStrollLocation(int quadrant = -1);
    virtual void checkHurtTarget(std::shared_ptr<Entity> target, float d);

public:
    virtual float getWalkTargetValue(int x, int y, int z);

protected:
    virtual std::shared_ptr<Entity> findAttackTarget();

public:
    virtual bool canSpawn();
    virtual bool isPathFinding();
    virtual void setPath(Path* path);
    virtual std::shared_ptr<Entity> getAttackTarget();
    virtual void setAttackTarget(std::shared_ptr<Entity> attacker);

    // might move to navigation, might make area
    virtual bool isWithinRestriction();
    virtual bool isWithinRestriction(int x, int y, int z);
    virtual void restrictTo(int x, int y, int z, int radius);
    virtual Pos* getRestrictCenter();
    virtual float getRestrictRadius();
    virtual void clearRestriction();
    virtual bool hasRestriction();

protected:
    void tickLeash();
    void onLeashDistance(float distanceToLeashHolder);

    // 4J added
public:
    virtual bool couldWander();
};
