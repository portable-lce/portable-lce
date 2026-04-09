#pragma once

#include "IPlatformStorage.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Avoids the static-initialization-order fiasco that the previous
// `extern IPlatformStorage& PlatformStorage;` form had: anything reading
// PlatformStorage during another translation unit's static init was UB.
//
// The macro lets the existing call sites keep writing `PlatformStorage.foo()`
// without each call site having to add the `()`. The expansion is just
// a call to a function returning a reference, so codegen is unchanged
// once inlined.
namespace platform_internal {
IPlatformStorage& PlatformStorage_get();
}

#define PlatformStorage (::platform_internal::PlatformStorage_get())
