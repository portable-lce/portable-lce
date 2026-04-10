#define GDRAW_ASSERTS

#include "gdraw.h"

#include <GL/gl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_video.h"
#include "app/common/Iggy/include/iggy.h"

#ifndef _ENABLEIGGY
void* IggyGDrawMallocAnnotated(SINTa size, const char* file, int line) {
    (void)file;
    (void)line;
    return malloc((size_t)size);
}

void IggyGDrawFree(void* ptr) { free(ptr); }

void IggyGDrawSendWarning(Iggy* f, char const* message, ...) {
    (void)f;
    va_list args;
    va_start(args, message);
    fprintf(stderr, "[Iggy GDraw Warning] ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void IggyDiscardVertexBufferCallback(void* owner, void* buf) {
    (void)owner;
    (void)buf;
}
#endif

// static void* get_gl_proc(const char* name) {
//     void* p = SDL_GL_GetProcAddress(name);
//     if (!p) p = dlsym(RTLD_DEFAULT, name);
//     if (!p) {
//         char buf[256];
//         strncpy(buf, name, sizeof(buf) - 1);
//         buf[255] = '\0';
//         char* ext = strstr(buf, "ARB");
//         if (!ext) ext = strstr(buf, "EXT");
//         if (ext && ext == buf + strlen(buf) - 3) {
//             *ext = '\0';
//             p = SDL_GL_GetProcAddress(buf);
//             if (!p) p = dlsym(RTLD_DEFAULT, buf);
//         }
//     }
//     return p;
// }

#define GDRAW_GL_EXTENSION_LIST                                                \
    /*  identifier                      import procname */                     \
    /* GL_ARB_vertex_buffer_object */                                          \
    GLE(GenBuffers, "GenBuffersARB", GENBUFFERSARB)                            \
    GLE(DeleteBuffers, "DeleteBuffersARB", DELETEBUFFERSARB)                   \
    GLE(BindBuffer, "BindBufferARB", BINDBUFFERARB)                            \
    GLE(BufferData, "BufferDataARB", BUFFERDATAARB)                            \
    GLE(MapBuffer, "MapBufferARB", MAPBUFFERARB)                               \
    GLE(UnmapBuffer, "UnmapBufferARB", UNMAPBUFFERARB)                         \
    GLE(VertexAttribPointer, "VertexAttribPointerARB", VERTEXATTRIBPOINTERARB) \
    GLE(EnableVertexAttribArray, "EnableVertexAttribArrayARB",                 \
        ENABLEVERTEXATTRIBARRAYARB)                                            \
    GLE(DisableVertexAttribArray, "DisableVertexAttribArrayARB",               \
        DISABLEVERTEXATTRIBARRAYARB)                                           \
    /* GL_ARB_shader_objects */                                                \
    GLE(CreateShader, "CreateShaderObjectARB", CREATESHADEROBJECTARB)          \
    GLE(DeleteShader, "DeleteObjectARB", DELETEOBJECTARB)                      \
    GLE(ShaderSource, "ShaderSourceARB", SHADERSOURCEARB)                      \
    GLE(CompileShader, "CompileShaderARB", COMPILESHADERARB)                   \
    GLE(GetShaderiv, "GetObjectParameterivARB", GETOBJECTPARAMETERIVARB)       \
    GLE(GetShaderInfoLog, "GetInfoLogARB", GETINFOLOGARB)                      \
    GLE(CreateProgram, "CreateProgramObjectARB", CREATEPROGRAMOBJECTARB)       \
    GLE(DeleteProgram, "DeleteObjectARB", DELETEOBJECTARB)                     \
    GLE(AttachShader, "AttachObjectARB", ATTACHOBJECTARB)                      \
    GLE(LinkProgram, "LinkProgramARB", LINKPROGRAMARB)                         \
    GLE(GetUniformLocation, "GetUniformLocationARB", GETUNIFORMLOCATIONARB)    \
    GLE(UseProgram, "UseProgramObjectARB", USEPROGRAMOBJECTARB)                \
    GLE(GetProgramiv, "GetObjectParameterivARB", GETOBJECTPARAMETERIVARB)      \
    GLE(GetProgramInfoLog, "GetInfoLogARB", GETINFOLOGARB)                     \
    GLE(Uniform1i, "Uniform1iARB", UNIFORM1IARB)                               \
    GLE(Uniform4f, "Uniform4fARB", UNIFORM4FARB)                               \
    GLE(Uniform4fv, "Uniform4fvARB", UNIFORM4FVARB)                            \
    /* GL_ARB_vertex_shader */                                                 \
    GLE(BindAttribLocation, "BindAttribLocationARB", BINDATTRIBLOCATIONARB)    \
    /* Missing from WGL but needed by shared code */                           \
    GLE(Uniform1f, "Uniform1fARB", UNIFORM1FARB)                               \
    /* GL_EXT_framebuffer_object */                                            \
    GLE(GenRenderbuffers, "GenRenderbuffersEXT", GENRENDERBUFFERSEXT)          \
    GLE(DeleteRenderbuffers, "DeleteRenderbuffersEXT", DELETERENDERBUFFERSEXT) \
    GLE(BindRenderbuffer, "BindRenderbufferEXT", BINDRENDERBUFFEREXT)          \
    GLE(RenderbufferStorage, "RenderbufferStorageEXT", RENDERBUFFERSTORAGEEXT) \
    GLE(GenFramebuffers, "GenFramebuffersEXT", GENFRAMEBUFFERSEXT)             \
    GLE(DeleteFramebuffers, "DeleteFramebuffersEXT", DELETEFRAMEBUFFERSEXT)    \
    GLE(BindFramebuffer, "BindFramebufferEXT", BINDFRAMEBUFFEREXT)             \
    GLE(CheckFramebufferStatus, "CheckFramebufferStatusEXT",                   \
        CHECKFRAMEBUFFERSTATUSEXT)                                             \
    GLE(FramebufferRenderbuffer, "FramebufferRenderbufferEXT",                 \
        FRAMEBUFFERRENDERBUFFEREXT)                                            \
    GLE(FramebufferTexture2D, "FramebufferTexture2DEXT",                       \
        FRAMEBUFFERTEXTURE2DEXT)                                               \
    GLE(GenerateMipmap, "GenerateMipmapEXT", GENERATEMIPMAPEXT)                \
    /* GL_EXT_framebuffer_blit */                                              \
    GLE(BlitFramebuffer, "BlitFramebufferEXT", BLITFRAMEBUFFEREXT)             \
    /* GL_EXT_framebuffer_multisample */                                       \
    GLE(RenderbufferStorageMultisample, "RenderbufferStorageMultisampleEXT",   \
        RENDERBUFFERSTORAGEMULTISAMPLEEXT)                                     \
    /* <end> */

// Shared .inl
#define gdraw_GLx_(id) gdraw_GL_##id
#define GDRAW_GLx_(id) GDRAW_GL_##id
#define GDRAW_SHADERS "gdraw_gl_shaders.inl"

// GLhandleARB is void* but shader functions use GLuint values.
// homework stolen from gdraw_gl_shared.inl.
#define GDrawGLProgram GLuint
typedef GLuint GLhandle;
typedef gdraw_gl_resourcetype gdraw_resourcetype;

#define GLE(id, import, procname) static PFNGL##procname##PROC gl##id;
GDRAW_GL_EXTENSION_LIST
#undef GLE

typedef const GLubyte*(APIENTRYP PFNGLGETSTRINGIPROC_)(GLenum name,
                                                       GLuint index);
static PFNGLGETSTRINGIPROC_ gdraw_glGetStringi = NULL;

typedef void(APIENTRYP PFNGLGENVERTEXARRAYSPROC_)(GLsizei n, GLuint* arrays);
typedef void(APIENTRYP PFNGLBINDVERTEXARRAYPROC_)(GLuint array);
static PFNGLGENVERTEXARRAYSPROC_ gdraw_glGenVertexArrays = NULL;
static PFNGLBINDVERTEXARRAYPROC_ gdraw_glBindVertexArray = NULL;
static GLuint gdraw_vao = 0;

typedef void(APIENTRYP gdraw_vtxattrib_fn)(GLuint, GLint, GLenum, GLboolean,
                                           GLsizei, const void*);
static gdraw_vtxattrib_fn gdraw_real_vtxattrib = NULL;
static GLuint gdraw_screenvbo = 0;
static const void* gdraw_screenvbo_base = NULL;
static size_t gdraw_expected_vbo_size = 0;

typedef void(APIENTRYP gdraw_drawelements_fn)(GLenum mode, GLsizei count,
                                              GLenum type, const void* indices);
static gdraw_drawelements_fn gdraw_real_drawelements = NULL;
static GLuint gdraw_screenibo = 0;

typedef GLuint(APIENTRYP gdraw_createshader_fn)(GLenum);
typedef void(APIENTRYP gdraw_shadersource_fn)(GLuint, GLsizei, const GLchar**,
                                              const GLint*);
typedef void(APIENTRYP gdraw_compileshader_fn)(GLuint);
typedef void(APIENTRYP gdraw_linkprogram_fn)(GLuint);
static gdraw_createshader_fn gdraw_real_createshader = NULL;
static gdraw_shadersource_fn gdraw_real_shadersource = NULL;
static gdraw_compileshader_fn gdraw_real_compileshader = NULL;
static gdraw_linkprogram_fn gdraw_real_linkprogram = NULL;

// some core reject p0

typedef void(APIENTRYP gdraw_useprogram_fn)(GLuint);
static gdraw_useprogram_fn gdraw_real_useprogram = NULL;
static GLuint gdraw_null_program = 0;

typedef void(APIENTRYP gdraw_teximage2d_fn)(GLenum, GLint, GLint, GLsizei,
                                            GLsizei, GLint, GLenum, GLenum,
                                            const void*);
typedef void(APIENTRYP gdraw_texsubimage2d_fn)(GLenum, GLint, GLint, GLint,
                                               GLsizei, GLsizei, GLenum, GLenum,
                                               const void*);
static gdraw_teximage2d_fn gdraw_real_teximage2d = NULL;
static gdraw_texsubimage2d_fn gdraw_real_texsubimage2d = NULL;

#define TRY(ptr, arb, core)             \
    do {                                \
        void* _p = SDL_GL_GetProcAddress(core);   \
        if (!_p) _p = SDL_GL_GetProcAddress(arb); \
        *(void**)&(ptr) = _p;           \
    } while (0)

static void load_extensions(void) {
// gl_shared requires ts shit ugh
#define GLE(id, import, procname) \
    gl##id = (PFNGL##procname##PROC)SDL_GL_GetProcAddress("gl" import);
    GDRAW_GL_EXTENSION_LIST
#undef GLE

    TRY(glCreateShader, "glCreateShaderObjectARB", "glCreateShader");
    TRY(glDeleteShader, "glDeleteObjectARB", "glDeleteShader");
    TRY(glShaderSource, "glShaderSourceARB", "glShaderSource");
    TRY(glCompileShader, "glCompileShaderARB", "glCompileShader");
    TRY(glGetShaderiv, "glGetObjectParameterivARB", "glGetShaderiv");
    TRY(glGetShaderInfoLog, "glGetInfoLogARB", "glGetShaderInfoLog");
    TRY(glCreateProgram, "glCreateProgramObjectARB", "glCreateProgram");
    TRY(glDeleteProgram, "glDeleteObjectARB", "glDeleteProgram");
    TRY(glAttachShader, "glAttachObjectARB", "glAttachShader");
    TRY(glLinkProgram, "glLinkProgramARB", "glLinkProgram");
    TRY(glGetUniformLocation, "glGetUniformLocationARB",
        "glGetUniformLocation");
    TRY(glUseProgram, "glUseProgramObjectARB", "glUseProgram");
    TRY(glGetProgramiv, "glGetObjectParameterivARB", "glGetProgramiv");
    TRY(glGetProgramInfoLog, "glGetInfoLogARB", "glGetProgramInfoLog");
    TRY(glUniform1i, "glUniform1iARB", "glUniform1i");
    TRY(glUniform4f, "glUniform4fARB", "glUniform4f");
    TRY(glUniform4fv, "glUniform4fvARB", "glUniform4fv");
    TRY(glUniform1f, "glUniform1fARB", "glUniform1f");
    TRY(glBindAttribLocation, "glBindAttribLocationARB",
        "glBindAttribLocation");

    TRY(glGenBuffers, "glGenBuffersARB", "glGenBuffers");
    TRY(glDeleteBuffers, "glDeleteBuffersARB", "glDeleteBuffers");
    TRY(glBindBuffer, "glBindBufferARB", "glBindBuffer");
    TRY(glBufferData, "glBufferDataARB", "glBufferData");
    TRY(glMapBuffer, "glMapBufferARB", "glMapBuffer");
    TRY(glUnmapBuffer, "glUnmapBufferARB", "glUnmapBuffer");
    TRY(glVertexAttribPointer, "glVertexAttribPointerARB",
        "glVertexAttribPointer");
    TRY(glEnableVertexAttribArray, "glEnableVertexAttribArrayARB",
        "glEnableVertexAttribArray");
    TRY(glDisableVertexAttribArray, "glDisableVertexAttribArrayARB",
        "glDisableVertexAttribArray");

    TRY(glGenRenderbuffers, "glGenRenderbuffersEXT", "glGenRenderbuffers");
    TRY(glDeleteRenderbuffers, "glDeleteRenderbuffersEXT",
        "glDeleteRenderbuffers");
    TRY(glBindRenderbuffer, "glBindRenderbufferEXT", "glBindRenderbuffer");
    TRY(glRenderbufferStorage, "glRenderbufferStorageEXT",
        "glRenderbufferStorage");
    TRY(glGenFramebuffers, "glGenFramebuffersEXT", "glGenFramebuffers");
    TRY(glDeleteFramebuffers, "glDeleteFramebuffersEXT",
        "glDeleteFramebuffers");
    TRY(glBindFramebuffer, "glBindFramebufferEXT", "glBindFramebuffer");
    TRY(glCheckFramebufferStatus, "glCheckFramebufferStatusEXT",
        "glCheckFramebufferStatus");
    TRY(glFramebufferRenderbuffer, "glFramebufferRenderbufferEXT",
        "glFramebufferRenderbuffer");
    TRY(glFramebufferTexture2D, "glFramebufferTexture2DEXT",
        "glFramebufferTexture2D");
    TRY(glGenerateMipmap, "glGenerateMipmapEXT", "glGenerateMipmap");
    TRY(glBlitFramebuffer, "glBlitFramebufferEXT", "glBlitFramebuffer");
    TRY(glRenderbufferStorageMultisample, "glRenderbufferStorageMultisampleEXT",
        "glRenderbufferStorageMultisample");

    // Save raw pointers before we #define over the names below
    gdraw_real_vtxattrib =
        (gdraw_vtxattrib_fn)SDL_GL_GetProcAddress("glVertexAttribPointer");
    gdraw_real_createshader =
        (gdraw_createshader_fn)SDL_GL_GetProcAddress("glCreateShader");
    gdraw_real_shadersource =
        (gdraw_shadersource_fn)SDL_GL_GetProcAddress("glShaderSource");
    gdraw_real_compileshader =
        (gdraw_compileshader_fn)SDL_GL_GetProcAddress("glCompileShader");
    gdraw_real_linkprogram = (gdraw_linkprogram_fn)SDL_GL_GetProcAddress("glLinkProgram");
    gdraw_real_teximage2d = (gdraw_teximage2d_fn)SDL_GL_GetProcAddress("glTexImage2D");
    gdraw_real_texsubimage2d =
        (gdraw_texsubimage2d_fn)SDL_GL_GetProcAddress("glTexSubImage2D");
    gdraw_real_useprogram = (gdraw_useprogram_fn)SDL_GL_GetProcAddress("glUseProgram");
    gdraw_real_drawelements =
        (gdraw_drawelements_fn)SDL_GL_GetProcAddress("glDrawElements");

    gdraw_glGetStringi = (PFNGLGETSTRINGIPROC_)SDL_GL_GetProcAddress("glGetStringi");
    gdraw_glGenVertexArrays =
        (PFNGLGENVERTEXARRAYSPROC_)SDL_GL_GetProcAddress("glGenVertexArrays");
    gdraw_glBindVertexArray =
        (PFNGLBINDVERTEXARRAYPROC_)SDL_GL_GetProcAddress("glBindVertexArray");

    if (gdraw_glGenVertexArrays && gdraw_glBindVertexArray && gdraw_vao == 0) {
        gdraw_glGenVertexArrays(1, &gdraw_vao);
        gdraw_glBindVertexArray(gdraw_vao);
    }
}

#undef TRY

// rebind vbo

static void clear_renderstate_platform_specific(void) {
    if (gdraw_glBindVertexArray && gdraw_vao)
        gdraw_glBindVertexArray(gdraw_vao);
}

static void error_msg_platform_specific(const char* msg) {
    fprintf(stderr, "[GDraw] %s\n", msg);
}

#define GDRAW_PLATFORM_REPORT_GL_SITE(site)                         \
    do {                                                            \
        if ((site) != NULL)                                         \
            fprintf(stderr, "[GDraw] GL error site: %s\n", (site)); \
    } while (0)

#define GDRAW_MULTISAMPLING

// i wish i could improve this function
#ifdef RR_BREAK
#undef RR_BREAK
#endif
#define RR_BREAK()                                                          \
    do {                                                                    \
        fprintf(stderr, "[GDraw] GL error at %s:%d\n", __FILE__, __LINE__); \
    } while (0)

// the magic number that tropical told me
#define GDRAW_MAX_SHADERS 64
static struct {
    GLuint handle;
    GLenum type;
} gdraw_shader_types[GDRAW_MAX_SHADERS];
static int gdraw_shader_type_count = 0;

static GLenum gdraw_get_shader_type(GLuint shader) {
    for (int i = 0; i < gdraw_shader_type_count; i++)
        if (gdraw_shader_types[i].handle == shader)
            return gdraw_shader_types[i].type;
    return GL_FRAGMENT_SHADER;
}

static GLuint gdraw_CreateShaderTracked(GLenum type) {
    GLuint h = gdraw_real_createshader(type);
    if (h && gdraw_shader_type_count < GDRAW_MAX_SHADERS) {
        gdraw_shader_types[gdraw_shader_type_count].handle = h;
        gdraw_shader_types[gdraw_shader_type_count].type = type;
        gdraw_shader_type_count++;
    }
    return h;
}

static void gdraw_CompileShaderAndLog(GLuint shader) {
    GLint status = 0;
    gdraw_real_compileshader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[2048];
        GLint len = 0;
        glGetShaderInfoLog(shader, (GLsizei)sizeof(log) - 1, &len, log);
        log[len] = '\0';
        fprintf(stderr, "[GDraw GLSL] compile FAILED shader=%u:\n%s\n", shader,
                log);
    }
}

