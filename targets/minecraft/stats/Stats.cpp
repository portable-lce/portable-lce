#include "Stats.h"

#include <string>
#include <vector>

#include "Achievements.h"
#include "GeneralStat.h"
#include "ItemStat.h"
#include "minecraft/stats/Stat.h"
#include "minecraft/stats/StatsCounter.h"
#include "minecraft/world/item/FishingRodItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/MapItem.h"
#include "minecraft/world/level/tile/GrassTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "util/StringHelpers.h"

class StatFormatter;

const int Stats::BLOCKS_MINED_OFFSET = 0x1000000;
const int Stats::ITEMS_COLLECTED_OFFSET = 0x1010000;
const int Stats::ITEMS_CRAFTED_OFFSET = 0x1020000;
const int Stats::ADDITIONAL_STATS_OFFSET =
    0x5010000;  // Needs to be higher than Achievements::ACHIEVEMENT_OFFSET =
                // 0x500000;

std::unordered_map<int, Stat*>* Stats::statsById =
    new std::unordered_map<int, Stat*>;

std::vector<Stat*>* Stats::all = new std::vector<Stat*>;
std::vector<Stat*>* Stats::generalStats = new std::vector<Stat*>;
std::vector<ItemStat*>* Stats::blocksMinedStats = new std::vector<ItemStat*>;
std::vector<ItemStat*>* Stats::itemsCollectedStats = new std::vector<ItemStat*>;
std::vector<ItemStat*>* Stats::itemsCraftedStats = new std::vector<ItemStat*>;

#if defined(_EXTENDED_ACHIEVEMENTS)
std::vector<ItemStat*>* Stats::blocksPlacedStats = new std::vector<ItemStat*>;
#endif

Stat* Stats::walkOneM = nullptr;
Stat* Stats::swimOneM = nullptr;
Stat* Stats::fallOneM = nullptr;
Stat* Stats::climbOneM = nullptr;
Stat* Stats::minecartOneM = nullptr;
Stat* Stats::boatOneM = nullptr;
Stat* Stats::pigOneM = nullptr;
Stat* Stats::portalsCreated = nullptr;
Stat* Stats::cowsMilked = nullptr;
Stat* Stats::netherLavaCollected = nullptr;
Stat* Stats::killsZombie = nullptr;
Stat* Stats::killsSkeleton = nullptr;
Stat* Stats::killsCreeper = nullptr;
Stat* Stats::killsSpider = nullptr;
Stat* Stats::killsSpiderJockey = nullptr;
Stat* Stats::killsZombiePigman = nullptr;
Stat* Stats::killsSlime = nullptr;
Stat* Stats::killsGhast = nullptr;
Stat* Stats::killsNetherZombiePigman = nullptr;

// 4J : WESTY : Added for new achievements.
Stat* Stats::befriendsWolf = nullptr;
Stat* Stats::totalBlocksMined = nullptr;
Stat* Stats::timePlayed = nullptr;

std::vector<Stat*> Stats::blocksMined;
std::vector<Stat*> Stats::itemsCollected;
std::vector<Stat*> Stats::itemsCrafted;

#if defined(_EXTENDED_ACHIEVEMENTS)
std::vector<Stat*> Stats::blocksPlaced;
std::vector<Stat*> Stats::rainbowCollection;
std::vector<Stat*> Stats::biomesVisisted;
#endif

Stat* Stats::killsEnderdragon =
    nullptr;  // The number of times this player has dealt the killing blow to
              // the Enderdragon
Stat* Stats::completeTheEnd =
    nullptr;  // The number of times this player has been
              // present when the Enderdragon has died

