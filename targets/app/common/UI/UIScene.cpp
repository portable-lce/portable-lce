#include "UIScene.h"

#include <cstddef>
#include <mutex>
#include <utility>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIController.h"
#include "app/common/UI/UIGroup.h"
#include "app/common/UI/UILayer.h"
#include "app/linux/Iggy/include/iggy.h"
#include "platform/PlatformTypes.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/linux/Stubs/iggy_stubs.h"
#endif
#include "app/linux/Iggy/include/rrCore.h"
#include "app/common/Game.h"
#include "app/linux/Linux_UIController.h"
#include "java/System.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/renderer/entity/ItemRenderer.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/item/ItemInstance.h"
#include "util/StringHelpers.h"

class MultiplayerLocalPlayer;

UIScene::UIScene(int iPad, UILayer* parentLayer) {
    m_parentLayer = parentLayer;
    m_iPad = iPad;
    swf = nullptr;
    m_pItemRenderer = nullptr;

    bHasFocus = false;
    m_hasTickedOnce = false;
    m_bFocussedOnce = false;
    m_bVisible = true;
    m_bCanHandleInput = false;
    m_bIsReloading = false;
    m_hasSetSafeZoneMethod = false;

    m_iFocusControl = -1;
    m_iFocusChild = 0;
    m_lastOpacity = 1.0f;
    m_bUpdateOpacity = false;

    m_backScene = nullptr;

    m_cacheSlotRenders = false;
    m_needsCacheRendered = true;
    m_expectedCachedSlotCount = 0;
    m_callbackUniqueId = 0;
}

UIScene::~UIScene() {
    /* Destroy the Iggy player. */
    IggyPlayerDestroy(swf);

    for (auto it = m_registeredTextures.begin();
         it != m_registeredTextures.end(); ++it) {
        ui.unregisterSubstitutionTexture(it->first, it->second);
    }

    if (m_callbackUniqueId != 0) {
        ui.UnregisterCallbackId(m_callbackUniqueId);
    }

    if (m_pItemRenderer != nullptr) delete m_pItemRenderer;
}

void UIScene::destroyMovie() {
    /* Destroy the Iggy player. */
    IggyPlayerDestroy(swf);
    swf = nullptr;
    m_hasSetSafeZoneMethod = false;

    // Clear out the controls collection (doesn't delete the controls, and they
    // get re-setup later)
    m_controls.clear();

    // Clear out all the fast names for the current movie
    m_fastNames.clear();
}

void UIScene::reloadMovie(bool force) {
    if (!force &&
        (stealsFocus() &&
         (getSceneType() != eUIScene_FullscreenProgress && !bHasFocus)))
        return;

    m_bIsReloading = true;
    if (swf) {
        /* Destroy the Iggy player. */
        IggyPlayerDestroy(swf);

        // Clear out the controls collection (doesn't delete the controls, and
        // they get re-setup later)
        m_controls.clear();
        m_hasSetSafeZoneMethod = false;

        // Clear out all the fast names for the current movie
        m_fastNames.clear();
    }

    // Reload everything
    initialiseMovie();

    handlePreReload();

    // Reload controls
    for (auto it = m_controls.begin(); it != m_controls.end(); ++it) {
        (*it)->ReInit();
    }

    updateComponents();
    handleReload();

    IggyDataValue result;
    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = m_iFocusControl;

    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetFocus, 1, value);

    m_needsCacheRendered = true;
    m_bIsReloading = false;
}

bool UIScene::needsReloaded() { return !swf && (!stealsFocus() || bHasFocus); }

bool UIScene::hasMovie() { return swf != nullptr; }

F64 UIScene::getSafeZoneHalfHeight() {
    float height = ui.getScreenHeight();

    float safeHeight = 0.0f;

    if (!PlatformRenderer.IsHiDef() && PlatformRenderer.IsWidescreen()) {
        // 90% safezone
        safeHeight = height * (0.15f / 2);
    } else {
        // 90% safezone
        safeHeight = height * (0.1f / 2);
    }
    return safeHeight;
}

F64 UIScene::getSafeZoneHalfWidth() {
    float width = ui.getScreenWidth();

    float safeWidth = 0.0f;
    if (!PlatformRenderer.IsHiDef() && PlatformRenderer.IsWidescreen()) {
        // 85% safezone
        safeWidth = width * (0.15f / 2);
    } else {
        // 90% safezone
        safeWidth = width * (0.1f / 2);
    }
    return safeWidth;
}

