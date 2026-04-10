#pragma once

#include <cstdint>

#include "platform/PlatformTypes.h"

class IPlatformRenderer {
public:
    enum eVertexType {
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1,
        VERTEX_TYPE_COMPRESSED,
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT,
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN,
        VERTEX_TYPE_COUNT
    };

    enum ePixelShaderType {
        PIXEL_SHADER_TYPE_STANDARD,
        PIXEL_SHADER_TYPE_PROJECTION,
        PIXEL_SHADER_TYPE_FORCELOD,
        PIXEL_SHADER_COUNT
    };

    enum eViewportType {
        VIEWPORT_TYPE_FULLSCREEN,
        VIEWPORT_TYPE_SPLIT_TOP,
        VIEWPORT_TYPE_SPLIT_BOTTOM,
        VIEWPORT_TYPE_SPLIT_LEFT,
        VIEWPORT_TYPE_SPLIT_RIGHT,
        VIEWPORT_TYPE_QUADRANT_TOP_LEFT,
        VIEWPORT_TYPE_QUADRANT_TOP_RIGHT,
        VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT,
        VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT,
    };

    enum ePrimitiveType {
        PRIMITIVE_TYPE_TRIANGLE_LIST,
        PRIMITIVE_TYPE_TRIANGLE_STRIP,
        PRIMITIVE_TYPE_TRIANGLE_FAN,
        PRIMITIVE_TYPE_QUAD_LIST,
        PRIMITIVE_TYPE_LINE_LIST,
        PRIMITIVE_TYPE_LINE_STRIP,
        PRIMITIVE_TYPE_COUNT
    };

    enum eTextureFormat { TEXTURE_FORMAT_RxGyBzAw, MAX_TEXTURE_FORMATS };

    virtual ~IPlatformRenderer() = default;

    // Lifecycle
    virtual void Initialise() = 0;
    virtual void InitialiseContext() = 0;
    virtual void Tick() = 0;
    virtual void StartFrame() = 0;
    virtual void Present() = 0;
    virtual void Clear(int flags) = 0;
    virtual void SetClearColour(const float colourRGBA[4]) = 0;
    virtual void Shutdown() = 0;
    virtual void Suspend() = 0;
    [[nodiscard]] virtual bool Suspended() = 0;
    virtual void Resume() = 0;

    // Window
    virtual void SetWindowSize(int w, int h) = 0;
    virtual void SetFullscreen(bool fs) = 0;
    [[nodiscard]] virtual bool IsWidescreen() = 0;
    [[nodiscard]] virtual bool IsHiDef() = 0;
    virtual void GetFramebufferSize(int& width, int& height) = 0;
    [[nodiscard]] virtual bool ShouldClose() = 0;
    virtual void Close() = 0;
    virtual void UpdateGamma(unsigned short usGamma) = 0;

    // Matrix stack
    virtual void MatrixMode(int type) = 0;
    virtual void MatrixSetIdentity() = 0;
    virtual void MatrixTranslate(float x, float y, float z) = 0;
    virtual void MatrixRotate(float angle, float x, float y, float z) = 0;
    virtual void MatrixScale(float x, float y, float z) = 0;
    virtual void MatrixPerspective(float fovy, float aspect, float zNear,
                                   float zFar) = 0;
    virtual void MatrixOrthogonal(float left, float right, float bottom,
                                  float top, float zNear, float zFar) = 0;
    virtual void MatrixPop() = 0;
    virtual void MatrixPush() = 0;
    virtual void MatrixMult(float* mat) = 0;
    [[nodiscard]] virtual const float* MatrixGet(int type) = 0;
    virtual void Set_matrixDirty() = 0;

    // Draw calls
    virtual void DrawVertices(ePrimitiveType PrimitiveType, int count,
                              void* dataIn, eVertexType vType,
                              ePixelShaderType psType) = 0;

    // Command buffers
    virtual void CBuffLockStaticCreations() = 0;
    [[nodiscard]] virtual int CBuffCreate(int count) = 0;
    virtual void CBuffDelete(int first, int count) = 0;
    virtual void CBuffDeleteAll() = 0;
    virtual void CBuffStart(int index, bool full = false) = 0;
    virtual void CBuffClear(int index) = 0;
    [[nodiscard]] virtual int CBuffSize(int index) = 0;
    virtual void CBuffEnd() = 0;
    [[nodiscard]] virtual bool CBuffCall(int index, bool full = true) = 0;
    virtual void CBuffTick() = 0;
    virtual void CBuffDeferredModeStart() = 0;
    virtual void CBuffDeferredModeEnd() = 0;

