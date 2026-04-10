#ifndef __LINUX_IGGY_GDRAW_H__
#define __LINUX_IGGY_GDRAW_H__

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/Iggy/include/gdraw.h"
#include "app/common/Iggy/include/iggy.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum gdraw_gl_resourcetype {
    GDRAW_GL_RESOURCE_rendertarget,
    GDRAW_GL_RESOURCE_texture,
    GDRAW_GL_RESOURCE_vertexbuffer,
    GDRAW_GL_RESOURCE__count,
} gdraw_gl_resourcetype;

struct IggyCustomDrawCallbackRegion;

extern int gdraw_GL_SetResourceLimits(gdraw_gl_resourcetype type,
                                      S32 num_handles, S32 num_bytes);
extern GDrawFunctions* gdraw_GL_CreateContext(S32 min_w, S32 min_h,
                                              S32 msaa_samples);
extern void gdraw_GL_DestroyContext(void);
extern void gdraw_GL_SetTileOrigin(S32 vx, S32 vy, unsigned int framebuffer);
extern void gdraw_GL_NoMoreGDrawThisFrame(void);
extern GDrawTexture* gdraw_GL_WrappedTextureCreate(S32 gl_texture_handle,
                                                   S32 width, S32 height,
                                                   int has_mipmaps);
extern void gdraw_GL_WrappedTextureChange(GDrawTexture* tex,
                                          S32 new_gl_texture_handle,
                                          S32 new_width, S32 new_height,
                                          int new_has_mipmaps);
extern void gdraw_GL_WrappedTextureDestroy(GDrawTexture* tex);
extern void gdraw_GL_BeginCustomDraw(
    struct IggyCustomDrawCallbackRegion* region, float* matrix);
extern void gdraw_GL_EndCustomDraw(struct IggyCustomDrawCallbackRegion* region);
extern void gdraw_GL_CalculateCustomDraw_4J(
    struct IggyCustomDrawCallbackRegion* region, float* matrix);
extern void gdraw_GL_BeginCustomDraw_4J(
    struct IggyCustomDrawCallbackRegion* region, float* matrix);
extern GDrawTexture* gdraw_GL_MakeTextureFromResource(
    unsigned char* resource_file, S32 resource_len,
    IggyFileTextureRaw* texture);
extern void gdraw_GL_DestroyTextureFromResource(GDrawTexture* tex);

#ifdef __cplusplus
}
#endif

#endif  // __LINUX_IGGY_GDRAW_H__