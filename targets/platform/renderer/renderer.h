#pragma once

#include "IPlatformRenderer.h"

// Defined in the linked renderer backend (currently
// platform/renderer/gl/GLRenderer.cpp). The legacy gl* macro shim used
// by older rendering call sites lives in
// platform/renderer/gl/gl_compat.h, which is brought in via
// platform/stubs.h on linux.
extern IPlatformRenderer& PlatformRenderer;
