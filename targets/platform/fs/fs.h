#pragma once

#include "IPlatformFilesystem.h"

// Function accessor backed by a function-local static (Meyers singleton).
// Avoids the static-initialization-order fiasco that the previous
// `extern IPlatformFilesystem& PlatformFilesystem;` form had: anything reading PlatformFilesystem
// during another translation unit's static init was UB.
//
// The macro lets the existing call sites keep writing `PlatformFilesystem.foo()`
// without each call site having to add the `()`. The expansion is just
// a call to a function returning a reference, so codegen is unchanged
// once inlined.
namespace platform_internal {
IPlatformFilesystem& PlatformFilesystem_get();
}

#define PlatformFilesystem (::platform_internal::PlatformFilesystem_get())
