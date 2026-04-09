#include "StubPlatformGame.h"

#include "platform/game/game.h"

namespace platform_internal {
IPlatformGame& PlatformGame_get() {
    static StubPlatformGame instance;
    return instance;
}
}  // namespace platform_internal
