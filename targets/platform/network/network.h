#pragma once

#include "INetworkPlayer.h"
#include "IPlatformNetwork.h"
#include "SessionInfo.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Same shape as platform/profile/profile.h: avoids the static-init-order
// fiasco. Call sites use the existing `PlatformNetwork.foo()` form via
// the macro.
namespace platform_internal {
IPlatformNetwork& PlatformNetwork_get();
}

#define PlatformNetwork (::platform_internal::PlatformNetwork_get())
