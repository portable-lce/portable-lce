#pragma once

#include "platform/leaderboard/IPlatformLeaderboard.h"

// No-op leaderboard backend. Returns success for session lifecycle and
// `false` for every read/write so consumers can short-circuit cleanly.
// This is the platform default; a real backend (Steam, EOS, Xbox Live,
// custom HTTP) would replace this at link time.

class StubLeaderboard : public IPlatformLeaderboard {
public:
    void Tick() override {}

    [[nodiscard]] bool OpenSession() override { return true; }
    void CloseSession() override {}
    void DeleteSession() override {}

    [[nodiscard]] bool WriteStats(unsigned int /*viewCount*/,
                                  ViewIn /*views*/) override {
        return false;
    }

    bool ReadStats_Friends(LeaderboardReadListener* /*callback*/,
                           int /*difficulty*/, EStatsType /*type*/,
                           PlayerUID /*myUID*/, unsigned int /*startIndex*/,
                           unsigned int /*readCount*/) override {
        return false;
    }
    bool ReadStats_MyScore(LeaderboardReadListener* /*callback*/,
                           int /*difficulty*/, EStatsType /*type*/,
                           PlayerUID /*myUID*/,
                           unsigned int /*readCount*/) override {
        return false;
    }
    bool ReadStats_TopRank(LeaderboardReadListener* /*callback*/,
                           int /*difficulty*/, EStatsType /*type*/,
                           unsigned int /*startIndex*/,
                           unsigned int /*readCount*/) override {
        return false;
    }

    void FlushStats() override {}
    void CancelOperation() override {}
    [[nodiscard]] bool isIdle() override { return true; }
};
