#pragma once

// Legacy gl* compatibility shim for code that still issues fixed-function
// OpenGL calls. Provides:
//   - GL_* constant defines for use with legacy gl* call sites
//   - Macros that route legacy glPushMatrix / glColor4f / etc. through
//     PlatformRenderer
//   - Template implementations of glLight_4J / glFog_4J / glTexGen_4J
//     etc., used by the legacy producers
//   - #define glLight glLight_4J style consumer-facing macros
//
// New rendering code should call IPlatformRenderer methods directly via
// the PlatformRenderer extern. This shim exists to keep the legacy
// rendering files compiling without dragging the concrete GLRenderer
// class into every minecraft TU.

#include "gl3_loader.h"
// NOTE: gl3_loader.h must be included before these two
#include <GL/gl.h>
#include <GL/glu.h>

#include <cstdint>
#include <cstdlib>

#include "renderer/IPlatformRenderer.h"
#include "renderer/renderer.h"

// OpenGL Interception Macros
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif

#ifndef GL_S
#define GL_S 0x2000
#endif
#ifndef GL_T
#define GL_T 0x2001
#endif
#ifndef GL_R
#define GL_R 0x2002
#endif
#ifndef GL_Q
#define GL_Q 0x2003
#endif

#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S 0x0C60
#endif
#ifndef GL_TEXTURE_GEN_T
#define GL_TEXTURE_GEN_T 0x0C61
#endif
#ifndef GL_TEXTURE_GEN_Q
#define GL_TEXTURE_GEN_Q 0x0C63
#endif
#ifndef GL_TEXTURE_GEN_R
#define GL_TEXTURE_GEN_R 0x0C62
#endif

#ifndef GL_TEXTURE_GEN_MODE
#define GL_TEXTURE_GEN_MODE 0x2500
#endif
#ifndef GL_OBJECT_LINEAR
#define GL_OBJECT_LINEAR 0x2401
#endif
#ifndef GL_EYE_LINEAR
#define GL_EYE_LINEAR 0x2400
#endif
#ifndef GL_OBJECT_PLANE
#define GL_OBJECT_PLANE 0x2501
#endif
#ifndef GL_EYE_PLANE
#define GL_EYE_PLANE 0x2502
#endif

#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LIGHT0
#define GL_LIGHT0 0x4000
#endif
#ifndef GL_LIGHT1
#define GL_LIGHT1 0x4001
#endif

#ifndef CLEAR_DEPTH_FLAG
#define CLEAR_DEPTH_FLAG 0x00000100
#endif
#ifndef CLEAR_COLOUR_FLAG
#define CLEAR_COLOUR_FLAG 0x00004000
#endif

#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif

#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_CONSTANT_ALPHA
#define GL_CONSTANT_ALPHA 0x8003
#endif
#ifndef GL_ONE_MINUS_CONSTANT_ALPHA
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#endif

#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif

#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif

#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_EXP
#define GL_EXP 0x0800
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif

#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif

#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif

#ifndef GL_POSITION
#define GL_POSITION 0x1203
#endif
#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif

#ifndef GL_LIGHT_MODEL_AMBIENT
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#endif

#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif

// glCallList / display list macros
#undef glNewList
#define glNewList(_list, _mode) PlatformRenderer.CBuffStart(_list)
#undef glEndList
#define glEndList() PlatformRenderer.CBuffEnd()
#undef glCallList
#define glCallList(_list) PlatformRenderer.CBuffCall(_list)

// glGenLists / glDeleteLists, lists are not supported in core!!!!!
#undef glGenLists
#define glGenLists(range) PlatformRenderer.CBuffCreate(range)
#undef glDeleteLists
#define glDeleteLists(list, range) PlatformRenderer.CBuffDelete(list, range)

#ifndef GL_SHADEMODEL_IS_FUNCTION
#undef glShadeModel
#define glShadeModel(mode) \
    do {                   \
    } while (0)
#endif

#undef glTranslatef
#define glTranslatef(x, y, z)                   \
    do {                                        \
        PlatformRenderer.MatrixTranslate(x, y, z); \
    } while (0)

#undef glRotatef
#define glRotatef(a, x, y, z)                                               \
    do {                                                                    \
        PlatformRenderer.MatrixRotate((a) * (3.14159265358979f / 180.f), x, y, \
                                   z);                                      \
    } while (0)

#undef glScalef
#define glScalef(x, y, z)                   \
    do {                                    \
        PlatformRenderer.MatrixScale(x, y, z); \
    } while (0)

