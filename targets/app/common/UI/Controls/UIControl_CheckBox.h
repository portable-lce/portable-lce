#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_CheckBox : public UIControl_Base {
private:
    IggyName m_checkedProp, m_funcEnable, m_funcSetCheckBox;

    bool m_bChecked, m_bEnabled;

public:
    UIControl_CheckBox();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    void init(UIString label, int id, bool checked);

    bool IsChecked();
    bool IsEnabled();
    void SetEnable(bool enable);
    void setChecked(bool checked);
    void TouchSetCheckbox(bool checked);

    virtual void ReInit();
};