static void gdraw_LinkProgramAndLog(GLuint program) {
    GLint status = 0;
    gdraw_real_linkprogram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[2048];
        GLint len = 0;
        glGetProgramInfoLog(program, (GLsizei)sizeof(log) - 1, &len, log);
        log[len] = '\0';
        fprintf(stderr, "[GDraw GLSL] link FAILED program=%u:\n%s\n", program,
                log);
    }
}

#undef glCreateShader
#define glCreateShader gdraw_CreateShaderTracked

// This is the part that turns the old ugly shaders to 330
static char* gdraw_strreplace(char* src, const char* find, const char* rep) {
    char* result;
    char* pos;
    char* base = src;
    size_t find_len = strlen(find);
    size_t rep_len = strlen(rep);
    size_t count = 0;
    char* tmp = src;

    while ((tmp = strstr(tmp, find))) {
        count++;
        tmp += find_len;
    }
    if (!count) return src;

    size_t src_len = strlen(src);
    ptrdiff_t delta = (ptrdiff_t)rep_len - (ptrdiff_t)find_len;
    size_t new_len = src_len + 1;
    if (delta > 0)
        new_len += (size_t)delta * count;
    else
        new_len -= (size_t)(-delta) * count;
    result = (char*)malloc(new_len);
    if (!result) return src;

    tmp = result;
    while ((pos = strstr(src, find))) {
        size_t before = (size_t)(pos - src);
        memcpy(tmp, src, before);
        tmp += before;
        memcpy(tmp, rep, rep_len);
        tmp += rep_len;
        src = pos + find_len;
    }
    memcpy(tmp, src, strlen(src) + 1);
    free(base);
    return result;
}

