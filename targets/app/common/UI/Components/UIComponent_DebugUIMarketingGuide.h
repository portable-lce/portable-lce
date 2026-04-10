#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif

class UILayer;

class UIComponent_DebugUIMarketingGuide : public UIScene {
private:
    IggyName m_funcSetPlatform;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_NAME(m_funcSetPlatform, "SetPlatform")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIComponent_DebugUIMarketingGuide(int iPad, void* initData,
                                      UILayer* parentLayer);

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    virtual EUIScene getSceneType() {
        return eUIComponent_DebugUIMarketingGuide;
    }

    // Returns true if this scene handles input
    virtual bool stealsFocus() { return false; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return false; }

    // Returns true if lower scenes in this scenes layer, or in any layer below
    // this scenes layers should be hidden
    virtual bool hidesLowerScenes() { return false; }
};