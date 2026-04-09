#include "PressurePlateTile.h"

#include <format>
#include <memory>
#include <vector>

#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/redstone/Redstone.h"
#include "minecraft/world/level/tile/BasePressurePlateTile.h"
#include "minecraft/world/phys/AABB.h"

class Material;

PressurePlateTile::PressurePlateTile(int id, const std::string& tex,
                                     Material* material,
                                     Sensitivity sensitivity)
    : BasePressurePlateTile(id, tex, material) {
    this->sensitivity = sensitivity;

    // 4J Stu - Move this from base class to use virtual function
    updateShape(getDataForSignal(Redstone::SIGNAL_MAX));
}

int PressurePlateTile::getDataForSignal(int signal) {
    return signal > 0 ? 1 : 0;
}

int PressurePlateTile::getSignalForData(int data) {
    return data == 1 ? Redstone::SIGNAL_MAX : 0;
}

int PressurePlateTile::getSignalStrength(Level* level, int x, int y, int z) {
    std::vector<std::shared_ptr<Entity> >* entities = nullptr;
    AABB at_bb = getSensitiveAABB(x, y, z);
    if (sensitivity == everything)
        entities = level->getEntities(nullptr, &at_bb);
    else if (sensitivity == mobs)
        entities = level->getEntitiesOfClass(typeid(LivingEntity), &at_bb);
    else if (sensitivity == players)
        entities = level->getEntitiesOfClass(typeid(Player), &at_bb);
    else
        assert(0);  // 4J-JEV: We're going to delete something at a random
                    // location.

    if (entities != nullptr && !entities->empty()) {
        for (auto it = entities->begin(); it != entities->end(); ++it) {
            std::shared_ptr<Entity> e = *it;
            if (!e->isIgnoringTileTriggers()) {
                if (sensitivity != everything) delete entities;
                return Redstone::SIGNAL_MAX;
            }
        }
    }

    if (sensitivity != everything) delete entities;
    return Redstone::SIGNAL_NONE;
}
