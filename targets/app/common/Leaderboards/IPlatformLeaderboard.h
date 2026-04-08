#pragma once

#include <cstdint>
#include <string>

#include "PlatformTypes.h"

class LeaderboardReadListener;

class IPlatformLeaderboard {
public:
    enum eStatsReturn {
        eStatsReturn_Success = 0,
        eStatsReturn_NoResults,
        eStatsReturn_NetworkError
    };

    enum eProperty_Kills {
        eProperty_Kills_Zombie = 0,
        eProperty_Kills_Skeleton,
        eProperty_Kills_Creeper,
        eProperty_Kills_Spider,
        eProperty_Kills_SpiderJockey,
        eProperty_Kills_ZombiePigman,
        eProperty_Kills_Slime,
        eProperty_Kills_Rating,
        eProperty_Kills_Max,
    };

    enum eProperty_Mining {
        eProperty_Mining_Dirt = 0,
        eProperty_Mining_Stone,
        eProperty_Mining_Sand,
        eProperty_Mining_Cobblestone,
        eProperty_Mining_Gravel,
        eProperty_Mining_Clay,
        eProperty_Mining_Obsidian,
        eProperty_Mining_Rating,
        eProperty_Mining_Max,
    };

    enum eProperty_Farming {
        eProperty_Farming_Egg = 0,
        eProperty_Farming_Wheat,
        eProperty_Farming_Mushroom,
        eProperty_Farming_Sugarcane,
        eProperty_Farming_Milk,
        eProperty_Farming_Pumpkin,
        eProperty_Farming_Rating,
        eProperty_Farming_Max,
    };

    enum eProperty_Travelling {
        eProperty_Travelling_Walked = 0,
        eProperty_Travelling_Fallen,
        eProperty_Travelling_Minecart,
        eProperty_Travelling_Boat,
        eProperty_Travelling_Rating,
        eProperty_Travelling_Max,
    };

    enum EStatsType {
        eStatsType_Travelling = 0,
        eStatsType_Mining,
        eStatsType_Farming,
        eStatsType_Kills,
        eStatsType_MAX,
        eStatsType_UNDEFINED
    };

    enum EFilterMode {
        eFM_Friends = 0,
        eFM_MyScore,
        eFM_TopRank,
        eNumFilterModes,
        eFM_UNDEFINED
    };

    struct KillsRecord {
        unsigned short m_zombie;
        unsigned short m_skeleton;
        unsigned short m_creeper;
        unsigned short m_spider;
        unsigned short m_spiderJockey;
        unsigned short m_zombiePigman;
        unsigned short m_slime;
    };

    struct MiningRecord {
        unsigned short m_dirt;
        unsigned short m_stone;
        unsigned short m_sand;
        unsigned short m_cobblestone;
        unsigned short m_gravel;
        unsigned short m_clay;
        unsigned short m_obsidian;
    };

    struct FarmingRecord {
        unsigned short m_eggs;
        unsigned short m_wheat;
        unsigned short m_mushroom;
        unsigned short m_sugarcane;
        unsigned short m_milk;
        unsigned short m_pumpkin;
    };

    struct TravellingRecord {
        unsigned int m_walked;
        unsigned int m_fallen;
        unsigned int m_minecart;
        unsigned int m_boat;
    };

    static constexpr int RECORD_SIZE = 40;

    struct StatsData {
        EStatsType m_statsType;
        union {
            KillsRecord m_kills;
            MiningRecord m_mining;
            FarmingRecord m_farming;
            TravellingRecord m_travelling;
            unsigned char m_padding[RECORD_SIZE];
        };
    };

    struct RegisterScore {
        int m_iPad;
        int m_score;
        int m_difficulty;
        StatsData m_commentData;
    };

    struct ReadScore {
        static constexpr unsigned int STATSDATA_MAX = 8;

        PlayerUID m_uid;
        unsigned long m_rank;
        std::string m_name;
        unsigned long m_totalScore;
        unsigned short m_statsSize;
        unsigned long m_statsData[STATSDATA_MAX];
        int m_idsErrorMessage;
    };

    struct ReadView {
        unsigned int m_numQueries;
        ReadScore* m_queries;
    };

    using ViewOut = ReadView;
    using ViewIn = RegisterScore*;

    virtual ~IPlatformLeaderboard() = default;

    virtual void Tick() = 0;
    [[nodiscard]] virtual bool OpenSession() = 0;
    virtual void CloseSession() = 0;
    virtual void DeleteSession() = 0;
    [[nodiscard]] virtual bool WriteStats(unsigned int viewCount,
                                          ViewIn views) = 0;
    virtual bool ReadStats_Friends(LeaderboardReadListener* callback,
                                   int difficulty, EStatsType type,
                                   PlayerUID myUID, unsigned int startIndex,
                                   unsigned int readCount) = 0;
    virtual bool ReadStats_MyScore(LeaderboardReadListener* callback,
                                   int difficulty, EStatsType type,
                                   PlayerUID myUID,
                                   unsigned int readCount) = 0;
    virtual bool ReadStats_TopRank(LeaderboardReadListener* callback,
                                   int difficulty, EStatsType type,
                                   unsigned int startIndex,
                                   unsigned int readCount) = 0;
    virtual void FlushStats() = 0;
    virtual void CancelOperation() = 0;
    [[nodiscard]] virtual bool isIdle() = 0;
};

class LeaderboardReadListener {
public:
    virtual ~LeaderboardReadListener() = default;
    virtual bool OnStatsReadComplete(IPlatformLeaderboard::eStatsReturn ret,
                                     int numResults,
                                     IPlatformLeaderboard::ViewOut results) = 0;
};
