#include "UIControl_DynamicLabel.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "util/StringHelpers.h"

UIControl_DynamicLabel::UIControl_DynamicLabel() {}

bool UIControl_DynamicLabel::setupControl(UIScene* scene, IggyValuePath* parent,
                                          const std::string& controlName) {
    UIControl::setControlType(UIControl::eDynamicLabel);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // Label specific initialisers
    m_funcAddText = registerFastName("AddText");
    m_funcTouchScroll = registerFastName("TouchScroll");
    m_funcGetRealWidth = registerFastName("GetRealWidth");
    m_funcGetRealHeight = registerFastName("GetRealHeight");

    return success;
}

void UIControl_DynamicLabel::addText(const std::string& text, bool bLastEntry) {
    IggyDataValue result;
    IggyDataValue value[2];

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(text.c_str());
    stringVal.length = text.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = bLastEntry;

    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcAddText, 2, value);
}

void UIControl_DynamicLabel::ReInit() { UIControl_Base::ReInit(); }

void UIControl_DynamicLabel::SetupTouch() {}

void UIControl_DynamicLabel::TouchScroll(S32 iY, bool bActive) {
    IggyDataValue result;
    IggyDataValue value[2];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iY;
    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = bActive;

    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcTouchScroll, 2, value);
}

S32 UIControl_DynamicLabel::GetRealWidth() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcGetRealWidth, 0, nullptr);

    S32 iRealWidth = m_width;
    if (result.type == IGGY_DATATYPE_number) {
        iRealWidth = (S32)result.number;
    }
    return iRealWidth;
}

S32 UIControl_DynamicLabel::GetRealHeight() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcGetRealHeight, 0, nullptr);

    S32 iRealHeight = m_height;
    if (result.type == IGGY_DATATYPE_number) {
        iRealHeight = (S32)result.number;
    }
    return iRealHeight;
}