void UIScene::updateSafeZone() {
    // Distance from edge
    F64 safeTop = 0.0;
    F64 safeBottom = 0.0;
    F64 safeLeft = 0.0;
    F64 safeRight = 0.0;

    switch (m_parentLayer->getViewport()) {
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
            safeTop = getSafeZoneHalfHeight();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
            safeBottom = getSafeZoneHalfHeight();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
            safeLeft = getSafeZoneHalfWidth();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            safeRight = getSafeZoneHalfWidth();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
            safeTop = getSafeZoneHalfHeight();
            safeLeft = getSafeZoneHalfWidth();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
            safeTop = getSafeZoneHalfHeight();
            safeRight = getSafeZoneHalfWidth();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
            safeBottom = getSafeZoneHalfHeight();
            safeLeft = getSafeZoneHalfWidth();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
            safeBottom = getSafeZoneHalfHeight();
            safeRight = getSafeZoneHalfWidth();
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        default:
            safeTop = getSafeZoneHalfHeight();
            safeBottom = getSafeZoneHalfHeight();
            safeLeft = getSafeZoneHalfWidth();
            safeRight = getSafeZoneHalfWidth();
            break;
    }
    setSafeZone(safeTop, safeBottom, safeLeft, safeRight);
}

void UIScene::setSafeZone(S32 safeTop, S32 safeBottom, S32 safeLeft,
                          S32 safeRight) {
    if (!m_hasSetSafeZoneMethod) return;

    IggyDataValue result;
    IggyDataValue value[4];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = safeTop;
    value[1].type = IGGY_DATATYPE_number;
    value[1].number = safeBottom;
    value[2].type = IGGY_DATATYPE_number;
    value[2].number = safeLeft;
    value[3].type = IGGY_DATATYPE_number;
    value[3].number = safeRight;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetSafeZone, 4, value);
}

void UIScene::initialiseMovie() {
    loadMovie();
    app.DebugPrintf("UIScene::initialiseMovie AFTER loadMovie CALL\n");
    mapElementsAndNames();
    app.DebugPrintf(
        "UIScene::initialiseMovie AFTER mapElementsAndNames CALL\n");

    updateSafeZone();
    app.DebugPrintf("UIScene::initialiseMovie AFTER updateSafeZone CALL\n");

    m_bUpdateOpacity = true;
}

bool UIScene::mapElementsAndNames() {
    m_rootPath = IggyPlayerRootPath(swf);

    m_funcRemoveObject = registerFastName("RemoveObject");
    m_funcSlideLeft = registerFastName("SlideLeft");
    m_funcSlideRight = registerFastName("SlideRight");
    m_funcSetSafeZone = registerFastName("SetSafeZone");
    m_funcSetAlpha = registerFastName("SetAlpha");
    m_funcSetFocus = registerFastName("SetFocus");
    m_funcHorizontalResizeCheck = registerFastName("DoHorizontalResizeCheck");

    IggyDatatype safeZoneType = IGGY_DATATYPE__invalid_request;
    IggyResult safeZoneResult = IggyValueGetTypeRS(
        m_rootPath, m_funcSetSafeZone, nullptr, &safeZoneType);
    m_hasSetSafeZoneMethod = safeZoneResult == IGGY_RESULT_SUCCESS &&
                             safeZoneType == IGGY_DATATYPE_function;
    return true;
}

extern std::mutex s_loadSkinCS;
void UIScene::loadMovie() {
    UIController::ms_reloadSkinCS.lock();  // MGH - added to prevent crash
                                           // loading Iggy movies while the
                                           // skins were being reloaded
    std::string moviePath = getMoviePath();

#if defined(_WINDOWS64)
    if (ui.getScreenHeight() == 720) {
        moviePath.append("720.swf");
        m_loadedResolution = eSceneResolution_720;
    } else if (ui.getScreenHeight() == 480) {
        moviePath.append("480.swf");
        m_loadedResolution = eSceneResolution_480;
    } else if (ui.getScreenHeight() < 720) {
        moviePath.append("Vita.swf");
        m_loadedResolution = eSceneResolution_Vita;
    } else {
        moviePath.append("1080.swf");
        m_loadedResolution = eSceneResolution_1080;
    }
#else
    moviePath.append("1080.swf");
    m_loadedResolution = eSceneResolution_1080;
#endif

    if (!app.hasArchiveFile(moviePath)) {
        app.DebugPrintf(
            "WARNING: Could not find iggy movie %s, falling back on 720\n",
            moviePath.c_str());

        moviePath = getMoviePath();
        moviePath.append("720.swf");
        m_loadedResolution = eSceneResolution_720;

        if (!app.hasArchiveFile(moviePath)) {
            app.DebugPrintf("ERROR: Could not find any iggy movie for %s!\n",
                            moviePath.c_str());
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            app.FatalLoadError();
        }
    }

    std::vector<uint8_t> baFile = ui.getMovieData(moviePath.c_str());
    int64_t beforeLoad = ui.iggyAllocCount;
    swf = IggyPlayerCreateFromMemory(baFile.data(), baFile.size(), nullptr);
    int64_t afterLoad = ui.iggyAllocCount;
    IggyPlayerInitializeAndTickRS(swf);
    int64_t afterTick = ui.iggyAllocCount;

    if (!swf) {
        app.DebugPrintf("ERROR: Failed to load iggy scene!\n");
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
        app.FatalLoadError();
    }
    app.DebugPrintf(app.USER_SR, "Loaded iggy movie %s\n", moviePath.c_str());
    IggyProperties* properties = IggyPlayerProperties(swf);
    m_movieHeight = properties->movie_height_in_pixels;
    m_movieWidth = properties->movie_width_in_pixels;

    m_renderWidth = m_movieWidth;
    m_renderHeight = m_movieHeight;

    S32 width, height;
    m_parentLayer->getRenderDimensions(width, height);
    IggyPlayerSetDisplaySize(swf, width, height);

    IggyPlayerSetUserdata(swf, this);

    // #ifdef _DEBUG
    UIController::ms_reloadSkinCS.unlock();
}

