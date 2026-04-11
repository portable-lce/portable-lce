#pragma once

#include "gl3_loader.h"
// NOTE: gl3_loader.h must be included before these two

#include <cstdint>

#include "platform/renderer/IPlatformRenderer.h"

extern IPlatformRenderer& PlatformRenderer;

class GLRenderer : public IPlatformRenderer {
public:
    void Tick();
    void UpdateGamma(unsigned short usGamma);

    // Matrix stack
    void MatrixMode(int type);
    void MatrixSetIdentity();
    void MatrixTranslate(float x, float y, float z);
    void MatrixRotate(float angle, float x, float y, float z);
    void MatrixScale(float x, float y, float z);
    void MatrixPerspective(float fovy, float aspect, float zNear, float zFar);
    void MatrixOrthogonal(float left, float right, float bottom, float top,
                          float zNear, float zFar);
    void MatrixPop();
    void MatrixPush();
    void MatrixMult(float* mat);
    const float* MatrixGet(int type);
    void Set_matrixDirty();

    // Core
    void Initialise();
    void InitialiseContext();
    void SetWindowSize(int w, int h);
    void SetFullscreen(bool fs);
    void StartFrame();
    void DoScreenGrabOnNextPresent();
    void Present();
    void Clear(int flags);
    void SetClearColour(const float colourRGBA[4]);
    void SetChunkOffset(float x, float y, float z);
    bool IsWidescreen();
    bool IsHiDef();
    void GetFramebufferSize(int& width, int& height);
    void CaptureThumbnail(ImageFileBuffer* pngOut);
    void CaptureScreen(ImageFileBuffer* jpgOut,
                       XSOCIAL_PREVIEWIMAGE* previewOut);
    void BeginConditionalSurvey(int identifier);
    void EndConditionalSurvey();
    void BeginConditionalRendering(int identifier);
    void EndConditionalRendering();

    void DrawVertices(ePrimitiveType PrimitiveType, int count, void* dataIn,
                      eVertexType vType, ePixelShaderType psType);

    // Command buffers
    void CBuffLockStaticCreations();
    int CBuffCreate(int count);
    void CBuffDelete(int first, int count);
    void CBuffDeleteAll();
    void CBuffStart(int index, bool full = false);
    void CBuffClear(int index);
    int CBuffSize(int index);
    void CBuffEnd();
    bool CBuffCall(int index, bool full = true);
    void CBuffTick();
    void CBuffDeferredModeStart();
    void CBuffDeferredModeEnd();

    // Textures
    int TextureCreate();
    void TextureFree(int idx);
    void TextureBind(int idx);
    void TextureBindVertex(int idx, bool scaleLight = false);
    void TextureSetTextureLevels(int levels);
    int TextureGetTextureLevels();
    void TextureData(int width, int height, void* data, int level,
                     eTextureFormat format = TEXTURE_FORMAT_RxGyBzAw);
    void TextureDataUpdate(int xoffset, int yoffset, int width, int height,
                           void* data, int level);
    void TextureSetParam(int param, int value);
    void TextureDynamicUpdateStart();
    void TextureDynamicUpdateEnd();

    int LoadTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo,
                        int** ppDataOut);
    int LoadTextureData(std::uint8_t* pbData, std::uint32_t byteCount,
                        D3DXIMAGE_INFO* pSrcInfo, int** ppDataOut);
    int SaveTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo,
                        int* ppDataOut);
    int SaveTextureDataToMemory(void* pOutput, int outputCapacity,
                                int* outputLength, int width, int height,
                                int* ppDataIn);

    void ReadPixels(int x, int y, int w, int h, void* buf);
    void TextureGetStats();
    void* TextureGetTexture(int idx);

    // State control
    void StateSetColour(float r, float g, float b, float a);
    void StateSetDepthMask(bool enable);
    void StateSetBlendEnable(bool enable);
    void StateSetBlendFunc(int src, int dst);
    void StateSetBlendFactor(unsigned int colour);
    void StateSetAlphaFunc(int func, float param);
    void StateSetDepthFunc(int func);
    void StateSetFaceCull(bool enable);
    void StateSetFaceCullCW(bool enable);
    void StateSetLineWidth(float width);
    void StateSetWriteEnable(bool red, bool green, bool blue, bool alpha);
    void StateSetDepthTestEnable(bool enable);
    void StateSetAlphaTestEnable(bool enable);
    void StateSetDepthSlopeAndBias(float slope, float bias);
    void StateSetFogEnable(bool enable);
    void StateSetFogMode(int mode);
    void StateSetFogNearDistance(float dist);
    void StateSetFogFarDistance(float dist);
    void StateSetFogDensity(float density);
    void StateSetFogColour(float red, float green, float blue);
    void StateSetLightingEnable(bool enable);
    void StateSetVertexTextureUV(float u, float v);
    void StateSetLightColour(int light, float red, float green, float blue);
    void StateSetLightAmbientColour(float red, float green, float blue);
    void StateSetLightDirection(int light, float x, float y, float z);
    void StateSetLightEnable(int light, bool enable);
    void StateSetViewport(eViewportType viewportType);
    void StateSetEnableViewportClipPlanes(bool enable);
    void StateSetTexGenCol(int col, float x, float y, float z, float w,
                           bool eyeSpace);
    void StateSetStencil(int Function, std::uint8_t stencil_ref,
                         std::uint8_t stencil_func_mask,
                         std::uint8_t stencil_write_mask);
    void StateSetForceLOD(int LOD);
    void StateSetTextureEnable(bool enable);
    void StateSetActiveTexture(int tex);

    // Event tracking
    void BeginEvent(const char* eventName);
    void EndEvent();

    // PLM event handling
    void Suspend();
    bool Suspended();
    void Resume();

    // Linux window management
    bool ShouldClose();
    void Close();
    void Shutdown();
};
