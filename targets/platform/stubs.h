#pragma once

// windows hack: Windows SDK OpenGL headers include WINGDIAPI in their declarations,
// which can only be found in the Windows API
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/gl.h>
#include <string.h>

#include "java/File.h"
#include "renderer/gl/gl_compat.h"

#undef GL_SMOOTH
#undef GL_FLAT
static const int GL_SMOOTH = 0x1D01;
static const int GL_FLAT = 0x1D00;

class FloatBuffer;
class IntBuffer;
class ByteBuffer;
class Minecraft;

void glGenTextures(IntBuffer*);
int glGenTextures();
void glDeleteTextures(IntBuffer*);
void glDeleteTextures(int);
void glLight(int, int, FloatBuffer*);
void glLightModel(int, FloatBuffer*);
void glGetFloat(int, FloatBuffer*);
void glTexGen(int, int, FloatBuffer*);
void glFog(int, FloatBuffer*);
void glTexCoordPointer(int, int, FloatBuffer*);
void glNormalPointer(int, ByteBuffer*);
void glColorPointer(int, bool, int, ByteBuffer*);
void glVertexPointer(int, int, FloatBuffer*);

void glEndList_4J(int vertexCount = 0);

void glTexImage2D(int, int, int, int, int, int, int, int, ByteBuffer*);
void glCallLists(IntBuffer*);

class GL11 {
public:
    static const int GL_SMOOTH = 0x1D01;
    static const int GL_FLAT = 0x1D00;
#undef glShadeModel
#define GL_SHADEMODEL_IS_FUNCTION
    static void glShadeModel(int mode) { ::glShadeModel(mode); }
};
#undef GL_ARRAY_BUFFER_ARB
#undef GL_STREAM_DRAW_ARB
class ARBVertexBufferObject {
public:
    static const int GL_ARRAY_BUFFER_ARB = 0x8892;
    static const int GL_STREAM_DRAW_ARB = 0x88E0;
    static void glBindBufferARB(int, int) {}
    static void glBufferDataARB(int, ByteBuffer*, int) {}
    static void glGenBuffersARB(IntBuffer*) {}
};

class Level;
class Player;
class Textures;
class Font;
class MapItemSavedData;
class Mob;

class Particles {
public:
    void render(float) {}
    void tick() {}
};

class BufferedImage;

class Graphics {
public:
    void drawImage(BufferedImage*, int, int, void*) {}
    void dispose() {}
};

class ZipEntry {};
class InputStream;

class ZipFile {
public:
    ZipFile(File* file) {}
    InputStream* getInputStream(ZipEntry* entry) { return nullptr; }
    ZipEntry* getEntry(const std::string& name) { return nullptr; }
    void close() {}
};

class ImageIO {
public:
    static BufferedImage* read(InputStream* in) { return nullptr; }
};

class Keyboard {
public:
    static void create() {}
    static void destroy() {}
    static bool isKeyDown(int) { return false; }
    static std::string getKeyName(int) { return "KEYNAME"; }
    static void enableRepeatEvents(bool) {}
    static const int KEY_A = 0;
    static const int KEY_B = 1;
    static const int KEY_C = 2;
    static const int KEY_D = 3;
    static const int KEY_E = 4;
    static const int KEY_F = 5;
    static const int KEY_G = 6;
    static const int KEY_H = 7;
    static const int KEY_I = 8;
    static const int KEY_J = 9;
    static const int KEY_K = 10;
    static const int KEY_L = 11;
    static const int KEY_M = 12;
    static const int KEY_N = 13;
    static const int KEY_O = 14;
    static const int KEY_P = 15;
    static const int KEY_Q = 16;
    static const int KEY_R = 17;
    static const int KEY_S = 18;
    static const int KEY_T = 19;
    static const int KEY_U = 20;
    static const int KEY_V = 21;
    static const int KEY_W = 22;
    static const int KEY_X = 23;
    static const int KEY_Y = 24;
    static const int KEY_Z = 25;
    static const int KEY_SPACE = 26;
    static const int KEY_LSHIFT = 27;
    static const int KEY_ESCAPE = 28;
    static const int KEY_BACK = 29;
    static const int KEY_RETURN = 30;
    static const int KEY_RSHIFT = 31;
    static const int KEY_UP = 32;
    static const int KEY_DOWN = 33;
    static const int KEY_TAB = 34;
};

class Mouse {
public:
    static void create() {}
    static void destroy() {}
    static int getX() { return 0; }
    static int getY() { return 0; }
    static bool isButtonDown(int) { return false; }
};

class Display {
public:
    static bool isActive() { return true; }
    static void update() {}
    static void swapBuffers();
    static void destroy() {}
};

class BackgroundDownloader {
public:
    BackgroundDownloader(File workDir, Minecraft* minecraft) {}
    void start() {}
    void halt() {}
    void forceReload() {}
};
