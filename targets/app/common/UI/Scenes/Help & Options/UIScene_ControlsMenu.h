#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

class UIScene_ControlsMenu : public UIScene {
private:
    enum EControl {
        // Buttons must be first three controls here
        eControl_Button0,
        eControl_Button1,
        eControl_Button2,
        eControl_InvertLook,
        eControl_Southpaw,
    };

    enum EPadButtons {
        e_PadBack = 0,
        e_PadLT,
        e_PadLB,
        e_PadDPadLeft,
        e_PadDPadRight,
        e_PadDPadUp,
        e_PadDPadDown,
        e_PadLS_1,
        e_PadLS_2,
        e_PadStart,
        e_PadRT,
        e_PadRB,
        e_PadY,
        e_PadB,
        e_PadA,
        e_PadX,
        e_PadRS_1,
        e_PadRS_2,
        e_PadTouch,

        e_PadCOUNT,
    };

    int m_iSchemeTextA[3];
    int m_iCurrentNavigatedControlsLayout;
    bool m_bCreativeMode;
    bool m_bLayoutChanged;

    UIControl_Label m_labelCurrentLayout;
    UIControl_Label m_labelVersion;
    UIControl_Label m_labelsPad[e_PadCOUNT];
    UIControl m_controlLines[e_PadCOUNT];
    UIControl_Button m_buttonLayouts[3];
    UIControl_CheckBox m_checkboxInvert, m_checkboxSouthpaw;
    IggyName m_funcSetPlatform, m_funcSetControllerLayout;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)

    UI_MAP_ELEMENT(m_labelCurrentLayout, "CurrentLayout")

    UI_MAP_ELEMENT(m_buttonLayouts[0], "Button1")
    UI_MAP_ELEMENT(m_buttonLayouts[1], "Button2")
    UI_MAP_ELEMENT(m_buttonLayouts[2], "Button3")

    UI_MAP_ELEMENT(m_labelsPad[e_PadBack], "LabelBack")
    UI_MAP_ELEMENT(m_labelsPad[e_PadLT], "LabelLT")
    UI_MAP_ELEMENT(m_labelsPad[e_PadLB], "LabelLB")
    UI_MAP_ELEMENT(m_labelsPad[e_PadDPadLeft], "LabelDPadLeft")
    UI_MAP_ELEMENT(m_labelsPad[e_PadDPadRight], "LabelDPadRight")
    UI_MAP_ELEMENT(m_labelsPad[e_PadDPadUp], "LabelDPadUp")
    UI_MAP_ELEMENT(m_labelsPad[e_PadDPadDown], "LabelDPadDown")
    UI_MAP_ELEMENT(m_labelsPad[e_PadLS_1], "LabelLS_1")
    UI_MAP_ELEMENT(m_labelsPad[e_PadLS_2], "LabelLS_2")
    UI_MAP_ELEMENT(m_labelsPad[e_PadStart], "LabelStart")
    UI_MAP_ELEMENT(m_labelsPad[e_PadRT], "LabelRT")
    UI_MAP_ELEMENT(m_labelsPad[e_PadRB], "LabelRB")
    UI_MAP_ELEMENT(m_labelsPad[e_PadY], "LabelY")
    UI_MAP_ELEMENT(m_labelsPad[e_PadB], "LabelB")
    UI_MAP_ELEMENT(m_labelsPad[e_PadA], "LabelA")
    UI_MAP_ELEMENT(m_labelsPad[e_PadX], "LabelX")
    UI_MAP_ELEMENT(m_labelsPad[e_PadRS_1], "LabelRS_1")
    UI_MAP_ELEMENT(m_labelsPad[e_PadRS_2], "LabelRS_2")
    UI_MAP_ELEMENT(m_labelsPad[e_PadTouch], "LabelTouch")

    UI_MAP_ELEMENT(m_controlLines[e_PadBack], "LineBack")
    UI_MAP_ELEMENT(m_controlLines[e_PadLT], "LineLT")
    UI_MAP_ELEMENT(m_controlLines[e_PadLB], "LineLB")
    UI_MAP_ELEMENT(m_controlLines[e_PadDPadLeft], "LineDpadLeft")
    UI_MAP_ELEMENT(m_controlLines[e_PadDPadRight], "LineDpadRight")
    UI_MAP_ELEMENT(m_controlLines[e_PadDPadUp], "LineDpadUp")
    UI_MAP_ELEMENT(m_controlLines[e_PadDPadDown], "LineDpadDown")
    UI_MAP_ELEMENT(m_controlLines[e_PadLS_1], "LineL3")
    UI_MAP_ELEMENT(m_controlLines[e_PadLS_2], "LineLeftStick")
    UI_MAP_ELEMENT(m_controlLines[e_PadStart], "LineStart")
    UI_MAP_ELEMENT(m_controlLines[e_PadRT], "LineRT")
    UI_MAP_ELEMENT(m_controlLines[e_PadRB], "LineRB")
    UI_MAP_ELEMENT(m_controlLines[e_PadY], "LineY")
    UI_MAP_ELEMENT(m_controlLines[e_PadB], "LineB")
    UI_MAP_ELEMENT(m_controlLines[e_PadA], "LineA")
    UI_MAP_ELEMENT(m_controlLines[e_PadX], "LineX")
    UI_MAP_ELEMENT(m_controlLines[e_PadRS_1], "LineR3")
    UI_MAP_ELEMENT(m_controlLines[e_PadRS_2], "LineRightStick")
    UI_MAP_ELEMENT(m_controlLines[e_PadTouch], "LineTouch")

    UI_MAP_ELEMENT(m_checkboxInvert, "InvertLook")
    UI_MAP_ELEMENT(m_checkboxSouthpaw, "SouthPaw")

    UI_MAP_NAME(m_funcSetPlatform, "SetPlatform")
    UI_MAP_NAME(m_funcSetControllerLayout, "SetControllerLayout")
    UI_MAP_ELEMENT(m_labelVersion, "Version")
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_ControlsMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_ControlsMenu; }

    virtual void updateTooltips();
    virtual void tick();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

    virtual void handleCheckboxToggled(F64 controlId, bool selected);
    virtual void handlePress(F64 controlId, F64 childId);
    virtual void handleFocusChange(F64 controlId, F64 childId);

private:
    void PositionText(int iPad, int iTextID, unsigned char ucAction);
    void PositionTextDirect(int iPad, int iTextID, int iControlDetailsIndex,
                            bool bShow);
    void PositionAllText(int iPad);
};