void UIScene::getDebugMemoryUseRecursive(const std::string& moviePath,
                                         IggyMemoryUseInfo& memoryInfo) {
    rrbool res;
    IggyMemoryUseInfo internalMemoryInfo;
    int internalIteration = 0;
    while ((res = IggyDebugGetMemoryUseInfo(swf, 0, memoryInfo.subcategory,
                                            memoryInfo.subcategory_stringlen,
                                            internalIteration,
                                            &internalMemoryInfo))) {
        app.DebugPrintf(
            app.USER_SR, "%s - %.*s static: %d ( %d ) dynamic: %d ( %d )\n",
            moviePath.c_str(), internalMemoryInfo.subcategory_stringlen,
            internalMemoryInfo.subcategory,
            internalMemoryInfo.static_allocation_bytes,
            internalMemoryInfo.static_allocation_count,
            internalMemoryInfo.dynamic_allocation_bytes,
            internalMemoryInfo.dynamic_allocation_count);
        ++internalIteration;
        if (internalMemoryInfo.subcategory_stringlen >
            memoryInfo.subcategory_stringlen)
            getDebugMemoryUseRecursive(moviePath, internalMemoryInfo);
    }
}

void UIScene::PrintTotalMemoryUsage(int64_t& totalStatic,
                                    int64_t& totalDynamic) {
    if (!swf) return;

    IggyMemoryUseInfo memoryInfo;
    rrbool res;
    int iteration = 0;
    int64_t sceneStatic = 0;
    int64_t sceneDynamic = 0;
    while ((res = IggyDebugGetMemoryUseInfo(swf, 0, "", 0, iteration,
                                            &memoryInfo))) {
        sceneStatic += memoryInfo.static_allocation_bytes;
        sceneDynamic += memoryInfo.dynamic_allocation_bytes;
        totalStatic += memoryInfo.static_allocation_bytes;
        totalDynamic += memoryInfo.dynamic_allocation_bytes;
        ++iteration;
    }

    app.DebugPrintf(
        app.USER_SR,
        "    \\- Scene static: %d , Scene dynamic: %d , Total: %d - %s\n",
        sceneStatic, sceneDynamic, sceneStatic + sceneDynamic,
        getMoviePath().c_str());
}

void UIScene::tick() {
    if (m_bIsReloading) return;
    if (m_hasTickedOnce) m_bCanHandleInput = true;
    while (IggyPlayerReadyToTick(swf)) {
        tickTimers();
        for (auto it = m_controls.begin(); it != m_controls.end(); ++it) {
            (*it)->tick();
        }
        IggyPlayerTickRS(swf);
        m_hasTickedOnce = true;
    }
}

UIControl* UIScene::GetMainPanel() { return nullptr; }

void UIScene::addTimer(int id, int ms) {
    int currentTime = System::currentTimeMillis();

    TimerInfo info;
    info.running = true;
    info.duration = ms;
    info.targetTime = currentTime + ms;
    m_timers[id] = info;
}

void UIScene::killTimer(int id) {
    auto it = m_timers.find(id);
    if (it != m_timers.end()) {
        it->second.running = false;
    }
}

