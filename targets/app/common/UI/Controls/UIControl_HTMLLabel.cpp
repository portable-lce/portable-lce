#include "UIControl_HTMLLabel.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

UIControl_HTMLLabel::UIControl_HTMLLabel() {}

bool UIControl_HTMLLabel::setupControl(UIScene* scene, IggyValuePath* parent,
                                       const std::string& controlName) {
    UIControl::setControlType(UIControl::eHTMLLabel);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // Label specific initialisers
    m_funcStartAutoScroll = registerFastName("StartAutoScroll");
    m_funcTouchScroll = registerFastName("TouchScroll");
    m_funcGetRealWidth = registerFastName("GetRealWidth");
    m_funcGetRealHeight = registerFastName("GetRealHeight");

    return success;
}

void UIControl_HTMLLabel::startAutoScroll() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcStartAutoScroll, 0, nullptr);
}

void UIControl_HTMLLabel::ReInit() {
    UIControl_Base::ReInit();
    // Don't set the label, HTML sizes will have changed. Let the scene update
    // us.
    init("");
}

// void UIControl_HTMLLabel::setLabel(const std::string& label) {
//     IggyDataValue result;
//     IggyDataValue value[1];
//     value[0].type = IGGY_DATATYPE_string_UTF8;
//     IggyStringUTF8 stringVal;

//     stringVal.string = const_cast<char*>((char*)label.c_str());
//     stringVal.length = label.length();
//     value[0].string8 = stringVal;

//     IggyResult out =
//         IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
//                                getIggyValuePath(), m_setLabelFunc, 1, value);
// }

void UIControl_HTMLLabel::SetupTouch() {}

void UIControl_HTMLLabel::TouchScroll(S32 iY, bool bActive) {
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

S32 UIControl_HTMLLabel::GetRealWidth() {
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

S32 UIControl_HTMLLabel::GetRealHeight() {
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