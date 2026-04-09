#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/iggy.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/linux/Stubs/iggy_stubs.h"
#endif
#include "app/linux/Iggy/include/rrCore.h"

class UILayer;

class UIComponent_Panorama : public UIScene {
private:
    bool m_bSplitscreen;
    bool m_bShowingDay;

protected:
    IggyName m_funcShowPanoramaDay;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_NAME(m_funcShowPanoramaDay, "ShowPanoramaDay");
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIComponent_Panorama(int iPad, void* initData, UILayer* parentLayer);

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    virtual EUIScene getSceneType() { return eUIComponent_Panorama; }

    // Returns true if this scene handles input
    virtual bool stealsFocus() { return false; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return false; }

    virtual void tick();

    // RENDERING
    virtual void render(S32 width, S32 height,
                        IPlatformRenderer::eViewportType viewport);

private:
    void setPanorama(bool isDay);
};
