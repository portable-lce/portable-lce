#pragma once

// windows hack: Windows SDK OpenGL headers include WINGDIAPI in their declarations,
// which can only be found in the Windows API
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
// #include <GL/glext.h>

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
    const char* ver = (const char*)glGetString(GL_VERSION);
    if (!ver) {
        fprintf(stderr, "[gl3_loader] ERROR: No active GL context found.\n");
        return false;
    }
    int major = 0, minor = 0;
    if (sscanf(ver, "%d.%d", &major, &minor) >= 2) {
        if (major < 3 || (major == 3 && minor < 3)) {
            fprintf(stderr,
                    "[gl3_loader] ERROR: Need GL 3.3, but system provides %s\n",
                    ver);
            return false;
        }
    }
    fprintf(
        stderr,
        "[gl3_loader] GL Version: %s, it's all okay!! until android support.\n",
        ver);
    return true;
}