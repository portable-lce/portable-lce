#include "UIComponent_Chat.h"

#include <memory>

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Gui.h"

UIComponent_Chat::UIComponent_Chat(int iPad, void* initData,
                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    for (unsigned int i = 0; i < CHAT_LINES_COUNT; ++i) {
        m_labelChatText[i].init("");
    }
    m_labelJukebox.init("");

    addTimer(0, 100);
}

std::string UIComponent_Chat::getMoviePath() {
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
            return "ComponentChatSplit";
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        default:
            m_bSplitscreen = false;
            return "ComponentChat";
            break;
    }
}

void UIComponent_Chat::handleTimerComplete(int id) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    bool anyVisible = false;
    if (pMinecraft->localplayers[m_iPad] != nullptr) {
        Gui* pGui = pMinecraft->gui;
        // uint32_t messagesToDisplay = std::min( CHAT_LINES_COUNT,
        // pGui->getMessagesCount(m_iPad) );
        for (unsigned int i = 0; i < CHAT_LINES_COUNT; ++i) {
            float opacity = pGui->getOpacity(m_iPad, i);
            if (opacity > 0) {
                m_controlLabelBackground[i].setOpacity(opacity);
                m_labelChatText[i].setOpacity(opacity);
                m_labelChatText[i].setLabel(pGui->getMessage(m_iPad, i));

                anyVisible = true;
            } else {
                m_controlLabelBackground[i].setOpacity(0);
                m_labelChatText[i].setOpacity(0);
                m_labelChatText[i].setLabel("");
            }
        }
        if (pGui->getJukeboxOpacity(m_iPad) > 0) anyVisible = true;
        m_labelJukebox.setOpacity(pGui->getJukeboxOpacity(m_iPad));
        m_labelJukebox.setLabel(pGui->getJukeboxMessage(m_iPad));
    } else {
        for (unsigned int i = 0; i < CHAT_LINES_COUNT; ++i) {
            m_controlLabelBackground[i].setOpacity(0);
            m_labelChatText[i].setOpacity(0);
            m_labelChatText[i].setLabel("");
        }
        m_labelJukebox.setOpacity(0);
    }

    setVisible(anyVisible);
}

void UIComponent_Chat::render(S32 width, S32 height,
                              IPlatformRenderer::eViewportType viewport) {
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
