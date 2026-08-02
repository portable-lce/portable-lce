#pragma once

#include <bgfx/bgfx.h>

#include <atomic>
#include <glm/glm.hpp>
#include <mutex>
#include <stack>
#include <unordered_map>
#include <vector>

#include "platform/renderer/IRenderPath.h"

struct SDL_Window;

class BgfxRenderPath final : public rp::IRenderPath {
public:
    explicit BgfxRenderPath(SDL_Window* window);
    ~BgfxRenderPath() override;

    // MARK: new APIs

    [[nodiscard]] rp::MeshHandle create_mesh(const rp::MeshDesc& desc) override;
    void update_mesh(rp::MeshHandle h, const rp::MeshDesc& desc) override;
    void destroy_mesh(rp::MeshHandle h) override;

    [[nodiscard]] rp::TextureHandle create_texture(
        const rp::TextureDesc& desc) override;
    void update_texture(rp::TextureHandle h,
                        const rp::TextureRegion& region) override;
    void destroy_texture(rp::TextureHandle h) override;

    [[nodiscard]] rp::MaterialHandle create_material(
        const rp::MaterialDesc& desc) override;
    void update_material(rp::MaterialHandle h,
                         const rp::MaterialDesc& desc) override;
    void destroy_material(rp::MaterialHandle h) override;

    [[nodiscard]] std::pair<rp::TransientVertexBuffer, std::span<std::byte>>
    alloc_transient_vertices(uint32_t vertex_count, rp::VertexLayout layout,
                             rp::PrimitiveType primitive) override;

    void render_frame(const rp::FrameDesc& frame) override;
    void resize(uint32_t w, uint32_t h) override;
    [[nodiscard]] const rp::FrameFramebuffer& framebuffer() const override;
    void read_framebuffer(const rp::TextureReadback& req) override;
    [[nodiscard]] rp::ResourceFootprint query_resource_footprint()
        const override;
    void seal_static_resource_tier() override;
    void begin_atomic_resource_batch() override;
    void end_atomic_resource_batch() override;
    void push_debug_event(const char* name) override;
    void pop_debug_event() override;
    void tick() override;

    // MARK: Legacy fixed-function state

    void MatrixMode(rp::MatrixStack stack) override;
    void MatrixSetIdentity() override;
    void MatrixTranslate(float x, float y, float z) override;
    void MatrixRotate(float angle, float x, float y, float z) override;
    void MatrixScale(float x, float y, float z) override;
    void MatrixPerspective(float fovy, float aspect, float zNear,
                           float zFar) override;
    void MatrixOrthogonal(float left, float right, float bottom, float top,
                          float zNear, float zFar) override;
    void MatrixPop() override;
    void MatrixPush() override;
    void MatrixMult(float* mat) override;
    [[nodiscard]] const float* MatrixGet(rp::MatrixStack stack) override;

    void DrawVertices(int primitiveType, int count, void* data, int vertexType,
                      int shaderType) override;

    [[nodiscard]] int CBuffCreate(int count) override;
    void CBuffDelete(int first, int count) override;
    void CBuffDeleteAll() override;
    void CBuffStart(int index, bool full) override;
    void CBuffClear(int index) override;
    [[nodiscard]] int CBuffSize(int index) override;
    void CBuffEnd() override;
    [[nodiscard]] bool CBuffCall(int index, bool full) override;
    void CBuffDeferredModeStart() override;
    void CBuffDeferredModeEnd() override;

    [[nodiscard]] int TextureCreate() override;
    void TextureFree(int idx) override;
    void TextureBind(int idx) override;
    void TextureBindVertex(int idx, bool scaleLight) override;
    void TextureSetTextureLevels(int levels) override;
    void TextureData(int width, int height, void* data, int level,
                     int format) override;
    void TextureDataUpdate(int xoff, int yoff, int w, int h, void* data,
                           int level) override;
    void TextureSetParam(int param, int value) override;

