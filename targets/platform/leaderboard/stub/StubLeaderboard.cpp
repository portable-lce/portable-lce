#include "StubLeaderboard.h"

#include "platform/leaderboard/leaderboard.h"

namespace platform_internal {
IPlatformLeaderboard& PlatformLeaderboard_get() {
    static StubLeaderboard instance;
    return instance;
}
}  // namespace platform_internal