static void gdraw_ShaderSourceUpgraded(GLuint shader, GLsizei count,
                                       const GLchar** strings,
                                       const GLint* lengths) {
    size_t total = 0;
    for (int i = 0; i < count; i++)
        total += lengths ? (lengths[i] >= 0 ? (size_t)lengths[i]
                                            : strlen(strings[i]))
                         : strlen(strings[i]);

    char* src = (char*)malloc(total + 1);
    if (!src) {
        gdraw_real_shadersource(shader, count, strings, lengths);
        return;
    }

    char* dst = src;
    for (int i = 0; i < count; i++) {
        size_t len = lengths ? (lengths[i] >= 0 ? (size_t)lengths[i]
                                                : strlen(strings[i]))
                             : strlen(strings[i]);
        memcpy(dst, strings[i], len);
        dst += len;
    }
    *dst = '\0';

    int is_vert = (gdraw_get_shader_type(shader) == GL_VERTEX_SHADER);

    // Strip any existing #version directive as i'll add our own
    {
        char* vp = strstr(src, "#version");
        if (vp) {
            char* nl = strchr(vp, '\n');
            if (nl)
                memmove(vp, nl + 1, strlen(nl + 1) + 1);
            else
                *vp = '\0';
        }
    }

    // Texture built-ins
    src = gdraw_strreplace(src, "texture2DRect", "texture");
    src = gdraw_strreplace(src, "texture2D", "texture");

    // Attribute -> in
    src = gdraw_strreplace(src, "attribute ", "in ");
    src = gdraw_strreplace(src, "attribute\t", "in\t");
    src = gdraw_strreplace(src, "attribute\n", "in\n");

    // Varying -> out (vert) / in (frag)
    if (is_vert) {
        src = gdraw_strreplace(src, "varying ", "out ");
        src = gdraw_strreplace(src, "varying\t", "out\t");
        src = gdraw_strreplace(src, "varying\n", "out\n");
    } else {
        src = gdraw_strreplace(src, "varying ", "in ");
        src = gdraw_strreplace(src, "varying\t", "in\t");
        src = gdraw_strreplace(src, "varying\n", "in\n");
        src = gdraw_strreplace(src, "gl_FragData[0]", "_gdraw_frag_out");
        src = gdraw_strreplace(src, "gl_FragColor", "_gdraw_frag_out");
    }

    const char* header = is_vert
                             ? "#version 330 core\n"
                             : "#version 330 core\nout vec4 _gdraw_frag_out;\n";
    char* patched = (char*)malloc(strlen(header) + strlen(src) + 2);
    if (!patched) {
        free(src);
        gdraw_real_shadersource(shader, count, strings, lengths);
        return;
    }
    strcpy(patched, header);
    strcat(patched, src);
    free(src);

    const GLchar* patched_ptr = (const GLchar*)patched;
    gdraw_real_shadersource(shader, 1, &patched_ptr, NULL);
    free(patched);
}

