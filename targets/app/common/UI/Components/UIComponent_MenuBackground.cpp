#include "UIComponent_MenuBackground.h"

#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/ConsoleUIController.h"

UIComponent_MenuBackground::UIComponent_MenuBackground(int iPad, void* initData,
                                                       UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    m_bSplitscreen = false;
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
}

std::string UIComponent_MenuBackground::getMoviePath() {
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
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        default:
            m_bSplitscreen = false;
            break;
    }

    // We use the fullscreen one even in splitscreen, just draw different parts
    // of it
    return "MenuBackground";
}

void UIComponent_MenuBackground::render(
    S32 width, S32 height, IPlatformRenderer::eViewportType viewport) {
    if (m_bSplitscreen) {
        S32 xPos = 0;
        S32 yPos = 0;
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
                yPos = (S32)(ui.getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
                xPos = (S32)(ui.getScreenWidth() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
                xPos = (S32)(ui.getScreenWidth() / 2);
                yPos = (S32)(ui.getScreenHeight() / 2);
                break;
            default:
                break;
        }
        ui.setupRenderPosition(xPos, yPos);

        S32 tileXStart = 0;
        S32 tileYStart = 0;
        S32 tileWidth = width;
        S32 tileHeight = height;

        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
                tileHeight = (S32)(ui.getScreenHeight());
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
                tileWidth = (S32)(ui.getScreenWidth());
                tileYStart = (S32)(m_movieHeight / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
                tileWidth = (S32)(ui.getScreenWidth());
                tileYStart = (S32)(m_movieHeight / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
                tileYStart = (S32)(m_movieHeight / 2);
                break;
            default:
                break;
        }

        IggyPlayerSetDisplaySize(getMovie(), m_movieWidth, m_movieHeight);

        IggyPlayerDrawTilesStart(getMovie());

        m_renderWidth = tileWidth;
        m_renderHeight = tileHeight;
        IggyPlayerDrawTile(getMovie(), tileXStart, tileYStart,
                           tileXStart + tileWidth, tileYStart + tileHeight, 0);
        IggyPlayerDrawTilesEnd(getMovie());
    } else {
        UIScene::render(width, height, viewport);
    }
}
