#include "MinecartRideable.h"

#include <memory>

#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/item/Minecart.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/Level.h"

MinecartRideable::MinecartRideable(Level* level) : Minecart(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
}

MinecartRideable::MinecartRideable(Level* level, double x, double y, double z)
    : Minecart(level, x, y, z) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
}

bool MinecartRideable::interact(std::shared_ptr<Player> player) {
    if (rider.lock() != nullptr && rider.lock()->instanceof
        (eTYPE_PLAYER) && rider.lock() != player)
        return true;
    if (rider.lock() != nullptr && rider.lock() != player) return false;
    if (!level->isClientSide) {
        player->ride(shared_from_this());
    }

    return true;
}

int MinecartRideable::getType() { return TYPE_RIDEABLE; }