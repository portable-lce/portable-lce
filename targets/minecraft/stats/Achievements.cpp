#include "Achievements.h"

#include <limits>
#include <string>
#include <vector>

#include "Achievement.h"
#include "minecraft/stats/Console_Awards_enum.h"
#include "minecraft/world/item/BowItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/level/tile/Tile.h"

const int Achievements::ACHIEVEMENT_OFFSET = 0x500000;

// maximum position of achievements (min and max)

int Achievements::xMin = std::numeric_limits<int>::max();
int Achievements::yMin = std::numeric_limits<int>::max();
int Achievements::xMax = 0;
int Achievements::yMax = 0;

std::vector<Achievement*>* Achievements::achievements =
    new std::vector<Achievement*>;

Achievement* Achievements::openInventory = nullptr;
Achievement* Achievements::mineWood = nullptr;
Achievement* Achievements::buildWorkbench = nullptr;
Achievement* Achievements::buildPickaxe = nullptr;
Achievement* Achievements::buildFurnace = nullptr;
Achievement* Achievements::acquireIron = nullptr;
Achievement* Achievements::buildHoe = nullptr;
Achievement* Achievements::makeBread = nullptr;
Achievement* Achievements::bakeCake = nullptr;
Achievement* Achievements::buildBetterPickaxe = nullptr;
Achievement* Achievements::cookFish = nullptr;
Achievement* Achievements::onARail = nullptr;
Achievement* Achievements::buildSword = nullptr;
Achievement* Achievements::killEnemy = nullptr;
Achievement* Achievements::killCow = nullptr;
Achievement* Achievements::flyPig = nullptr;

Achievement* Achievements::snipeSkeleton = nullptr;
Achievement* Achievements::diamonds = nullptr;
// Achievement *Achievements::portal = nullptr;
Achievement* Achievements::ghast = nullptr;
Achievement* Achievements::blazeRod = nullptr;
Achievement* Achievements::potion = nullptr;
Achievement* Achievements::theEnd = nullptr;
Achievement* Achievements::winGame = nullptr;
Achievement* Achievements::enchantments = nullptr;
// Achievement *Achievements::overkill = nullptr;
// Achievement *Achievements::bookcase = nullptr;

// 4J : WESTY : Added new acheivements.
Achievement* Achievements::leaderOfThePack = nullptr;
Achievement* Achievements::MOARTools = nullptr;
Achievement* Achievements::dispenseWithThis = nullptr;
Achievement* Achievements::InToTheNether = nullptr;

// 4J : WESTY : Added other awards.
Achievement* Achievements::socialPost = nullptr;
Achievement* Achievements::eatPorkChop = nullptr;
Achievement* Achievements::play100Days = nullptr;
Achievement* Achievements::arrowKillCreeper = nullptr;
Achievement* Achievements::mine100Blocks = nullptr;
Achievement* Achievements::kill10Creepers = nullptr;

#if defined(_EXTENDED_ACHIEVEMENTS)
Achievement* Achievements::overkill = nullptr;  // Restored old achivements.
Achievement* Achievements::bookcase = nullptr;  // Restored old achivements.

// 4J-JEV: New Achievements for Orbis.
Achievement* Achievements::adventuringTime = nullptr;
Achievement* Achievements::repopulation = nullptr;
// Achievement *Achievements::porkChop = nullptr;
Achievement* Achievements::diamondsToYou = nullptr;
// Achievement *Achievements::passingTheTime = nullptr;
// Achievement *Achievements::archer = nullptr;
Achievement* Achievements::theHaggler = nullptr;
Achievement* Achievements::potPlanter = nullptr;
Achievement* Achievements::itsASign = nullptr;
Achievement* Achievements::ironBelly = nullptr;
Achievement* Achievements::haveAShearfulDay = nullptr;
Achievement* Achievements::rainbowCollection = nullptr;
Achievement* Achievements::stayinFrosty = nullptr;
Achievement* Achievements::chestfulOfCobblestone = nullptr;
Achievement* Achievements::renewableEnergy = nullptr;
Achievement* Achievements::musicToMyEars = nullptr;
Achievement* Achievements::bodyGuard = nullptr;
Achievement* Achievements::ironMan = nullptr;
Achievement* Achievements::zombieDoctor = nullptr;
Achievement* Achievements::lionTamer = nullptr;
#endif