void UIScene::tickTimers() {
    int currentTime = System::currentTimeMillis();
    for (auto it = m_timers.begin(); it != m_timers.end();) {
        if (!it->second.running) {
            it = m_timers.erase(it);
        } else {
            if (currentTime > it->second.targetTime) {
                handleTimerComplete(it->first);

                // Auto-restart
                it->second.targetTime = it->second.duration + currentTime;
            }
            ++it;
        }
    }
}

IggyName UIScene::registerFastName(const std::string& name) {
    IggyName var;
    auto it = m_fastNames.find(name);
    if (it != m_fastNames.end()) {
        var = it->second;
    } else {
        // 4jcraft: shiggy has no IggyPlayerCreateFastNameUTF8 unfortunately
        var = IggyPlayerCreateFastName(getMovie(),
                                       string_to_u16string(name).c_str(), -1);

        m_fastNames[name] = var;
    }
    return var;
}

void UIScene::removeControl(UIControl_Base* control, bool centreScene) {
    IggyDataValue result;
    IggyDataValue value[2];

    std::string name = control->getControlName();
    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>((char*)name.c_str());
    stringVal.length = name.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = centreScene;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcRemoveObject, 2, value);
}

void UIScene::slideLeft() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSlideLeft, 0, nullptr);
}

void UIScene::slideRight() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSlideRight, 0, nullptr);
}

void UIScene::doHorizontalResizeCheck() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(
        getMovie(), &result, IggyPlayerRootPath(getMovie()),
        m_funcHorizontalResizeCheck, 0, nullptr);
}

void UIScene::render(S32 width, S32 height,
                     IPlatformRenderer::eViewportType viewport) {
    if (m_bIsReloading) return;
    if (!m_hasTickedOnce || !swf) return;
    ui.setupRenderPosition(viewport);
    IggyPlayerSetDisplaySize(swf, width, height);
    IggyPlayerDraw(swf);
}

void UIScene::setOpacity(float percent) {
    if (percent != m_lastOpacity || (m_bUpdateOpacity && getMovie())) {
        m_lastOpacity = percent;

        // 4J-TomK once a scene has been freshly loaded or re-loaded we force
        // update opacity via initialiseMovie
        if (m_bUpdateOpacity) m_bUpdateOpacity = false;

        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_number;
        value[0].number = percent;

        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcSetAlpha, 1, value);
    }
}

void UIScene::setVisible(bool visible) { m_bVisible = visible; }

void UIScene::customDraw(IggyCustomDrawCallbackRegion* region) {
    app.DebugPrintf("Handling custom draw for scene with no override!\n");
}

void UIScene::customDrawSlotControl(IggyCustomDrawCallbackRegion* region,
                                    int iPad,
                                    std::shared_ptr<ItemInstance> item,
                                    float fAlpha, bool isFoil,
                                    bool bDecorations) {
    if (item != nullptr) {
        if (m_cacheSlotRenders) {
            if ((m_cachedSlotDraw.size() + 1) == m_expectedCachedSlotCount) {
                // Make sure that pMinecraft->player is the correct player so
                // that player specific rendering
                //  eg clock and compass, are rendered correctly
                Minecraft* pMinecraft = Minecraft::GetInstance();
                std::shared_ptr<MultiplayerLocalPlayer> oldPlayer =
                    pMinecraft->player;
                if (iPad >= 0 && iPad < XUSER_MAX_COUNT)
                    pMinecraft->player = pMinecraft->localplayers[iPad];

                // Setup GDraw, normal game render states and matrices
                // CustomDrawData *customDrawRegion =
                // ui.setupCustomDraw(this,region);
                CustomDrawData* customDrawRegion =
                    ui.calculateCustomDraw(region);
                ui.beginIggyCustomDraw4J(region, customDrawRegion);
                ui.setupCustomDrawGameState();

                int list = m_parentLayer->m_parentGroup->getCommandBufferList();

                bool useCommandBuffers = false;

                if (!useCommandBuffers || m_needsCacheRendered) {
                    if (useCommandBuffers)
                        PlatformRenderer.CBuffStart(list, true);
                    ui.setupCustomDrawMatrices(this, customDrawRegion);
                    _customDrawSlotControl(customDrawRegion, iPad, item, fAlpha,
                                           isFoil, bDecorations,
                                           useCommandBuffers);
                    delete customDrawRegion;

                    // Draw all the cached slots
                    for (auto it = m_cachedSlotDraw.begin();
                         it != m_cachedSlotDraw.end(); ++it) {
                        CachedSlotDrawData* drawData = *it;
                        ui.setupCustomDrawMatrices(this,
                                                   drawData->customDrawRegion);
                        _customDrawSlotControl(
                            drawData->customDrawRegion, iPad, drawData->item,
                            drawData->fAlpha, drawData->isFoil,
                            drawData->bDecorations, useCommandBuffers);
                        delete drawData->customDrawRegion;
                        delete drawData;
                    }

                    if (useCommandBuffers) PlatformRenderer.CBuffEnd();
                }
                m_cachedSlotDraw.clear();

                if (useCommandBuffers) (void)PlatformRenderer.CBuffCall(list);

                // Finish GDraw and anything else that needs to be finalised
                ui.endCustomDraw(region);

                pMinecraft->player = oldPlayer;
            } else {
                CachedSlotDrawData* drawData = new CachedSlotDrawData();
                drawData->item = item;
                drawData->fAlpha = fAlpha;
                drawData->isFoil = isFoil;
                drawData->bDecorations = bDecorations;
                drawData->customDrawRegion = ui.calculateCustomDraw(region);

                m_cachedSlotDraw.push_back(drawData);
            }
        } else {
            // Setup GDraw, normal game render states and matrices
            CustomDrawData* customDrawRegion = ui.setupCustomDraw(this, region);

            Minecraft* pMinecraft = Minecraft::GetInstance();

            // Make sure that pMinecraft->player is the correct player so that
            // player specific rendering
            //  eg clock and compass, are rendered correctly
            std::shared_ptr<MultiplayerLocalPlayer> oldPlayer =
                pMinecraft->player;
            if (iPad >= 0 && iPad < XUSER_MAX_COUNT)
                pMinecraft->player = pMinecraft->localplayers[iPad];

            _customDrawSlotControl(customDrawRegion, iPad, item, fAlpha, isFoil,
                                   bDecorations, false);
            delete customDrawRegion;
            pMinecraft->player = oldPlayer;

            // Finish GDraw and anything else that needs to be finalised
            ui.endCustomDraw(region);
        }
    }
}