void Stats::staticCtor() {
    Stats::walkOneM = (new GeneralStat(2000, "stat.walkOneM",
                                       (StatFormatter*)Stat::distanceFormatter))
                          ->setAwardLocallyOnly()
                          ->postConstruct();
    Stats::swimOneM = (new GeneralStat(2001, "stat.swimOneM",
                                       (StatFormatter*)Stat::distanceFormatter))
                          ->setAwardLocallyOnly()
                          ->postConstruct();
    Stats::fallOneM = (new GeneralStat(2002, "stat.fallOneM",
                                       (StatFormatter*)Stat::distanceFormatter))
                          ->setAwardLocallyOnly()
                          ->postConstruct();
    Stats::climbOneM =
        (new GeneralStat(2003, "stat.climbOneM",
                         (StatFormatter*)Stat::distanceFormatter))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Stats::minecartOneM =
        (new GeneralStat(2004, "stat.minecartOneM",
                         (StatFormatter*)Stat::distanceFormatter))
            ->setAwardLocallyOnly()
            ->postConstruct();
    Stats::boatOneM = (new GeneralStat(2005, "stat.boatOneM",
                                       (StatFormatter*)Stat::distanceFormatter))
                          ->setAwardLocallyOnly()
                          ->postConstruct();
    Stats::pigOneM = (new GeneralStat(2006, "stat.pigOneM",
                                      (StatFormatter*)Stat::distanceFormatter))
                         ->setAwardLocallyOnly()
                         ->postConstruct();
    Stats::portalsCreated =
        (new GeneralStat(2007, "stat.portalsUsed"))->postConstruct();
    Stats::cowsMilked =
        (new GeneralStat(2008, "stat.cowsMilked"))->postConstruct();
    Stats::netherLavaCollected =
        (new GeneralStat(2009, "stat.netherLavaCollected"))->postConstruct();
    Stats::killsZombie =
        (new GeneralStat(2010, "stat.killsZombie"))->postConstruct();
    Stats::killsSkeleton =
        (new GeneralStat(2011, "stat.killsSkeleton"))->postConstruct();
    Stats::killsCreeper =
        (new GeneralStat(2012, "stat.killsCreeper"))->postConstruct();
    Stats::killsSpider =
        (new GeneralStat(2013, "stat.killsSpider"))->postConstruct();
    Stats::killsSpiderJockey =
        (new GeneralStat(2014, "stat.killsSpiderJockey"))->postConstruct();
    Stats::killsZombiePigman =
        (new GeneralStat(2015, "stat.killsZombiePigman"))->postConstruct();
    Stats::killsSlime =
        (new GeneralStat(2016, "stat.killsSlime"))->postConstruct();
    Stats::killsGhast =
        (new GeneralStat(2017, "stat.killsGhast"))->postConstruct();
    Stats::killsNetherZombiePigman =
        (new GeneralStat(2018, "stat.killsNetherZombiePigman"))
            ->postConstruct();

    // 4J : WESTY : Added for new achievements.
    Stats::befriendsWolf =
        (new GeneralStat(2019, "stat.befriendsWolf"))->postConstruct();
    Stats::totalBlocksMined =
        (new GeneralStat(2020, "stat.totalBlocksMined"))->postConstruct();

    // 4J-PB - don't want the time played going to the server
    Stats::timePlayed = (new GeneralStat(2021, "stat.timePlayed"))
                            ->setAwardLocallyOnly()
                            ->postConstruct();

    // WARNING: NO NEW STATS CAN BE ADDED HERE
    // These stats are directly followed by the achievemnts in the profile data,
    // so cannot be changed without migrating the profile data

    buildBlockStats();

    Achievements::init();
    Achievements::staticCtor();

    // 4J Stu - Added this function to allow us to add news stats from TU9
    // onwards
    buildAdditionalStats();
}

void Stats::init() {}

bool Stats::blockStatsLoaded = false;