#undef glShaderSource
#define glShaderSource gdraw_ShaderSourceUpgraded

// Remap all the deprecated internal formats to their modern equivalents
// (idk why but just the word "swizzle" is cracking me up)
static void gdraw_apply_swizzle(GLenum internal_fmt) {
    if (internal_fmt == 0x1906 /* GL_ALPHA */ || internal_fmt == GL_RED) {
        GLint sw[4] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, sw);
    } else if (internal_fmt == 0x1909 /* GL_LUMINANCE */) {
        GLint sw[4] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, sw);
    } else if (internal_fmt == 0x190A /* GL_LUMINANCE_ALPHA */) {
        GLint sw[4] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, sw);
    }
}

static GLenum gdraw_remap_fmt(GLenum fmt) {
    switch (fmt) {
        case 0x1906:
            return GL_RED;  // GL_ALPHA
        case 0x1909:
            return GL_RED;  // GL_LUMINANCE
        case 0x190A:
            return GL_RG;  // GL_LUMINANCE_ALPHA
        case 0x8033:
            return GL_RG;  // GL_LUMINANCE4_ALPHA4
        case 0x8045:
            return GL_R8;  // GL_LUMINANCE8
        case 0x8048:
            return GL_RG8;  // GL_LUMINANCE8_ALPHA8
        case 0x804F:
            return GL_R8;  // GL_INTENSITY4
        case 0x8050:
            return GL_R8;  // GL_INTENSITY8
        default:
            return fmt;
    }
}

