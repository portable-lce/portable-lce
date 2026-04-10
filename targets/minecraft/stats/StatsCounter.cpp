#include "StatsCounter.h"

#include <assert.h>
#include <limits.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "app/common/App_structs.h"
#include "app/common/Game.h"
#include "minecraft/stats/Achievement.h"
#include "minecraft/stats/Achievements.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/stats/Stat.h"
#include "minecraft/stats/Stats.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/leaderboard/IPlatformLeaderboard.h"
#include "platform/profile/profile.h"

Stat** StatsCounter::LARGE_STATS[] = {&Stats::walkOneM,     &Stats::swimOneM,
                                      &Stats::fallOneM,     &Stats::climbOneM,
                                      &Stats::minecartOneM, &Stats::boatOneM,
                                      &Stats::pigOneM,      &Stats::timePlayed};

std::unordered_map<Stat*, int> StatsCounter::statBoards;

StatsCounter::StatsCounter(IPlatformLeaderboard& leaderboard)
    : m_leaderboard(leaderboard) {
    requiresSave = false;
    saveCounter = 0;
    modifiedBoards = 0;
    flushCounter = 0;
}

void StatsCounter::award(Stat* stat, unsigned int difficulty,
                         unsigned int count) {
    if (stat->isAchievement()) difficulty = 0;

    StatsMap::iterator val = stats.find(stat);
    if (val == stats.end()) {
        StatContainer newVal;
        newVal.stats[difficulty] = count;
        stats.insert(std::make_pair(stat, newVal));
    } else {
        val->second.stats[difficulty] += count;

        if (stat != GenericStats::timePlayed()) Log::info("");

        // If value has wrapped, cap it to UINT_MAX
        if (val->second.stats[difficulty] <
            (val->second.stats[difficulty] - count))
            val->second.stats[difficulty] = UINT_MAX;

        // If value is larger than USHRT_MAX and is not designated as large, cap
        // it to USHRT_MAX
        if (val->second.stats[difficulty] > USHRT_MAX && !isLargeStat(stat))
            val->second.stats[difficulty] = USHRT_MAX;
    }

    requiresSave = true;

    // If this stat is on a leaderboard, mark that leaderboard as needing
    // updated
    std::unordered_map<Stat*, int>::iterator leaderboardEntry =
        statBoards.find(stat);
    if (leaderboardEntry != statBoards.end()) {
        Log::info("[StatsCounter] award(): %X\n",
                  leaderboardEntry->second << difficulty);
        modifiedBoards |= (leaderboardEntry->second << difficulty);
        if (flushCounter == 0) flushCounter = FLUSH_DELAY;
    }
}

bool StatsCounter::hasTaken(Achievement* ach) {
    return stats.find(ach) != stats.end();
}

bool StatsCounter::canTake(Achievement* ach) {
    // 4J Gordon: Remove achievement dependencies, always able to take
    return true;
}

unsigned int StatsCounter::getValue(Stat* stat, unsigned int difficulty) {
    StatsMap::iterator val = stats.find(stat);
    if (val != stats.end()) return val->second.stats[difficulty];
    return 0;
}

unsigned int StatsCounter::getTotalValue(Stat* stat) {
    StatsMap::iterator val = stats.find(stat);
    if (val != stats.end())
        return val->second.stats[0] + val->second.stats[1] +
               val->second.stats[2] + val->second.stats[3];
    return 0;
}

void StatsCounter::tick(int player) {
    if (saveCounter > 0) --saveCounter;

    if (requiresSave && saveCounter == 0) save(player);

    // 4J-JEV, we don't want to write leaderboards in the middle of a game.
    // EDIT: Yes we do, people were not ending their games properly and not
    // updating scores.
    // #if 1
    if (flushCounter > 0) {
        --flushCounter;
        if (flushCounter == 0) flushLeaderboards();
    }
    // #endif
}

void StatsCounter::clear() {
    // clear out the stats when someone signs out
    stats.clear();
}

