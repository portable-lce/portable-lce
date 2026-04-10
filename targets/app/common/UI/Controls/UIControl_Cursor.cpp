#include "UIControl_Cursor.h"

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif

UIControl_Cursor::UIControl_Cursor() {}

bool UIControl_Cursor::setupControl(UIScene* scene, IggyValuePath* parent,
                                    const std::string& controlName) {
    UIControl::setControlType(UIControl::eCursor);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // Label specific initialisers

    return success;
}
