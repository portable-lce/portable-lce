#include "UIControl_BitmapIcon.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "util/StringHelpers.h"

bool UIControl_BitmapIcon::setupControl(UIScene* scene, IggyValuePath* parent,
                                        const std::string& controlName) {
    UIControl::setControlType(UIControl::eBitmapIcon);
    bool success = UIControl::setupControl(scene, parent, controlName);

    // SlotList specific initialisers
    m_funcSetTextureName = registerFastName("SetTextureName");

    return success;
}

void UIControl_BitmapIcon::setTextureName(const std::string& iconName) {
    IggyDataValue result;
    IggyDataValue value[1];

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(iconName.c_str());
    stringVal.length = iconName.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcSetTextureName, 1, value);
}
