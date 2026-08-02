#include "BgfxRenderPath.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <cstdio>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bgfx/defines.h"
#include "platform/PlatformTypes.h"

#define STB_IMAGE_IMPLEMENTATION
#include "shaders/fs_4jcraft.bin.h"
#include "shaders/vs_4jcraft.bin.h"
#include "stb_image.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#endif

using namespace rp;

thread_local int BgfxRenderPath::cbuf_rec_id_ = -1;
thread_local std::vector<uint8_t> BgfxRenderPath::cbuf_rec_verts_;
thread_local std::vector<BgfxRenderPath::CBuffDrawCmd>
    BgfxRenderPath::cbuf_rec_draws_;
thread_local BgfxRenderPath::RenderState BgfxRenderPath::state_;

static constexpr uint32_t TRANSIENT_ARENA_SIZE = 16 * 1024 * 1024;
constexpr float Z_BIAS_EPSILON = 6e-5f;

static const char* bgfx_renderer_name(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Noop:
            return "Noop";
        case bgfx::RendererType::Agc:
            return "Agc";
        case bgfx::RendererType::Direct3D11:
            return "Direct3D11";
        case bgfx::RendererType::Direct3D12:
            return "Direct3D12";
        case bgfx::RendererType::Gnm:
            return "Gnm";
        case bgfx::RendererType::Metal:
            return "Metal";
        case bgfx::RendererType::Nvn:
            return "Nvn";
        case bgfx::RendererType::OpenGL:
            return "OpenGL";
        case bgfx::RendererType::OpenGLES:
            return "OpenGLES";
        case bgfx::RendererType::Vulkan:
            return "Vulkan";
        case bgfx::RendererType::WebGPU:
            return "WebGPU";
        default:
            return "Unknown";
    }
}

static uint64_t bgfx_blend_factor(rp::BlendFactor f) {
    switch (f) {
        case rp::BlendFactor::zero:
            return BGFX_STATE_BLEND_ZERO;
        case rp::BlendFactor::one:
            return BGFX_STATE_BLEND_ONE;
        case rp::BlendFactor::src_color:
            return BGFX_STATE_BLEND_SRC_COLOR;
        case rp::BlendFactor::one_minus_src_color:
            return BGFX_STATE_BLEND_INV_SRC_COLOR;
        case rp::BlendFactor::src_alpha:
            return BGFX_STATE_BLEND_SRC_ALPHA;
        case rp::BlendFactor::one_minus_src_alpha:
            return BGFX_STATE_BLEND_INV_SRC_ALPHA;
        case rp::BlendFactor::dst_color:
            return BGFX_STATE_BLEND_DST_COLOR;
        case rp::BlendFactor::one_minus_dst_color:
            return BGFX_STATE_BLEND_INV_DST_COLOR;
        case rp::BlendFactor::dst_alpha:
            return BGFX_STATE_BLEND_DST_ALPHA;
        case rp::BlendFactor::one_minus_dst_alpha:
            return BGFX_STATE_BLEND_INV_DST_ALPHA;
        // PLCE: these were BGFX_STATE_BLEND_FACTOR /
        // BGFX_STATE_BLEND_INV_FACTOR, switching them to SRC_ALPHA /
        // INV_SRC_ALPHA fixes the invisible HUD.
        case rp::BlendFactor::constant_alpha:
            return BGFX_STATE_BLEND_SRC_ALPHA;
        case rp::BlendFactor::one_minus_constant_alpha:
            return BGFX_STATE_BLEND_INV_SRC_ALPHA;
    }
    return BGFX_STATE_BLEND_ONE;
}

static uint64_t bgfx_depth_func(rp::DepthTest t) {
    switch (t) {
        case rp::DepthTest::off:
            return BGFX_STATE_DEPTH_TEST_ALWAYS;
        case rp::DepthTest::less:
            return BGFX_STATE_DEPTH_TEST_LESS;
        case rp::DepthTest::less_equal:
            return BGFX_STATE_DEPTH_TEST_LEQUAL;
        case rp::DepthTest::equal:
            return BGFX_STATE_DEPTH_TEST_EQUAL;
        case rp::DepthTest::greater:
            return BGFX_STATE_DEPTH_TEST_GREATER;
        case rp::DepthTest::greater_equal:
            return BGFX_STATE_DEPTH_TEST_GEQUAL;
        case rp::DepthTest::always:
            return BGFX_STATE_DEPTH_TEST_ALWAYS;
    }
    return BGFX_STATE_DEPTH_TEST_LEQUAL;
}

// Embedded minimal shaders (GLSL source for bgfx's GL backend).
// In production these would be compiled with shaderc; for bootstrap we
// use bgfx's built-in debug text program or the noop renderer.

BgfxRenderPath::BgfxRenderPath(SDL_Window* window) : window_(window) {
    transient_arena_.resize(TRANSIENT_ARENA_SIZE);
    projection_stack_.push(glm::mat4(1.0f));
    modelview_stack_.push(glm::mat4(1.0f));
    texture_stack_.push(glm::mat4(1.0f));

    SDL_GetWindowSize(window_, (int*)&width_, (int*)&height_);

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(window_, &wmi);

    bgfx::renderFrame();  // single-threaded mode signal

    bgfx::Init init;
#if defined(__linux__)
    if (wmi.subsystem == SDL_SYSWM_X11) {
        init.platformData.ndt = wmi.info.x11.display;
        init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
    } else if (wmi.subsystem == SDL_SYSWM_WAYLAND) {
        init.platformData.ndt = wmi.info.wl.display;
        init.platformData.nwh = wmi.info.wl.surface;
        init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
    } else {
        // Unknown subsystem
        assert(false && "Unsupported windowing system");
    }
#elif defined(_WIN32)
    init.platformData.nwh = wmi.info.win.window;
#elif defined(__APPLE__)
    init.platformData.nwh = wmi.info.cocoa.window;
#endif

    #ifdef BGFX_RENDERER_VULKAN
        init.type = bgfx::RendererType::Vulkan;
    #endif
    #ifdef BGFX_RENDERER_OPENGL
        init.type = bgfx::RendererType::OpenGL;
    #endif
    #ifdef BGFX_RENDERER_METAL
        init.type = bgfx::RendererType::Metal;
    #endif
    #ifdef BGFX_RENDERER_D3D12
        init.Type = bgfx:RendererType::Direct3D12;
    #endif


    init.resolution.width = width_;
    init.resolution.height = height_;
#ifdef ENABLE_VSYNC
init.resolution.reset = BGFX_RESET_VSYNC;
#else
  init.resolution.reset = BGFX_RESET_NONE;
#endif //

    init.limits.maxTransientVbSize = 32 * 1024 * 1024;
    bgfx::init(init);

    bgfx::setViewRect(0, 0, 0, width_, height_);

    // Vertex layout matching world_standard (32 bytes)
    // Must match the GL backend's vertex layout exactly:
    // attr 0: Position  - 3 float  @ offset 0  (12 bytes)
    // attr 1: TexCoord0 - 2 float  @ offset 12 (8 bytes)
    // attr 2: Color0    - 4 ubyte  @ offset 20 (4 bytes)
    // attr 3: Normal    - 3 byte   @ offset 24 (3 bytes + 1 pad)
    // attr 4: TexCoord1 - 2 short  @ offset 28 (4 bytes)
    // Total stride: 32
    vl_world_standard_.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8,
             true)  // 3 bytes + 1 pad read as 4
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Int16)
        .end();

    fb_.width = width_;
    fb_.height = height_;
    fb_.aspect = (float)width_ / (float)height_;
    fb_.is_widescreen = fb_.aspect > 1.5f;
    fb_.is_hi_def = height_ >= 720;

    // PLCE: son what :sob:
    // so BGFX was writing in RGBA and not simple RGB..
    // making the game look transparent
    // It is quite funny but not what we're looking for
    // if you want it back, add "BGFX_STATE_WRITE_A" to the BGFX state
    state_.bgfx_state = BGFX_STATE_WRITE_RGB |
                        BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LEQUAL;

    // Load shaders
    const uint8_t* vs_data = nullptr;
    uint32_t vs_size = 0;
    const uint8_t* fs_data = nullptr;
    uint32_t fs_size = 0;

    switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Vulkan:
            vs_data = vs_4jcraft_spv;
            vs_size = sizeof(vs_4jcraft_spv);
            fs_data = fs_4jcraft_spv;
            fs_size = sizeof(fs_4jcraft_spv);
            break;
        case bgfx::RendererType::OpenGL:
            vs_data = vs_4jcraft_glsl;
            vs_size = sizeof(vs_4jcraft_glsl);
            fs_data = fs_4jcraft_glsl;
            fs_size = sizeof(fs_4jcraft_glsl);
            break;
        case bgfx::RendererType::Metal:
            vs_data = vs_4jcraft_mtl;
            vs_size = sizeof(vs_4jcraft_mtl);
            fs_data = fs_4jcraft_mtl;
            fs_size = sizeof(fs_4jcraft_mtl);
            break;
        default:
            assert(0 && "shaders not yet compiled for this renderer");
            break;
    }

    bgfx::ShaderHandle vsh =
        bgfx::createShader(bgfx::makeRef(vs_data, vs_size));
    bgfx::ShaderHandle fsh =
        bgfx::createShader(bgfx::makeRef(fs_data, fs_size));
    program_ = bgfx::createProgram(vsh, fsh, true);
    std::fprintf(stderr, "[bgfx] renderer=%s program_valid=%d viewport=%ux%u\n",
                 bgfx_renderer_name(bgfx::getRendererType()),
                 bgfx::isValid(program_) ? 1 : 0, width_, height_);

    // Vertex uniforms
    u_baseColor_ = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
    u_chunkOffset_ =
        bgfx::createUniform("u_chunkOffset", bgfx::UniformType::Vec4);
    u_lightParams_ =
        bgfx::createUniform("u_lightParams", bgfx::UniformType::Vec4);
    u_light0Dir_ = bgfx::createUniform("u_light0Dir", bgfx::UniformType::Vec4);
    u_light1Dir_ = bgfx::createUniform("u_light1Dir", bgfx::UniformType::Vec4);
    u_lightDiffuse_ =
        bgfx::createUniform("u_lightDiffuse", bgfx::UniformType::Vec4);
    u_lightAmbient_ =
        bgfx::createUniform("u_lightAmbient", bgfx::UniformType::Vec4);
    u_fogParams_ = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);
    u_lmTransform_ =
        bgfx::createUniform("u_lmTransform", bgfx::UniformType::Vec4);
    u_globalLM_ = bgfx::createUniform("u_globalLM", bgfx::UniformType::Vec4);
    u_texMatrix_ = bgfx::createUniform("u_texMatrix", bgfx::UniformType::Mat4);
    // Fragment uniforms
    u_fragParams_ =
        bgfx::createUniform("u_fragParams", bgfx::UniformType::Vec4);
    u_fogColor_ = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
    s_tex0_ = bgfx::createUniform("s_tex0", bgfx::UniformType::Sampler);
    s_tex1_ = bgfx::createUniform("s_tex1", bgfx::UniformType::Sampler);
    // PLCE: read fs_4jcraft.sc pls
    const uint8_t whitePx[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    white_tex_ = bgfx::createTexture2D(
        1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0,
        bgfx::copy(whitePx, sizeof(whitePx)));
}

