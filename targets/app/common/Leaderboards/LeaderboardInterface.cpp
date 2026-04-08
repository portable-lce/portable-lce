#include "LeaderboardInterface.h"

#include <assert.h>

#include "app/common/Leaderboards/LeaderboardManager.h"

LeaderboardInterface::LeaderboardInterface(IPlatformLeaderboard* man) {
    m_manager = man;
    m_pending = false;

    m_filter = (IPlatformLeaderboard::EFilterMode)-1;
    m_callback = nullptr;
    m_difficulty = 0;
    m_type = IPlatformLeaderboard::eStatsType_UNDEFINED;
    m_startIndex = 0;
    m_readCount = 0;

    (void)m_manager->OpenSession();
}

LeaderboardInterface::~LeaderboardInterface() {
    m_manager->CancelOperation();
    m_manager->CloseSession();
}

void LeaderboardInterface::ReadStats_Friends(
    LeaderboardReadListener* callback, int difficulty,
    IPlatformLeaderboard::EStatsType type, PlayerUID myUID,
    unsigned int startIndex, unsigned int readCount) {
    m_filter = IPlatformLeaderboard::eFM_Friends;
    m_pending = true;

    m_callback = callback;
    m_difficulty = difficulty;
    m_type = type;
    m_myUID = myUID;
    m_startIndex = startIndex;
    m_readCount = readCount;

    tick();
}

void LeaderboardInterface::ReadStats_MyScore(
    LeaderboardReadListener* callback, int difficulty,
    IPlatformLeaderboard::EStatsType type, PlayerUID myUID,
    unsigned int readCount) {
    m_filter = IPlatformLeaderboard::eFM_MyScore;
    m_pending = true;

    m_callback = callback;
    m_difficulty = difficulty;
    m_type = type;
    m_myUID = myUID;
    m_readCount = readCount;

    tick();
}

void LeaderboardInterface::ReadStats_TopRank(
    LeaderboardReadListener* callback, int difficulty,
    IPlatformLeaderboard::EStatsType type, unsigned int startIndex,
    unsigned int readCount) {
    m_filter = IPlatformLeaderboard::eFM_TopRank;
    m_pending = true;

    m_callback = callback;
    m_difficulty = difficulty;
    m_type = type;
    m_startIndex = startIndex;
    m_readCount = readCount;

    tick();
}

void LeaderboardInterface::CancelOperation() {
    m_manager->CancelOperation();
    m_pending = false;
}

void LeaderboardInterface::tick() {
    if (m_pending) m_pending = !callManager();
}

bool LeaderboardInterface::callManager() {
    switch (m_filter) {
        case IPlatformLeaderboard::eFM_Friends:
            return m_manager->ReadStats_Friends(m_callback, m_difficulty,
                                                m_type, m_myUID, m_startIndex,
                                                m_readCount);
        case IPlatformLeaderboard::eFM_MyScore:
            return m_manager->ReadStats_MyScore(m_callback, m_difficulty,
                                                m_type, m_myUID, m_readCount);
        case IPlatformLeaderboard::eFM_TopRank:
            return m_manager->ReadStats_TopRank(
                m_callback, m_difficulty, m_type, m_startIndex, m_readCount);
        default:
            assert(false);
            return true;
    }
}