void UIScene::_customDrawSlotControl(CustomDrawData* region, int iPad,
                                     std::shared_ptr<ItemInstance> item,
                                     float fAlpha, bool isFoil,
                                     bool bDecorations,
                                     bool usingCommandBuffer) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    float bwidth, bheight;
    bwidth = region->x1 - region->x0;
    bheight = region->y1 - region->y0;

    float x = region->x0;
    float y = region->y0;

    // Base scale on height of this control, compared to height of what the item
    // renderer normally renders (16 pixels high). Potentially we might want
    // separate x & y scales here

    float scaleX = bwidth / 16.0f;
    float scaleY = bheight / 16.0f;

    // 4jcraft: make sure we cull the back to not make transparent blocks (like
    // leaves) look weird
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // 4jcraft: needed for transparency in the item renders (like in the
    // crafting menu)
    if (fAlpha < 1) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glEnable(GL_RESCALE_NORMAL);
    glPushMatrix();
    Lighting::turnOn();
    glRotatef(120, 1, 0, 0);
    glPopMatrix();

    float pop = item->popTime;
    if (pop > 0) {
        glPushMatrix();
        float squeeze = 1 + pop / (float)Inventory::POP_TIME_DURATION;
        float sx = x;
        float sy = y;
        float sxoffs = 8 * scaleX;
        float syoffs = 12 * scaleY;
        glTranslatef((float)(sx + sxoffs), (float)(sy + syoffs), 0);
        glScalef(1 / squeeze, (squeeze + 1) / 2, 1);
        glTranslatef((float)-(sx + sxoffs), (float)-(sy + syoffs), 0);
    }

    if (m_pItemRenderer == nullptr) m_pItemRenderer = new ItemRenderer();
    m_pItemRenderer->renderAndDecorateItem(
        pMinecraft->font, pMinecraft->textures, item, x, y, scaleX, scaleY,
        fAlpha, isFoil, false, !usingCommandBuffer);

    if (pop > 0) {
        glPopMatrix();
    }

    if (bDecorations) {
        if ((scaleX != 1.0f) || (scaleY != 1.0f)) {
            glPushMatrix();
            glScalef(scaleX, scaleY, 1.0f);
            int iX = (int)(0.5f + ((float)x) / scaleX);
            int iY = (int)(0.5f + ((float)y) / scaleY);

            m_pItemRenderer->renderGuiItemDecorations(
                pMinecraft->font, pMinecraft->textures, item, iX, iY, fAlpha);
            glPopMatrix();
        } else {
            m_pItemRenderer->renderGuiItemDecorations(
                pMinecraft->font, pMinecraft->textures, item, (int)x, (int)y,
                fAlpha);
        }
    }

    Lighting::turnOff();
    glDisable(GL_RESCALE_NORMAL);
    glDisable(GL_CULL_FACE);
    if (fAlpha < 1) {
        glDisable(GL_BLEND);
    }
}