void StatsCounter::parse(void* data) {
    // Check that we don't already have any stats
    assert(stats.size() == 0);

    // Pointer to current position in stat array
    std::uint8_t* pbData = reinterpret_cast<std::uint8_t*>(data);
    pbData += sizeof(GAME_SETTINGS);
    std::uint8_t* statData = pbData;

    // Value being read
    StatContainer newVal;

    // For each stat
    std::vector<Stat*>::iterator end = Stats::all->end();
    for (std::vector<Stat*>::iterator iter = Stats::all->begin(); iter != end;
         ++iter) {
        if (!(*iter)->isAchievement()) {
            if (!isLargeStat(*iter)) {
                std::uint16_t difficultyStats[eDifficulty_Max] = {};
                std::memcpy(difficultyStats, statData, sizeof(difficultyStats));
                if (difficultyStats[0] != 0 || difficultyStats[1] != 0 ||
                    difficultyStats[2] != 0 || difficultyStats[3] != 0) {
                    newVal.stats[0] = difficultyStats[0];
                    newVal.stats[1] = difficultyStats[1];
                    newVal.stats[2] = difficultyStats[2];
                    newVal.stats[3] = difficultyStats[3];
                    stats.insert(std::make_pair(*iter, newVal));
                }
                statData += sizeof(difficultyStats);
            } else {
                std::uint32_t largeStatData[eDifficulty_Max] = {};
                std::memcpy(largeStatData, statData, sizeof(largeStatData));
                if (largeStatData[0] != 0 || largeStatData[1] != 0 ||
                    largeStatData[2] != 0 || largeStatData[3] != 0) {
                    newVal.stats[0] = largeStatData[0];
                    newVal.stats[1] = largeStatData[1];
                    newVal.stats[2] = largeStatData[2];
                    newVal.stats[3] = largeStatData[3];
                    stats.insert(std::make_pair(*iter, newVal));
                }
                statData += sizeof(largeStatData);
            }
        } else {
            std::uint16_t achievementValue = 0;
            std::memcpy(&achievementValue, statData, sizeof(achievementValue));
            if (achievementValue != 0) {
                newVal.stats[0] = achievementValue;
                newVal.stats[1] = 0;
                newVal.stats[2] = 0;
                newVal.stats[3] = 0;
                stats.insert(std::make_pair(*iter, newVal));
            }
            statData += sizeof(achievementValue);
        }
    }

    dumpStatsToTTY();
}

void StatsCounter::save(int player, bool force) {
    // Check we're going to have enough room to store all possible stats
    unsigned int uiTotalStatsSize =
        (Stats::all->size() * 4 * sizeof(unsigned short)) -
        (Achievements::achievements->size() * 3 * sizeof(unsigned short)) +
        (LARGE_STATS_COUNT * 4 *
         (sizeof(unsigned int) - sizeof(unsigned short)));
    assert(uiTotalStatsSize <=
           (Game::GAME_DEFINED_PROFILE_DATA_BYTES - sizeof(GAME_SETTINGS)));

    // Retrieve the data pointer from the profile
    std::uint8_t* pbData = reinterpret_cast<std::uint8_t*>(
        PlatformProfile.GetGameDefinedProfileData(player));
    pbData += sizeof(GAME_SETTINGS);

    // Pointer to current position in stat array
    std::uint8_t* statData = pbData;

    // Reset all the data to 0 (we're going to replace it with the map data)
    memset(statData, 0,
           Game::GAME_DEFINED_PROFILE_DATA_BYTES - sizeof(GAME_SETTINGS));

    // For each stat
    StatsMap::iterator val;
    std::vector<Stat*>::iterator end = Stats::all->end();
    for (std::vector<Stat*>::iterator iter = Stats::all->begin(); iter != end;
         ++iter) {
        // If the stat is in the map write out it's value
        val = stats.find(*iter);
        if (!(*iter)->isAchievement()) {
            if (!isLargeStat(*iter)) {
                std::uint16_t difficultyStats[eDifficulty_Max] = {};
                if (val != stats.end()) {
                    difficultyStats[0] =
                        static_cast<std::uint16_t>(val->second.stats[0]);
                    difficultyStats[1] =
                        static_cast<std::uint16_t>(val->second.stats[1]);
                    difficultyStats[2] =
                        static_cast<std::uint16_t>(val->second.stats[2]);
                    difficultyStats[3] =
                        static_cast<std::uint16_t>(val->second.stats[3]);
                }
                std::memcpy(statData, difficultyStats, sizeof(difficultyStats));
                statData += sizeof(difficultyStats);
            } else {
                std::uint32_t largeStatData[eDifficulty_Max] = {};
                if (val != stats.end()) {
                    largeStatData[0] = val->second.stats[0];
                    largeStatData[1] = val->second.stats[1];
                    largeStatData[2] = val->second.stats[2];
                    largeStatData[3] = val->second.stats[3];
                }
                std::memcpy(statData, largeStatData, sizeof(largeStatData));
                statData += sizeof(largeStatData);
            }
        } else {
            std::uint16_t achievementValue = 0;
            if (val != stats.end()) {
                achievementValue =
                    static_cast<std::uint16_t>(val->second.stats[0]);
            }
            std::memcpy(statData, &achievementValue, sizeof(achievementValue));
            statData += sizeof(achievementValue);
        }
    }

    saveCounter = SAVE_DELAY;
}

