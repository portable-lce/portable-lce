#include "UIComponent_Logo.h"

#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "platform/renderer/renderer.h"

UIComponent_Logo::UIComponent_Logo(int iPad, void* initData,
                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
}

std::string UIComponent_Logo::getMoviePath() {
    switch (m_parentLayer->getViewport()) {
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
            return "ComponentLogoSplit";
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        default:
            return "ComponentLogo";
            break;
    }
}