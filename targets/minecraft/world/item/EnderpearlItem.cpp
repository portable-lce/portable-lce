#include "EnderpearlItem.h"

#include <memory>

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/ThrownEnderpearl.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"

EnderpearlItem::EnderpearlItem(int id) : Item(id) { maxStackSize = 16; }

bool EnderpearlItem::TestUse(std::shared_ptr<ItemInstance> itemInstance,
                             Level* level, std::shared_ptr<Player> player) {
    return true;
}

std::shared_ptr<ItemInstance> EnderpearlItem::use(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    // 4J-PB - Not sure why this was disabled for creative mode, so commenting
    // out
    // if (player->abilities.instabuild) return instance;
    if (player->riding != nullptr) return instance;
    if (!player->abilities.instabuild) {
        instance->count--;
    }

    level->playEntitySound(player, eSoundType_RANDOM_BOW, 0.5f,
                           0.4f / (random->nextFloat() * 0.4f + 0.8f));
    if (!level->isClientSide) {
        level->addEntity(std::shared_ptr<ThrownEnderpearl>(
            new ThrownEnderpearl(level, player)));
    }
    return instance;
}