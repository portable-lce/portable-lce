#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"
#include "platform/renderer/renderer.h"

class UILayer;

#define CHAT_LINES_COUNT 10

class UIComponent_Chat : public UIScene {
private:
    bool m_bSplitscreen;

protected:
    UIControl_Label m_labelChatText[CHAT_LINES_COUNT];
    UIControl_Label m_labelJukebox;
    UIControl m_controlLabelBackground[CHAT_LINES_COUNT];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_labelChatText[0], "Label1")
    UI_MAP_ELEMENT(m_labelChatText[1], "Label2")
    UI_MAP_ELEMENT(m_labelChatText[2], "Label3")
    UI_MAP_ELEMENT(m_labelChatText[3], "Label4")
    UI_MAP_ELEMENT(m_labelChatText[4], "Label5")
    UI_MAP_ELEMENT(m_labelChatText[5], "Label6")
    UI_MAP_ELEMENT(m_labelChatText[6], "Label7")
    UI_MAP_ELEMENT(m_labelChatText[7], "Label8")
    UI_MAP_ELEMENT(m_labelChatText[8], "Label9")
    UI_MAP_ELEMENT(m_labelChatText[9], "Label10")

    UI_MAP_ELEMENT(m_controlLabelBackground[0], "Label1Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[1], "Label2Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[2], "Label3Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[3], "Label4Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[4], "Label5Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[5], "Label6Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[6], "Label7Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[7], "Label8Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[8], "Label9Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[9], "Label10Background")

    UI_MAP_ELEMENT(m_labelJukebox, "Jukebox")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIComponent_Chat(int iPad, void* initData, UILayer* parentLayer);

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    virtual EUIScene getSceneType() { return eUIComponent_Chat; }

    // Returns true if this scene handles input
    virtual bool stealsFocus() { return false; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return false; }

    // Returns true if lower scenes in this scenes layer, or in any layer below
    // this scenes layers should be hidden
    virtual bool hidesLowerScenes() { return false; }

    // RENDERING
    virtual void render(S32 width, S32 height,
                        IPlatformRenderer::eViewportType viewport);

protected:
    void handleTimerComplete(int id);
};