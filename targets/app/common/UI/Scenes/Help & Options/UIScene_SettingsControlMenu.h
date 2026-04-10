#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

class UIScene_SettingsControlMenu : public UIScene {
private:
    enum EControls { eControl_SensitivityInGame, eControl_SensitivityInMenu };

    UIControl_Slider m_sliderSensitivityInGame,
        m_sliderSensitivityInMenu;  // Sliders
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_sliderSensitivityInGame, "SensitivityInGame")
    UI_MAP_ELEMENT(m_sliderSensitivityInMenu, "SensitivityInMenu")
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_SettingsControlMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_SettingsControlMenu();

    virtual EUIScene getSceneType() { return eUIScene_SettingsControlMenu; }

    virtual void updateTooltips();
    virtual void updateComponents();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

    virtual void handleSliderMove(F64 sliderId, F64 currentValue);
};