void Achievements::staticCtor() {
    Achievements::openInventory =
        (new Achievement(eAward_TakingInventory, "openInventory", 0, 0,
                         Item::book, nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::mineWood =
        (new Achievement(eAward_GettingWood, "mineWood", 2, 1, Tile::treeTrunk,
                         (Achievement*)openInventory))
            ->postConstruct();
    Achievements::buildWorkbench =
        (new Achievement(eAward_Benchmarking, "buildWorkBench", 4, -1,
                         Tile::workBench, (Achievement*)mineWood))
            ->postConstruct();
    Achievements::buildPickaxe =
        (new Achievement(eAward_TimeToMine, "buildPickaxe", 4, 2,
                         Item::pickAxe_wood, (Achievement*)buildWorkbench))
            ->postConstruct();
    Achievements::buildFurnace =
        (new Achievement(eAward_HotTopic, "buildFurnace", 3, 4,
                         Tile::furnace_lit, (Achievement*)buildPickaxe))
            ->postConstruct();
    Achievements::acquireIron =
        (new Achievement(eAward_AquireHardware, "acquireIron", 1, 4,
                         Item::ironIngot, (Achievement*)buildFurnace))
            ->postConstruct();
    Achievements::buildHoe =
        (new Achievement(eAward_TimeToFarm, "buildHoe", 2, -3, Item::hoe_wood,
                         (Achievement*)buildWorkbench))
            ->postConstruct();
    Achievements::makeBread =
        (new Achievement(eAward_BakeBread, "makeBread", -1, -3, Item::bread,
                         (Achievement*)buildHoe))
            ->postConstruct();
    Achievements::bakeCake =
        (new Achievement(eAward_TheLie, "bakeCake", 0, -5, Item::cake,
                         (Achievement*)buildHoe))
            ->postConstruct();
    Achievements::buildBetterPickaxe =
        (new Achievement(eAward_GettingAnUpgrade, "buildBetterPickaxe", 6, 2,
                         Item::pickAxe_stone, (Achievement*)buildPickaxe))
            ->postConstruct();
    Achievements::cookFish =
        (new Achievement(eAward_DeliciousFish, "cookFish", 2, 6,
                         Item::fish_cooked, (Achievement*)buildFurnace))
            ->postConstruct();
    Achievements::onARail =
        (new Achievement(eAward_OnARail, "onARail", 2, 3, Tile::rail,
                         (Achievement*)acquireIron))
            ->setGolden()
            ->postConstruct();
    Achievements::buildSword =
        (new Achievement(eAward_TimeToStrike, "buildSword", 6, -1,
                         Item::sword_wood, (Achievement*)buildWorkbench))
            ->postConstruct();
    Achievements::killEnemy =
        (new Achievement(eAward_MonsterHunter, "killEnemy", 8, -1, Item::bone,
                         (Achievement*)buildSword))
            ->postConstruct();
    Achievements::killCow =
        (new Achievement(eAward_CowTipper, "killCow", 7, -3, Item::leather,
                         (Achievement*)buildSword))
            ->postConstruct();
    Achievements::flyPig =
        (new Achievement(eAward_WhenPigsFly, "flyPig", 8, -4, Item::saddle,
                         (Achievement*)killCow))
            ->setGolden()
            ->postConstruct();

    // 4J Stu - The order of these achievemnts is very important, as they map
    // directly to data stored in the profile data. New achievements should be
    // added at the end.

    // 4J : WESTY : Added new achievements. Note, params "x", "y", "icon" and
    // "requires" are ignored on xbox.
    Achievements::leaderOfThePack =
        (new Achievement(eAward_LeaderOfThePack, "leaderOfThePack", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::MOARTools =
        (new Achievement(eAward_MOARTools, "MOARTools", 0, 0, Tile::treeTrunk,
                         (Achievement*)buildSword))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::dispenseWithThis =
        (new Achievement(eAward_DispenseWithThis, "dispenseWithThis", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->postConstruct();
    Achievements::InToTheNether =
        (new Achievement(eAward_InToTheNether, "InToTheNether", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->postConstruct();

    // 4J : WESTY : Added other awards.
    Achievements::mine100Blocks =
        (new Achievement(eAward_mine100Blocks, "mine100Blocks", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::kill10Creepers =
        (new Achievement(eAward_kill10Creepers, "kill10Creepers", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->setAwardLocallyOnly()
            ->postConstruct();
#if defined(_EXTENDED_ACHIEVEMENTS)
    Achievements::eatPorkChop =
        (new Achievement(eAward_eatPorkChop, "eatPorkChop", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->setAwardLocallyOnly()
            ->postConstruct();
#else
    Achievements::eatPorkChop =
        (new Achievement(eAward_eatPorkChop, "eatPorkChop", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->postConstruct();
#endif
    Achievements::play100Days =
        (new Achievement(eAward_play100Days, "play100Days", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::arrowKillCreeper =
        (new Achievement(eAward_arrowKillCreeper, "arrowKillCreeper", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->postConstruct();
    Achievements::socialPost =
        (new Achievement(eAward_socialPost, "socialPost", 0, 0,
                         Tile::treeTrunk, (Achievement*)buildSword))
            ->postConstruct();

    // WARNING: NO NEW ACHIEVMENTS CAN BE ADDED HERE
    // These stats (achievements) are directly followed by new
    // stats/achievements in the profile data, so cannot be changed without
    // migrating the profile data

    // 4J Stu - All new Java achievements removed to stop them using the profile
    // data

    // 4J Stu - This achievment added in 1.8.2, but does not map to any Xbox
    // achievements
    Achievements::snipeSkeleton =
        (new Achievement(eAward_snipeSkeleton, "snipeSkeleton", 7, 0,
                         Item::bow, (Achievement*)killEnemy))
            ->setGolden()
            ->postConstruct();

    // 4J Stu - These added in 1.0.1, but do not map to any Xbox achievements
    Achievements::diamonds =
        (new Achievement(eAward_diamonds, "diamonds", -1, 5, Item::diamond,
                         (Achievement*)acquireIron))
            ->postConstruct();
    // Achievements::portal				= (new
    // Achievement(eAward_portal,				"portal",
    // -1, 7,	Tile::obsidian,		(Achievement *)diamonds)
    // )->postConstruct();
    Achievements::ghast =
        (new Achievement(eAward_ghast, "ghast", -4, 8, Item::ghastTear,
                         (Achievement*)ghast))
            ->setGolden()
            ->postConstruct();
    Achievements::blazeRod =
        (new Achievement(eAward_blazeRod, "blazeRod", 0, 9, Item::blazeRod,
                         (Achievement*)blazeRod))
            ->postConstruct();
    Achievements::potion = (new Achievement(eAward_potion, "potion", 2, 8,
                                            Item::potion, (Achievement*)potion))
                               ->postConstruct();
    Achievements::theEnd =
        (new Achievement(eAward_theEnd, "theEnd", 3, 10, Item::eyeOfEnder,
                         (Achievement*)theEnd))
            ->setGolden()
            ->postConstruct();
    Achievements::winGame =
        (new Achievement(eAward_winGame, "theEnd2", 4, 13, Tile::dragonEgg,
                         (Achievement*)winGame))
            ->setGolden()
            ->postConstruct();
    Achievements::enchantments =
        (new Achievement(eAward_enchantments, "enchantments", -4, 4,
                         Tile::enchantTable, (Achievement*)enchantments))
            ->postConstruct();
    //   Achievements::overkill				= (new
    //   Achievement(eAward_overkill,				"overkill",
    //   -4, 1,	Item::sword_diamond, (Achievement *)enchantments)
    //   )->setGolden()->postConstruct(); Achievements::bookcase
    //   = (new Achievement(eAward_bookcase,
    //   "bookcase",			-3, 6,	Tile::bookshelf,
    //   (Achievement *)enchantments) )->postConstruct();

#if defined(_EXTENDED_ACHIEVEMENTS)
    Achievements::overkill =
        (new Achievement(eAward_overkill, "overkill", -4, 1,
                         Item::sword_diamond, (Achievement*)enchantments))
            ->setGolden()
            ->postConstruct();
    Achievements::bookcase =
        (new Achievement(eAward_bookcase, "bookcase", -3, 6, Tile::bookshelf,
                         (Achievement*)enchantments))
            ->postConstruct();

    Achievements::adventuringTime =
        (new Achievement(eAward_adventuringTime, "adventuringTime", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::repopulation =
        (new Achievement(eAward_repopulation, "repopulation", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    // Achievements::porkChoop			// // //
    // // // //
    Achievements::diamondsToYou =
        (new Achievement(eAward_diamondsToYou, "diamondsToYou", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    // Achievements::passingTheTime		= (new
    // Achievement(eAward_play100Days, "passingTheTime",
    // 0,0,	Tile::bookshelf,		(Achievement*) nullptr)
    // )->postConstruct(); Achievements::archer				= (new
    // Achievement(eAward_arrowKillCreeper,			"archer",
    // 0,0,	Tile::bookshelf,		(Achievement*) nullptr)
    // )->postConstruct();
    Achievements::theHaggler =
        (new Achievement(eAward_theHaggler, "theHaggler", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::potPlanter =
        (new Achievement(eAward_potPlanter, "potPlanter", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::itsASign =
        (new Achievement(eAward_itsASign, "itsASign", 0, 0, Tile::bookshelf,
                         (Achievement*)nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::ironBelly =
        (new Achievement(eAward_ironBelly, "ironBelly", 0, 0, Tile::bookshelf,
                         (Achievement*)nullptr))
            ->postConstruct();
    Achievements::haveAShearfulDay =
        (new Achievement(eAward_haveAShearfulDay, "haveAShearfulDay", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    Achievements::rainbowCollection =
        (new Achievement(eAward_rainbowCollection, "rainbowCollection", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::stayinFrosty =
        (new Achievement(eAward_stayinFrosty, "stayingFrosty", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    Achievements::chestfulOfCobblestone =
        (new Achievement(eAward_chestfulOfCobblestone, "chestfulOfCobblestone",
                         0, 0, Tile::bookshelf, (Achievement*)nullptr))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Achievements::renewableEnergy =
        (new Achievement(eAward_renewableEnergy, "renewableEnergy", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    Achievements::musicToMyEars =
        (new Achievement(eAward_musicToMyEars, "musicToMyEars", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    Achievements::bodyGuard =
        (new Achievement(eAward_bodyGuard, "bodyGuard", 0, 0, Tile::bookshelf,
                         (Achievement*)nullptr))
            ->postConstruct();
    Achievements::ironMan =
        (new Achievement(eAward_ironMan, "ironMan", 0, 0, Tile::bookshelf,
                         (Achievement*)nullptr))
            ->postConstruct();
    Achievements::zombieDoctor =
        (new Achievement(eAward_zombieDoctor, "zombieDoctor", 0, 0,
                         Tile::bookshelf, (Achievement*)nullptr))
            ->postConstruct();
    Achievements::lionTamer =
        (new Achievement(eAward_lionTamer, "lionTamer", 0, 0, Tile::bookshelf,
                         (Achievement*)nullptr))
            ->postConstruct();
#endif
}

// Static { System.out.println(achievements.size() + " achievements"); }
// TODO

void Achievements::init() {}
