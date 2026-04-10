#pragma once

#include <string>

#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_Label : public UIControl_Base {
private:
    bool m_reinitEnabled;

public:
    UIControl_Label();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    void init(UIString label);
    virtual void ReInit();

    void disableReinitialisation() { m_reinitEnabled = false; }
};