    void StateSetColour(float r, float g, float b, float a) override;
    void StateSetDepthMask(bool enable) override;
    void StateSetBlendEnable(bool enable) override;
    void StateSetBlendFunc(rp::BlendFactor src, rp::BlendFactor dst) override;
    void StateSetBlendFactor(unsigned int colour) override;
    void StateSetAlphaFunc(rp::AlphaTest func, float param) override;
    void StateSetDepthFunc(rp::DepthTest func) override;
    void StateSetFaceCull(bool enable) override;
    void StateSetLineWidth(float width) override;
    void StateSetWriteEnable(bool r, bool g, bool b, bool a) override;
    void StateSetDepthTestEnable(bool enable) override;
    void StateSetAlphaTestEnable(bool enable) override;
    void StateSetDepthSlopeAndBias(float slope, float bias) override;

    void StateSetFogEnable(bool enable) override;
    void StateSetFogMode(rp::FogMode mode) override;
    void StateSetFogNearDistance(float dist) override;
    void StateSetFogFarDistance(float dist) override;
    void StateSetFogDensity(float density) override;
    void StateSetFogColour(float r, float g, float b) override;

    void StateSetLightingEnable(bool enable) override;
    void StateSetLightColour(int light, float r, float g, float b) override;
    void StateSetLightAmbientColour(float r, float g, float b) override;
    void StateSetLightDirection(int light, float x, float y, float z) override;
    void StateSetLightEnable(int light, bool enable) override;

    void StateSetViewport(int viewportType) override;
    void StateSetEnableViewportClipPlanes(bool enable) override;
    void StateSetStencil(int func, uint8_t ref, uint8_t funcMask,
                         uint8_t writeMask) override;
    void StateSetForceLOD(int lod) override;
    void StateSetTextureEnable(bool enable) override;
    void StateSetActiveTexture(int tex) override;

    void SetChunkOffset(float x, float y, float z) override;

    [[nodiscard]] int TextureGetTextureLevels() override;
    void ReadPixels(int x, int y, int w, int h, void* buf) override;
    [[nodiscard]] int LoadTextureData(const char* filename, void* srcInfo,
                                      int** dataOut) override;
    [[nodiscard]] int LoadTextureData(uint8_t* data, uint32_t bytes,
                                      void* srcInfo, int** dataOut) override;

    void StateSetVertexTextureUV(float u, float v) override;

    void StartFrame() override;
    void Present() override;
    void Clear(int flags) override;
    void SetClearColour(const float rgba[4]) override;
    void Set_matrixDirty() override;
    void CBuffLockStaticCreations() override;

    void GetFramebufferSize(int& w, int& h) override;
    [[nodiscard]] bool IsWidescreen() override;
    [[nodiscard]] bool IsHiDef() override;
    void Close() override;
    [[nodiscard]] bool ShouldClose() override;
    void SetWindowSize(int w, int h) override;
    void SetFullscreen(bool fs) override;
    void UpdateGamma(unsigned short gamma) override;
    void Suspend() override;
    [[nodiscard]] bool Suspended() override;
    void Resume() override;

    void BeginEvent(const char* name) override;
    void EndEvent() override;

    void submit_immediate(const rp::DrawCall& dc) override;

private:
    std::stack<glm::mat4>& current_stack();

    SDL_Window* window_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    rp::FrameFramebuffer fb_{};
    bool should_close_ = false;

    // Software matrix stack
    rp::MatrixStack matrix_mode_ = rp::MatrixStack::modelview;  // GL_MODELVIEW
    std::stack<glm::mat4> projection_stack_;
    std::stack<glm::mat4> modelview_stack_;
    std::stack<glm::mat4> texture_stack_;
    glm::mat4 projection_top_{1.0f};
    glm::mat4 modelview_top_{1.0f};

    // bgfx vertex layouts
    bgfx::VertexLayout vl_world_standard_;

    // bgfx shader program
    bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_baseColor_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_chunkOffset_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_lightParams_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_light0Dir_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_light1Dir_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_lightDiffuse_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_lightAmbient_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fogParams_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_lmTransform_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_globalLM_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_texMatrix_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fragParams_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fogColor_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_tex0_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_tex1_ = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle white_tex_ = BGFX_INVALID_HANDLE;

    // View ID management
    uint16_t current_view_id_ = 0;