// 4J Stu - Not threadsafe
// void UIScene::navigateForward(int iPad, EUIScene scene, void *initData)
//{
//	if(m_parentLayer == nullptr)
//	{
//		app.DebugPrintf("A scene is trying to navigate forwards, but
// it's parent layer is nullptr!\n"); #ifndef _CONTENT_PACKAGE
//		assert(0);
// #endif
//	}
//	else
//	{
//		m_parentLayer->NavigateToScene(iPad,scene,initData);
//	}
//}

void UIScene::navigateBack() {
    // CD - Added for audio
    ui.PlayUISFX(eSFX_Back);

    ui.NavigateBack(m_iPad);

    if (m_parentLayer == nullptr) {
    } else {
        //		m_parentLayer->removeScene(this);
    }
}

void UIScene::gainFocus() {
    if (!bHasFocus && stealsFocus()) {
        // 4J Stu - Don't do this
        /*
        IggyEvent event;
        IggyMakeEventFocusGained( &event , 0);

        IggyEventResult result;
        IggyPlayerDispatchEventRS( getMovie() , &event , &result );

        app.DebugPrintf("Sent gain focus event to scene\n");
        */
        bHasFocus = true;
        if (needsReloaded()) {
            reloadMovie();
        }

        updateTooltips();
        updateComponents();

        if (!m_bFocussedOnce) {
            IggyDataValue result;
            IggyDataValue value[1];

            value[0].type = IGGY_DATATYPE_number;
            value[0].number = -1;

            IggyResult out = IggyPlayerCallMethodRS(
                getMovie(), &result, IggyPlayerRootPath(getMovie()),
                m_funcSetFocus, 1, value);
        }

        handleGainFocus(m_bFocussedOnce);
        if (bHasFocus) m_bFocussedOnce = true;
    } else if (bHasFocus && stealsFocus()) {
        updateTooltips();
    }
}

void UIScene::loseFocus() {
    if (bHasFocus) {
        // 4J Stu - Don't do this
        /*
        IggyEvent event;
        IggyMakeEventFocusLost( &event );
        IggyEventResult result;
        IggyPlayerDispatchEventRS ( getMovie() , &event , &result );
        */

        app.DebugPrintf("Sent lose focus event to scene\n");
        bHasFocus = false;
        handleLoseFocus();
    }
}

void UIScene::handleGainFocus(bool navBack) {}

void UIScene::updateTooltips() {
    if (!ui.IsReloadingSkin()) ui.SetTooltips(m_iPad, -1);
}

void UIScene::sendInputToMovie(int key, bool repeat, bool pressed,
                               bool released) {
    if (!swf) return;

    int iggyKeyCode = convertGameActionToIggyKeycode(key);

    if (iggyKeyCode < 0) {
        app.DebugPrintf(
            "UI WARNING: Ignoring input as game action does not translate to "
            "an Iggy keycode\n");
        return;
    }
    IggyEvent keyEvent;
    // 4J Stu - Keyloc is always standard as we don't care about shift/alt
    IggyMakeEventKey(&keyEvent, pressed ? IGGY_KEYEVENT_Down : IGGY_KEYEVENT_Up,
                     (IggyKeycode)iggyKeyCode, IGGY_KEYLOC_Standard);

    IggyEventResult result;
    IggyPlayerDispatchEventRS(swf, &keyEvent, &result);
}

int UIScene::convertGameActionToIggyKeycode(int action) {
    // TODO: This action to key mapping should probably use the control mapping
    int keycode = -1;
    switch (action) {
        case ACTION_MENU_A:
            keycode = IGGY_KEYCODE_ENTER;
            break;
        case ACTION_MENU_B:
            keycode = IGGY_KEYCODE_ESCAPE;
            break;
        case ACTION_MENU_X:
            keycode = IGGY_KEYCODE_F1;
            break;
        case ACTION_MENU_Y:
            keycode = IGGY_KEYCODE_F2;
            break;
        case ACTION_MENU_OK:
            keycode = IGGY_KEYCODE_ENTER;
            break;
        case ACTION_MENU_CANCEL:
            keycode = IGGY_KEYCODE_ESCAPE;
            break;
        case ACTION_MENU_UP:
            keycode = IGGY_KEYCODE_UP;
            break;
        case ACTION_MENU_DOWN:
            keycode = IGGY_KEYCODE_DOWN;
            break;
        case ACTION_MENU_RIGHT:
            keycode = IGGY_KEYCODE_RIGHT;
            break;
        case ACTION_MENU_LEFT:
            keycode = IGGY_KEYCODE_LEFT;
            break;
        case ACTION_MENU_PAGEUP:
            keycode = IGGY_KEYCODE_PAGE_UP;
            break;
        case ACTION_MENU_PAGEDOWN: {
            keycode = IGGY_KEYCODE_PAGE_DOWN;
        } break;
        case ACTION_MENU_RIGHT_SCROLL:
            keycode = IGGY_KEYCODE_F3;
            break;
        case ACTION_MENU_LEFT_SCROLL:
            keycode = IGGY_KEYCODE_F4;
            break;
        case ACTION_MENU_STICK_PRESS:
            break;
        case ACTION_MENU_OTHER_STICK_PRESS:
            keycode = IGGY_KEYCODE_F5;
            break;
        case ACTION_MENU_OTHER_STICK_UP:
            keycode = IGGY_KEYCODE_F11;
            break;
        case ACTION_MENU_OTHER_STICK_DOWN:
            keycode = IGGY_KEYCODE_F12;
            break;
        case ACTION_MENU_OTHER_STICK_LEFT:
            break;
        case ACTION_MENU_OTHER_STICK_RIGHT:
            break;
    };

    return keycode;
}

