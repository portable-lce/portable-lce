#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_DynamicLabel.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"
#include "app/common/Iggy/include/rrCore.h"

class UIControl_DynamicLabel : public UIControl_Label {
private:
    IggyName m_funcAddText, m_funcTouchScroll, m_funcGetRealWidth,
        m_funcGetRealHeight;

public:
    UIControl_DynamicLabel();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    virtual void addText(const std::string& text, bool bLastEntry);

    virtual void ReInit();

    virtual void SetupTouch();

    virtual void TouchScroll(S32 iY, bool bActive);

    S32 GetRealWidth();
    S32 GetRealHeight();
};