#undef glScaled
#define glScaled(x, y, z)                                              \
    do {                                                               \
        PlatformRenderer.MatrixScale((float)(x), (float)(y), (float)(z)); \
    } while (0)

#undef glPushMatrix
#define glPushMatrix()              \
    do {                            \
        PlatformRenderer.MatrixPush(); \
    } while (0)

#undef glPopMatrix
#define glPopMatrix()              \
    do {                           \
        PlatformRenderer.MatrixPop(); \
    } while (0)

#undef glLoadIdentity
#define glLoadIdentity()                   \
    do {                                   \
        PlatformRenderer.MatrixSetIdentity(); \
    } while (0)

#undef glMatrixMode
#define glMatrixMode(mode)              \
    do {                                \
        PlatformRenderer.MatrixMode(mode); \
    } while (0)

#undef glMultMatrixf
#define glMultMatrixf(m)             \
    do {                             \
        PlatformRenderer.MatrixMult(m); \
    } while (0)

#undef glColor4f
#define glColor4f(r, g, b, a)                     \
    do {                                          \
        PlatformRenderer.StateSetColour(r, g, b, a); \
    } while (0)

#undef glColor3f
#define glColor3f(r, g, b)                           \
    do {                                             \
        PlatformRenderer.StateSetColour(r, g, b, 1.0f); \
    } while (0)

#undef glAlphaFunc
#define glAlphaFunc(func, ref)                      \
    do {                                            \
        PlatformRenderer.StateSetAlphaFunc(func, ref); \
    } while (0)

#undef glEnable
#define glEnable(cap)                                                   \
    do {                                                                \
        if ((cap) == 0x0B60 /*GL_FOG*/)                                 \
            PlatformRenderer.StateSetFogEnable(true);                      \
        else if ((cap) == 0x0B50 /*GL_LIGHTING*/)                       \
            PlatformRenderer.StateSetLightingEnable(true);                 \
        else if ((cap) == 0x0BC0 /*GL_ALPHA_TEST*/)                     \
            PlatformRenderer.StateSetAlphaTestEnable(true);                \
        else if ((cap) == 0x0DE1 /*GL_TEXTURE_2D*/)                     \
            PlatformRenderer.StateSetTextureEnable(true);                  \
        else if ((cap) == 0x0BE2 /*GL_BLEND*/)                          \
            PlatformRenderer.StateSetBlendEnable(true);                    \
        else if ((cap) == 0x0B44 /*GL_CULL_FACE*/)                      \
            PlatformRenderer.StateSetFaceCull(true);                       \
        else if ((cap) == 0x0B71 /*GL_DEPTH_TEST*/)                     \
            PlatformRenderer.StateSetDepthTestEnable(true);                \
        else if ((cap) == 0x4000 /*GL_LIGHT0*/)                         \
            PlatformRenderer.StateSetLightEnable(0, true);                 \
        else if ((cap) == 0x4001 /*GL_LIGHT1*/)                         \
            PlatformRenderer.StateSetLightEnable(1, true);                 \
        else if ((cap) == 0x0B57    /*GL_COLOR_MATERIAL*/               \
                 || (cap) == 0x0BA1 /*GL_NORMALIZE*/                    \
                 || (cap) == 0x803A /*GL_RESCALE_NORMAL*/               \
                 || (cap) == 0x0C60 /*GL_TEXTURE_GEN_S*/                \
                 || (cap) == 0x0C61 /*GL_TEXTURE_GEN_T*/                \
                 || (cap) == 0x0C62 /*GL_TEXTURE_GEN_R*/                \
                 || (cap) == 0x0C63 /*GL_TEXTURE_GEN_Q*/) { /* empty */ \
        } else                                                          \
            ::glEnable(cap);                                            \
    } while (0)

