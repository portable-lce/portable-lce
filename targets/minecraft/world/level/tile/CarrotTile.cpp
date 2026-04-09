#include "CarrotTile.h"

#include <string>

#include "minecraft/world/IconRegister.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/tile/CropTile.h"
#include "util/StringHelpers.h"

CarrotTile::CarrotTile(int id) : CropTile(id) {}

Icon* CarrotTile::getTexture(int face, int data) {
    if (data < 7) {
        if (data == 6) {
            data = 5;
        }
        return icons[data >> 1];
    } else {
        return icons[3];
    }
}

int CarrotTile::getBaseSeedId() { return Item::carrots_Id; }

int CarrotTile::getBasePlantId() { return Item::carrots_Id; }

void CarrotTile::registerIcons(IconRegister* iconRegister) {
    for (int i = 0; i < 4; i++) {
        icons[i] = iconRegister->registerIcon(getIconName() + "_stage_" +
                                              toWString(i));
    }
}