#include "WeaponRecipes.h"

#include <vector>

#include "Recipes.h"
#include "java/Class.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"

// 4J-PB - adding "" on the end of these so we can detect it
std::string WeaponRecipies::shapes[][4] = {
    {"X",       //
     "X",       //
     "#", ""},  //
};

void WeaponRecipies::_init() {
    map = new std::vector<Object*>[MAX_WEAPON_RECIPES];

    ADD_OBJECT(map[0], Tile::wood);
    ADD_OBJECT(map[0], Tile::cobblestone);
    ADD_OBJECT(map[0], Item::ironIngot);
    ADD_OBJECT(map[0], Item::diamond);
    ADD_OBJECT(map[0], Item::goldIngot);

    ADD_OBJECT(map[1], Item::sword_wood);
    ADD_OBJECT(map[1], Item::sword_stone);
    ADD_OBJECT(map[1], Item::sword_iron);
    ADD_OBJECT(map[1], Item::sword_diamond);
    ADD_OBJECT(map[1], Item::sword_gold);
}

void WeaponRecipies::addRecipes(Recipes* r) {
    char wchTypes[7];
    wchTypes[6] = 0;

    for (unsigned int m = 0; m < map[0].size(); m++) {
        Object* pObjMaterial = map[0].at(m);

        for (int t = 0; t < MAX_WEAPON_RECIPES - 1; t++) {
            Item* target = map[t + 1].at(m)->item;

            wchTypes[0] = 'w';
            wchTypes[1] = 'c';
            wchTypes[2] = 'i';
            wchTypes[3] = 'c';
            wchTypes[5] = 'g';
            if (pObjMaterial->GetType() == eType_TILE) {
                wchTypes[4] = 't';
                r->addShapedRecipy(
                    new ItemInstance(target), wchTypes, shapes[t],

                    '#', Item::stick, 'X', pObjMaterial->tile, 'T');
            } else {
                // must be Item
                wchTypes[4] = 'i';
                r->addShapedRecipy(
                    new ItemInstance(target), wchTypes, shapes[t],

                    '#', Item::stick, 'X', pObjMaterial->item, 'T');
            }
        }
    }

    /* 4J-PB - moved out to main recipes so we can avoid them stacking on the
    group display name r->addShapedRecipy(new ItemInstance(Item::bow, 1), //
            "ssscicig",
            " #X", //
            "# X", //
            " #X", //

            'X', Item::string,//
            '#', Item::stick,
            'T');

    r->addShapedRecipy(new ItemInstance(Item::arrow, 4), //
            "ssscicicig",
            "X", //
            "#", //
            "Y", //

            'Y', Item::feather,//
            'X', Item::flint,//
            '#', Item::stick,
            'T');
            */
}