static void gdraw_TexImage2D(GLenum target, GLint level, GLint ifmt, GLsizei w,
                             GLsizei h, GLint border, GLenum fmt, GLenum type,
                             const void* data) {
    // ES strictly requires explicitly sized formats & stuff
    if (ifmt == GL_RGBA && data == NULL) ifmt = GL_RGBA8;

    GLenum new_ifmt = gdraw_remap_fmt((GLenum)ifmt);
    GLenum new_fmt = gdraw_remap_fmt(fmt);
    gdraw_real_teximage2d(target, level, (GLint)new_ifmt, w, h, border, new_fmt,
                          type, data);
    if (new_ifmt != (GLenum)ifmt) gdraw_apply_swizzle((GLenum)ifmt);
}

static void gdraw_TexSubImage2D(GLenum target, GLint level, GLint xoff,
                                GLint yoff, GLsizei w, GLsizei h, GLenum fmt,
                                GLenum type, const void* data) {
    GLenum new_fmt = gdraw_remap_fmt(fmt);
    gdraw_real_texsubimage2d(target, level, xoff, yoff, w, h, new_fmt, type,
                             data);
}

#undef glTexImage2D
#define glTexImage2D gdraw_TexImage2D
#undef glTexSubImage2D
#define glTexSubImage2D gdraw_TexSubImage2D

