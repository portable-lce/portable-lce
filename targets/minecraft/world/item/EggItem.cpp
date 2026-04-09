
#include "EggItem.h"

#include <memory>

#include "java/Random.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/ThrownEgg.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"

EggItem::EggItem(int id) : Item(id) { maxStackSize = 16; }

std::shared_ptr<ItemInstance> EggItem::use(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    if (!player->abilities.instabuild) {
        instance->count--;
    }
    level->playEntitySound(player, eSoundType_RANDOM_BOW, 0.5f,
                           0.4f / (random->nextFloat() * 0.4f + 0.8f));
    if (!level->isClientSide)
        level->addEntity(std::make_shared<ThrownEgg>(level, player));
    return instance;
}
