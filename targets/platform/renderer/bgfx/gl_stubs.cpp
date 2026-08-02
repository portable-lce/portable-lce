#include "platform/renderer/IRenderPath.h"

#include "java/FloatBuffer.h"
#include "java/IntBuffer.h"

// Stub implementations for legacy texture/buffer functions
// that the game's texture system still calls.

static int next_tex_id = 1;

void glGetFloat(int, FloatBuffer*) {}

int glGenTextures_4J() { return next_tex_id++; }
void glGenTextures_4J(int n, unsigned int* out) {
    for (int i = 0; i < n; i++) out[i] = next_tex_id++;
}
void glDeleteTextures_4J(int) {}
void glDeleteTextures_4J(int, const unsigned int*) {}
void glTexImage2D_4J(int, int, int, int, int, int, int, int, void*) {}

void glCallLists_4J(IntBuffer*) {}

void glLight_4J(int, int, FloatBuffer*) {}
void glLightModel_4J(int, FloatBuffer*) {}
void glFog_4J(int, FloatBuffer*) {}
void glTexGen_4J(int, int, FloatBuffer*) {}

void glReadPixels_4J(int, int, int, int, int, int, void*) {}
void glReadPixels_4J(int, int, int, int, int, int, unsigned char*) {}
