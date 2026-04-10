#pragma once

#include <string>

#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_Cursor.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_Cursor : public UIControl_Base {
public:
    UIControl_Cursor();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);
};