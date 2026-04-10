#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_Button : public UIControl_Base {
private:
    IggyName m_funcEnableButton;

public:
    UIControl_Button();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    void init(UIString label, int id);
    // void init(const std::string &label, int id) {
    // init(UIString::CONSTANT(label), id); }

    virtual void ReInit();

    void setEnable(bool enable);
};