bool UIScene::allowRepeat(int key) {
    // 4J-PB - ignore repeats of action ABXY buttons
    // fix for PS3 213 - [MAIN MENU] Holding down buttons will continue to
    // activate every prompt.
    switch (key) {
        case ACTION_MENU_OK:
        case ACTION_MENU_CANCEL:
        case ACTION_MENU_A:
        case ACTION_MENU_B:
        case ACTION_MENU_X:
        case ACTION_MENU_Y:
            return false;
    }
    return true;
}

void UIScene::externalCallback(IggyExternalFunctionCallUTF16* call) {
    if (std::char_traits<char16_t>::compare(call->function_name.string,
                                            u"handlePress", 12) == 0) {
        if (call->num_arguments != 2) {
            app.DebugPrintf(
                "Callback for handlePress did not have the correct number of "
                "arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        if (call->arguments[0].type != IGGY_DATATYPE_number ||
            call->arguments[1].type != IGGY_DATATYPE_number) {
            app.DebugPrintf(
                "Arguments for handlePress were not of the correct type\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        handlePress(call->arguments[0].number, call->arguments[1].number);
    } else if (std::char_traits<char16_t>::compare(
                   call->function_name.string, u"handleFocusChange", 18) == 0) {
        if (call->num_arguments != 2) {
            app.DebugPrintf(
                "Callback for handleFocusChange did not have the correct "
                "number of arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        if (call->arguments[0].type != IGGY_DATATYPE_number ||
            call->arguments[1].type != IGGY_DATATYPE_number) {
            app.DebugPrintf(
                "Arguments for handleFocusChange were not of the correct "
                "type\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        _handleFocusChange(call->arguments[0].number,
                           call->arguments[1].number);
    } else if (std::char_traits<char16_t>::compare(
                   call->function_name.string, u"handleInitFocus", 16) == 0) {
        if (call->num_arguments != 2) {
            app.DebugPrintf(
                "Callback for handleInitFocus did not have the correct number "
                "of arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        if (call->arguments[0].type != IGGY_DATATYPE_number ||
            call->arguments[1].type != IGGY_DATATYPE_number) {
            app.DebugPrintf(
                "Arguments for handleInitFocus were not of the correct type\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        _handleInitFocus(call->arguments[0].number, call->arguments[1].number);
    } else if (std::char_traits<char16_t>::compare(call->function_name.string,
                                                   u"handleCheckboxToggled",
                                                   22) == 0) {
        if (call->num_arguments != 2) {
            app.DebugPrintf(
                "Callback for handleCheckboxToggled did not have the correct "
                "number of arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        if (call->arguments[0].type != IGGY_DATATYPE_number ||
            call->arguments[1].type != IGGY_DATATYPE_boolean) {
            app.DebugPrintf(
                "Arguments for handleCheckboxToggled were not of the correct "
                "type\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        handleCheckboxToggled(call->arguments[0].number,
                              call->arguments[1].boolval);
    } else if (std::char_traits<char16_t>::compare(
                   call->function_name.string, u"handleSliderMove", 17) == 0) {
        if (call->num_arguments != 2) {
            app.DebugPrintf(
                "Callback for handleSliderMove did not have the correct number "
                "of arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        if (call->arguments[0].type != IGGY_DATATYPE_number ||
            call->arguments[1].type != IGGY_DATATYPE_number) {
            app.DebugPrintf(
                "Arguments for handleSliderMove were not of the correct "
                "type\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        handleSliderMove(call->arguments[0].number, call->arguments[1].number);
    } else if (std::char_traits<char16_t>::compare(call->function_name.string,
                                                   u"handleAnimationEnd",
                                                   19) == 0) {
        if (call->num_arguments != 0) {
            app.DebugPrintf(
                "Callback for handleAnimationEnd did not have the correct "
                "number of arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        handleAnimationEnd();
    } else if (std::char_traits<char16_t>::compare(call->function_name.string,
                                                   u"handleSelectionChanged",
                                                   23) == 0) {
        if (call->num_arguments != 1) {
            app.DebugPrintf(
                "Callback for handleSelectionChanged did not have the correct "
                "number of arguments\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        if (call->arguments[0].type != IGGY_DATATYPE_number) {
            app.DebugPrintf(
                "Arguments for handleSelectionChanged were not of the correct "
                "type\n");
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
            return;
        }
        handleSelectionChanged(call->arguments[0].number);
    } else if (std::char_traits<char16_t>::compare(call->function_name.string,
                                                   u"handleRequestMoreData",
                                                   22) == 0) {
        if (call->num_arguments == 0) {
            handleRequestMoreData(0, false);
        } else {
            if (call->num_arguments != 2) {
                app.DebugPrintf(
                    "Callback for handleRequestMoreData did not have the "
                    "correct number of arguments\n");
#if !defined(_CONTENT_PACKAGE)
                assert(0);
#endif
                return;
            }
            if (call->arguments[0].type != IGGY_DATATYPE_number ||
                call->arguments[1].type != IGGY_DATATYPE_boolean) {
                app.DebugPrintf(
                    "Arguments for handleRequestMoreData were not of the "
                    "correct type\n");
#if !defined(_CONTENT_PACKAGE)
                assert(0);
#endif
                return;
            }
            handleRequestMoreData(call->arguments[0].number,
                                  call->arguments[1].boolval);
        }
    } else if (std::char_traits<char16_t>::compare(call->function_name.string,
                                                   u"handleTouchBoxRebuild",
                                                   22) == 0) {
        handleTouchBoxRebuild();
    } else {
        app.DebugPrintf("Unhandled callback: %s\n", call->function_name.string);
    }
}

void UIScene::registerSubstitutionTexture(const std::string& textureName,
                                          std::uint8_t* pbData,
                                          unsigned int dwLength,
                                          bool deleteData) {
    m_registeredTextures[textureName] = deleteData;
    ;
    ui.registerSubstitutionTexture(textureName, pbData, dwLength);
}

bool UIScene::hasRegisteredSubstitutionTexture(const std::string& textureName) {
    auto it = m_registeredTextures.find(textureName);

    return it != m_registeredTextures.end();
}

void UIScene::_handleFocusChange(F64 controlId, F64 childId) {
    m_iFocusControl = (int)controlId;
    m_iFocusChild = (int)childId;

    handleFocusChange(controlId, childId);
    ui.PlayUISFX(eSFX_Focus);
}

void UIScene::_handleInitFocus(F64 controlId, F64 childId) {
    m_iFocusControl = (int)controlId;
    m_iFocusChild = (int)childId;

    // handleInitFocus(controlId, childId);
    handleFocusChange(controlId, childId);
}

bool UIScene::controlHasFocus(int iControlId) {
    return m_iFocusControl == iControlId;
}

bool UIScene::controlHasFocus(UIControl_Base* control) {
    return controlHasFocus(control->getId());
}

int UIScene::getControlChildFocus() { return m_iFocusChild; }

int UIScene::getControlFocus() { return m_iFocusControl; }

void UIScene::setBackScene(UIScene* scene) { m_backScene = scene; }

UIScene* UIScene::getBackScene() { return m_backScene; }

void UIScene::HandleMessage(EUIMessage message, void* data) {}

std::size_t UIScene::GetCallbackUniqueId() {
    if (m_callbackUniqueId == 0) {
        m_callbackUniqueId = ui.RegisterForCallbackId(this);
    }
    return m_callbackUniqueId;
}

bool UIScene::isReadyToDelete() { return true; }

int UIScene::parseSlotId(const char16_t* s) {
    if (s == nullptr || std::char_traits<char16_t>::length(s) <= 5 ||
        std::char_traits<char16_t>::compare(s, u"slot_", 5) != 0) {
        return -1;
    }

    // keep consuming digits until we reach a non-digit. each digit scales the
    // existing id value by 10 plus the actual digit value. (this is called a
    // 'number' by the way)
    int i = 5;
    int id = 0;
    while (s[i] >= u'0' && s[i] <= u'9') {
        id = id * 10 + (s[i] - u'0');
        i++;
    }

    return id;
}