void StatsCounter::flushLeaderboards() {
    if (m_leaderboard.OpenSession()) {
        writeStats();
        m_leaderboard.FlushStats();
    } else {
        Log::info(
            "Failed to open a session in order to write to leaderboard\n");

        // 4J-JEV: If user was not signed in it would hit this.
        // assert(false);// && "Failed to open a session in order to write to
        // leaderboard");
    }

    modifiedBoards = 0;
}

void StatsCounter::saveLeaderboards() {
    if (m_leaderboard.OpenSession()) {
        writeStats();
        m_leaderboard.CloseSession();
    } else {
        Log::info(
            "Failed to open a session in order to write to leaderboard\n");

        // 4J-JEV: If user was not signed in it would hit this.
        // assert(false);// && "Failed to open a session in order to write to
        // leaderboard");
    }

    modifiedBoards = 0;
}

void StatsCounter::writeStats() {
    // unsigned int locale = XGetLocale();

    int viewCount = 0;
    int iPad = PlatformProfile.GetLockedProfile();
}

void StatsCounter::setupStatBoards() {
    statBoards.insert(
        std::make_pair(Stats::killsZombie, LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::killsSkeleton, LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::killsCreeper, LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::killsSpider, LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::killsSpiderJockey, LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::killsZombiePigman, LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::killsNetherZombiePigman,
                                     LEADERBOARD_KILLS_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::killsSlime, LEADERBOARD_KILLS_PEACEFUL));

    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::dirt->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::cobblestone->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::sand->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::stone->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::gravel->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::clay->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::obsidian->id],
                                     LEADERBOARD_MININGBLOCKS_PEACEFUL));

    statBoards.insert(std::make_pair(Stats::itemsCollected[Item::egg->id],
                                     LEADERBOARD_FARMING_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::wheat_Id],
                                     LEADERBOARD_FARMING_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::blocksMined[Tile::mushroom_brown_Id],
                       LEADERBOARD_FARMING_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::blocksMined[Tile::reeds_Id],
                                     LEADERBOARD_FARMING_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::cowsMilked, LEADERBOARD_FARMING_PEACEFUL));
    statBoards.insert(std::make_pair(Stats::itemsCollected[Tile::pumpkin->id],
                                     LEADERBOARD_FARMING_PEACEFUL));

    statBoards.insert(
        std::make_pair(Stats::walkOneM, LEADERBOARD_TRAVELLING_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::fallOneM, LEADERBOARD_TRAVELLING_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::minecartOneM, LEADERBOARD_TRAVELLING_PEACEFUL));
    statBoards.insert(
        std::make_pair(Stats::boatOneM, LEADERBOARD_TRAVELLING_PEACEFUL));
}

bool StatsCounter::isLargeStat(Stat* stat) {
    Stat*** end = &LARGE_STATS[LARGE_STATS_COUNT];
    for (Stat*** iter = LARGE_STATS; iter != end; ++iter)
        if ((*(*iter))->id == stat->id) return true;
    return false;
}

void StatsCounter::dumpStatsToTTY() {
    std::vector<Stat*>::iterator statsEnd = Stats::all->end();
    for (std::vector<Stat*>::iterator statsIter = Stats::all->begin();
         statsIter != statsEnd; ++statsIter) {
        Log::info("%s\t\t%u\t%u\t%u\t%u\n", (*statsIter)->name.c_str(),
                  getValue(*statsIter, 0), getValue(*statsIter, 1),
                  getValue(*statsIter, 2), getValue(*statsIter, 3));
    }
}

#if defined(_DEBUG)

// To clear leaderboards set DEBUG_ENABLE_CLEAR_LEADERBOARDS to 1 and set
// DEBUG_CLEAR_LEADERBOARDS to be the bitmask of what you want to clear
// Leaderboards are updated on game exit so enter and exit a level to trigger
// the clear

// #define DEBUG_CLEAR_LEADERBOARDS			(LEADERBOARD_KILLS_EASY
// | LEADERBOARD_KILLS_NORMAL | LEADERBOARD_KILLS_HARD)
#define DEBUG_CLEAR_LEADERBOARDS (0xFFFFFFFF)
#define DEBUG_ENABLE_CLEAR_LEADERBOARDS

void StatsCounter::WipeLeaderboards() {}
#endif
