#pragma once
#ifdef _DEBUG_MENUS_ENABLED
#include <string>

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UIScene.h"

class UILayer;

class UIScene_DebugSetCamera : public UIScene {
private:
    enum eControls {
        eControl_CamX,
        eControl_CamY,
        eControl_CamZ,
        eControl_YRot,
        eControl_Elevation,
        eControl_LockPlayer,
        eControl_Teleport,
    };

    typedef struct _FreezePlayerParam {
        int player;
        bool freeze;
    } FreezePlayerParam;

    // Local UI state collected from the form. Sent to the server as a
    // SetCameraLocation action when the user hits Teleport.
    struct CameraFormState {
        int player = 0;
        double m_camX = 0.0;
        double m_camY = 0.0;
        double m_camZ = 0.0;
        double m_yRot = 0.0;
        double m_elev = 0.0;
    };

    CameraFormState currentPosition;
    FreezePlayerParam* fpp;

    eControls m_keyboardCallbackControl;

public:
    UIScene_DebugSetCamera(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_DebugSetCamera; }

protected:
    UIControl_TextInput m_textInputX, m_textInputY, m_textInputZ,
        m_textInputYRot, m_textInputElevation;
    UIControl_CheckBox m_checkboxLockPlayer;
    UIControl_Button m_buttonTeleport;
    UIControl_Label m_labelTitle, m_labelCamX, m_labelCamY, m_labelCamZ,
        m_labelYRotElev;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_textInputX, "CamX")
    UI_MAP_ELEMENT(m_textInputY, "CamY")
    UI_MAP_ELEMENT(m_textInputZ, "CamZ")
    UI_MAP_ELEMENT(m_textInputYRot, "YRot")
    UI_MAP_ELEMENT(m_textInputElevation, "Elevation")
    UI_MAP_ELEMENT(m_checkboxLockPlayer, "LockPlayer")
    UI_MAP_ELEMENT(m_buttonTeleport, "Teleport")

    UI_MAP_ELEMENT(m_labelTitle, "LabelTitle")
    UI_MAP_ELEMENT(m_labelCamX, "LabelCamX")
    UI_MAP_ELEMENT(m_labelCamY, "LabelCamY")
    UI_MAP_ELEMENT(m_labelCamZ, "LabelCamZ")
    UI_MAP_ELEMENT(m_labelYRotElev, "LabelYRotElev")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
    virtual void handleCheckboxToggled(F64 controlId, bool selected);

private:
    int handleKeyboardComplete(bool bRes);
};
#endif