// WARNING: NO NEW STATS CAN BE ADDED HERE
// These stats are directly followed by the achievemnts in the profile data, so
// cannot be changed without migrating the profile data
void Stats::buildBlockStats() {
    blocksMined = std::vector<Stat*>(32000);

    ItemStat* newStat =
        new ItemStat(BLOCKS_MINED_OFFSET + 0, "mineBlock.dirt", Tile::dirt->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::dirt->id] = newStat;
    blocksMined[Tile::grass->id] = newStat;
    blocksMined[Tile::farmland->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 1, "mineBlock.stone",
                           Tile::cobblestone->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::cobblestone->id] = newStat;
    newStat->postConstruct();

    newStat =
        new ItemStat(BLOCKS_MINED_OFFSET + 2, "mineBlock.sand", Tile::sand->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::sand->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 3, "mineBlock.cobblestone",
                           Tile::stone->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::stone->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 4, "mineBlock.gravel",
                           Tile::gravel->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::gravel->id] = newStat;
    newStat->postConstruct();

    newStat =
        new ItemStat(BLOCKS_MINED_OFFSET + 5, "mineBlock.clay", Tile::clay->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::clay->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 6, "mineBlock.obsidian",
                           Tile::obsidian->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::obsidian->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 7, "mineBlock.coal",
                           Tile::coalOre->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::coalOre->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 8, "mineBlock.iron",
                           Tile::ironOre->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::ironOre->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 9, "mineBlock.gold",
                           Tile::goldOre->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::goldOre->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 10, "mineBlock.diamond",
                           Tile::diamondOre->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::diamondOre->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 11, "mineBlock.redstone",
                           Tile::redStoneOre->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::redStoneOre->id] = newStat;
    blocksMined[Tile::redStoneOre_lit->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 12, "mineBlock.lapisLazuli",
                           Tile::lapisOre->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::lapisOre->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 13, "mineBlock.netherrack",
                           Tile::netherRack->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::netherRack->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 14, "mineBlock.soulSand",
                           Tile::soulsand->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::soulsand->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 15, "mineBlock.glowstone",
                           Tile::glowstone->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::glowstone->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 16, "mineBlock.wood",
                           Tile::treeTrunk->id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::treeTrunk->id] = newStat;
    newStat->postConstruct();

    // WARNING: NO NEW STATS CAN BE ADDED HERE
    // These stats are directly followed by the achievemnts in the profile data,
    // so cannot be changed without migrating the profile data

    blockStatsLoaded = true;
    buildCraftableStats();
}

bool Stats::itemStatsLoaded = false;

void Stats::buildItemStats() {
    itemStatsLoaded = true;
    buildCraftableStats();
}

bool Stats::craftableStatsLoaded = false;

