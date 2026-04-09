#include "ClothDyeRecipes.h"

#include <vector>

#include "Recipes.h"
#include "minecraft/world/item/DyePowderItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/ColoredTile.h"
#include "minecraft/world/level/tile/Tile.h"

void ClothDyeRecipes::addRecipes(Recipes* r) {
    // recipes for converting cloth to colored cloth using dye
    for (int i = 0; i < 16; i++) {
        r->addShapelessRecipy(
            new ItemInstance(Tile::wool, 1,
                             ColoredTile::getItemAuxValueForTileData(i)),  //
            "zzg", new ItemInstance(Item::dye_powder, 1, i),
            new ItemInstance(Item::items[Tile::wool_Id], 1, 0), 'D');
        r->addShapedRecipy(
            new ItemInstance(Tile::clayHardened_colored, 8,
                             ColoredTile::getItemAuxValueForTileData(i)),  //
            "sssczczg", "###", "#X#", "###", '#',
            new ItemInstance(Tile::clayHardened), 'X',
            new ItemInstance(Item::dye_powder, 1, i), 'D');
    }

    // some dye recipes
    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::YELLOW), "tg",
        Tile::flower, 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::RED), "tg",
        Tile::rose, 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 3, DyePowderItem::WHITE), "ig",
        Item::bone, 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::PINK),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::RED),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::ORANGE),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::RED),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::YELLOW), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::LIME),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::GREEN),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::GRAY),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLACK),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::SILVER),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::GRAY),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 3, DyePowderItem::SILVER),  //
        "zzzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLACK),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::LIGHT_BLUE),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::CYAN),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::GREEN), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::PURPLE),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 2, DyePowderItem::MAGENTA),  //
        "zzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::PURPLE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::PINK), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 3, DyePowderItem::MAGENTA),  //
        "zzzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::RED),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::PINK), 'D');

    r->addShapelessRecipy(
        new ItemInstance(Item::dye_powder, 4, DyePowderItem::MAGENTA),  //
        "zzzzg", new ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::RED),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::RED),
        new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 'D');

    for (int i = 0; i < 16; i++) {
        r->addShapedRecipy(new ItemInstance(Tile::woolCarpet, 3, i), "sczg",
                           "##", '#', new ItemInstance(Tile::wool, 1, i), 'D');
    }
}
