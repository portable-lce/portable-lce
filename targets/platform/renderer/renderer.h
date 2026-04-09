#pragma once

#include "IPlatformRenderer.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Avoids the static-initialization-order fiasco that the previous
// `extern IPlatformRenderer& PlatformRenderer;` form had: anything reading
// PlatformRenderer during another translation unit's static init was UB.
//
// The macro lets the existing call sites keep writing `PlatformRenderer.foo()`
// without each call site having to add the `()`. The expansion is just
// a call to a function returning a reference, so codegen is unchanged
// once inlined.
namespace platform_internal {
IPlatformRenderer& PlatformRenderer_get();
}

#define PlatformRenderer (::platform_internal::PlatformRenderer_get())