// vbo emu
static void gdraw_ClientVertexAttribPointer(GLuint index, GLint size,
                                            GLenum type, GLboolean normalized,
                                            GLsizei stride,
                                            const void* pointer) {
    if (gdraw_glBindVertexArray && gdraw_vao) {
        GLint current_vao = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
        if ((GLuint)current_vao != gdraw_vao)
            gdraw_glBindVertexArray(gdraw_vao);
    }

    GLint current_vbo = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_vbo);

    if (current_vbo != 0 && current_vbo != (GLint)gdraw_screenvbo) {
        // no touchies
        gdraw_real_vtxattrib(index, size, type, normalized, stride, pointer);
        return;
    }

    if (pointer == NULL) {
        gdraw_real_vtxattrib(index, size, type, normalized, stride, pointer);
        return;
    }

    ptrdiff_t offset =
        gdraw_screenvbo_base
            ? ((const char*)pointer - (const char*)gdraw_screenvbo_base)
            : -1;

    if (gdraw_screenvbo_base == NULL || offset < 0 ||
        offset >= (ptrdiff_t)gdraw_expected_vbo_size) {
        if (!gdraw_screenvbo) glGenBuffers(1, &gdraw_screenvbo);
        glBindBuffer(GL_ARRAY_BUFFER, gdraw_screenvbo);

        size_t upload_size = gdraw_expected_vbo_size > 0
                                 ? (gdraw_expected_vbo_size + 256)
                                 : 65536;
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)upload_size, pointer,
                     GL_STREAM_DRAW);

        gdraw_screenvbo_base = pointer;
        gdraw_real_vtxattrib(index, size, type, normalized, stride,
                             (const void*)0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, gdraw_screenvbo);
        gdraw_real_vtxattrib(index, size, type, normalized, stride,
                             (const void*)offset);
    }
}

#undef glVertexAttribPointer
#define glVertexAttribPointer gdraw_ClientVertexAttribPointer

