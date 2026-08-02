#pragma once

// Stub declarations for legacy texture/buffer functions that the game's
// texture system still calls. Implementations are in bgfx/gl_stubs.cpp.

class FloatBuffer;
class IntBuffer;

void glGetFloat(int pname, FloatBuffer* params);
int glGenTextures_4J();
void glGenTextures_4J(int n, unsigned int* textures);
void glDeleteTextures_4J(int id);
void glDeleteTextures_4J(int n, const unsigned int* textures);
void glTexImage2D_4J(int, int, int, int, int, int, int, int, void*);
void glReadPixels_4J(int x, int y, int w, int h, int format, int type,
                     void* pixels);
void glReadPixels_4J(int x, int y, int w, int h, int format, int type,
                     unsigned char* pixels);

template <typename T>
inline void glGenTextures_4J(T* buf) {
    buf->put(0);
    buf->flip();
}
template <typename T>
inline void glDeleteTextures_4J(T*) {}
template <typename T>
inline void glTexImage2D_4J(int, int, int, int, int, int, int, int, T*) {}
template <typename T>
inline void glReadPixels_4J(int, int, int, int, int, int, T*) {}

#define glGenTextures(...) glGenTextures_4J(__VA_ARGS__)
#define glDeleteTextures(...) glDeleteTextures_4J(__VA_ARGS__)
#define glTexImage2D(a, b, c, d, e, f, g, h, i) \
    glTexImage2D_4J(a, b, c, d, e, f, g, h, i)
#define glReadPixels(a, b, c, d, e, f, g) glReadPixels_4J(a, b, c, d, e, f, g)