BgfxRenderPath::~BgfxRenderPath() {
    {
        std::lock_guard<std::mutex> lk(cbuf_mtx_);
        shutdown_.store(true, std::memory_order_release);
        for (auto h : cbuf_destroy_queue_) {
            if (bgfx::isValid(h)) bgfx::destroy(h);
        }
        cbuf_destroy_queue_.clear();
        for (auto& [id, cb] : cbuf_pool_) {
            if (bgfx::isValid(cb.vbh)) bgfx::destroy(cb.vbh);
        }
        cbuf_pool_.clear();
    }

    for (auto& slot : textures_) {
        if (slot.occupied && bgfx::isValid(slot.bgfx_handle))
            bgfx::destroy(slot.bgfx_handle);
    }
    textures_.clear();

    for (auto& [id, slot] : gl_tex_to_bgfx_) {
        if (bgfx::isValid(slot.handle)) bgfx::destroy(slot.handle);
    }
    gl_tex_to_bgfx_.clear();

    for (auto* u :
         {&u_baseColor_, &u_chunkOffset_, &u_lightParams_, &u_light0Dir_,
          &u_light1Dir_, &u_lightDiffuse_, &u_lightAmbient_, &u_fogParams_,
           &u_lmTransform_, &u_globalLM_, &u_texMatrix_, &u_fragParams_,
           &u_fogColor_, &s_tex0_, &s_tex1_}) {
        if (bgfx::isValid(*u)) bgfx::destroy(*u);
    }

    if (bgfx::isValid(white_tex_)) bgfx::destroy(white_tex_);

    if (bgfx::isValid(program_)) bgfx::destroy(program_);
    bgfx::shutdown();
}

// MARK: Matrix stack

std::stack<glm::mat4>& BgfxRenderPath::current_stack() {
    switch (matrix_mode_) {
        case rp::MatrixStack::projection:
            return projection_stack_;
        case rp::MatrixStack::texture:
            return texture_stack_;
        default:
            return modelview_stack_;
    }
}

void BgfxRenderPath::MatrixMode(rp::MatrixStack stack) { matrix_mode_ = stack; }
void BgfxRenderPath::MatrixSetIdentity() {
    current_stack().top() = glm::mat4(1.0f);
}
void BgfxRenderPath::MatrixPush() {
    current_stack().push(current_stack().top());
}
void BgfxRenderPath::MatrixPop() {
    if (current_stack().size() > 1) current_stack().pop();
}

void BgfxRenderPath::MatrixTranslate(float x, float y, float z) {
    current_stack().top() =
        glm::translate(current_stack().top(), glm::vec3(x, y, z));
}

void BgfxRenderPath::MatrixRotate(float angle, float x, float y, float z) {
    current_stack().top() =
        glm::rotate(current_stack().top(), angle, glm::vec3(x, y, z));
}

void BgfxRenderPath::MatrixScale(float x, float y, float z) {
    current_stack().top() =
        glm::scale(current_stack().top(), glm::vec3(x, y, z));
}

void BgfxRenderPath::MatrixPerspective(float fovy, float aspect, float zNear,
                                       float zFar) {
    current_stack().top() =
        glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
}

void BgfxRenderPath::MatrixOrthogonal(float left, float right, float bottom,
                                      float top, float zNear, float zFar) {
    current_stack().top() = glm::ortho(left, right, bottom, top, zNear, zFar);
}

void BgfxRenderPath::MatrixMult(float* m) {
    current_stack().top() *= glm::make_mat4(m);
}

const float* BgfxRenderPath::MatrixGet(rp::MatrixStack stack) {
    switch (stack) {
        case rp::MatrixStack::projection:
            return glm::value_ptr(projection_stack_.top());
        case rp::MatrixStack::texture:
            return glm::value_ptr(texture_stack_.top());
        default:
            return glm::value_ptr(modelview_stack_.top());
    }
}

// MARK: State accumulator

void BgfxRenderPath::StateSetColour(float r, float g, float b, float a) {
    state_.tint_color[0] = r;
    state_.tint_color[1] = g;
    state_.tint_color[2] = b;
    state_.tint_color[3] = a;
}

void BgfxRenderPath::StateSetDepthMask(bool e) {
    state_.depth_write = e;
    state_.bgfx_state = (state_.bgfx_state & ~BGFX_STATE_WRITE_Z) |
                        (e ? BGFX_STATE_WRITE_Z : 0);
}

void BgfxRenderPath::StateSetBlendEnable(bool e) {
    state_.blend_enabled = e;
    state_.bgfx_state &= ~BGFX_STATE_BLEND_MASK;
    if (e) {
        state_.bgfx_state |=
            BGFX_STATE_BLEND_FUNC(bgfx_blend_factor(state_.blend_src),
                                  bgfx_blend_factor(state_.blend_dst));
    }
}

