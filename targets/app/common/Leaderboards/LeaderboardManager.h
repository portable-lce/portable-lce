#pragma once

#include <string>

#include "platform/leaderboard/IPlatformLeaderboard.h"

class LeaderboardManager : public IPlatformLeaderboard {
public:
    static const std::string filterNames[eNumFilterModes];

    LeaderboardManager();
    virtual ~LeaderboardManager() {}

    // Singleton
    static IPlatformLeaderboard* Instance() { return m_instance; }
    static void DeleteInstance();

    // IPlatformLeaderboard pure virtuals - subclasses must implement:
    //   Tick, OpenSession, CloseSession, DeleteSession, WriteStats,
    //   FlushStats, CancelOperation, isIdle

    // Base implementations for read operations
    bool ReadStats_Friends(LeaderboardReadListener* callback, int difficulty,
                           EStatsType type, PlayerUID myUID,
                           unsigned int startIndex,
                           unsigned int readCount) override;
    bool ReadStats_MyScore(LeaderboardReadListener* callback, int difficulty,
                           EStatsType type, PlayerUID myUID,
                           unsigned int readCount) override;
    bool ReadStats_TopRank(LeaderboardReadListener* callback, int difficulty,
                           EStatsType type, unsigned int startIndex,
                           unsigned int readCount) override;

    static void printStats(ReadView& view);

protected:
    virtual void zeroReadParameters();

    EFilterMode m_eFilterMode;
    int m_difficulty;
    EStatsType m_statsType;
    LeaderboardReadListener* m_readListener;
    PlayerUID m_myXUID;
    unsigned int m_startIndex, m_readCount;

private:
    static LeaderboardManager* m_instance;
};

class DebugReadListener : public LeaderboardReadListener {
public:
    bool OnStatsReadComplete(IPlatformLeaderboard::eStatsReturn ret,
                             int numResults,
                             IPlatformLeaderboard::ViewOut results) override;
    static DebugReadListener* m_instance;
};