// fake ibo
static void hooked_glDrawElements(GLenum mode, GLsizei count, GLenum type,
                                  const void* indices) {
    GLint current_ibo = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &current_ibo);

    if (current_ibo == 0 && indices != NULL) {
        if (!gdraw_screenibo) glGenBuffers(1, &gdraw_screenibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gdraw_screenibo);

        size_t index_size = (type == GL_UNSIGNED_SHORT)  ? 2
                            : (type == GL_UNSIGNED_BYTE) ? 1
                                                         : 4;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(count * index_size),
                     indices, GL_STREAM_DRAW);

        gdraw_real_drawelements(mode, count, type, (const void*)0);
    } else {
        gdraw_real_drawelements(mode, count, type, indices);
    }
}

#define glDrawElements hooked_glDrawElements

// dummy shader for glUseProgram(0) safety
static void gdraw_UseProgramSafe(GLuint program) {
    if (!program) {
        if (!gdraw_null_program && gdraw_real_useprogram) {
            const char* vs =
                "#version 330 core\nvoid main(){gl_Position=vec4(0);}";
            const char* fs =
                "#version 330 core\nout vec4 c;\nvoid main(){c=vec4(0);}";
            GLuint v = gdraw_real_createshader(GL_VERTEX_SHADER);
            GLuint f = gdraw_real_createshader(GL_FRAGMENT_SHADER);
            gdraw_real_shadersource(v, 1, &vs, NULL);
            gdraw_real_shadersource(f, 1, &fs, NULL);
            gdraw_real_compileshader(v);
            gdraw_real_compileshader(f);
            gdraw_null_program = glCreateProgram();
            glAttachShader(gdraw_null_program, v);
            glAttachShader(gdraw_null_program, f);
            gdraw_real_linkprogram(gdraw_null_program);
            glDeleteShader(v);
            glDeleteShader(f);
        }
        gdraw_real_useprogram(gdraw_null_program);
        return;
    }
    gdraw_real_useprogram(program);
}

#undef glUseProgram
#define glUseProgram gdraw_UseProgramSafe
#undef glCompileShader
#define glCompileShader gdraw_CompileShaderAndLog
#undef glLinkProgram
#define glLinkProgram gdraw_LinkProgramAndLog

static void gdraw_FramebufferRenderbufferSafe(GLenum target, GLenum attachment,
                                              GLenum renderbuffertarget,
                                              GLuint renderbuffer) {
    static GLuint last_depth_rb = 0;

    if (attachment == GL_DEPTH_ATTACHMENT) {
        last_depth_rb = renderbuffer;
        (glFramebufferRenderbuffer)(target, attachment, renderbuffertarget,
                                    renderbuffer);
    } else if (attachment == GL_STENCIL_ATTACHMENT) {
        if (renderbuffer == last_depth_rb && renderbuffer != 0) {
            // If identical, bind as packed depth-stencil to satisfy strict GLES
            // ^ how greedy -n-
            (glFramebufferRenderbuffer)(
                target, 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */,
                renderbuffertarget, renderbuffer);
        } else {
            (glFramebufferRenderbuffer)(target, attachment, renderbuffertarget,
                                        renderbuffer);
        }
    } else {
        (glFramebufferRenderbuffer)(target, attachment, renderbuffertarget,
                                    renderbuffer);
    }
}
#define glFramebufferRenderbuffer_SAFE gdraw_FramebufferRenderbufferSafe
#define glFramebufferRenderbuffer glFramebufferRenderbuffer_SAFE

#include "gdraw_gl_shared.inl"

#undef glVertexAttribPointer
#define glVertexAttribPointer gdraw_real_vtxattrib

static int hasext_core(const char* name) {
    GLint n = 0;
    if (!gdraw_glGetStringi) return 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++) {
        const char* e =
            (const char*)gdraw_glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (e && strcmp(e, name) == 0) return 1;
    }
    return 0;
}

static gdraw_draw_indexed_triangles* real_DrawIndexedTriangles = NULL;

static void RADLINK hooked_DrawIndexedTriangles(GDrawRenderState* r,
                                                GDrawPrimitive* prim,
                                                GDrawVertexBuffer* buf,
                                                GDrawStats* stats) {
    if (buf == NULL && prim != NULL && prim->vertices != NULL) {
        size_t stride = 8;
        if (prim->vertex_format == GDRAW_vformat_v2aa)
            stride = 16;
        else if (prim->vertex_format == GDRAW_vformat_v2tc2)
            stride = 16;
        else if (prim->vertex_format == GDRAW_vformat_ihud1)
            stride = 20;
        gdraw_expected_vbo_size = prim->num_vertices * stride;
    } else {
        gdraw_expected_vbo_size = 0;
    }
    gdraw_screenvbo_base = NULL;  // Force VBO re-upload for each primitive
    real_DrawIndexedTriangles(r, prim, buf, stats);
}

static gdraw_filter_quad* real_FilterQuad = NULL;

static void RADLINK hooked_FilterQuad(GDrawRenderState* r, S32 x0, S32 y0,
                                      S32 x1, S32 y1, GDrawStats* stats) {
    gdraw_expected_vbo_size = 4 * 20;  // 4 vertices, max stride
    gdraw_screenvbo_base = NULL;
    real_FilterQuad(r, x0, y0, x1, y1, stats);
}

