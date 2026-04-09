#include "minecraft/world/item/crafting/Recipes.h"

#include <stdarg.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "minecraft/util/Log.h"
#include "minecraft/world/inventory/CraftingContainer.h"
#include "minecraft/world/item/CoalItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/crafting/ArmorRecipes.h"
#include "minecraft/world/item/crafting/ClothDyeRecipes.h"
#include "minecraft/world/item/crafting/FireworksRecipe.h"
#include "minecraft/world/item/crafting/FoodRecipes.h"
#include "minecraft/world/item/crafting/OreRecipes.h"
#include "minecraft/world/item/crafting/Recipy.h"
#include "minecraft/world/item/crafting/ShapedRecipy.h"
#include "minecraft/world/item/crafting/ShapelessRecipy.h"
#include "minecraft/world/item/crafting/StructureRecipes.h"
#include "minecraft/world/item/crafting/ToolRecipes.h"
#include "minecraft/world/item/crafting/WeaponRecipes.h"
#include "minecraft/world/level/tile/DaylightDetectorTile.h"
#include "minecraft/world/level/tile/HalfSlabTile.h"
#include "minecraft/world/level/tile/HopperTile.h"
#include "minecraft/world/level/tile/StoneSlabTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/TreeTile.h"
#include "minecraft/world/level/tile/TripWireSourceTile.h"
#include "minecraft/world/level/tile/WallTile.h"
#include "minecraft/world/level/tile/piston/PistonBaseTile.h"

class Level;

Recipes* Recipes::instance = nullptr;
ArmorRecipes* Recipes::pArmorRecipes = nullptr;
ClothDyeRecipes* Recipes::pClothDyeRecipes = nullptr;
FoodRecipies* Recipes::pFoodRecipies = nullptr;
OreRecipies* Recipes::pOreRecipies = nullptr;
StructureRecipies* Recipes::pStructureRecipies = nullptr;
ToolRecipies* Recipes::pToolRecipies = nullptr;
WeaponRecipies* Recipes::pWeaponRecipies = nullptr;
FireworksRecipe* Recipes::pFireworksRecipes = nullptr;

void Recipes::staticCtor() { Recipes::instance = new Recipes(); }

void Recipes::_init() {
    // 4J Jev: instance = new Recipes();
    recipies = new std::vector<Recipy*>();
}

