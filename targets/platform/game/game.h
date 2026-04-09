#pragma once

#include "IPlatformGame.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Same shape as platform/profile/profile.h.
namespace platform_internal {
IPlatformGame& PlatformGame_get();
}

#define PlatformGame (::platform_internal::PlatformGame_get())
