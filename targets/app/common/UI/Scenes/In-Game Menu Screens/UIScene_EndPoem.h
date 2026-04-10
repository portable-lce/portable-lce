#pragma once

#include <string>
#include <vector>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

class UIScene_EndPoem : public UIScene {
private:
    std::string noNoiseString;
    std::string noiseString;
    std::vector<int> m_noiseLengths;
    bool m_bIgnoreInput;
    int m_requestedLabel;

    std::vector<std::string> m_paragraphs;

    IggyName m_funcSetNextLabel;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_NAME(m_funcSetNextLabel, "SetNextLabel")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIScene_EndPoem(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_EndPoem; }
    virtual void updateTooltips();

protected:
    virtual std::string getMoviePath();

public:
    virtual void tick();

    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    virtual void handleDestroy();

    virtual void handleRequestMoreData(F64 startIndex, bool up);

private:
    void updateNoise();
};