#include "FireChargeItem.h"

#include <memory>
#include <string>

#include "java/Random.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"

FireChargeItem::FireChargeItem(int id) : Item(id) {
    m_dragonFireballIcon = nullptr;
}

bool FireChargeItem::useOn(std::shared_ptr<ItemInstance> instance,
                           std::shared_ptr<Player> player, Level* level, int x,
                           int y, int z, int face, float clickX, float clickY,
                           float clickZ, bool bTestUseOnOnly) {
    if (level->isClientSide) {
        return true;
    }

    if (face == 0) y--;
    if (face == 1) y++;
    if (face == 2) z--;
    if (face == 3) z++;
    if (face == 4) x--;
    if (face == 5) x++;

    if (!player->mayUseItemAt(x, y, z, face, instance)) {
        return false;
    }

    // 4J-PB - Adding a test only version to allow tooltips to be displayed
    if (bTestUseOnOnly) {
        return true;
    }

    int targetType = level->getTile(x, y, z);

    if (targetType == 0) {
        level->playSound(x + 0.5, y + 0.5, z + 0.5, eSoundType_FIRE_NEWIGNITE,
                         1, random->nextFloat() * 0.4f + 0.8f);
        level->setTileAndUpdate(x, y, z, Tile::fire_Id);
    }

    if (!player->abilities.instabuild) {
        instance->count--;
    }
    return true;
}

Icon* FireChargeItem::getIcon(int itemAuxValue) {
    if (itemAuxValue > 0) return m_dragonFireballIcon;
    return Item::getIcon(itemAuxValue);
}

void FireChargeItem::registerIcons(IconRegister* iconRegister) {
    Item::registerIcons(iconRegister);
    m_dragonFireballIcon = iconRegister->registerIcon("dragonFireball");
}