// WARNING: NO NEW STATS CAN BE ADDED HERE
// These stats are directly followed by the achievemnts in the profile data, so
// cannot be changed without migrating the profile data
void Stats::buildCraftableStats() {
    if (!blockStatsLoaded || !itemStatsLoaded || craftableStatsLoaded) {
        // still waiting for the JVM to load stuff
        // Or stats already loaded
        return;
    }

    craftableStatsLoaded = true;

    // Collected stats

    itemsCollected = std::vector<Stat*>(32000);

    ItemStat* newStat = new ItemStat(ITEMS_COLLECTED_OFFSET + 0,
                                     "collectItem.egg", Item::egg->id);
    itemsCollectedStats->push_back(newStat);
    itemsCollected[Item::egg->id] = newStat;
    newStat->postConstruct();

    // 4J Stu - The following stats were added as it was too easy to cheat the
    // leaderboards by dropping and picking up these items They are now changed
    // to mining the block which involves a tiny bit more effort
    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 18, "mineBlock.wheat",
                           Tile::wheat_Id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::wheat_Id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 19, "mineBlock.mushroom1",
                           Tile::mushroom_brown_Id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::mushroom_brown_Id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(BLOCKS_MINED_OFFSET + 17, "mineBlock.sugar",
                           Tile::reeds_Id);
    blocksMinedStats->push_back(newStat);
    blocksMined[Tile::reeds_Id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_COLLECTED_OFFSET + 4, "collectItem.pumpkin",
                           Tile::pumpkin->id);
    itemsCollectedStats->push_back(newStat);
    itemsCollected[Tile::pumpkin->id] = newStat;
    itemsCollected[Tile::litPumpkin->id] = newStat;
    newStat->postConstruct();

    // Crafted stats

    itemsCrafted = std::vector<Stat*>(32000);

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 0, "craftItem.plank",
                           Tile::wood->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Tile::wood->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 1, "craftItem.workbench",
                           Tile::workBench->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Tile::workBench->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 2, "craftItem.stick",
                           Item::stick->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::stick->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 3, "craftItem.woodenShovel",
                           Item::shovel_wood->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::shovel_wood->id] = newStat;
    newStat->postConstruct();

    // 4J : WESTY : Added for new achievements.
    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 4, "craftItem.woodenPickAxe",
                           Item::pickAxe_wood->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::pickAxe_wood->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 5, "craftItem.stonePickAxe",
                           Item::pickAxe_stone->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::pickAxe_stone->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 6, "craftItem.ironPickAxe",
                           Item::pickAxe_iron->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::pickAxe_iron->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 7, "craftItem.diamondPickAxe",
                           Item::pickAxe_diamond->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::pickAxe_diamond->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 8, "craftItem.goldPickAxe",
                           Item::pickAxe_gold->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::pickAxe_gold->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 9, "craftItem.stoneShovel",
                           Item::shovel_stone->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::shovel_stone->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 10, "craftItem.ironShovel",
                           Item::shovel_iron->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::shovel_iron->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 11, "craftItem.diamondShovel",
                           Item::shovel_diamond->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::shovel_diamond->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 12, "craftItem.goldShovel",
                           Item::shovel_gold->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::shovel_gold->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 13, "craftItem.woodenAxe",
                           Item::hatchet_wood->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hatchet_wood->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 14, "craftItem.stoneAxe",
                           Item::hatchet_stone->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hatchet_stone->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 15, "craftItem.ironAxe",
                           Item::hatchet_iron->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hatchet_iron->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 16, "craftItem.diamondAxe",
                           Item::hatchet_diamond->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hatchet_diamond->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 17, "craftItem.goldAxe",
                           Item::hatchet_gold->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hatchet_gold->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 18, "craftItem.woodenHoe",
                           Item::hoe_wood->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hoe_wood->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 19, "craftItem.stoneHoe",
                           Item::hoe_stone->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hoe_stone->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 20, "craftItem.ironHoe",
                           Item::hoe_iron->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hoe_iron->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 21, "craftItem.diamondHoe",
                           Item::hoe_diamond->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hoe_diamond->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 22, "craftItem.goldHoe",
                           Item::hoe_gold->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::hoe_gold->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 23, "craftItem.glowstone",
                           Tile::glowstone_Id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Tile::glowstone_Id] = newStat;
    newStat->postConstruct();

    newStat =
        new ItemStat(ITEMS_CRAFTED_OFFSET + 24, "craftItem.tnt", Tile::tnt_Id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Tile::tnt_Id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 25, "craftItem.bowl",
                           Item::bowl->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::bowl->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 26, "craftItem.bucket",
                           Item::bucket_empty->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::bucket_empty->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 27, "craftItem.flintAndSteel",
                           Item::flintAndSteel->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::flintAndSteel->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 28, "craftItem.fishingRod",
                           Item::fishingRod->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::fishingRod->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 29, "craftItem.clock",
                           Item::clock->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::clock->id] = newStat;
    newStat->postConstruct();

    newStat = new ItemStat(ITEMS_CRAFTED_OFFSET + 30, "craftItem.compass",
                           Item::compass->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::compass->id] = newStat;
    newStat->postConstruct();

    newStat =
        new ItemStat(ITEMS_CRAFTED_OFFSET + 31, "craftItem.map", Item::map->id);
    itemsCraftedStats->push_back(newStat);
    itemsCrafted[Item::map->id] = newStat;
    newStat->postConstruct();

    // WARNING: NO NEW STATS CAN BE ADDED HERE
    // These stats are directly followed by the achievemnts in the profile data,
    // so cannot be changed without migrating the profile data

    // This sets up a static list of stat/leaderboard pairings, used to tell
    // which leaderboards need an update
    StatsCounter::setupStatBoards();
}