void BgfxRenderPath::StateSetBlendFunc(rp::BlendFactor s, rp::BlendFactor d) {
    state_.blend_src = s;
    state_.blend_dst = d;
    state_.bgfx_state &= ~BGFX_STATE_BLEND_MASK;
    if (state_.blend_enabled) {
        state_.bgfx_state |=
            BGFX_STATE_BLEND_FUNC(bgfx_blend_factor(s), bgfx_blend_factor(d));
    }
}

void BgfxRenderPath::StateSetBlendFactor(unsigned int argb) {
    uint32_t a = (argb >> 24) & 0xFF;
    uint32_t r = (argb >> 16) & 0xFF;
    uint32_t g = (argb >> 8) & 0xFF;
    uint32_t b = argb & 0xFF;
    state_.blend_factor_rgba = (r << 24) | (g << 16) | (b << 8) | a;
}

void BgfxRenderPath::StateSetAlphaFunc(rp::AlphaTest, float r) {
    state_.alpha_ref = r;
}

void BgfxRenderPath::StateSetDepthFunc(rp::DepthTest t) {
    state_.depth_func_bits = bgfx_depth_func(t);
    state_.bgfx_state =
        (state_.bgfx_state & ~BGFX_STATE_DEPTH_TEST_MASK) |
        (state_.depth_test_enabled ? state_.depth_func_bits : 0);
}

void BgfxRenderPath::StateSetFaceCull(bool e) {
    state_.cull_enabled = e;
    state_.bgfx_state &= ~BGFX_STATE_CULL_MASK;
    if (e) state_.bgfx_state |= BGFX_STATE_CULL_CW;
}

void BgfxRenderPath::StateSetLineWidth(float) {}