    std::vector<std::pair<glm::mat4, uint16_t>> proj_view_cache_;
    uint16_t resolveView(const glm::mat4& proj);
    glm::vec4 lightDirUniform(int l) const;
    // Transient arena
    std::vector<std::byte> transient_arena_;
    uint32_t transient_offset_ = 0;
    uint32_t current_frame_ = 0;

    // Material storage
    struct MaterialSlot {
        rp::MaterialDesc desc;
        uint32_t generation = 0;
        bool occupied = false;
    };
    std::vector<MaterialSlot> materials_;

    // Texture storage (maps our handles to bgfx handles)
    struct TextureSlot {
        bgfx::TextureHandle bgfx_handle = BGFX_INVALID_HANDLE;
        uint32_t generation = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        bool occupied = false;
    };
    std::vector<TextureSlot> textures_;

    // GL texture ID to our handle map (for legacy TextureCreate/Bind)
    struct GlTexSlot {
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
        uint32_t sampler_flags =
            BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
        uint16_t mip_levels = 1;
        uint16_t created_mips = 1;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    std::unordered_map<int, GlTexSlot> gl_tex_to_bgfx_;
    int next_gl_tex_id_ = 1;

    // CBuffer (display list) system
    struct CBuffDrawCmd {
        int prim;
        int first;
        int count;
    };
    struct CommandBuffer {
        bgfx::DynamicVertexBufferHandle vbh = BGFX_INVALID_HANDLE;
        std::vector<uint8_t> raw_verts;
        std::vector<CBuffDrawCmd> draws;
        bool valid = false;
        bool vb_ready = false;
    };
    struct RenderState {
        uint64_t bgfx_state = 0;
        bool depth_write = true;
        bool blend_enabled = false;
        bool depth_test_enabled = true;
        bool cull_enabled = true;
        bool texture_enabled = true;
        bool fog_enabled = false;
        bool lighting_enabled = false;
        int bound_texture = -1;
        uint32_t bound_texture_sampler_flags =
            BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
        float tint_color[4] = {1, 1, 1, 1};
        float chunk_offset[3] = {0, 0, 0};
        float fog_mode = 0;
        float fog_start = 0;
        float fog_end = 100;
        float fog_density = 0;
        float fog_color[4] = {0.5f, 0.7f, 1.0f, 1.0f};
        float light0_dir[4] = {-0.173913f, 0.869565f, -0.608696f, 0};
        float light1_dir[4] = {0.173913f, -0.869565f, 0.608696f, 0};
        float light_diffuse[4] = {0.6f, 0.6f, 0.6f, 1};
        float light_ambient[4] = {0.4f, 0.4f, 0.4f, 1};
        float global_lm[4] = {
            240.0, 240.0, 0.0,
            0.0};  // actually a vec2, last two components ignored
        float alpha_ref = 0.1f;
        float clear_color[4] = {0.19f, 0.19f, 0.19f, 1.0f};
        float depth_slope_bias = 0.0f;
        float depth_z_bias = 0.0f;
        float gamma = 1.0f;
        rp::BlendFactor blend_src = rp::BlendFactor::src_alpha;
        rp::BlendFactor blend_dst = rp::BlendFactor::one_minus_src_alpha;
        uint32_t blend_factor_rgba = 0xFFFFFFFF;
        bool alpha_test_enabled = false;
        uint64_t depth_func_bits = BGFX_STATE_DEPTH_TEST_LEQUAL;
        float lightmap_transform[4] = {1.0, 1.0, 0.0, 0.0};
        bool use_lightmap = false;
        bgfx::TextureHandle bound_lightmap = BGFX_INVALID_HANDLE;
    };

    std::mutex cbuf_mtx_;
    std::atomic<bool> shutdown_{false};
    std::unordered_map<int, CommandBuffer> cbuf_pool_;
    std::vector<bgfx::DynamicVertexBufferHandle> cbuf_destroy_queue_;
    int cbuf_next_id_ = 1;
    static thread_local int cbuf_rec_id_;
    static thread_local std::vector<uint8_t> cbuf_rec_verts_;
    static thread_local std::vector<CBuffDrawCmd> cbuf_rec_draws_;
    static thread_local RenderState state_;
};
