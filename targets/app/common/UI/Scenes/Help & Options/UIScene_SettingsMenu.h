#pragma once

#include <string>

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UIScene.h"
#include "platform/storage/storage.h"

class UILayer;

#define BUTTON_ALL_OPTIONS 0
#define BUTTON_ALL_AUDIO 1
#define BUTTON_ALL_CONTROL 2
#define BUTTON_ALL_GRAPHICS 4
#define BUTTON_ALL_UI 5
#define BUTTON_ALL_RESETTODEFAULTS 6
#define BUTTONS_ALL_MAX BUTTON_ALL_RESETTODEFAULTS + 1

class UIScene_SettingsMenu : public UIScene {
private:
    UIControl_Button m_buttons[BUTTONS_ALL_MAX];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttons[BUTTON_ALL_OPTIONS], "Button1")
    UI_MAP_ELEMENT(m_buttons[BUTTON_ALL_AUDIO], "Button2")
    UI_MAP_ELEMENT(m_buttons[BUTTON_ALL_CONTROL], "Button3")
    UI_MAP_ELEMENT(m_buttons[BUTTON_ALL_GRAPHICS], "Button4")
    UI_MAP_ELEMENT(m_buttons[BUTTON_ALL_UI], "Button5")
    UI_MAP_ELEMENT(m_buttons[BUTTON_ALL_RESETTODEFAULTS], "Button6")
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_SettingsMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_SettingsMenu();

    virtual EUIScene getSceneType() { return eUIScene_SettingsMenu; }

    virtual void updateTooltips();
    virtual void updateComponents();
    virtual void handleReload();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);

    static int ResetDefaultsDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
};