#pragma once

#include <string>

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UIScene.h"

class UILayer;

class UIScene_ReinstallMenu : public UIScene {
private:
    enum EControls {
        eControl_Theme,
        eControl_Gamerpic1,
        eControl_Gamerpic2,
        eControl_Avatar1,
        eControl_Avatar2,
        eControl_Avatar3,
        eControl_COUNT,
    };
    UIControl_Button m_buttons[eControl_COUNT];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttons[eControl_Theme], "Button1")
    UI_MAP_ELEMENT(m_buttons[eControl_Gamerpic1], "Button2")
    UI_MAP_ELEMENT(m_buttons[eControl_Gamerpic2], "Button3")
    UI_MAP_ELEMENT(m_buttons[eControl_Avatar1], "Button4")
    UI_MAP_ELEMENT(m_buttons[eControl_Avatar2], "Button5")
    UI_MAP_ELEMENT(m_buttons[eControl_Avatar3], "Button6")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    // bool m_bNotInGame;
public:
    UIScene_ReinstallMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_ReinstallMenu; }

    virtual void updateTooltips();
    virtual void updateComponents();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
};
