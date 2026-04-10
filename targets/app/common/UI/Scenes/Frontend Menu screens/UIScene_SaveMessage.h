#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

class UIScene_SaveMessage : public UIScene {
private:
    enum EControls {
        eControl_Confirm,
    };

    bool m_bIgnoreInput;

    UIControl_Button m_buttonConfirm;
    UIControl_Label m_labelDescription;
    IggyName m_funcAutoResize;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonConfirm, "Confirm")
    UI_MAP_ELEMENT(m_labelDescription, "Description")
    UI_MAP_NAME(m_funcAutoResize, "AutoResize")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIScene_SaveMessage(int iPad, void* initData, UILayer* parentLayer);
    ~UIScene_SaveMessage();

    virtual EUIScene getSceneType() { return eUIScene_SaveMessage; }
    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return bHasFocus; }
    virtual void updateTooltips();

protected:
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    virtual void handleTimerComplete(int id);

protected:
    void handlePress(F64 controlId, F64 childId);

    virtual long long getDefaultGtcButtons() { return 0; }
};