Recipes::Recipes() {
    int iCount = 0;
    _init();

    pArmorRecipes = new ArmorRecipes;
    pClothDyeRecipes = new ClothDyeRecipes;
    pFoodRecipies = new FoodRecipies;
    pOreRecipies = new OreRecipies;
    pStructureRecipies = new StructureRecipies;
    pToolRecipies = new ToolRecipies;
    pWeaponRecipies = new WeaponRecipies;

    // 4J Stu - These just don't work with our crafting menu
    // recipies->push_back(new ArmorDyeRecipe());
    // recipies->add(new MapCloningRecipe());
    // recipies->add(new MapExtendingRecipe());
    // recipies->add(new FireworksRecipe());
    pFireworksRecipes = new FireworksRecipe();

    addShapedRecipy(new ItemInstance(Tile::wood, 4, 0),  //
                    "sczg",
                    "#",  //

                    '#', new ItemInstance(Tile::treeTrunk, 1, 0), 'S');

    // TU9 - adding coloured wood
    addShapedRecipy(
        new ItemInstance(Tile::wood, 4, TreeTile::BIRCH_TRUNK),  //
        "sczg",
        "#",  //

        '#', new ItemInstance(Tile::treeTrunk, 1, TreeTile::BIRCH_TRUNK), 'S');

    addShapedRecipy(
        new ItemInstance(Tile::wood, 4, TreeTile::DARK_TRUNK),  //
        "sczg",
        "#",  //

        '#', new ItemInstance(Tile::treeTrunk, 1, TreeTile::DARK_TRUNK), 'S');

    addShapedRecipy(
        new ItemInstance(Tile::wood, 4, TreeTile::JUNGLE_TRUNK),  //
        "sczg",
        "#",  //

        '#', new ItemInstance(Tile::treeTrunk, 1, TreeTile::JUNGLE_TRUNK), 'S');

    addShapedRecipy(new ItemInstance(Item::stick, 4),  //
                    "ssctg",
                    "#",  //
                    "#",  //

                    '#', Tile::wood, 'S');

    pToolRecipies->addRecipes(this);
    pFoodRecipies->addRecipes(this);
    pStructureRecipies->addRecipes(this);

    // 4J-PB - changing the order to the way we want to have things in the
    // crafting menu bed
    addShapedRecipy(new ItemInstance(Item::bed, 1),  //
                    "ssctctg",
                    "###",  //
                    "XXX",  //
                    '#', Tile::wool, 'X', Tile::wood, 'S');

    addShapedRecipy(new ItemInstance(Tile::enchantTable, 1),  //
                    "sssctcicig",
                    " B ",  //
                    "D#D",  //
                    "###",  //

                    '#', Tile::obsidian, 'B', Item::book, 'D', Item::diamond,
                    'S');

    addShapedRecipy(new ItemInstance(Tile::anvil, 1),  //
                    "sssctcig",
                    "III",  //
                    " i ",  //
                    "iii",  //

                    'I', Tile::ironBlock, 'i', Item::ironIngot, 'S');

    // 4J Stu - Reordered for crafting menu
    addShapedRecipy(new ItemInstance(Tile::ladder, 3),  //
                    "ssscig",
                    "# #",  //
                    "###",  //
                    "# #",  //

                    '#', Item::stick, 'S');

    addShapedRecipy(new ItemInstance(Tile::fenceGate, 1),  //
                    "sscictg",
                    "#W#",  //
                    "#W#",  //

                    '#', Item::stick, 'W', Tile::wood, 'S');

    addShapedRecipy(new ItemInstance(Tile::fence, 2),  //
                    "sscig",
                    "###",  //
                    "###",  //

                    '#', Item::stick, 'S');

    addShapedRecipy(new ItemInstance(Tile::netherFence, 6),  //
                    "ssctg",
                    "###",  //
                    "###",  //

                    '#', Tile::netherBrick, 'S');

    addShapedRecipy(new ItemInstance(Tile::ironFence, 16),  //
                    "sscig",
                    "###",  //
                    "###",  //

                    '#', Item::ironIngot, 'S');

    addShapedRecipy(
        new ItemInstance(Tile::cobbleWall, 6, WallTile::TYPE_NORMAL),  //
        "ssctg",
        "###",  //
        "###",  //

        '#', Tile::cobblestone, 'S');

    addShapedRecipy(
        new ItemInstance(Tile::cobbleWall, 6, WallTile::TYPE_MOSSY),  //
        "ssctg",
        "###",  //
        "###",  //

        '#', Tile::mossyCobblestone, 'S');

    addShapedRecipy(new ItemInstance(Item::door_wood, 1),  //
                    "sssctg",
                    "##",  //
                    "##",  //
                    "##",  //

                    '#', Tile::wood, 'S');

    addShapedRecipy(new ItemInstance(Item::door_iron, 1),  //
                    "ssscig",
                    "##",  //
                    "##",  //
                    "##",  //

                    '#', Item::ironIngot, 'S');

    addShapedRecipy(new ItemInstance(Tile::stairs_wood, 4),  //
                    "sssczg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', new ItemInstance(Tile::wood, 1, 0), 'S');

    addShapedRecipy(new ItemInstance(Tile::trapdoor, 2),  //
                    "ssctg",
                    "###",  //
                    "###",  //

                    '#', Tile::wood, 'S');
    addShapedRecipy(new ItemInstance(Tile::stairs_stone, 4),  //
                    "sssctg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', Tile::cobblestone, 'S');

    addShapedRecipy(new ItemInstance(Tile::stairs_bricks, 4),  //
                    "sssctg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', Tile::redBrick, 'S');

    addShapedRecipy(new ItemInstance(Tile::stairs_stoneBrickSmooth, 4),  //
                    "sssctg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', Tile::cobblestone, 'S');

    addShapedRecipy(new ItemInstance(Tile::stairs_netherBricks, 4),  //
                    "sssctg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', Tile::netherBrick, 'S');

    addShapedRecipy(new ItemInstance(Tile::stairs_sandstone, 4),  //
                    "sssctg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', Tile::sandStone, 'S');

    addShapedRecipy(new ItemInstance(Tile::woodStairsBirch, 4),  //
                    "sssczg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', new ItemInstance(Tile::wood, 1, TreeTile::BIRCH_TRUNK),
                    'S');

    addShapedRecipy(new ItemInstance(Tile::woodStairsDark, 4),  //
                    "sssczg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', new ItemInstance(Tile::wood, 1, TreeTile::DARK_TRUNK),
                    'S');

    addShapedRecipy(
        new ItemInstance(Tile::woodStairsJungle, 4),  //
        "sssczg",
        "#  ",  //
        "## ",  //
        "###",  //

        '#', new ItemInstance(Tile::wood, 1, TreeTile::JUNGLE_TRUNK), 'S');

    addShapedRecipy(new ItemInstance(Tile::stairs_quartz, 4),  //
                    "sssctg",
                    "#  ",  //
                    "## ",  //
                    "###",  //

                    '#', Tile::quartzBlock, 'S');

    pArmorRecipes->addRecipes(this);
    // iCount=getRecipies()->size();

    pClothDyeRecipes->addRecipes(this);

    addShapedRecipy(new ItemInstance(Tile::snow, 1),  //
                    "sscig",
                    "##",  //
                    "##",  //

                    '#', Item::snowBall, 'S');

    addShapedRecipy(new ItemInstance(Tile::topSnow, 6),  //
                    "sctg",
                    "###",  //

                    '#', Tile::snow, 'S');

    addShapedRecipy(new ItemInstance(Tile::clay, 1),  //
                    "sscig",
                    "##",  //
                    "##",  //

                    '#', Item::clay, 'S');

    addShapedRecipy(new ItemInstance(Tile::redBrick, 1),  //
                    "sscig",
                    "##",  //
                    "##",  //

                    '#', Item::brick, 'S');

    addShapedRecipy(new ItemInstance(Tile::wool, 1),  //
                    "sscig",
                    "##",  //
                    "##",  //

                    '#', Item::string, 'D');

    addShapedRecipy(new ItemInstance(Tile::tnt, 1),  //
                    "ssscictg",
                    "X#X",  //
                    "#X#",  //
                    "X#X",  //

                    'X', Item::gunpowder,  //
                    '#', Tile::sand, 'T');

    addShapedRecipy(
        new ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::SAND_SLAB),  //
        "sctg",
        "###",  //

        '#', Tile::sandStone, 'S');

    addShapedRecipy(
        new ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::STONE_SLAB),  //
        "sctg",
        "###",  //

        '#', Tile::stone, 'S');
    addShapedRecipy(new ItemInstance(Tile::stoneSlabHalf, 6,
                                     StoneSlabTile::COBBLESTONE_SLAB),  //
                    "sctg",
                    "###",  //

                    '#', Tile::cobblestone, 'S');

    addShapedRecipy(
        new ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::BRICK_SLAB),  //
        "sctg",
        "###",  //

        '#', Tile::redBrick, 'S');

    addShapedRecipy(new ItemInstance(Tile::stoneSlabHalf, 6,
                                     StoneSlabTile::SMOOTHBRICK_SLAB),  //
                    "sctg",
                    "###",  //

                    '#', Tile::stoneBrick, 'S');

    addShapedRecipy(new ItemInstance(Tile::stoneSlabHalf, 6,
                                     StoneSlabTile::NETHERBRICK_SLAB),  //
                    "sctg",
                    "###",  //

                    '#', Tile::netherBrick, 'S');

    addShapedRecipy(new ItemInstance(Tile::stoneSlabHalf, 6,
                                     StoneSlabTile::QUARTZ_SLAB),  //
                    "sctg",
                    "###",  //

                    '#', Tile::quartzBlock, 'S');

    addShapedRecipy(new ItemInstance(Tile::woodSlabHalf, 6, 0),  //
                    "sczg",
                    "###",  //

                    '#', new ItemInstance(Tile::wood, 1, 0), 'S');
    // TU9 - adding wood slabs

    addShapedRecipy(
        new ItemInstance(Tile::woodSlabHalf, 6, TreeTile::BIRCH_TRUNK),  //
        "sczg",
        "###",  //

        '#', new ItemInstance(Tile::wood, 1, TreeTile::BIRCH_TRUNK), 'S');

    addShapedRecipy(
        new ItemInstance(Tile::woodSlabHalf, 6, TreeTile::DARK_TRUNK),  //
        "sczg",
        "###",  //

        '#', new ItemInstance(Tile::wood, 1, TreeTile::DARK_TRUNK), 'S');

    addShapedRecipy(
        new ItemInstance(Tile::woodSlabHalf, 6, TreeTile::JUNGLE_TRUNK),  //
        "sczg",
        "###",  //

        '#', new ItemInstance(Tile::wood, 1, TreeTile::JUNGLE_TRUNK), 'S');

    // iCount=getRecipies()->size();

    addShapedRecipy(new ItemInstance(Item::cake, 1),  //
                    "ssscicicicig",
                    "AAA",  //
                    "BEB",  //
                    "CCC",  //

                    'A', Item::bucket_milk,  //
                    'B', Item::sugar,        //
                    'C', Item::wheat, 'E', Item::egg, 'F');

    addShapedRecipy(new ItemInstance(Item::sugar, 1),  //
                    "scig",
                    "#",  //

                    '#', Item::reeds, 'F');

    addShapedRecipy(new ItemInstance(Tile::rail, 16),  //
                    "ssscicig",
                    "X X",  //
                    "X#X",  //
                    "X X",  //

                    'X', Item::ironIngot,  //
                    '#', Item::stick, 'V');

    addShapedRecipy(new ItemInstance(Tile::goldenRail, 6),  //
                    "ssscicicig",
                    "X X",  //
                    "X#X",  //
                    "XRX",  //

                    'X', Item::goldIngot,  //
                    'R', Item::redStone,   //
                    '#', Item::stick, 'V');

    addShapedRecipy(new ItemInstance(Tile::activatorRail, 6),  //
                    "ssscictcig",
                    "XSX",  //
                    "X#X",  //
                    "XSX",  //

                    'X', Item::ironIngot,         //
                    '#', Tile::redstoneTorch_on,  //
                    'S', Item::stick, 'V');

    addShapedRecipy(new ItemInstance(Tile::detectorRail, 6),  //
                    "ssscicictg",
                    "X X",  //
                    "X#X",  //
                    "XRX",  //

                    'X', Item::ironIngot,  //
                    'R', Item::redStone,   //
                    '#', Tile::pressurePlate_stone, 'V');

    addShapedRecipy(new ItemInstance(Item::minecart, 1),  //
                    "sscig",
                    "# #",  //
                    "###",  //

                    '#', Item::ironIngot, 'V');

    addShapedRecipy(new ItemInstance(Item::minecart_chest, 1),  //
                    "ssctcig",
                    "A",  //
                    "B",  //

                    'A', Tile::chest, 'B', Item::minecart, 'V');

    addShapedRecipy(new ItemInstance(Item::minecart_furnace, 1),  //
                    "ssctcig",
                    "A",  //
                    "B",  //

                    'A', Tile::furnace, 'B', Item::minecart, 'V');

    addShapedRecipy(new ItemInstance(Item::minecart_tnt, 1),  //
                    "ssctcig",
                    "A",  //
                    "B",  //

                    'A', Tile::tnt, 'B', Item::minecart, 'V');

    addShapedRecipy(new ItemInstance(Item::minecart_hopper, 1),  //
                    "ssctcig",
                    "A",  //
                    "B",  //

                    'A', Tile::hopper, 'B', Item::minecart, 'V');

    addShapedRecipy(new ItemInstance(Item::boat, 1),  //
                    "ssctg",
                    "# #",  //
                    "###",  //

                    '#', Tile::wood, 'V');

    addShapedRecipy(new ItemInstance((Item*)Item::fishingRod, 1),  //
                    "ssscicig",
                    "  #",  //
                    " #X",  //
                    "# X",  //

                    '#', Item::stick, 'X', Item::string, 'T');

    addShapedRecipy(new ItemInstance(Item::carrotOnAStick, 1),  //
                    "sscicig",
                    "# ",  //
                    " X",  //

                    '#', Item::fishingRod, 'X', Item::carrots, 'T')
        ->keepTag();

    addShapedRecipy(new ItemInstance(Item::flintAndSteel, 1),  //
                    "sscicig",
                    "A ",  //
                    " B",  //

                    'A', Item::ironIngot, 'B', Item::flint, 'T');

    addShapedRecipy(new ItemInstance(Item::bread, 1),  //
                    "scig",
                    "###",  //

                    '#', Item::wheat, 'F');

    // Moved bow and arrow in from weapons to avoid stacking on the group name
    // display
    addShapedRecipy(new ItemInstance((Item*)Item::bow, 1),  //
                    "ssscicig",
                    " #X",  //
                    "# X",  //
                    " #X",  //

                    'X', Item::string,  //
                    '#', Item::stick, 'T');

    addShapedRecipy(new ItemInstance(Item::arrow, 4),  //
                    "ssscicicig",
                    "X",  //
                    "#",  //
                    "Y",  //

                    'Y', Item::feather,  //
                    'X', Item::flint,    //
                    '#', Item::stick, 'T');

    pWeaponRecipies->addRecipes(this);

    addShapedRecipy(new ItemInstance(Item::bucket_empty, 1),  //
                    "sscig",
                    "# #",  //
                    " # ",  //

                    '#', Item::ironIngot, 'T');

    addShapedRecipy(new ItemInstance(Item::bowl, 4),  //
                    "ssctg",
                    "# #",  //
                    " # ",  //

                    '#', Tile::wood, 'T');

    addShapedRecipy(new ItemInstance(Item::glassBottle, 3),  //
                    "ssctg",
                    "# #",  //
                    " # ",  //

                    '#', Tile::glass, 'T');

    addShapedRecipy(new ItemInstance(Item::flowerPot, 1),  //
                    "sscig",
                    "# #",  //
                    " # ",  //

                    '#', Item::brick, 'D');

    // torch made of charcoal - moved to be the default due to the tutorial
    // using it
    addShapedRecipy(new ItemInstance(Tile::torch, 4),  //
                    "ssczcig",
                    "X",  //
                    "#",  //

                    'X',
                    new ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL),  //
                    '#', Item::stick, 'T');

    addShapedRecipy(new ItemInstance(Tile::torch, 4),  //
                    "ssczcig",
                    "X",  //
                    "#",  //
                    'X',
                    new ItemInstance(Item::coal, 1, CoalItem::STONE_COAL),  //
                    '#', Item::stick, 'T');

    addShapedRecipy(new ItemInstance(Tile::glowstone, 1),  //
                    "sscig",
                    "##",  //
                    "##",  //

                    '#', Item::yellowDust, 'T');

    addShapedRecipy(new ItemInstance(Tile::quartzBlock, 1),  //
                    "sscig",
                    "##",  //
                    "##",  //

                    '#', Item::netherQuartz, 'S');

    addShapedRecipy(new ItemInstance(Tile::lever, 1),  //
                    "ssctcig",
                    "X",  //
                    "#",  //

                    '#', Tile::cobblestone, 'X', Item::stick, 'M');

    addShapedRecipy(new ItemInstance(Tile::tripWireSource, 2),  //
                    "sssctcicig",
                    "I",  //
                    "S",  //
                    "#",  //

                    '#', Tile::wood, 'S', Item::stick, 'I', Item::ironIngot,
                    'M');

    addShapedRecipy(new ItemInstance(Tile::redstoneTorch_on, 1),  //
                    "sscicig",
                    "X",  //
                    "#",  //

                    '#', Item::stick, 'X', Item::redStone, 'M');

    addShapedRecipy(new ItemInstance(Item::repeater, 1),  //
                    "ssctcictg",
                    "#X#",  //
                    "III",  //

                    '#', Tile::redstoneTorch_on, 'X', Item::redStone, 'I',
                    Tile::stone, 'M');

    addShapedRecipy(new ItemInstance(Item::comparator, 1),  //
                    "sssctcictg",
                    " # ",  //
                    "#X#",  //
                    "III",  //

                    '#', Tile::redstoneTorch_on, 'X', Item::netherQuartz, 'I',
                    Tile::stone, 'M');

    addShapedRecipy(new ItemInstance(Tile::daylightDetector), "sssctcictg",
                    "GGG", "QQQ", "WWW",

                    'G', Tile::glass, 'Q', Item::netherQuartz, 'W',
                    Tile::woodSlabHalf, 'M');

    addShapedRecipy(new ItemInstance(Tile::hopper), "ssscictg",
                    "I I",  //
                    "ICI",  //
                    " I ",  //

                    'I', Item::ironIngot, 'C', Tile::chest, 'M');

    addShapedRecipy(new ItemInstance(Item::clock, 1),  //
                    "ssscicig",
                    " # ",  //
                    "#X#",  //
                    " # ",  //
                    '#', Item::goldIngot, 'X', Item::redStone, 'T');

    addShapelessRecipy(new ItemInstance(Item::eyeOfEnder, 1),  //
                       "iig", Item::enderPearl, Item::blazePowder, 'T');

    addShapelessRecipy(new ItemInstance(Item::fireball, 3),  //
                       "iiig", Item::gunpowder, Item::blazePowder, Item::coal,
                       'T');

    addShapelessRecipy(new ItemInstance(Item::fireball, 3),  //
                       "iizg", Item::gunpowder, Item::blazePowder,
                       new ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL),
                       'T');

    addShapedRecipy(new ItemInstance(Item::lead, 2),  //
                    "ssscicig",
                    "~~ ",  //
                    "~O ",  //
                    "  ~",  //

                    '~', Item::string, 'O', Item::slimeBall, 'T');

    addShapedRecipy(new ItemInstance(Item::compass, 1),  //
                    "ssscicig",
                    " # ",  //
                    "#X#",  //
                    " # ",  //

                    '#', Item::ironIngot, 'X', Item::redStone, 'T');

    addShapedRecipy(new ItemInstance(Item::map, 1),  //
                    "ssscicig",
                    "###",  //
                    "#X#",  //
                    "###",  //

                    '#', Item::paper, 'X', Item::compass, 'T');

    addShapedRecipy(new ItemInstance(Tile::button, 1),  //
                    "sctg",
                    "#",  //

                    '#', Tile::stone, 'M');

    addShapedRecipy(new ItemInstance(Tile::button_wood, 1),  //
                    "sctg",
                    "#",  //

                    '#', Tile::wood, 'M');

    addShapedRecipy(new ItemInstance(Tile::pressurePlate_wood, 1),  //
                    "sctg",
                    "##",  //
                    '#', Tile::wood, 'M');

    addShapedRecipy(new ItemInstance(Tile::pressurePlate_stone, 1),  //
                    "sctg",
                    "##",  //
                    '#', Tile::stone, 'M');

    addShapedRecipy(new ItemInstance(Tile::weightedPlate_heavy, 1),  //
                    "scig",
                    "##",  //

                    '#', Item::ironIngot, 'M');

    addShapedRecipy(new ItemInstance(Tile::weightedPlate_light, 1),  //
                    "scig",
                    "##",  //

                    '#', Item::goldIngot, 'M');

    addShapedRecipy(new ItemInstance(Tile::dispenser, 1),  //
                    "sssctcicig",
                    "###",  //
                    "#X#",  //
                    "#R#",  //
                    '#', Tile::cobblestone, 'X', Item::bow, 'R', Item::redStone,
                    'M');

    addShapedRecipy(new ItemInstance(Tile::dropper, 1),  //
                    "sssctcig",
                    "###",  //
                    "# #",  //
                    "#R#",  //

                    '#', Tile::cobblestone, 'R', Item::redStone, 'M');

    addShapedRecipy(new ItemInstance(Item::cauldron, 1),  //
                    "ssscig",
                    "# #",  //
                    "# #",  //
                    "###",  //

                    '#', Item::ironIngot, 'T');

    addShapedRecipy(new ItemInstance(Item::brewingStand, 1),  //
                    "ssctcig",
                    " B ",  //
                    "###",  //

                    '#', Tile::cobblestone, 'B', Item::blazeRod, 'S');

    addShapedRecipy(new ItemInstance(Tile::litPumpkin, 1),  //
                    "ssctctg",
                    "A",  //
                    "B",  //

                    'A', Tile::pumpkin, 'B', Tile::torch, 'T');

    addShapedRecipy(new ItemInstance(Tile::jukebox, 1),  //
                    "sssctcig",
                    "###",  //
                    "#X#",  //
                    "###",  //

                    '#', Tile::wood, 'X', Item::diamond, 'D');

    addShapedRecipy(new ItemInstance(Item::paper, 3),  //
                    "scig",
                    "###",  //

                    '#', Item::reeds, 'D');

    addShapelessRecipy(new ItemInstance(Item::book, 1), "iiiig", Item::paper,
                       Item::paper, Item::paper, Item::leather, 'D');

    // addShapelessRecipy(new ItemInstance(Item.writingBook, 1), //
    //             Item.book, new ItemInstance(Item.dye_powder, 1,
    //             DyePowderItem.BLACK), Item.feather);

    addShapedRecipy(new ItemInstance(Tile::noteblock, 1),  //
                    "sssctcig",
                    "###",  //
                    "#X#",  //
                    "###",  //

                    '#', Tile::wood, 'X', Item::redStone, 'M');

    addShapedRecipy(new ItemInstance(Tile::bookshelf, 1),  //
                    "sssctcig",
                    "###",  //
                    "XXX",  //
                    "###",  //

                    '#', Tile::wood, 'X', Item::book, 'D');

    addShapedRecipy(new ItemInstance(Item::painting, 1),  //
                    "ssscictg",
                    "###",  //
                    "#X#",  //
                    "###",  //

                    '#', Item::stick, 'X', Tile::wool, 'D');

    addShapedRecipy(new ItemInstance(Item::frame, 1),  //
                    "ssscicig",
                    "###",  //
                    "#X#",  //
                    "###",  //

                    '#', Item::stick, 'X', Item::leather, 'D');

    pOreRecipies->addRecipes(this);

    addShapedRecipy(new ItemInstance(Item::goldIngot),  //
                    "ssscig",
                    "###",  //
                    "###",  //
                    "###",  //

                    '#', Item::goldNugget, 'D');

    addShapedRecipy(new ItemInstance(Item::goldNugget, 9),  //
                    "scig",
                    "#",  //
                    '#', Item::goldIngot, 'D');

    // 4J-PB - moving into decorations to make the structures list smaller
    addShapedRecipy(new ItemInstance(Item::sign, 3),  //
                    "sssctcig",
                    "###",  //
                    "###",  //
                    " X ",  //

                    '#', Tile::wood, 'X', Item::stick, 'D');

    // 4J - TODO - put these new 1.7.3 items in required place within recipes
    addShapedRecipy(new ItemInstance((Tile*)Tile::pistonBase, 1),  //
                    "sssctcicictg",
                    "TTT",  //
                    "#X#",  //
                    "#R#",  //

                    '#', Tile::cobblestone, 'X', Item::ironIngot, 'R',
                    Item::redStone, 'T', Tile::wood, 'M');

    addShapedRecipy(new ItemInstance((Tile*)Tile::pistonStickyBase, 1),  //
                    "sscictg",
                    "S",  //
                    "P",  //

                    'S', Item::slimeBall, 'P', Tile::pistonBase, 'M');

    // 4J Stu - Added some dummy firework recipes to allow us to navigate
    // forward to the fireworks scene
    addShapedRecipy(new ItemInstance(Item::fireworks, 1),  //
                    "sscicig",
                    " P ",  //
                    " G ",  //

                    'P', Item::paper, 'G', Item::gunpowder, 'D');

    addShapedRecipy(new ItemInstance(Item::fireworksCharge, 1),  //
                    "sscicig",
                    " D ",  //
                    " G ",  //

                    'D', Item::dye_powder, 'G', Item::gunpowder, 'D');

    addShapedRecipy(new ItemInstance(Item::fireworksCharge, 1),  //
                    "sscicig",
                    " D ",  //
                    " C ",  //

                    'D', Item::dye_powder, 'C', Item::fireworksCharge, 'D');

    // Sort so the largest recipes get checked first!
    /* 4J-PB - TODO
    Collections.sort(recipies, new Comparator<Recipy>()
    {
    public: int compare(Recipy r0, Recipy r1)
                    {

                            // shapeless recipes are put in the back of the list
                            if (r0 instanceof ShapelessRecipy && r1 instanceof
    ShapedRecipy)
                            {
                                    return 1;
                            }
                            if (r1 instanceof ShapelessRecipy && r0 instanceof
    ShapedRecipy)
                            {
                                    return -1;
                            }

                            if (r1.size() < r0.size()) return -1;
                            if (r1.size() > r0.size()) return 1;
                            return 0;
                    }
    });
    */

    // 4J-PB removed System.out.println(recipies->size() + " recipes");

    // 4J-PB - build the array of ingredients required per recipe
    buildRecipeIngredientsArray();
}

// 4J-PB - this function has been substantially changed due to the differences
// with a va_list of classes in C++ and Java
ShapedRecipy* Recipes::addShapedRecipy(ItemInstance* result, ...) {
    std::string map = "";
    int p = 0;
    int width = 0;
    int height = 0;
    int group = ShapedRecipy::eGroupType_Decoration;
    va_list vl;
    char* wchTypes;
    char* pwchString;
    std::string wString;
    std::string* wStringA;
    ItemInstance* pItemInstance;
    Tile* pTile;
    Item* pItem;
    char wchFrom;
    int iCount;
    ItemInstance** ids = nullptr;

    myMap* mappings = new std::unordered_map<char, ItemInstance*>();

    va_start(vl, result);
    // 4J-PB - second argument is a list of the types
    // s - string
    // w - string array
    // a - char *
    // c - char
    // z - ItemInstance *
    // i - Item *
    // t - Tile *
    // g - group [wt] - which group does the item created by the recipe belong
    // in. Set a default until all recipes have a group

    wchTypes = va_arg(vl, char*);

    for (int i = 0; wchTypes[i] != '\0'; ++i) {
        if (wchTypes[i + 1] == '\0' && wchTypes[i] != 'g') {
            Log::info("Missing group type\n");
        }

        switch (wchTypes[i]) {
            case 'a':
                pwchString = va_arg(vl, char*);
                wString = pwchString;
                height++;
                width = (int)wString.length();
                map += wString;
                break;
            case 's':
                pwchString = va_arg(vl, char*);
                wString = pwchString;
                height++;
                width = (int)wString.length();
                map += wString;
                break;
            case 'w':
                wStringA = va_arg(vl, std::string*);
                iCount = 0;
                do {
                    wString = wStringA[iCount++];
                    if (!wString.empty()) {
                        height++;
                        width = (int)wString.length();
                        map += wString;
                    }
                } while (!wString.empty());

                break;
            case 'c':
                wchFrom = (char)va_arg(vl, int);
                break;
            case 'z':
                pItemInstance = va_arg(vl, ItemInstance*);
                mappings->insert(myMap::value_type(wchFrom, pItemInstance));
                break;
            case 'i':
                pItem = va_arg(vl, Item*);
                pItemInstance = new ItemInstance(pItem, 1, ANY_AUX_VALUE);
                mappings->insert(myMap::value_type(wchFrom, pItemInstance));
                break;
            case 't':
                pTile = va_arg(vl, Tile*);
                pItemInstance = new ItemInstance(pTile, 1, ANY_AUX_VALUE);
                mappings->insert(myMap::value_type(wchFrom, pItemInstance));
                break;
            case 'g':
                wchFrom = (char)va_arg(vl, int);
                switch (wchFrom) {
                        // 			case 'W':
                        // 				group=ShapedRecipy::eGroupType_Weapon;
                        // 				break;
                    case 'T':
                        group = ShapedRecipy::eGroupType_Tool;
                        break;
                    case 'A':
                        group = ShapedRecipy::eGroupType_Armour;
                        break;
                    case 'S':
                        group = ShapedRecipy::eGroupType_Structure;
                        break;
                    case 'V':
                        group = ShapedRecipy::eGroupType_Transport;
                        break;
                    case 'M':
                        group = ShapedRecipy::eGroupType_Mechanism;
                        break;
                    case 'F':
                        group = ShapedRecipy::eGroupType_Food;
                        break;
                    case 'D':
                    default:
                        group = ShapedRecipy::eGroupType_Decoration;
                        break;
                }
                break;
        }

        ids = new ItemInstance*[width * height];

        for (int j = 0; j < width * height; j++) {
            char ch = map[j];
            myMap::iterator it = mappings->find(ch);
            if (it != mappings->end()) {
                ids[j] = it->second;
            } else {
                ids[j] = nullptr;
            }
        }
    }

    va_end(vl);

    ShapedRecipy* recipe = new ShapedRecipy(width, height, ids, result, group);
    recipies->push_back(recipe);
    return recipe;
}

void Recipes::addShapelessRecipy(ItemInstance* result, ...) {
    va_list vl;
    char* szTypes;
    std::string String;
    ItemInstance* pItemInstance;
    Tile* pTile;
    Item* pItem;
    Recipy::_eGroupType group = Recipy::eGroupType_Decoration;
    char wchFrom;
    std::vector<ItemInstance*>* ingredients = new std::vector<ItemInstance*>();

    va_start(vl, result);
    // 4J-PB - second argument is a list of the types
    // z - ItemInstance *
    // i - Item *
    // t - Tile *
    szTypes = va_arg(vl, char*);

    for (int i = 0; szTypes[i] != '\0'; ++i) {
        switch (szTypes[i]) {
            case 'z':
                pItemInstance = va_arg(vl, ItemInstance*);
                // 4J-PB - original code copies the item instance, copy the
                // pointer isnt the same...
                // TODO
                ingredients->push_back(pItemInstance->copy_not_shared());
                break;
            case 'i':
                pItem = va_arg(vl, Item*);
                pItemInstance = new ItemInstance(pItem);
                ingredients->push_back(pItemInstance);
                break;
            case 't':
                pTile = va_arg(vl, Tile*);
                ingredients->push_back(new ItemInstance(pTile));
                break;
            case 'g':
                wchFrom = (char)va_arg(vl, int);
                switch (wchFrom) {
                    case 'T':
                        group = Recipy::eGroupType_Tool;
                        break;
                    case 'A':
                        group = Recipy::eGroupType_Armour;
                        break;
                    case 'S':
                        group = Recipy::eGroupType_Structure;
                        break;
                    case 'V':
                        group = Recipy::eGroupType_Transport;
                        break;
                    case 'M':
                        group = Recipy::eGroupType_Mechanism;
                        break;
                    case 'F':
                        group = Recipy::eGroupType_Food;
                        break;
                    case 'D':
                    default:
                        group = Recipy::eGroupType_Decoration;
                        break;
                }
                break;
        }
    }

    recipies->push_back(new ShapelessRecipy(result, ingredients, group));
}

std::shared_ptr<ItemInstance> Recipes::getItemFor(
    std::shared_ptr<CraftingContainer> craftSlots, Level* level,
    Recipy* recipesClass /*= nullptr*/) {
    int count = 0;
    std::shared_ptr<ItemInstance> first = nullptr;
    std::shared_ptr<ItemInstance> second = nullptr;
    for (int i = 0; i < craftSlots->getContainerSize(); i++) {
        std::shared_ptr<ItemInstance> item = craftSlots->getItem(i);
        if (item != nullptr) {
            if (count == 0) first = item;
            if (count == 1) second = item;
            count++;
        }
    }

    if (count == 2 && first->id == second->id && first->count == 1 &&
        second->count == 1 && Item::items[first->id]->canBeDepleted()) {
        Item* item = Item::items[first->id];
        int remaining1 = item->getMaxDamage() - first->getDamageValue();
        int remaining2 = item->getMaxDamage() - second->getDamageValue();
        int remaining =
            (remaining1 + remaining2) + item->getMaxDamage() * 5 / 100;
        int resultDamage = item->getMaxDamage() - remaining;
        if (resultDamage < 0) resultDamage = 0;
        return std::shared_ptr<ItemInstance>(
            new ItemInstance(first->id, 1, resultDamage));
    }

    if (recipesClass != nullptr) {
        if (recipesClass->matches(craftSlots, level))
            return recipesClass->assemble(craftSlots);
    } else {
        auto itEnd = recipies->end();
        for (auto it = recipies->begin(); it != itEnd; it++) {
            Recipy* r = *it;  // recipies->at(i);
            if (r->matches(craftSlots, level)) return r->assemble(craftSlots);
        }
    }
    return nullptr;
}

std::vector<Recipy*>* Recipes::getRecipies() { return recipies; }

// 4J-PB - added to deal with Xb0x 'crafting'
std::shared_ptr<ItemInstance> Recipes::getItemForRecipe(Recipy* r) {
    return r->assemble(nullptr);
}

// 4J-PB - build the required ingredients for recipes
void Recipes::buildRecipeIngredientsArray(void) {
    // std::vector<Recipy*> *recipes = ((Recipes
    // *)Recipes::getInstance())->getRecipies();

    int iRecipeC = (int)recipies->size();

    m_pRecipeIngredientsRequired = new Recipy::INGREDIENTS_REQUIRED[iRecipeC];

    int iCount = 0;
    auto itEndRec = recipies->end();
    for (auto it = recipies->begin(); it != itEndRec; it++) {
        Recipy* recipe = *it;
        // printf("RECIPE - [%d] is
        // %w\n",iCount,recipe->getResultItem()->getItem()->getName());
        recipe->collectRequirements(&m_pRecipeIngredientsRequired[iCount++]);
    }

    // printf("Total recipes in buildRecipeIngredientsArray - %d",iCount);
}

Recipy::INGREDIENTS_REQUIRED* Recipes::getRecipeIngredientsArray(void) {
    return m_pRecipeIngredientsRequired;
}