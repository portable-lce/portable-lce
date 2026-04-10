#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif

class UILayer;

class UIScene_Intro : public UIScene {
private:
    bool m_bIgnoreNavigate;
    bool m_bAnimationEnded;

    IggyName m_funcSetIntroPlatform;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_NAME(m_funcSetIntroPlatform, "SetIntroPlatform")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIScene_Intro(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_Intro; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return bHasFocus; }

protected:
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

    virtual void handleAnimationEnd();
    virtual void handleGainFocus(bool navBack);

#if !defined(_ENABLEIGGY)
    virtual void tick();
#endif
};
