#pragma once

#include "platform/sound/IPlatformSound.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Same pattern as input/renderer/profile/storage/fs - SIOF-safe.
//
// The macro lets call sites keep using `PlatformSound.foo()` syntax;
// the expansion is just a function call returning a reference, which
// LTO inlines.

namespace platform_internal {
::platform::sound::IPlatformSound& PlatformSound_get();
}

#define PlatformSound (::platform_internal::PlatformSound_get())
