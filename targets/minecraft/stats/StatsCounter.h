#pragma once
#include <cstdint>
#include <unordered_map>

class IPlatformLeaderboard;
class Stat;
class Achievement;
class StatsSyncher;
class User;

class StatsCounter {
private:
    enum eDifficulty {
        eDifficulty_Peaceful = 0,
        eDifficulty_Easy,
        eDifficulty_Normal,
        eDifficulty_Hard,
        eDifficulty_Max
    };

    struct StatContainer {
        unsigned int stats[eDifficulty_Max];

        StatContainer() {
            stats[eDifficulty_Peaceful] = stats[eDifficulty_Easy] =
                stats[eDifficulty_Normal] = stats[eDifficulty_Hard] = 0;
        }
    };

    typedef std::unordered_map<Stat*, StatContainer> StatsMap;

    // static const int STAT_DATA_OFFSET = 32;
    static const int LARGE_STATS_COUNT = 8;
    static Stat** LARGE_STATS[LARGE_STATS_COUNT];
    static const int SAVE_DELAY = 30 * 60;
    static const int FLUSH_DELAY = 30 * 60 * 5;

    typedef enum {
        LEADERBOARD_KILLS_PEACEFUL = 0x00000001,
        LEADERBOARD_KILLS_EASY = 0x00000002,
        LEADERBOARD_KILLS_NORMAL = 0x00000004,
        LEADERBOARD_KILLS_HARD = 0x00000008,
        LEADERBOARD_MININGBLOCKS_PEACEFUL = 0x00000010,
        LEADERBOARD_MININGBLOCKS_EASY = 0x00000020,
        LEADERBOARD_MININGBLOCKS_NORMAL = 0x00000040,
        LEADERBOARD_MININGBLOCKS_HARD = 0x00000080,
        LEADERBOARD_MININGORE_PEACEFUL = 0x00000100,
        LEADERBOARD_MININGORE_EASY = 0x00000200,
        LEADERBOARD_MININGORE_NORMAL = 0x00000400,
        LEADERBOARD_MININGORE_HARD = 0x00000800,
        LEADERBOARD_FARMING_PEACEFUL = 0x00001000,
        LEADERBOARD_FARMING_EASY = 0x00002000,
        LEADERBOARD_FARMING_NORMAL = 0x00004000,
        LEADERBOARD_FARMING_HARD = 0x00008000,
        LEADERBOARD_TRAVELLING_PEACEFUL = 0x00010000,
        LEADERBOARD_TRAVELLING_EASY = 0x00020000,
        LEADERBOARD_TRAVELLING_NORMAL = 0x00040000,
        LEADERBOARD_TRAVELLING_HARD = 0x00080000,
        LEADERBOARD_NETHER_PEACEFUL = 0x00100000,
        LEADERBOARD_NETHER_EASY = 0x00200000,
        LEADERBOARD_NETHER_NORMAL = 0x00400000,
        LEADERBOARD_NETHER_HARD = 0x00800000,
        LEADERBOARD_TRAVELLING_TOTAL = 0x01000000
    } LEADERBOARD_FLAG;

    StatsMap stats;
    bool requiresSave;
    int saveCounter;

    int modifiedBoards;
    static std::unordered_map<Stat*, int> statBoards;
    int flushCounter;

    // Borrowed from the composition root - StatsCounter does not own the
    // backend. The reference is valid for the lifetime of the StatsCounter
    // (the leaderboard outlives all StatsCounter instances).
    IPlatformLeaderboard& m_leaderboard;

public:
    explicit StatsCounter(IPlatformLeaderboard& leaderboard);
    void award(Stat* stat, unsigned int difficulty, unsigned int count);
    bool hasTaken(Achievement* ach);
    bool canTake(Achievement* ach);
    unsigned int getValue(Stat* stat, unsigned int difficulty);
    unsigned int getTotalValue(Stat* stat);
    void tick(int player);
    void parse(void* data);
    void clear();
    void save(int player, bool force = false);
    void flushLeaderboards();
    void saveLeaderboards();
    static void setupStatBoards();
#if defined(_DEBUG)
    void WipeLeaderboards();
#endif

private:
    bool isLargeStat(Stat* stat);
    void dumpStatsToTTY();

    void writeStats();
};
