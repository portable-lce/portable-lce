#pragma once

#include "IPlatformLeaderboard.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Same shape as platform/profile/profile.h: avoids the static-init-order
// fiasco and lets call sites use the existing `PlatformLeaderboard.foo()`
// shape via the macro expansion.
namespace platform_internal {
IPlatformLeaderboard& PlatformLeaderboard_get();
}

#define PlatformLeaderboard (::platform_internal::PlatformLeaderboard_get())
