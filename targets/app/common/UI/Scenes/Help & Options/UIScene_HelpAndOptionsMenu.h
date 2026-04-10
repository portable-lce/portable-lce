#pragma once

#include <string>

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UIScene.h"

class UILayer;

#define BUTTON_HAO_CHANGESKIN 0
#define BUTTON_HAO_HOWTOPLAY 1
#define BUTTON_HAO_CONTROLS 2
#define BUTTON_HAO_SETTINGS 3
#define BUTTON_HAO_CREDITS 4
#define BUTTON_HAO_REINSTALL 5
#define BUTTON_HAO_DEBUG 6
#define BUTTONS_HAO_MAX BUTTON_HAO_DEBUG + 1

class UIScene_HelpAndOptionsMenu : public UIScene {
private:
    UIControl_Button m_buttons[BUTTONS_HAO_MAX];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_CHANGESKIN], "Button1")
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_HOWTOPLAY], "Button2")
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_CONTROLS], "Button3")
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_SETTINGS], "Button4")
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_CREDITS], "Button5")
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_REINSTALL], "Button6")
    UI_MAP_ELEMENT(m_buttons[BUTTON_HAO_DEBUG], "Button7")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    bool m_bNotInGame;

public:
    UIScene_HelpAndOptionsMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_HelpAndOptionsMenu();

    virtual EUIScene getSceneType() { return eUIScene_HelpAndOptionsMenu; }

    virtual void updateTooltips();
    virtual void updateComponents();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    virtual void handleReload();

    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
};