void BgfxRenderPath::StateSetWriteEnable(bool r, bool g, bool b, bool a) {
    state_.bgfx_state &= ~(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    if (r || g || b) state_.bgfx_state |= BGFX_STATE_WRITE_RGB;
    if (a) state_.bgfx_state |= BGFX_STATE_WRITE_A;
}

void BgfxRenderPath::StateSetDepthTestEnable(bool e) {
    state_.depth_test_enabled = e;
    state_.bgfx_state = (state_.bgfx_state & ~BGFX_STATE_DEPTH_TEST_MASK) |
                        (e ? state_.depth_func_bits : 0);
}

void BgfxRenderPath::StateSetAlphaTestEnable(bool e) {
    state_.alpha_test_enabled = e;
    state_.alpha_ref = e ? 0.1f : 0.0f;
}
void BgfxRenderPath::StateSetDepthSlopeAndBias(float slope, float bias) {
    state_.depth_slope_bias = slope;
    state_.depth_z_bias = bias;
}

// MARK: Fog
void BgfxRenderPath::StateSetFogEnable(bool e) { state_.fog_enabled = e; }
void BgfxRenderPath::StateSetFogMode(rp::FogMode m) {
    state_.fog_mode = static_cast<float>(m);
}
void BgfxRenderPath::StateSetFogNearDistance(float d) { state_.fog_start = d; }
void BgfxRenderPath::StateSetFogFarDistance(float d) { state_.fog_end = d; }
void BgfxRenderPath::StateSetFogDensity(float d) { state_.fog_density = d; }
void BgfxRenderPath::StateSetFogColour(float r, float g, float b) {
    state_.fog_color[0] = r;
    state_.fog_color[1] = g;
    state_.fog_color[2] = b;
    state_.fog_color[3] = 1;
}

// MARK: Lighting
void BgfxRenderPath::StateSetLightingEnable(bool e) {
    state_.lighting_enabled = e;
}
void BgfxRenderPath::StateSetLightColour(int, float r, float g, float b) {
    state_.light_diffuse[0] = r;
    state_.light_diffuse[1] = g;
    state_.light_diffuse[2] = b;
}
void BgfxRenderPath::StateSetLightAmbientColour(float r, float g, float b) {
    state_.light_ambient[0] = r;
    state_.light_ambient[1] = g;
    state_.light_ambient[2] = b;
}
void BgfxRenderPath::StateSetLightDirection(int l, float x, float y, float z) {
    // PLCE: bgfx likes the raw model-space direction
    glm::vec3 d = glm::normalize(glm::vec3(x, y, z));
    float* dst = (l == 0) ? state_.light0_dir : state_.light1_dir;
    dst[0] = d.x;
    dst[1] = d.y;
    dst[2] = d.z;
    dst[3] = 0;
}
glm::vec4 BgfxRenderPath::lightDirUniform(int l) const {
    const float* src = (l == 0) ? state_.light0_dir : state_.light1_dir;
    glm::vec3 d = glm::normalize(
        glm::mat3(modelview_stack_.top()) * glm::vec3(src[0], src[1], src[2]));
    return glm::vec4(d, 0.0f);
}
void BgfxRenderPath::StateSetLightEnable(int, bool) {}

void BgfxRenderPath::StateSetViewport(int) {
    bgfx::setViewRect(current_view_id_, 0, 0, width_, height_);
}
void BgfxRenderPath::StateSetEnableViewportClipPlanes(bool) {}
void BgfxRenderPath::StateSetStencil(int, uint8_t, uint8_t, uint8_t) {}
void BgfxRenderPath::StateSetForceLOD(int) {}
void BgfxRenderPath::StateSetTextureEnable(bool e) {
    state_.texture_enabled = e;
}
void BgfxRenderPath::StateSetActiveTexture(int) {}

void BgfxRenderPath::SetChunkOffset(float x, float y, float z) {
    state_.chunk_offset[0] = x;
    state_.chunk_offset[1] = y;
    state_.chunk_offset[2] = z;
}

void BgfxRenderPath::StateSetVertexTextureUV(float u, float v) {
    state_.global_lm[0] = u;
    state_.global_lm[1] = v;
}

// MARK: DrawVertices

static int s_frameCount = 0;
void BgfxRenderPath::DrawVertices(int primType, int count, void* data,
                                  int vType, int) {
    if (count <= 0 || !data) return;

    uint32_t stride = vl_world_standard_.getStride();  // 32

    // Expand compact 16-byte vertices to 32-byte world_standard
    static thread_local std::vector<uint8_t> expand_buf;
    if (vType == 1) {
        expand_buf.resize((size_t)count * 32);
        const int16_t* csrc = (const int16_t*)data;
        uint8_t* dst = expand_buf.data();
        for (int i = 0; i < count; i++) {
            auto* dstF = (float*)dst;
            dstF[0] = csrc[0] / 1024.0f;
            dstF[1] = csrc[1] / 1024.0f;
            dstF[2] = csrc[2] / 1024.0f;
            dstF[3] = csrc[4] / 8192.0f;
            dstF[4] = csrc[5] / 8192.0f;
            uint16_t packed = (uint16_t)((int)csrc[3] + 32768);
            dst[20] = 255;
            dst[21] = (uint8_t)((packed & 0x1F) * 255 / 31);
            dst[22] = (uint8_t)(((packed >> 5) & 0x3F) * 255 / 63);
            dst[23] = (uint8_t)(((packed >> 11) & 0x1F) * 255 / 31);
            dst[24] = 0;
            dst[25] = 127;
            dst[26] = 0;
            dst[27] = 0;
            auto* dstS = (int16_t*)(dst + 28);
            dstS[0] = csrc[6];
            dstS[1] = csrc[7];
            csrc += 8;
            dst += 32;
        }
        data = expand_buf.data();
    }

    // Convert unsupported primitive types to triangle list into a CPU buffer
    static thread_local std::vector<uint8_t> conv_buf;
    bool isFan = (primType == 0x0006);   // GL_TRIANGLE_FAN
    bool isQuad = (primType == 0x0007);  // GL_QUADS
    const uint8_t* src = (const uint8_t*)data;
    int submitCount = count;

    if (isQuad && count >= 4) {
        int numQuads = count / 4;
        submitCount = numQuads * 6;
        conv_buf.resize(submitCount * stride);
        uint8_t* dst = conv_buf.data();
        for (int q = 0; q < numQuads; q++) {
            const uint8_t* v0 = src + (q * 4 + 0) * stride;
            const uint8_t* v1 = src + (q * 4 + 1) * stride;
            const uint8_t* v2 = src + (q * 4 + 2) * stride;
            const uint8_t* v3 = src + (q * 4 + 3) * stride;
            // first triangle (v0, v1, v2) ccw
            memcpy(dst, v0, stride);
            dst += stride;
            memcpy(dst, v1, stride);
            dst += stride;
            memcpy(dst, v2, stride);
            dst += stride;
            // second triangle (v2, v3, v0) ccw
            memcpy(dst, v2, stride);
            dst += stride;
            memcpy(dst, v3, stride);
            dst += stride;
            memcpy(dst, v0, stride);
            dst += stride;
        }
        src = conv_buf.data();
    } else if (isFan && count >= 3) {
        submitCount = (count - 2) * 3;
        conv_buf.resize(submitCount * stride);
        uint8_t* dst = conv_buf.data();
        for (int i = 1; i < count - 1; i++) {
            memcpy(dst, src, stride);
            dst += stride;
            memcpy(dst, src + i * stride, stride);
            dst += stride;
            memcpy(dst, src + (i + 1) * stride, stride);
            dst += stride;
        }
        src = conv_buf.data();
    }

    size_t bytes = (size_t)submitCount * stride;

    // CBuffer recording mode - append to thread-local buffer, skip bgfx calls
    if (cbuf_rec_id_ >= 0) {
        int first = (int)(cbuf_rec_verts_.size() / stride);
        cbuf_rec_verts_.insert(cbuf_rec_verts_.end(), src, src + bytes);
        cbuf_rec_draws_.push_back({primType, first, submitCount});
        return;
    }

    // Immediate mode - submit through bgfx
    if (bgfx::getAvailTransientVertexBuffer(submitCount, vl_world_standard_) <
        (uint32_t)submitCount)
        return;
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, submitCount, vl_world_standard_);
    memcpy(tvb.data, src, bytes);

    // Set primitive type in bgfx state
    uint64_t primState = 0;
    if (primType == 0x0005)
        primState = BGFX_STATE_PT_TRISTRIP;  // GL_TRIANGLE_STRIP
    else if (primType == 0x0001)
        primState = BGFX_STATE_PT_LINES;  // GL_LINES
    else if (primType == 0x0003)
        primState = BGFX_STATE_PT_LINESTRIP;  // GL_LINE_STRIP

    glm::mat4 proj = projection_stack_.top();

    // PLCE: bgfx exposes no polygon offset state, so the constant bias is
    // baked into clip-space Z via the view projection here. Slope bias would
    // need a shader-side implementation and is currently ignored.
    if (state_.depth_z_bias != 0.0f) {
        proj[3][2] += state_.depth_z_bias * Z_BIAS_EPSILON;
    }
    const uint16_t view = resolveView(proj);

    bgfx::setTransform(glm::value_ptr(modelview_stack_.top()));
    bgfx::setState(state_.bgfx_state | primState, state_.blend_factor_rgba);
    bgfx::setVertexBuffer(0, &tvb);

    // Vertex uniforms
    bgfx::setUniform(u_baseColor_, state_.tint_color);
    float chunkOff[4] = {state_.chunk_offset[0], state_.chunk_offset[1],
                         state_.chunk_offset[2], 0};
    bgfx::setUniform(u_chunkOffset_, chunkOff);
    float lp[4] = {state_.lighting_enabled ? 1.0f : 0.0f, 1.0f, 0, 0};
    bgfx::setUniform(u_lightParams_, lp);
    const glm::vec4 l0 = lightDirUniform(0);
    const glm::vec4 l1 = lightDirUniform(1);
    bgfx::setUniform(u_light0Dir_, glm::value_ptr(l0));
    bgfx::setUniform(u_light1Dir_, glm::value_ptr(l1));
    bgfx::setUniform(u_lightDiffuse_, state_.light_diffuse);
    bgfx::setUniform(u_lightAmbient_, state_.light_ambient);
    float fp[4] = {state_.fog_enabled ? state_.fog_mode : 0.0f,
                   state_.fog_start, state_.fog_end, state_.fog_density};
    bgfx::setUniform(u_fogParams_, fp);
    bgfx::setUniform(u_lmTransform_, state_.lightmap_transform);
    bgfx::setUniform(u_globalLM_, state_.global_lm);
    bgfx::setUniform(u_texMatrix_, glm::value_ptr(texture_stack_.top()));
    bool hasTexture = false;
    bgfx::TextureHandle tex0 = white_tex_;
    uint32_t tex0Flags = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
    if (state_.texture_enabled && state_.bound_texture >= 0) {
        auto it = gl_tex_to_bgfx_.find(state_.bound_texture);
        if (it != gl_tex_to_bgfx_.end()) {
            tex0 = it->second.handle;
            tex0Flags = it->second.sampler_flags;
            hasTexture = true;
        }
    }
    bgfx::setTexture(0, s_tex0_, tex0, tex0Flags);
    const bgfx::TextureHandle lightmap =
        (state_.use_lightmap && bgfx::isValid(state_.bound_lightmap))
            ? state_.bound_lightmap
            : white_tex_;
    bgfx::setTexture(1, s_tex1_, lightmap,
                     BGFX_SAMPLER_MIN_ANISOTROPIC |
                         BGFX_SAMPLER_MAG_ANISOTROPIC |
                         BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    float fragP[4] = {hasTexture ? 1.0f : 0.0f,
                      state_.use_lightmap ? 1.0f : 0.0f, state_.alpha_ref,
                      state_.fog_enabled ? 1.0f : 0.0f};
    bgfx::setUniform(u_fragParams_, fragP);
    bgfx::setUniform(u_fogColor_, state_.fog_color);

    if (bgfx::isValid(program_))
        bgfx::submit(view, program_);
    else
        bgfx::discard();
}

// MARK: Resource methods

MeshHandle BgfxRenderPath::create_mesh(const MeshDesc&) { return kInvalidMesh; }
void BgfxRenderPath::update_mesh(MeshHandle, const MeshDesc&) {}
void BgfxRenderPath::destroy_mesh(MeshHandle) {}

TextureHandle BgfxRenderPath::create_texture(const TextureDesc& desc) {
    auto th = bgfx::createTexture2D(desc.width, desc.height,
                                    desc.mip_levels > 1, 1,
                                    bgfx::TextureFormat::RGBA8, 0);  // no mem

    if (!desc.initial_data.empty()) {
        auto mem =
            bgfx::copy(desc.initial_data.data(), desc.initial_data.size());
        bgfx::updateTexture2D(th, 0, 0, 0, 0, desc.width, desc.height, mem);
    }

    for (uint32_t i = 0; i < textures_.size(); ++i) {
        if (!textures_[i].occupied) {
            textures_[i] = {th, ++textures_[i].generation, desc.width,
                            desc.height, true};
            return {i, textures_[i].generation};
        }
    }
    uint32_t idx = (uint32_t)textures_.size();
    textures_.push_back({th, 1, desc.width, desc.height, true});
    return {idx, 1};
}

void BgfxRenderPath::update_texture(TextureHandle h, const TextureRegion& r) {
    if (h.index >= textures_.size() ||
        textures_[h.index].generation != h.generation) {
        return;
    }
    const auto& slot = textures_[h.index];
    if (!bgfx::isValid(slot.bgfx_handle) || r.data.empty()) return;
    auto mem = bgfx::copy(r.data.data(), (uint32_t)r.data.size());
    bgfx::updateTexture2D(slot.bgfx_handle, 0, r.mip_level, (uint16_t)r.x,
                          (uint16_t)r.y, (uint16_t)r.width, (uint16_t)r.height,
                          mem);
}

void BgfxRenderPath::destroy_texture(TextureHandle h) {
    if (h.index < textures_.size() &&
        textures_[h.index].generation == h.generation) {
        bgfx::destroy(textures_[h.index].bgfx_handle);
        textures_[h.index].occupied = false;
    }
}

MaterialHandle BgfxRenderPath::create_material(const MaterialDesc& desc) {
    for (uint32_t i = 0; i < materials_.size(); ++i) {
        if (!materials_[i].occupied) {
            materials_[i] = {desc, ++materials_[i].generation, true};
            return {i, materials_[i].generation};
        }
    }
    uint32_t idx = (uint32_t)materials_.size();
    materials_.push_back({desc, 1, true});
    return {idx, 1};
}

void BgfxRenderPath::update_material(MaterialHandle h,
                                     const MaterialDesc& desc) {
    if (h.index < materials_.size() &&
        materials_[h.index].generation == h.generation)
        materials_[h.index].desc = desc;
}

void BgfxRenderPath::destroy_material(MaterialHandle h) {
    if (h.index < materials_.size() &&
        materials_[h.index].generation == h.generation)
        materials_[h.index].occupied = false;
}

std::pair<TransientVertexBuffer, std::span<std::byte>>
BgfxRenderPath::alloc_transient_vertices(uint32_t count, VertexLayout,
                                         PrimitiveType prim) {
    uint32_t stride = vl_world_standard_.getStride();
    uint32_t bytes = count * stride;
    if (transient_offset_ + bytes > transient_arena_.size()) return {{}, {}};
    TransientVertexBuffer tvb;
    tvb.frame_index = current_frame_;
    tvb.offset = transient_offset_;
    tvb.vertex_count = count;
    tvb.primitive = prim;
    auto span = std::span<std::byte>(
        transient_arena_.data() + transient_offset_, bytes);
    transient_offset_ += bytes;
    return {tvb, span};
}

// MARK: Frame submission

void BgfxRenderPath::render_frame(const FrameDesc& frame) {
    transient_offset_ = 0;
    current_frame_++;
    fb_ = frame.framebuffer;

    // Destroy deferred VBs on main thread
    {
        std::lock_guard<std::mutex> lk(cbuf_mtx_);
        for (auto h : cbuf_destroy_queue_) {
            if (bgfx::isValid(h)) bgfx::destroy(h);
        }
        cbuf_destroy_queue_.clear();
    }

    s_frameCount++;
    bgfx::frame();
}

void BgfxRenderPath::resize(uint32_t w, uint32_t h) {
    width_ = w;
    height_ = h;
#ifdef ENABLE_VSYNC
    bgfx::reset(w, h, BGFX_RESET_VSYNC);
#else
    bgfx::reset(w, h, BGFX_RESET_NONE);
#endif
}

const FrameFramebuffer& BgfxRenderPath::framebuffer() const { return fb_; }
void BgfxRenderPath::read_framebuffer(const TextureReadback&) {}
ResourceFootprint BgfxRenderPath::query_resource_footprint() const {
    return {};
}
void BgfxRenderPath::seal_static_resource_tier() {}
void BgfxRenderPath::begin_atomic_resource_batch() {}
void BgfxRenderPath::end_atomic_resource_batch() {}
void BgfxRenderPath::push_debug_event(const char*) {}
void BgfxRenderPath::pop_debug_event() {}
void BgfxRenderPath::tick() {}

// MARK: Command buffer stubs

int BgfxRenderPath::CBuffCreate(int count) {
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) return 0;
    int base = cbuf_next_id_;
    cbuf_next_id_ += count;
    return base;
}

