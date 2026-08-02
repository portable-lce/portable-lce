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
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            return "ComponentLogoSplit";
            break;
        case 0:
        default:
            return "ComponentLogo";
            break;
    }
}