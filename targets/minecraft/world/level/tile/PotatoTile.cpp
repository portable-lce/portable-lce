#include "PotatoTile.h"

#include <memory>
#include <string>

#include "java/Random.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/CropTile.h"
#include "util/StringHelpers.h"

PotatoTile::PotatoTile(int id) : CropTile(id) {}

Icon* PotatoTile::getTexture(int face, int data) {
    if (data < 7) {
        if (data == 6) {
            data = 5;
        }
        return icons[data >> 1];
    } else {
        return icons[3];
    }
}

int PotatoTile::getBaseSeedId() { return Item::potato_Id; }

int PotatoTile::getBasePlantId() { return Item::potato_Id; }

void PotatoTile::spawnResources(Level* level, int x, int y, int z, int data,
                                float odds, int playerBonus) {
    CropTile::spawnResources(level, x, y, z, data, odds, playerBonus);

    if (level->isClientSide) {
        return;
    }
    if (data >= 7) {
        if (level->random->nextInt(50) == 0) {
            popResource(level, x, y, z,
                        std::shared_ptr<ItemInstance>(
                            new ItemInstance(Item::potatoPoisonous)));
        }
    }
}

void PotatoTile::registerIcons(IconRegister* iconRegister) {
    for (int i = 0; i < 4; i++) {
        icons[i] = iconRegister->registerIcon(getIconName() + "_stage_" +
                                              toWString<int>(i));
    }
}