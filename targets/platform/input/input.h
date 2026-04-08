#pragma once

#include "IPlatformInput.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Avoids the static-initialization-order fiasco that the previous
// `extern IPlatformInput& PlatformInput;` form had: anything reading PlatformInput
// during another translation unit's static init was UB.
//
// The macro lets the existing call sites keep writing `PlatformInput.foo()`
// without each call site having to add the `()`. The expansion is just
// a call to a function returning a reference, so codegen is unchanged
// once inlined.
namespace platform_internal {
IPlatformInput& PlatformInput_get();
}

#define PlatformInput (::platform_internal::PlatformInput_get())
