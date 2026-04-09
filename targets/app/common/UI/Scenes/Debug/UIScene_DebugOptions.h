#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/Console_Debug_enum.h"

class UILayer;

class UIScene_DebugOptionsMenu : public UIScene {
private:
    static const char* m_DebugCheckboxTextA[eDebugSetting_Max + 1];

    int m_iTotalCheckboxElements;

public:
    UIScene_DebugOptionsMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_DebugOptions; }

protected:
    UIControl_CheckBox m_checkboxes[21];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_checkboxes[0], "checkbox1")
    UI_MAP_ELEMENT(m_checkboxes[1], "checkbox2")
    UI_MAP_ELEMENT(m_checkboxes[2], "checkbox3")
    UI_MAP_ELEMENT(m_checkboxes[3], "checkbox4")
    UI_MAP_ELEMENT(m_checkboxes[4], "checkbox5")
    UI_MAP_ELEMENT(m_checkboxes[5], "checkbox6")
    UI_MAP_ELEMENT(m_checkboxes[6], "checkbox7")
    UI_MAP_ELEMENT(m_checkboxes[7], "checkbox8")
    UI_MAP_ELEMENT(m_checkboxes[8], "checkbox9")
    UI_MAP_ELEMENT(m_checkboxes[9], "checkbox10")
    UI_MAP_ELEMENT(m_checkboxes[10], "checkbox11")
    UI_MAP_ELEMENT(m_checkboxes[11], "checkbox12")
    UI_MAP_ELEMENT(m_checkboxes[12], "checkbox13")
    UI_MAP_ELEMENT(m_checkboxes[13], "checkbox14")
    UI_MAP_ELEMENT(m_checkboxes[14], "checkbox15")
    UI_MAP_ELEMENT(m_checkboxes[15], "checkbox16")
    UI_MAP_ELEMENT(m_checkboxes[16], "checkbox17")
    UI_MAP_ELEMENT(m_checkboxes[17], "checkbox18")
    UI_MAP_ELEMENT(m_checkboxes[18], "checkbox19")
    UI_MAP_ELEMENT(m_checkboxes[19], "checkbox20")
    UI_MAP_ELEMENT(m_checkboxes[20], "checkbox21")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
};
