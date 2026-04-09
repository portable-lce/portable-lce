#include "UIComponent_Panorama.h"

#include <stdint.h>

#include <mutex>

#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/iggy.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/linux/Stubs/iggy_stubs.h"
#endif
#include "app/linux/Iggy/include/rrCore.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/storage/LevelData.h"

UIComponent_Panorama::UIComponent_Panorama(int iPad, void* initData,
                                           UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_bShowingDay = true;

    while (!m_hasTickedOnce) tick();
}

std::string UIComponent_Panorama::getMoviePath() {
    switch (m_parentLayer->getViewport()) {
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
            m_bSplitscreen = true;
            return "PanoramaSplit";
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        default:
            m_bSplitscreen = false;
            return "Panorama";
            break;
    }
}

void UIComponent_Panorama::tick() {
    if (!hasMovie()) return;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    {
        std::lock_guard<std::recursive_mutex> lock(pMinecraft->m_setLevelCS);
        if (pMinecraft->level != nullptr) {
            int64_t i64TimeOfDay = 0;
            // are we in the Nether? - Leave the time as 0 if we are, so we show
            // daylight
            if (pMinecraft->level->dimension->id == 0) {
                i64TimeOfDay =
                    pMinecraft->level->getLevelData()->getGameTime() % 24000;
            }

            if (i64TimeOfDay > 14000) {
                setPanorama(false);
            } else {
                setPanorama(true);
            }
        } else {
            setPanorama(true);
        }
    }

    UIScene::tick();
}

void UIComponent_Panorama::render(S32 width, S32 height,
                                  IPlatformRenderer::eViewportType viewport) {
    bool specialViewport =
        (viewport == IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP) ||
        (viewport == IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM) ||
        (viewport == IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT) ||
        (viewport == IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT);
    if (m_bSplitscreen && specialViewport) {
        S32 xPos = 0;
        S32 yPos = 0;
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
                yPos = (S32)(ui.getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
                xPos = (S32)(ui.getScreenWidth() / 2);
                break;
            default:
                break;
        }
        ui.setupRenderPosition(xPos, yPos);

        if ((viewport == IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT) ||
            (viewport == IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT)) {
            // Need to render at full height, but only the left side of the
            // scene
            S32 tileXStart = 0;
            S32 tileYStart = 0;
            S32 tileWidth = width;
            S32 tileHeight = (S32)(ui.getScreenHeight());

            IggyPlayerSetDisplaySize(getMovie(), m_movieWidth, m_movieHeight);

            IggyPlayerDrawTilesStart(getMovie());

            m_renderWidth = tileWidth;
            m_renderHeight = tileHeight;
            IggyPlayerDrawTile(getMovie(), tileXStart, tileYStart,
                               tileXStart + tileWidth, tileYStart + tileHeight,
                               0);
            IggyPlayerDrawTilesEnd(getMovie());
        } else {
            // Need to render at full height, and full width. But compressed
            // into the viewport
            IggyPlayerSetDisplaySize(getMovie(), ui.getScreenWidth(),
                                     ui.getScreenHeight() / 2);
            IggyPlayerDraw(getMovie());
        }
    } else {
        UIScene::render(width, height, viewport);
    }
}

void UIComponent_Panorama::setPanorama(bool isDay) {
    if (isDay != m_bShowingDay) {
        m_bShowingDay = isDay;

        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_boolean;
        value[0].boolval = isDay;

        IggyResult out = IggyPlayerCallMethodRS(
            getMovie(), &result, IggyPlayerRootPath(getMovie()),
            m_funcShowPanoramaDay, 1, value);
    }
}