#undef glDisable
#define glDisable(cap)                                                  \
    do {                                                                \
        if ((cap) == 0x0B60 /*GL_FOG*/)                                 \
            PlatformRenderer.StateSetFogEnable(false);                     \
        else if ((cap) == 0x0B50 /*GL_LIGHTING*/)                       \
            PlatformRenderer.StateSetLightingEnable(false);                \
        else if ((cap) == 0x0BC0 /*GL_ALPHA_TEST*/)                     \
            PlatformRenderer.StateSetAlphaTestEnable(false);               \
        else if ((cap) == 0x0DE1 /*GL_TEXTURE_2D*/)                     \
            PlatformRenderer.StateSetTextureEnable(false);                 \
        else if ((cap) == 0x0BE2 /*GL_BLEND*/)                          \
            PlatformRenderer.StateSetBlendEnable(false);                   \
        else if ((cap) == 0x0B44 /*GL_CULL_FACE*/)                      \
            PlatformRenderer.StateSetFaceCull(false);                      \
        else if ((cap) == 0x0B71 /*GL_DEPTH_TEST*/)                     \
            PlatformRenderer.StateSetDepthTestEnable(false);               \
        else if ((cap) == 0x4000 /*GL_LIGHT0*/)                         \
            PlatformRenderer.StateSetLightEnable(0, false);                \
        else if ((cap) == 0x4001 /*GL_LIGHT1*/)                         \
            PlatformRenderer.StateSetLightEnable(1, false);                \
        else if ((cap) == 0x0B57    /*GL_COLOR_MATERIAL*/               \
                 || (cap) == 0x0BA1 /*GL_NORMALIZE*/                    \
                 || (cap) == 0x803A /*GL_RESCALE_NORMAL*/               \
                 || (cap) == 0x0C60 /*GL_TEXTURE_GEN_S*/                \
                 || (cap) == 0x0C61 /*GL_TEXTURE_GEN_T*/                \
                 || (cap) == 0x0C62 /*GL_TEXTURE_GEN_R*/                \
                 || (cap) == 0x0C63 /*GL_TEXTURE_GEN_Q*/) { /* empty */ \
        } else                                                          \
            ::glDisable(cap);                                           \
    } while (0)

#undef glFogi
#define glFogi(pname, param)                      \
    do {                                          \
        if ((pname) == 0x0B65 /*GL_FOG_MODE*/)    \
            PlatformRenderer.StateSetFogMode(param); \
    } while (0)

#undef glFogf
#define glFogf(pname, param)                              \
    do {                                                  \
        if ((pname) == 0x0B63 /*GL_FOG_START*/)           \
            PlatformRenderer.StateSetFogNearDistance(param); \
        else if ((pname) == 0x0B64 /*GL_FOG_END*/)        \
            PlatformRenderer.StateSetFogFarDistance(param);  \
        else if ((pname) == 0x0B62 /*GL_FOG_DENSITY*/)    \
            PlatformRenderer.StateSetFogDensity(param);      \
    } while (0)

#undef glOrtho
#define glOrtho(left, right, bottom, top, zNear, zFar)                         \
    do {                                                                       \
        PlatformRenderer.MatrixOrthogonal(left, right, bottom, top, zNear, zFar); \
    } while (0)

#undef glMultiTexCoord2f
#define glMultiTexCoord2f(tex, u, v)                     \
    do {                                                 \
        if ((tex) == 0x84C1 /*GL_TEXTURE1*/)             \
            PlatformRenderer.StateSetVertexTextureUV(u, v); \
    } while (0)

#undef glActiveTexture
#define glActiveTexture(tex)                      \
    do {                                          \
        PlatformRenderer.StateSetActiveTexture(tex); \
        ::glActiveTexture(tex);                   \
    } while (0)

#undef glClientActiveTexture
#define glClientActiveTexture(tex)                \
    do {                                          \
        PlatformRenderer.StateSetActiveTexture(tex); \
    } while (0)

// declarations
int glGenTextures_4J();
void glGenTextures_4J(int n, unsigned int* textures);
void glDeleteTextures_4J(int id);
void glDeleteTextures_4J(int n, const unsigned int* textures);
void glTexImage2D_4J(int target, int level, int internalformat, int width,
                     int height, int border, int format, int type,
                     void* pixels);

// helprs
void glGenQueries_4J_Helper(unsigned int* id);
void glGetQueryObjectu_4J_Helper(unsigned int id, unsigned int pname,
                                 unsigned int* val);

template <typename T>
inline void glGenTextures_4J(T* buf) {
    unsigned int id = 0;
    ::glGenTextures(1, &id);
    buf->put((int)id);
    buf->flip();
}
template <typename T>
inline void glDeleteTextures_4J(T* buf) {
    if (buf->limit() > 0) {
        unsigned int id = (unsigned int)buf->get(0);
        ::glDeleteTextures(1, &id);
    }
}
template <typename T>
inline void glTexCoordPointer_4J(int size, int type, T* pointer) {}
template <typename T>
inline void glNormalPointer_4J(int type, T* pointer) {}
template <typename T>
inline void glColorPointer_4J(int size, bool normalized, int stride,
                              T* pointer) {}
