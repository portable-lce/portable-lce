#include "StructureRecipes.h"

#include "Recipes.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/BeaconTile.h"
#include "minecraft/world/level/tile/ChestTile.h"
#include "minecraft/world/level/tile/HalfSlabTile.h"
#include "minecraft/world/level/tile/QuartzBlockTile.h"
#include "minecraft/world/level/tile/SandStoneTile.h"
#include "minecraft/world/level/tile/StoneSlabTile.h"
#include "minecraft/world/level/tile/Tile.h"

void StructureRecipies::addRecipes(Recipes* r) {
    r->addShapedRecipy(new ItemInstance(Tile::sandStone),  //
                       "ssctg",
                       "##",  //
                       "##",  //

                       '#', Tile::sand, 'S');

    r->addShapedRecipy(new ItemInstance(Tile::sandStone, 4,
                                        SandStoneTile::TYPE_SMOOTHSIDE),  //
                       "ssczg",
                       "##",  //
                       "##",  //

                       '#', new ItemInstance(Tile::sandStone), 'S');

    r->addShapedRecipy(
        new ItemInstance(Tile::sandStone, 1,
                         SandStoneTile::TYPE_HEIROGLYPHS),  //
        "ssczg",
        "#",  //
        "#",  //

        '#', new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SAND_SLAB),
        'S');

    r->addShapedRecipy(
        new ItemInstance(Tile::quartzBlock, 1,
                         QuartzBlockTile::TYPE_CHISELED),  //
        "ssczg",
        "#",  //
        "#",  //

        '#',
        new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::QUARTZ_SLAB),
        'S');

    r->addShapedRecipy(
        new ItemInstance(Tile::quartzBlock, 2,
                         QuartzBlockTile::TYPE_LINES_Y),  //
        "ssczg",
        "#",  //
        "#",  //

        '#',
        new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_DEFAULT),
        'S');

    // 4J Stu - Changed the order, as the blocks that go with sandstone cause a
    // 3-icon scroll that touches the text "Structures" in the title in 720
    // fullscreen.
    r->addShapedRecipy(new ItemInstance(Tile::workBench),  //
                       "ssctg",
                       "##",  //
                       "##",  //

                       '#', Tile::wood, 'S');

    r->addShapedRecipy(new ItemInstance(Tile::furnace),  //
                       "sssctg",
                       "###",  //
                       "# #",  //
                       "###",  //

                       '#', Tile::cobblestone, 'S');

    r->addShapedRecipy(new ItemInstance((Tile*)Tile::chest),  //
                       "sssctg",
                       "###",  //
                       "# #",  //
                       "###",  //

                       '#', Tile::wood, 'S');

    r->addShapedRecipy(new ItemInstance(Tile::chest_trap),  //
                       "sctctg",
                       "#-",  //

                       '#', Tile::chest, '-', Tile::tripWireSource, 'S');

    r->addShapedRecipy(new ItemInstance(Tile::enderChest),  //
                       "sssctcig",
                       "###",  //
                       "#E#",  //
                       "###",  //

                       '#', Tile::obsidian, 'E', Item::eyeOfEnder, 'S');

    r->addShapedRecipy(new ItemInstance(Tile::stoneBrick, 4),  //
                       "ssctg",
                       "##",  //
                       "##",  //

                       '#', Tile::stone, 'S');

    // 4J Stu - Move this into "Recipes" to change the order things are
    // displayed on the crafting menu
    // r->addShapedRecipy(new ItemInstance(Tile::ironFence, 16), //
    //	"sscig",
    //	"###", //
    //	"###", //

    //	'#', Item::ironIngot,
    //	'S');

    r->addShapedRecipy(new ItemInstance(Tile::thinGlass, 16),  //
                       "ssctg",
                       "###",  //
                       "###",  //

                       '#', Tile::glass, 'D');

    r->addShapedRecipy(new ItemInstance(Tile::netherBrick, 1),  //
                       "sscig",
                       "NN",  //
                       "NN",  //

                       'N', Item::netherbrick, 'S');

    r->addShapedRecipy(new ItemInstance(Tile::redstoneLight, 1),  //
                       "ssscictg",
                       " R ",  //
                       "RGR",  //
                       " R ",  //
                       'R', Item::redStone, 'G', Tile::glowstone, 'M');

    r->addShapedRecipy(new ItemInstance(Tile::beacon, 1),  //
                       "sssctcictg",
                       "GGG",  //
                       "GSG",  //
                       "OOO",  //

                       'G', Tile::glass, 'S', Item::netherStar, 'O',
                       Tile::obsidian, 'M');
}