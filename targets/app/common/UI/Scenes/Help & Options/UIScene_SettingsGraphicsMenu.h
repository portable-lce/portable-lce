#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

class UIScene_SettingsGraphicsMenu : public UIScene {
private:
    enum EControls {
        eControl_Clouds,
        eControl_BedrockFog,
        eControl_CustomSkinAnim,
        eControl_Gamma,
        eControl_InterfaceOpacity
    };

    UIControl_CheckBox m_checkboxClouds, m_checkboxBedrockFog,
        m_checkboxCustomSkinAnim;                              // Checkboxes
    UIControl_Slider m_sliderGamma, m_sliderInterfaceOpacity;  // Sliders
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_checkboxClouds, "Clouds")
    UI_MAP_ELEMENT(m_checkboxBedrockFog, "BedrockFog")
    UI_MAP_ELEMENT(m_checkboxCustomSkinAnim, "CustomSkinAnim")
    UI_MAP_ELEMENT(m_sliderGamma, "Gamma")
    UI_MAP_ELEMENT(m_sliderInterfaceOpacity, "InterfaceOpacity")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    bool m_bNotInGame;

public:
    UIScene_SettingsGraphicsMenu(int iPad, void* initData,
                                 UILayer* parentLayer);
    virtual ~UIScene_SettingsGraphicsMenu();

    virtual EUIScene getSceneType() { return eUIScene_SettingsGraphicsMenu; }

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