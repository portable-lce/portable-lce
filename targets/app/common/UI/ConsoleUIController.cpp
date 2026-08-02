

// GDraw GL backend for Linux
#include "ConsoleUIController.h"

#include "app/common/Iggy/gdraw/gdraw.h"
#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Game.h"
#include "app/common/Iggy/include/gdraw.h"
#include "app/common/Iggy/include/rrCore.h"

ConsoleUIController ui;

static void restoreFixedFunctionStateAfterIggy() {
    RenderPath.StateSetColour(1.0f, 1.0f, 1.0f, 1.0f);
    RenderPath.StateSetAlphaTestEnable(true);
    RenderPath.StateSetAlphaFunc(rp::AlphaTest::greater, 0.1f);
    RenderPath.StateSetDepthTestEnable(true);
    RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
    RenderPath.StateSetFaceCull(true);
    (void)0;

    RenderPath.StateSetActiveTexture(0x84C1);
    RenderPath.StateSetActiveTexture(0x84C1);
    RenderPath.StateSetTextureEnable(false);
    RenderPath.MatrixMode(rp::MatrixStack::texture);
    RenderPath.MatrixSetIdentity();

    RenderPath.StateSetActiveTexture(0x84C0);
    RenderPath.StateSetActiveTexture(0x84C0);
    RenderPath.StateSetTextureEnable(true);
    RenderPath.MatrixMode(rp::MatrixStack::texture);
    RenderPath.MatrixSetIdentity();

    RenderPath.MatrixMode(rp::MatrixStack::modelview);
}

void ConsoleUIController::init(S32 w, S32 h) {
#ifdef _ENABLEIGGY
    // Shared init
    preInit(w, h);

    // init
    gdraw_funcs = gdraw_GL_CreateContext(w, h, 0);

    if (!gdraw_funcs) {
        app.DebugPrintf("Failed to initialise GDraw GL!\n");
        app.FatalLoadError();
    }

    gdraw_GL_SetResourceLimits(GDRAW_GL_RESOURCE_vertexbuffer, 5000,
                               16 * 1024 * 1024);
    gdraw_GL_SetResourceLimits(GDRAW_GL_RESOURCE_texture, 5000,
                               128 * 1024 * 1024);
    gdraw_GL_SetResourceLimits(GDRAW_GL_RESOURCE_rendertarget, 10,
                               64 * 1024 * 1024);

    IggySetGDraw(gdraw_funcs);
#endif
    postInit();
}

void ConsoleUIController::render() {
#ifdef _ENABLEIGGY
    if (!gdraw_funcs) return;

    gdraw_GL_SetTileOrigin(0, 0, 0);
    if (!app.GetGameStarted() && gdraw_funcs->ClearID) {
        gdraw_funcs->ClearID();
    }

    // render
    renderScenes();

    gdraw_GL_NoMoreGDrawThisFrame();
    restoreFixedFunctionStateAfterIggy();
#endif
}

void ConsoleUIController::beginIggyCustomDraw4J(
    IggyCustomDrawCallbackRegion* region, CustomDrawData* customDrawRegion) {
    gdraw_GL_BeginCustomDraw_4J(region, customDrawRegion->mat);
}

CustomDrawData* ConsoleUIController::setupCustomDraw(
    UIScene* scene, IggyCustomDrawCallbackRegion* region) {
    CustomDrawData* customDrawRegion = new CustomDrawData();
    customDrawRegion->x0 = region->x0;
    customDrawRegion->x1 = region->x1;
    customDrawRegion->y0 = region->y0;
    customDrawRegion->y1 = region->y1;

    gdraw_GL_BeginCustomDraw_4J(region, customDrawRegion->mat);

    setupCustomDrawGameStateAndMatrices(scene, customDrawRegion);

    return customDrawRegion;
}

CustomDrawData* ConsoleUIController::calculateCustomDraw(
    IggyCustomDrawCallbackRegion* region) {
    CustomDrawData* customDrawRegion = new CustomDrawData();
    customDrawRegion->x0 = region->x0;
    customDrawRegion->x1 = region->x1;
    customDrawRegion->y0 = region->y0;
    customDrawRegion->y1 = region->y1;

    gdraw_GL_CalculateCustomDraw_4J(region, customDrawRegion->mat);

    return customDrawRegion;
}

void ConsoleUIController::endCustomDraw(IggyCustomDrawCallbackRegion* region) {
    endCustomDrawGameStateAndMatrices();

    gdraw_GL_EndCustomDraw(region);
}

void ConsoleUIController::setTileOrigin(S32 xPos, S32 yPos) {
    gdraw_GL_SetTileOrigin(xPos, yPos, 0);
}

GDrawTexture* ConsoleUIController::getSubstitutionTexture(int textureId) {
    // todo impl
    return nullptr;
}

void ConsoleUIController::destroySubstitutionTexture(void* destroyCallBackData,
                                                     GDrawTexture* handle) {
    if (handle) gdraw_GL_WrappedTextureDestroy(handle);
}

void ConsoleUIController::shutdown() {
#ifdef _ENABLEIGGY
    if (gdraw_funcs) {
        gdraw_GL_DestroyContext();
        gdraw_funcs = nullptr;
    }
#endif
}