// 4J Stu - Added this function to allow us to add news stats from TU9 onwards
void Stats::buildAdditionalStats() {
    int offset = ADDITIONAL_STATS_OFFSET;

    // The order of these stats should not be changed, as the map directly to
    // bits in the profile data

    // The number of times this player has dealt the killing blow to the
    // Enderdragon
    Stats::killsEnderdragon =
        (new GeneralStat(offset++, "stat.killsEnderdragon"))->postConstruct();

    // The number of times this player has been present when the Enderdragon has
    // died
    Stats::completeTheEnd =
        (new GeneralStat(offset++, "stat.completeTheEnd"))->postConstruct();

#if defined(_EXTENDED_ACHIEVEMENTS)
    {
        ItemStat* itemStat =
            new ItemStat(offset++, "craftItem.flowerPot", Item::flowerPot_Id);
        itemsCraftedStats->push_back(itemStat);
        itemsCrafted[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        itemStat = new ItemStat(offset++, "craftItem.sign", Item::sign_Id);
        itemsCraftedStats->push_back(itemStat);
        itemsCrafted[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        itemStat =
            new ItemStat(offset++, "mineBlock.emerald", Tile::emeraldOre_Id);
        blocksMinedStats->push_back(itemStat);
        blocksMined[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        // 4J-JEV: We don't need itemsCollected(emerald) so I'm using it to
        // stor itemsBought(emerald) so I don't have to make yet another massive
        // std::vector<Stat*>& for Items Bought.
        itemStat =
            new ItemStat(offset++, "itemsBought.emerald", Item::emerald_Id);
        itemsCollectedStats->push_back(itemStat);
        itemsCollected[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        // 4J-JEV:	WHY ON EARTH DO THESE ARRAYS HAVE TO BE SO PAINFULLY
        // LARGE WHEN THEY ARE GOING TO BE MOSTLY EMPTY!!!
        //			Either way, I'm making this one smaller because
        // we don't need those record items (and we only need 2).
        blocksPlaced = std::vector<Stat*>(1000);

        itemStat =
            new ItemStat(offset++, "blockPlaced.flowerPot", Tile::flowerPot_Id);
        blocksPlacedStats->push_back(itemStat);
        blocksPlaced[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        itemStat = new ItemStat(offset++, "blockPlaced.sign", Tile::sign_Id);
        blocksPlacedStats->push_back(itemStat);
        blocksPlaced[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        itemStat =
            new ItemStat(offset++, "blockPlaced.wallsign", Tile::wallSign_Id);
        blocksPlacedStats->push_back(itemStat);
        blocksPlaced[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        GeneralStat* generalStat = nullptr;

        rainbowCollection = std::vector<Stat*>(16);
        for (unsigned int i = 0; i < 16; i++) {
            generalStat = new GeneralStat(
                offset++, "rainbowCollection." + toWString<unsigned int>(i));
            generalStats->push_back(generalStat);
            rainbowCollection[i] = generalStat;
            generalStat->postConstruct();
        }

        biomesVisisted = std::vector<Stat*>(23);
        for (unsigned int i = 0; i < 23; i++) {
            generalStat = new GeneralStat(
                offset++, "biomesVisited." + toWString<unsigned int>(i));
            generalStats->push_back(generalStat);
            biomesVisisted[i] = generalStat;
            generalStat->postConstruct();
        }

        itemStat = new ItemStat(offset++, "itemCrafted.porkchop",
                                Item::porkChop_cooked_Id);
        itemsCraftedStats->push_back(itemStat);
        itemsCrafted[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();

        itemStat = new ItemStat(offset++, "itemEaten.porkchop",
                                Item::porkChop_cooked_Id);
        blocksPlacedStats->push_back(itemStat);
        blocksPlaced[itemStat->getItemId()] = itemStat;
        itemStat->postConstruct();
    }
#endif
}

Stat* Stats::get(int key) { return statsById->at(key); }
