#include "SnowballItem.h"

#include <memory>

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/Snowball.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"

class Entity;

SnowballItem::SnowballItem(int id) : Item(id) { this->maxStackSize = 16; }

std::shared_ptr<ItemInstance> SnowballItem::use(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    if (!player->abilities.instabuild) {
        instance->count--;
    }
    level->playEntitySound((std::shared_ptr<Entity>)player,
                           eSoundType_RANDOM_BOW, 0.5f,
                           0.4f / (random->nextFloat() * 0.4f + 0.8f));
    if (!level->isClientSide)
        level->addEntity(std::make_shared<Snowball>(level, player));
    return instance;
}