static gdraw_rendering_begin* real_RenderingBegin = NULL;

// stupid hack
static void RADLINK hooked_RenderingBegin(void) {
    if (real_RenderingBegin) real_RenderingBegin();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    OPENGL_CHECK_SITE("hooked_RenderingBegin:post_state");
}

// Creating the context
GDrawFunctions* gdraw_GL_CreateContext(S32 w, S32 h, S32 msaa_samples) {
    static const TextureFormatDesc tex_formats[] = {
        {IFT_FORMAT_rgba_8888, 1, 1, 4, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
        {IFT_FORMAT_rgba_4444_LE, 1, 1, 2, GL_RGBA4, GL_RGBA,
         GL_UNSIGNED_SHORT_4_4_4_4},
        {IFT_FORMAT_rgba_5551_LE, 1, 1, 2, GL_RGB5_A1, GL_RGBA,
         GL_UNSIGNED_SHORT_5_5_5_1},
        {IFT_FORMAT_la_88, 1, 1, 2, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA,
         GL_UNSIGNED_BYTE},
        {IFT_FORMAT_la_44, 1, 1, 1, GL_LUMINANCE4_ALPHA4, GL_LUMINANCE_ALPHA,
         GL_UNSIGNED_BYTE},
        {IFT_FORMAT_i_8, 1, 1, 1, GL_INTENSITY8, GL_ALPHA, GL_UNSIGNED_BYTE},
        {IFT_FORMAT_i_4, 1, 1, 1, GL_INTENSITY4, GL_ALPHA, GL_UNSIGNED_BYTE},
        {IFT_FORMAT_l_8, 1, 1, 1, GL_LUMINANCE8, GL_LUMINANCE,
         GL_UNSIGNED_BYTE},
        {IFT_FORMAT_l_4, 1, 1, 1, GL_LUMINANCE4, GL_LUMINANCE,
         GL_UNSIGNED_BYTE},
        {IFT_FORMAT_DXT1, 4, 4, 8, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0,
         GL_UNSIGNED_BYTE},
        {IFT_FORMAT_DXT3, 4, 4, 16, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0,
         GL_UNSIGNED_BYTE},
        {IFT_FORMAT_DXT5, 4, 4, 16, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0,
         GL_UNSIGNED_BYTE},
        {0, 0, 0, 0, 0, 0, 0},
    };

    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major < 3) {
        fprintf(stderr, "[GDraw] GL 3.0 or higher required (got %d.%d)\n",
                major, minor);
        return NULL;
    }

    load_extensions();

    if (gdraw_glBindVertexArray && gdraw_vao)
        gdraw_glBindVertexArray(gdraw_vao);

    GDrawFunctions* funcs = create_context(w, h);
    if (!funcs) return NULL;

    // hook the vtable entries for VBO reset and render state
    real_DrawIndexedTriangles = funcs->DrawIndexedTriangles;
    funcs->DrawIndexedTriangles = hooked_DrawIndexedTriangles;

    real_FilterQuad = funcs->FilterQuad;
    funcs->FilterQuad = hooked_FilterQuad;

    real_RenderingBegin = funcs->RenderingBegin;
    funcs->RenderingBegin = hooked_RenderingBegin;
    funcs->ClearID = gdraw_ClearID;

    gdraw->tex_formats = tex_formats;
    gdraw->has_mapbuffer = false;
    gdraw->has_depth24 = true;
    gdraw->has_texture_max_level = true;

    gdraw->has_packed_depth_stencil = true;

    GLint n = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &n);
    gdraw->has_conditional_non_power_of_two = (n < 8192);

    if (msaa_samples > 1) {
        glGetIntegerv(GL_MAX_SAMPLES, &n);
        gdraw->multisampling = RR_MIN(msaa_samples, n);
    }

    opengl_check();
    fprintf(stderr, "[GDraw] Context created successfully (%dx%d, msaa=%d)\n",
            w, h, msaa_samples);
    return funcs;
}

// Custom draw callbacks
void gdraw_GL_BeginCustomDraw_4J(IggyCustomDrawCallbackRegion* region,
                                 F32* matrix) {
    // rebind vbo
    if (gdraw_glBindVertexArray && gdraw_vao)
        gdraw_glBindVertexArray(gdraw_vao);
    clear_renderstate();
    gdraw_GetObjectSpaceMatrix(matrix, region->o2w, gdraw->projection,
                               depth_from_id(0), 0);
}

void gdraw_GL_CalculateCustomDraw_4J(IggyCustomDrawCallbackRegion* region,
                                     F32* matrix) {
    gdraw_GetObjectSpaceMatrix(matrix, region->o2w, gdraw->projection, 0.0f, 0);
}