template <typename T>
inline void glVertexPointer_4J(int size, int type, T* pointer) {}
template <typename T>
inline void glTexImage2D_4J(int target, int level, int internalformat,
                            int width, int height, int border, int format,
                            int type, T* pixels) {
    void* data = pixels ? pixels->getBuffer() : nullptr;
    ::glTexImage2D((unsigned int)target, level, internalformat, width, height,
                   border, (unsigned int)format, (unsigned int)type, data);
}
template <typename T>
inline void glCallLists_4J(T* lists) {
    int base = lists->position();
    int count = lists->limit() - base;
    for (int i = 0; i < count; i++) {
        PlatformRenderer.CBuffCall(lists->get(base + i));
    }
}
template <typename T>
inline void glGenQueries_4J(T* buf) {
    unsigned int id = 0;
    glGenQueries_4J_Helper(&id);
    buf->put((int)id);
    buf->flip();
}
template <typename T>
inline void glGetQueryObjectu_4J(int id, int pname, T* params) {
    unsigned int val = 0;
    glGetQueryObjectu_4J_Helper((unsigned int)id, (unsigned int)pname, &val);
    params->put((int)val);
    params->flip();
}
template <typename T>
inline void glFog_4J(int pname, T* params) {
    float* p = params->_getDataPointer();
    if (pname == 0x0B66 /* GL_FOG_COLOR */)
        PlatformRenderer.StateSetFogColour(p[0], p[1], p[2]);
}
template <typename T>
inline void glLight_4J(int light, int pname, T* params) {
    float* p = params->_getDataPointer();
    if (pname == 0x1203 /* GL_POSITION */)
        PlatformRenderer.StateSetLightDirection(light == 0x4000 ? 0 : 1, p[0],
                                             p[1], p[2]);
    else if (pname == 0x1200 /* GL_AMBIENT */)
        PlatformRenderer.StateSetLightAmbientColour(p[0], p[1], p[2]);
    else if (pname == 0x1201 /* GL_DIFFUSE */)
        PlatformRenderer.StateSetLightColour(light == 0x4000 ? 0 : 1, p[0], p[1],
                                          p[2]);
}
template <typename T>
inline void glLightModel_4J(int pname, T* params) {
    float* p = params->_getDataPointer();
    if (pname == 0x0B53 /* GL_LIGHT_MODEL_AMBIENT */)
        PlatformRenderer.StateSetLightAmbientColour(p[0], p[1], p[2]);
}
template <typename T>
inline void glTexGen_4J(int coord, int pname, T* params) {}
inline void glReadPixels_4J(int x, int y, int width, int height, int format,
                            int type, void* pixels) {
    ::glReadPixels(x, y, width, height, (unsigned int)format,
                   (unsigned int)type, pixels);
}
inline void glReadPixels_4J(int x, int y, int width, int height, int format,
                            int type, unsigned char* pixels) {
    ::glReadPixels(x, y, width, height, (unsigned int)format,
                   (unsigned int)type, (void*)pixels);
}
// T -> .getBuffer()
template <typename T>
inline void glReadPixels_4J(int x, int y, int width, int height, int format,
                            int type, T* pixels) {
    ::glReadPixels(x, y, width, height, (unsigned int)format,
                   (unsigned int)type, pixels->getBuffer());
}
void glBeginQuery_4J_Helper(unsigned int target, unsigned int id);
void glEndQuery_4J_Helper(unsigned int target);
void glGenQueries_4J_Helper(unsigned int* id);
void glGetQueryObjectu_4J_Helper(unsigned int id, unsigned int pname,
                                 unsigned int* val);

// redirect the functions to my own implementation, no more 2.1 funcs
#define glGenTextures(...) glGenTextures_4J(__VA_ARGS__)
#define glDeleteTextures(...) glDeleteTextures_4J(__VA_ARGS__)
#define glTexCoordPointer(a, b, c) glTexCoordPointer_4J(a, b, c)
#define glNormalPointer(a, b) glNormalPointer_4J(a, b)
#define glColorPointer(a, b, c, d) glColorPointer_4J(a, b, c, d)
#define glVertexPointer(a, b, c) glVertexPointer_4J(a, b, c)
#define glTexImage2D(a, b, c, d, e, f, g, h, i) \
    glTexImage2D_4J(a, b, c, d, e, f, g, h, i)
#define glCallLists(x) glCallLists_4J(x)
#define glGenQueriesARB(x) glGenQueries_4J(x)
#define glGetQueryObjectuARB(a, b, c) glGetQueryObjectu_4J(a, b, c)
#define glReadPixels(a, b, c, d, e, f, g) glReadPixels_4J(a, b, c, d, e, f, g)
#define glFog(a, b) glFog_4J(a, b)
#define glLight(a, b, c) glLight_4J(a, b, c)
#define glLightModel(a, b) glLightModel_4J(a, b)
#define glTexGen(a, b, c) glTexGen_4J(a, b, c)