void BgfxRenderPath::CBuffDelete(int first, int count) {
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) return;
    for (int i = first; i < first + count; i++) {
        auto it = cbuf_pool_.find(i);
        if (it != cbuf_pool_.end()) {
            if (bgfx::isValid(it->second.vbh))
                cbuf_destroy_queue_.push_back(it->second.vbh);
            cbuf_pool_.erase(it);
        }
    }
}

void BgfxRenderPath::CBuffDeleteAll() {
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) return;
    for (auto& [id, cb] : cbuf_pool_) {
        if (bgfx::isValid(cb.vbh)) cbuf_destroy_queue_.push_back(cb.vbh);
    }
    cbuf_pool_.clear();
    cbuf_next_id_ = 1;
}

void BgfxRenderPath::CBuffStart(int index, bool) {
    cbuf_rec_id_ = index;
    cbuf_rec_verts_.clear();
    cbuf_rec_draws_.clear();
}

void BgfxRenderPath::CBuffClear(int index) {
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) return;
    auto it = cbuf_pool_.find(index);
    if (it != cbuf_pool_.end()) {
        if (bgfx::isValid(it->second.vbh))
            cbuf_destroy_queue_.push_back(it->second.vbh);
        cbuf_pool_.erase(it);
    }
}

int BgfxRenderPath::CBuffSize(int index) {
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) return 0;
    auto it = cbuf_pool_.find(index);
    if (it == cbuf_pool_.end()) return 0;
    return it->second.valid ? 1 : 0;
}

void BgfxRenderPath::CBuffEnd() {
    if (cbuf_rec_id_ < 0) return;
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) {
        cbuf_rec_id_ = -1;
        return;
    }
    auto& cb = cbuf_pool_[cbuf_rec_id_];
    if (cbuf_rec_verts_.empty()) {
        if (bgfx::isValid(cb.vbh)) {
            cbuf_destroy_queue_.push_back(cb.vbh);
            cb.vbh = BGFX_INVALID_HANDLE;
        }
        cbuf_pool_.erase(cbuf_rec_id_);
        cbuf_rec_id_ = -1;
        return;
    }
    cb.raw_verts = std::move(cbuf_rec_verts_);
    cb.draws = std::move(cbuf_rec_draws_);
    cb.valid = true;
    cb.vb_ready = false;
    cbuf_rec_id_ = -1;
}