    // Textures
    [[nodiscard]] virtual int TextureCreate() = 0;
    virtual void TextureFree(int idx) = 0;
    virtual void TextureBind(int idx) = 0;
    virtual void TextureBindVertex(int idx, bool scaleLight = false) = 0;
    virtual void TextureSetTextureLevels(int levels) = 0;
    [[nodiscard]] virtual int TextureGetTextureLevels() = 0;
    virtual void TextureData(
        int width, int height, void* data, int level,
        eTextureFormat format = TEXTURE_FORMAT_RxGyBzAw) = 0;
    virtual void TextureDataUpdate(int xoffset, int yoffset, int width,
                                   int height, void* data, int level) = 0;
    virtual void TextureSetParam(int param, int value) = 0;
    virtual void TextureDynamicUpdateStart() = 0;
    virtual void TextureDynamicUpdateEnd() = 0;
    [[nodiscard]] virtual int LoadTextureData(const char* szFilename,
                                              D3DXIMAGE_INFO* pSrcInfo,
                                              int** ppDataOut) = 0;
    [[nodiscard]] virtual int LoadTextureData(std::uint8_t* pbData,
                                              std::uint32_t byteCount,
                                              D3DXIMAGE_INFO* pSrcInfo,
                                              int** ppDataOut) = 0;
    [[nodiscard]] virtual int SaveTextureData(const char* szFilename,
                                              D3DXIMAGE_INFO* pSrcInfo,
                                              int* ppDataOut) = 0;
    [[nodiscard]] virtual int SaveTextureDataToMemory(void* pOutput,
                                                      int outputCapacity,
                                                      int* outputLength,
                                                      int width, int height,
                                                      int* ppDataIn) = 0;
    virtual void ReadPixels(int x, int y, int w, int h, void* buf) = 0;
    virtual void TextureGetStats() = 0;
    [[nodiscard]] virtual void* TextureGetTexture(int idx) = 0;

    // Render state
    virtual void StateSetColour(float r, float g, float b, float a) = 0;
    virtual void StateSetDepthMask(bool enable) = 0;
    virtual void StateSetBlendEnable(bool enable) = 0;
    virtual void StateSetBlendFunc(int src, int dst) = 0;
    virtual void StateSetBlendFactor(unsigned int colour) = 0;
    virtual void StateSetAlphaFunc(int func, float param) = 0;
    virtual void StateSetDepthFunc(int func) = 0;
    virtual void StateSetFaceCull(bool enable) = 0;
    virtual void StateSetFaceCullCW(bool enable) = 0;
    virtual void StateSetLineWidth(float width) = 0;
    virtual void StateSetWriteEnable(bool red, bool green, bool blue,
                                     bool alpha) = 0;
    virtual void StateSetDepthTestEnable(bool enable) = 0;
    virtual void StateSetAlphaTestEnable(bool enable) = 0;
    virtual void StateSetDepthSlopeAndBias(float slope, float bias) = 0;

    // Fog
    virtual void StateSetFogEnable(bool enable) = 0;
    virtual void StateSetFogMode(int mode) = 0;
    virtual void StateSetFogNearDistance(float dist) = 0;
    virtual void StateSetFogFarDistance(float dist) = 0;
    virtual void StateSetFogDensity(float density) = 0;
    virtual void StateSetFogColour(float red, float green, float blue) = 0;

    // Lighting
    virtual void StateSetLightingEnable(bool enable) = 0;
    virtual void StateSetVertexTextureUV(float u, float v) = 0;
    virtual void StateSetLightColour(int light, float red, float green,
                                     float blue) = 0;
    virtual void StateSetLightAmbientColour(float red, float green,
                                            float blue) = 0;
    virtual void StateSetLightDirection(int light, float x, float y,
                                        float z) = 0;
    virtual void StateSetLightEnable(int light, bool enable) = 0;

    // Viewport & clipping
    virtual void StateSetViewport(eViewportType viewportType) = 0;
    virtual void StateSetEnableViewportClipPlanes(bool enable) = 0;
    virtual void StateSetTexGenCol(int col, float x, float y, float z, float w,
                                   bool eyeSpace) = 0;
    virtual void StateSetStencil(int Function, std::uint8_t stencil_ref,
                                 std::uint8_t stencil_func_mask,
                                 std::uint8_t stencil_write_mask) = 0;
    virtual void StateSetForceLOD(int LOD) = 0;
    virtual void StateSetTextureEnable(bool enable) = 0;
    virtual void StateSetActiveTexture(int tex) = 0;

    // Chunks
    virtual void SetChunkOffset(float x, float y, float z) = 0;

    // Occlusion
    virtual void BeginConditionalSurvey(int identifier) = 0;
    virtual void EndConditionalSurvey() = 0;
    virtual void BeginConditionalRendering(int identifier) = 0;
    virtual void EndConditionalRendering() = 0;

    // Screenshots
    virtual void DoScreenGrabOnNextPresent() = 0;
    virtual void CaptureThumbnail(ImageFileBuffer* pngOut) = 0;
    virtual void CaptureScreen(ImageFileBuffer* jpgOut,
                               XSOCIAL_PREVIEWIMAGE* previewOut) = 0;

    // Events
    virtual void BeginEvent(const char* eventName) = 0;
    virtual void EndEvent() = 0;
};
