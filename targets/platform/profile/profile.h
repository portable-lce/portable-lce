#pragma once

#include "IPlatformProfile.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Avoids the static-initialization-order fiasco that the previous
// `extern IPlatformProfile& PlatformProfile;` form had: anything reading PlatformProfile
// during another translation unit's static init was UB.
//
// The macro lets the existing call sites keep writing `PlatformProfile.foo()`
// without each call site having to add the `()`. The expansion is just
// a call to a function returning a reference, so codegen is unchanged
// once inlined.
namespace platform_internal {
IPlatformProfile& PlatformProfile_get();
}

#define PlatformProfile (::platform_internal::PlatformProfile_get())
