#pragma once

#include <string>

#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_Progress.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_Progress : public UIControl_Base {
private:
    IggyName m_setProgressFunc, m_showBarFunc;
    int m_min;
    int m_max;
    int m_current;
    float m_lastPercent;
    bool m_showingBar;

public:
    UIControl_Progress();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    void init(UIString label, int id, int min, int max, int current);
    virtual void ReInit();

    void setProgress(int current);
    void showBar(bool show);
};