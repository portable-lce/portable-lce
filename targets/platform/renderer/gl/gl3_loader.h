#pragma once

// windows hack: Windows SDK OpenGL headers include WINGDIAPI in their declarations,
// which can only be found in the Windows API
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/glew.h>

#include <cstdio>

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88B4
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif

static inline bool gl3_load() {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "[gl_loader] ERROR: glewInit failed: %s\n",
                glewGetErrorString(err));
        return false;
    }

    if (!GLEW_VERSION_3_3) {
        fprintf(stderr, "[gl_loader] ERROR: Need GL 3.3, not supported.\n");
        return false;
    }

    fprintf(stderr, "[gl_loader] GL %s loaded successfully.\n",
            (const char*)glewGetString(GLEW_VERSION));
    return true;
}