bool BgfxRenderPath::CBuffCall(int index, bool) {
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    if (shutdown_.load(std::memory_order_acquire)) return false;
    auto it = cbuf_pool_.find(index);
    if (it == cbuf_pool_.end() || !it->second.valid) return false;
    auto& cb = it->second;
    if (cb.draws.empty()) return false;

    // Lazily upload geometry on main thread. Reuse the same dynamic VB
    // across remeshes; BGFX_BUFFER_ALLOW_RESIZE lets bgfx grow on update.
    if (!cb.vb_ready) {
        if (cb.raw_verts.empty()) return false;
        auto* mem =
            bgfx::copy(cb.raw_verts.data(), (uint32_t)cb.raw_verts.size());
        if (!bgfx::isValid(cb.vbh)) {
            cb.vbh = bgfx::createDynamicVertexBuffer(mem, vl_world_standard_,
                                                     BGFX_BUFFER_ALLOW_RESIZE);
        } else {
            bgfx::update(cb.vbh, 0, mem);
        }
        cb.raw_verts.clear();
        cb.raw_verts.shrink_to_fit();
        cb.vb_ready = true;
    }

    if (!bgfx::isValid(cb.vbh)) return false;

    glm::mat4 proj = projection_stack_.top();
    // PLCE: this is a hack to avoid implementing this in a shader. these values
    // are used to prevent Z-fighting and this does effectively the same thing
    // by breaking the tie in the depth buffer.
    if (state_.depth_z_bias != 0.0f) {
        proj[3][2] += state_.depth_z_bias * Z_BIAS_EPSILON;
    }
    const uint16_t view = resolveView(proj);
    const glm::mat4 modelview = modelview_stack_.top();
    const glm::mat4 texMatrix = texture_stack_.top();

    float chunkOff[4] = {state_.chunk_offset[0], state_.chunk_offset[1],
                         state_.chunk_offset[2], 0};
    float lp[4] = {state_.lighting_enabled ? 1.0f : 0.0f, 1.0f, 0, 0};
    float fp[4] = {state_.fog_enabled ? state_.fog_mode : 0.0f,
                   state_.fog_start, state_.fog_end, state_.fog_density};

    bool hasTexture = false;
    bgfx::TextureHandle texHandle = white_tex_;
    uint32_t texSamplerFlags = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
    if (state_.texture_enabled && state_.bound_texture >= 0) {
        auto tex_it = gl_tex_to_bgfx_.find(state_.bound_texture);
        if (tex_it != gl_tex_to_bgfx_.end()) {
            texHandle = tex_it->second.handle;
            texSamplerFlags = tex_it->second.sampler_flags;
            hasTexture = true;
        }
    }
    float fragP[4] = {hasTexture ? 1.0f : 0.0f,
                      state_.use_lightmap ? 1.0f : 0.0f, state_.alpha_ref,
                      state_.fog_enabled ? 1.0f : 0.0f};

    bgfx::setUniform(u_baseColor_, state_.tint_color);
    bgfx::setUniform(u_chunkOffset_, chunkOff);
    bgfx::setUniform(u_lightParams_, lp);
    const glm::vec4 l0 = lightDirUniform(0);
    const glm::vec4 l1 = lightDirUniform(1);
    bgfx::setUniform(u_light0Dir_, glm::value_ptr(l0));
    bgfx::setUniform(u_light1Dir_, glm::value_ptr(l1));
    bgfx::setUniform(u_lightDiffuse_, state_.light_diffuse);
    bgfx::setUniform(u_lightAmbient_, state_.light_ambient);
    bgfx::setUniform(u_fogParams_, fp);
    bgfx::setUniform(u_lmTransform_, state_.lightmap_transform);
    bgfx::setUniform(u_globalLM_, state_.global_lm);
    bgfx::setUniform(u_texMatrix_, glm::value_ptr(texMatrix));
    bgfx::setUniform(u_fragParams_, fragP);
    bgfx::setUniform(u_fogColor_, state_.fog_color);

    for (const auto& dc : cb.draws) {
        bgfx::setTexture(0, s_tex0_, texHandle, texSamplerFlags);
        const bgfx::TextureHandle lightmap =
            (state_.use_lightmap && bgfx::isValid(state_.bound_lightmap))
                ? state_.bound_lightmap
                : white_tex_;
        bgfx::setTexture(1, s_tex1_, lightmap,
                         BGFX_SAMPLER_MIN_ANISOTROPIC |
                             BGFX_SAMPLER_MAG_ANISOTROPIC |
                             BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

        uint64_t primBits = 0;
        if (dc.prim == 0x0005)
            primBits = BGFX_STATE_PT_TRISTRIP;
        else if (dc.prim == 0x0001)
            primBits = BGFX_STATE_PT_LINES;
        else if (dc.prim == 0x0003)
            primBits = BGFX_STATE_PT_LINESTRIP;

        bgfx::setTransform(glm::value_ptr(modelview));
        bgfx::setState(state_.bgfx_state | primBits, state_.blend_factor_rgba);
        bgfx::setVertexBuffer(0, cb.vbh, dc.first, dc.count);

        if (bgfx::isValid(program_))
            bgfx::submit(view, program_);
        else
            bgfx::discard();
    }

    return true;
}

void BgfxRenderPath::CBuffDeferredModeStart() {}
void BgfxRenderPath::CBuffDeferredModeEnd() {}

// MARK: Texture legacy stubs

int BgfxRenderPath::TextureCreate() {
    int id = next_gl_tex_id_++;
    return id;
}

void BgfxRenderPath::TextureFree(int idx) {
    auto it = gl_tex_to_bgfx_.find(idx);
    if (it != gl_tex_to_bgfx_.end()) {
        bgfx::destroy(it->second.handle);
        gl_tex_to_bgfx_.erase(it);
    }
}

void BgfxRenderPath::TextureBind(int idx) {
    state_.bound_texture = idx;
    if (idx >= 0) {
        auto it = gl_tex_to_bgfx_.find(idx);
        if (it != gl_tex_to_bgfx_.end())
            state_.bound_texture_sampler_flags = it->second.sampler_flags;
    }
}

void BgfxRenderPath::TextureBindVertex(int idx, bool scaleLight) {
    if (idx < 0) {
        state_.use_lightmap = false;
        return;
    }

    auto it = gl_tex_to_bgfx_.find(idx);
    if (it != gl_tex_to_bgfx_.end()) {
        state_.bound_lightmap = it->second.handle;
        state_.use_lightmap = true;
    } else {
        state_.bound_lightmap = {bgfx::kInvalidHandle};
        state_.use_lightmap = false;
    }

    state_.lightmap_transform[0] = 1.0f;
    state_.lightmap_transform[1] = 1.0f;
    state_.lightmap_transform[2] = scaleLight ? 8.f / 256.f : 0.0f;
    state_.lightmap_transform[3] = scaleLight ? 8.f / 256.f : 0.0f;
}

void BgfxRenderPath::TextureSetTextureLevels(int levels) {
    if (state_.bound_texture < 0) return;
    gl_tex_to_bgfx_[state_.bound_texture].mip_levels =
        (uint16_t)(levels > 1 ? levels : 1);
}

void BgfxRenderPath::TextureSetParam(int param, int value) {
    // translate OpenGL sampler params to bgfx sampler flags

    constexpr int GL_TEXTURE_MIN_FILTER = 0x2801;
    constexpr int GL_TEXTURE_MAG_FILTER = 0x2800;
    constexpr int GL_TEXTURE_WRAP_S = 0x2802;
    constexpr int GL_TEXTURE_WRAP_T = 0x2803;
    constexpr int GL_NEAREST = 0x2600;
    constexpr int GL_LINEAR = 0x2601;
    constexpr int GL_NEAREST_MIPMAP_NEAREST = 0x2700;
    constexpr int GL_LINEAR_MIPMAP_NEAREST = 0x2701;
    constexpr int GL_NEAREST_MIPMAP_LINEAR = 0x2702;
    constexpr int GL_LINEAR_MIPMAP_LINEAR = 0x2703;
    constexpr int GL_REPEAT = 0x2901;
    constexpr int GL_CLAMP_TO_EDGE = 0x812F;
    constexpr int GL_MIRRORED_REPEAT = 0x8370;

    if (state_.bound_texture < 0) return;

    auto& pending = gl_tex_to_bgfx_[state_.bound_texture];

    uint32_t& f = pending.sampler_flags;

    switch (param) {
        case GL_TEXTURE_MIN_FILTER:
            f &= ~(BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MIP_POINT);
            switch (value) {
                case GL_NEAREST:
                    f |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MIP_POINT;
                    break;
                case GL_LINEAR:
                    f |= BGFX_SAMPLER_MIP_POINT;
                    break;
                case GL_NEAREST_MIPMAP_NEAREST:
                    f |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MIP_POINT;
                    break;
                case GL_LINEAR_MIPMAP_NEAREST:
                    f |= BGFX_SAMPLER_MIP_POINT;
                    break;
                case GL_NEAREST_MIPMAP_LINEAR:
                    f |= BGFX_SAMPLER_MIN_POINT;
                    break;
                case GL_LINEAR_MIPMAP_LINEAR:
                    // both point bits already cleared above
                    break;
                default:
                    break;
            }
            break;

        case GL_TEXTURE_MAG_FILTER:
            f &= ~BGFX_SAMPLER_MAG_POINT;
            if (value == GL_NEAREST) f |= BGFX_SAMPLER_MAG_POINT;
            // bgfx default is bilinear mag
            break;

        case GL_TEXTURE_WRAP_S:
            f &= ~(BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_U_MIRROR);
            switch (value) {
                case GL_CLAMP_TO_EDGE:
                    f |= BGFX_SAMPLER_U_CLAMP;
                    break;
                case GL_MIRRORED_REPEAT:
                    f |= BGFX_SAMPLER_U_MIRROR;
                    break;
                case GL_REPEAT:
                default:
                    // bgfx default is wrap
                    break;
            }
            break;

        case GL_TEXTURE_WRAP_T:
            f &= ~(BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_V_MIRROR);
            switch (value) {
                case GL_CLAMP_TO_EDGE:
                    f |= BGFX_SAMPLER_V_CLAMP;
                    break;
                case GL_MIRRORED_REPEAT:
                    f |= BGFX_SAMPLER_V_MIRROR;
                    break;
                case GL_REPEAT:
                default:
                    break;
            }
            break;

        default:
            break;
    }

    auto it = gl_tex_to_bgfx_.find(state_.bound_texture);
    if (it != gl_tex_to_bgfx_.end()) {
        it->second.sampler_flags = f;
        if (state_.bound_texture == it->first)
            state_.bound_texture_sampler_flags = f;
    }
}

static uint16_t mipChainCount(uint32_t w, uint32_t h) {
    const uint32_t longest = w > h ? w : h;
    uint16_t levels = 1;
    while ((1u << levels) < longest) levels++;
    return (uint16_t)(levels + 1);
}

// PLCE: the game only uploads the mip levels it needs, the remaining levels
// are filled by nearest-sampling the last uploaded level which is COMPLETELY
// WRONG. the others would bleed neighbor tiles of the atlas into each other!!!

// PLCE: turns out GL backend ignores BGFX_SAMPLER_LOD_CLAMP so OpenGL samples
// the wrong mipmaps ;~;
static void fillMipTail(uint16_t lastLevel, const uint8_t* data, int w, int h,
                        bgfx::TextureHandle handle, uint16_t chain) {
    std::vector<uint8_t> buf((size_t)(w / 2 + 1) * (h / 2 + 1) * 4);
    for (uint16_t lvl = (uint16_t)(lastLevel + 1); lvl < chain; lvl++) {
        const uint16_t shift = (uint16_t)(lvl - lastLevel);
        const int dstW = w > (1 << shift) ? (w >> shift) : 1;
        const int dstH = h > (1 << shift) ? (h >> shift) : 1;
        for (int y = 0; y < dstH; y++) {
            const int sy = (int)((size_t)y * h / dstH);
            for (int x = 0; x < dstW; x++) {
                const int sx = (int)((size_t)x * w / dstW);
                const uint8_t* p = data + ((size_t)sy * w + sx) * 4;
                uint8_t* d = buf.data() + ((size_t)y * dstW + x) * 4;
                d[0] = p[0];
                d[1] = p[1];
                d[2] = p[2];
                d[3] = p[3];
            }
        }
        const bgfx::Memory* mem =
            bgfx::copy(buf.data(), (uint32_t)((size_t)dstW * dstH * 4));
        bgfx::updateTexture2D(handle, 0, lvl, 0, 0, (uint16_t)dstW,
                              (uint16_t)dstH, mem);
    }
}

void BgfxRenderPath::TextureData(int width, int height, void* data, int level,
                                 int) {
    if (state_.bound_texture < 0 || !data || width <= 0 || height <= 0) return;

    auto& slot = gl_tex_to_bgfx_[state_.bound_texture];

    const bool wantMips = slot.mip_levels > 1;
    const uint16_t wanted =
        wantMips ? mipChainCount((uint32_t)width, (uint32_t)height) : 1;

    // PLCE: don't sample deeper than the last level the game uploads.
    if (wantMips) {
        slot.sampler_flags &= ~BGFX_SAMPLER_LOD_MASK;
        slot.sampler_flags |= BGFX_SAMPLER_LOD_CLAMP
            | ((uint32_t)(slot.mip_levels - 1) << BGFX_SAMPLER_LOD_SHIFT);
    }

    // PLCE: this used to crash on Nvidia(?).
    if (level == 0
    &&  ( !bgfx::isValid(slot.handle)
       || slot.width  != (uint32_t)width
       || slot.height != (uint32_t)height
       || slot.created_mips != wanted ) ) {
        if (bgfx::isValid(slot.handle)) bgfx::destroy(slot.handle);

        slot.handle = bgfx::createTexture2D((uint16_t)width, (uint16_t)height,
                                            wantMips, 1,
                                            bgfx::TextureFormat::RGBA8,
                                            slot.sampler_flags);
        slot.created_mips = wanted;
        slot.width  = (uint32_t)width;
        slot.height = (uint32_t)height;
        state_.bound_texture_sampler_flags = slot.sampler_flags;
    }

    if (level < 0 || (uint16_t)level >= slot.created_mips) return;

    const bgfx::Memory* mem = bgfx::copy(data, (uint32_t)(width * height * 4));
    bgfx::updateTexture2D(slot.handle, 0, (uint8_t)level, 0, 0,
                          (uint16_t)width, (uint16_t)height, mem);

    if (slot.mip_levels > 1 && (uint16_t)level == slot.mip_levels - 1
    &&  slot.mip_levels < slot.created_mips) {
        fillMipTail((uint16_t)level, (const uint8_t*)data, width, height,
                    slot.handle, slot.created_mips);
    }
}

void BgfxRenderPath::TextureDataUpdate(int xoff, int yoff, int w, int h,
                                       void* data, int level) {
    if (state_.bound_texture < 0 || !data) return;
    auto it = gl_tex_to_bgfx_.find(state_.bound_texture);
    if (it == gl_tex_to_bgfx_.end()) return;
    auto& slot = it->second;
    if (level < 0 || (uint16_t)level >= slot.created_mips) return;
    const bgfx::Memory* mem = bgfx::copy(data, w * h * 4);
    bgfx::updateTexture2D(slot.handle, 0, (uint8_t)level, (uint16_t)xoff,
                          (uint16_t)yoff, (uint16_t)w, (uint16_t)h, mem);

    if (slot.mip_levels > 1 && (uint16_t)level == slot.mip_levels - 1
    &&  slot.mip_levels < slot.created_mips && xoff == 0 && yoff == 0) {
        fillMipTail((uint16_t)level, (const uint8_t*)data, w, h, slot.handle,
                    slot.created_mips);
    }
}

int BgfxRenderPath::TextureGetTextureLevels() {
    auto it = gl_tex_to_bgfx_.find(state_.bound_texture);
    return it != gl_tex_to_bgfx_.end() ? (int)it->second.created_mips : 1;
}

void BgfxRenderPath::ReadPixels(int, int, int, int, void*) {}
static int* stb_pixels_to_argb(unsigned char* pixels, int w, int h) {
    int* px = new int[w * h];
    for (int i = 0; i < w * h; i++) {
        unsigned char r = pixels[i * 4], g = pixels[i * 4 + 1],
                      b = pixels[i * 4 + 2], a = pixels[i * 4 + 3];
        px[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
    return px;
}

int BgfxRenderPath::LoadTextureData(const char* filename, void* srcInfo,
                                    int** dataOut) {
    int w, h, channels;
    unsigned char* pixels = stbi_load(filename, &w, &h, &channels, 4);
    if (!pixels) return -1;
    auto* info = static_cast<D3DXIMAGE_INFO*>(srcInfo);
    if (info) {
        info->Width = w;
        info->Height = h;
    }
    *dataOut = stb_pixels_to_argb(pixels, w, h);
    stbi_image_free(pixels);
    return 0;
}

int BgfxRenderPath::LoadTextureData(uint8_t* data, uint32_t bytes,
                                    void* srcInfo, int** dataOut) {
    int w, h, channels;
    unsigned char* pixels =
        stbi_load_from_memory(data, bytes, &w, &h, &channels, 4);
    if (!pixels) return -1;
    auto* info = static_cast<D3DXIMAGE_INFO*>(srcInfo);
    if (info) {
        info->Width = w;
        info->Height = h;
    }
    *dataOut = stb_pixels_to_argb(pixels, w, h);
    stbi_image_free(pixels);
    return 0;
}

// MARK: Frame lifecycle

uint16_t BgfxRenderPath::resolveView(const glm::mat4& proj) {
    for (const auto& [p, v] : proj_view_cache_) {
        if (p == proj) return v;
    }
    const uint16_t view = (uint16_t)proj_view_cache_.size();
    if (view >= bgfx::getCaps()->limits.maxViews) return 0;
    bgfx::setViewRect(view, 0, 0, width_, height_);
    bgfx::setViewTransform(view, NULL, glm::value_ptr(proj));
    bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
    proj_view_cache_.emplace_back(proj, view);
    return view;
}

void BgfxRenderPath::StartFrame() {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    width_ = w;
    height_ = h;
    fb_.width = w;
    fb_.height = h;
    fb_.aspect = h > 0 ? (float)w / (float)h : 1.0f;
    fb_.is_widescreen = fb_.aspect > 1.5f;
    fb_.is_hi_def = h >= 720;
    current_view_id_ = 0;
    proj_view_cache_.clear();
    uint32_t rgba = (uint32_t(state_.clear_color[0] * 255) << 24) |
                    (uint32_t(state_.clear_color[1] * 255) << 16) |
                    (uint32_t(state_.clear_color[2] * 255) << 8) | 0xFF;
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, rgba, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width_, height_);
    bgfx::setViewMode(0, bgfx::ViewMode::Sequential);
    bgfx::touch(0);
}
void BgfxRenderPath::Present() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            should_close_ = true;
        } else if (ev.type == SDL_WINDOWEVENT) {
            if (ev.window.event == SDL_WINDOWEVENT_CLOSE)
                should_close_ = true;
            else if (ev.window.event == SDL_WINDOWEVENT_RESIZED)
                resize(ev.window.data1, ev.window.data2);
        }
    }
}
void BgfxRenderPath::Clear(int flags) {
    if (flags & rp::CLEAR_COLOR) {
        uint32_t rgba = (uint32_t(state_.clear_color[0] * 255) << 24) |
                        (uint32_t(state_.clear_color[1] * 255) << 16) |
                        (uint32_t(state_.clear_color[2] * 255) << 8) | 0xFF;
        uint16_t bgfx_flags = BGFX_CLEAR_COLOR;
        if (flags & rp::CLEAR_DEPTH) bgfx_flags |= BGFX_CLEAR_DEPTH;
        bgfx::setViewClear(current_view_id_, bgfx_flags, rgba, 1.0f, 0);
    }
}
void BgfxRenderPath::SetClearColour(const float rgba[4]) {
    state_.clear_color[0] = rgba[0];
    state_.clear_color[1] = rgba[1];
    state_.clear_color[2] = rgba[2];
    state_.clear_color[3] = rgba[3];
}
void BgfxRenderPath::Set_matrixDirty() {}
void BgfxRenderPath::CBuffLockStaticCreations() {}

