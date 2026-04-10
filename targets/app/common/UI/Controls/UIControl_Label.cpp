#include "UIControl_Label.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "util/StringHelpers.h"

UIControl_Label::UIControl_Label() {}

bool UIControl_Label::setupControl(UIScene* scene, IggyValuePath* parent,
                                   const std::string& controlName) {
    UIControl::setControlType(UIControl::eLabel);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // Label specific initialisers

    return success;
}

void UIControl_Label::init(UIString label) {
    m_label = label;

    const std::string labelString = label.getString();

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_string_UTF8;
    IggyStringUTF8 stringVal;

    stringVal.string = const_cast<char*>(labelString.c_str());
    stringVal.length = labelString.length();
    value[0].string8 = stringVal;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_initFunc, 1, value);
}

void UIControl_Label::ReInit() {
    UIControl_Base::ReInit();

    // 4J-JEV: This can't be reinitialised.
    if (m_reinitEnabled) {
        init(m_label);
    }
}
