#pragma once
#ifdef _DEBUG_MENUS_ENABLED
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "java/Class.h"

class UILayer;

class UIScene_DebugOverlay : public UIScene {
private:
    enum eControls {
        eControl_SetCamera,
        eControl_ResetTutorial,
        eControl_Schematic,
        eControl_Thunder,
        eControl_Rain,
        eControl_FOV,
        eControl_SetDay,
        eControl_SetNight,
        eControl_Time,
        eControl_Mobs,
        eControl_Enchantments,
        eControl_Items,
    };

    std::vector<int> m_itemIds;
    std::vector<eINSTANCEOF> m_mobFactories;
    std::vector<std::pair<int, int> > m_enchantmentIdAndLevels;

public:
    UIScene_DebugOverlay(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_DebugOverlay; }

protected:
    UIControl_ButtonList m_buttonListItems, m_buttonListMobs,
        m_buttonListEnchantments;
    UIControl_Slider m_sliderFov, m_sliderTime;
    UIControl_Button m_buttonRain, m_buttonThunder, m_buttonSchematic,
        m_buttonResetTutorial, m_buttonSetCamera, m_buttonSetDay,
        m_buttonSetNight;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonListItems, "itemsList")
    UI_MAP_ELEMENT(m_buttonListEnchantments, "enchantmentsList")
    UI_MAP_ELEMENT(m_buttonListMobs, "mobList")
    UI_MAP_ELEMENT(m_sliderFov, "fov")
    UI_MAP_ELEMENT(m_sliderTime, "time")
    UI_MAP_ELEMENT(m_buttonSetDay, "setDay")
    UI_MAP_ELEMENT(m_buttonSetNight, "setNight")
    UI_MAP_ELEMENT(m_buttonRain, "rain")
    UI_MAP_ELEMENT(m_buttonThunder, "thunder")
    UI_MAP_ELEMENT(m_buttonSchematic, "schematic")
    UI_MAP_ELEMENT(m_buttonResetTutorial, "resetTutorial")
    UI_MAP_ELEMENT(m_buttonSetCamera, "setCamera")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual std::string getMoviePath();

public:
    virtual void customDraw(IggyCustomDrawCallbackRegion* region);

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
    virtual void handleSliderMove(F64 sliderId, F64 currentValue);
};
#endif