// MARK: Window

void BgfxRenderPath::GetFramebufferSize(int& w, int& h) {
    w = width_;
    h = height_;
}
bool BgfxRenderPath::IsWidescreen() { return fb_.is_widescreen; }
bool BgfxRenderPath::IsHiDef() { return fb_.is_hi_def; }
void BgfxRenderPath::Close() {
    should_close_ = true;
    std::lock_guard<std::mutex> lk(cbuf_mtx_);
    shutdown_.store(true, std::memory_order_release);
}
bool BgfxRenderPath::ShouldClose() { return should_close_; }
void BgfxRenderPath::SetWindowSize(int w, int h) {
    SDL_SetWindowSize(window_, w, h);
    resize(w, h);
}
void BgfxRenderPath::SetFullscreen(bool fs) {
    SDL_SetWindowFullscreen(window_, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}
void BgfxRenderPath::UpdateGamma(unsigned short) {}
void BgfxRenderPath::Suspend() {}
bool BgfxRenderPath::Suspended() { return false; }
void BgfxRenderPath::Resume() {}

void BgfxRenderPath::BeginEvent(const char*) {}
void BgfxRenderPath::EndEvent() {}

void BgfxRenderPath::submit_immediate(const DrawCall& dc) {
    if (dc.source != rp::VertexSource::transient) return;
    const auto& tvb = dc.transient;
    uint32_t vertCount = tvb.vertex_count;
    if (vertCount == 0) return;
    uint32_t stride = vl_world_standard_.getStride();
    const uint8_t* src =
        reinterpret_cast<const uint8_t*>(transient_arena_.data() + tvb.offset);
    bool isFan = (tvb.primitive == rp::PrimitiveType::triangle_fan);
    uint32_t submitCount = vertCount;
    if (isFan && vertCount >= 3) submitCount = (vertCount - 2) * 3;
    if (bgfx::getAvailTransientVertexBuffer(submitCount, vl_world_standard_) <
        submitCount)
        return;
    bgfx::TransientVertexBuffer bvb;
    bgfx::allocTransientVertexBuffer(&bvb, submitCount, vl_world_standard_);
    if (isFan && vertCount >= 3) {
        uint8_t* dst = bvb.data;
        for (uint32_t i = 1; i < vertCount - 1; i++) {
            memcpy(dst, src, stride);
            dst += stride;
            memcpy(dst, src + i * stride, stride);
            dst += stride;
            memcpy(dst, src + (i + 1) * stride, stride);
            dst += stride;
        }
    } else {
        memcpy(bvb.data, src, vertCount * stride);
    }
    glm::mat4 proj = projection_stack_.top();
    if (state_.depth_z_bias != 0.0f) {
        proj[3][2] += state_.depth_z_bias * Z_BIAS_EPSILON;
    }
    const uint16_t view = resolveView(proj);
    bgfx::setTransform(glm::value_ptr(modelview_stack_.top()));
    bgfx::setState(state_.bgfx_state, state_.blend_factor_rgba);
    bgfx::setVertexBuffer(0, &bvb);

    bgfx::setUniform(u_baseColor_, dc.tint_color);
    float chunkOff[4] = {state_.chunk_offset[0], state_.chunk_offset[1],
                         state_.chunk_offset[2], 0};
    bgfx::setUniform(u_chunkOffset_, chunkOff);
    float lp[4] = {state_.lighting_enabled ? 1.0f : 0.0f, 1.0f, 0, 0};
    bgfx::setUniform(u_lightParams_, lp);
    const glm::vec4 l0 = lightDirUniform(0);
    const glm::vec4 l1 = lightDirUniform(1);
    bgfx::setUniform(u_light0Dir_, glm::value_ptr(l0));
    bgfx::setUniform(u_light1Dir_, glm::value_ptr(l1));
    bgfx::setUniform(u_lightDiffuse_, state_.light_diffuse);
    bgfx::setUniform(u_lightAmbient_, state_.light_ambient);
    float fp[4] = {state_.fog_enabled ? state_.fog_mode : 0.0f,
                   state_.fog_start, state_.fog_end, state_.fog_density};
    bgfx::setUniform(u_fogParams_, fp);
    bgfx::setUniform(u_lmTransform_, state_.lightmap_transform);
    bgfx::setUniform(u_globalLM_, state_.global_lm);
    bgfx::setUniform(u_texMatrix_, glm::value_ptr(texture_stack_.top()));

    bool hasTexture = false;
    bgfx::TextureHandle tex0 = white_tex_;
    uint32_t tex0Flags = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
    if (state_.texture_enabled && state_.bound_texture >= 0) {
        auto it = gl_tex_to_bgfx_.find(state_.bound_texture);
        if (it != gl_tex_to_bgfx_.end()) {
            tex0 = it->second.handle;
            tex0Flags = it->second.sampler_flags;
            hasTexture = true;
        }
    }
    bgfx::setTexture(0, s_tex0_, tex0, tex0Flags);
    const bgfx::TextureHandle lightmap =
        (state_.use_lightmap && bgfx::isValid(state_.bound_lightmap))
            ? state_.bound_lightmap
            : white_tex_;
    bgfx::setTexture(1, s_tex1_, lightmap,
                     BGFX_SAMPLER_MIN_ANISOTROPIC |
                         BGFX_SAMPLER_MAG_ANISOTROPIC |
                         BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    float fragP[4] = {hasTexture ? 1.0f : 0.0f,
                      state_.use_lightmap ? 1.0f : 0.0f, state_.alpha_ref,
                      state_.fog_enabled ? 1.0f : 0.0f};
    bgfx::setUniform(u_fragParams_, fragP);
    bgfx::setUniform(u_fogColor_, state_.fog_color);

    if (bgfx::isValid(program_))
        bgfx::submit(view, program_);
    else
        bgfx::discard();
}

// MARK: Factory

std::unique_ptr<rp::IRenderPath> make_bgfx_render_path(SDL_Window* window) {
    return std::make_unique<BgfxRenderPath>(window);
}
