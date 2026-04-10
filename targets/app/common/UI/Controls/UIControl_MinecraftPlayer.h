#pragma once

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_MinecraftPlayer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl.h"

class UIControl_MinecraftPlayer : public UIControl {
private:
    float m_fScreenWidth, m_fScreenHeight;
    float m_fRawWidth, m_fRawHeight;

public:
    UIControl_MinecraftPlayer();

    void render(IggyCustomDrawCallbackRegion* region);
};