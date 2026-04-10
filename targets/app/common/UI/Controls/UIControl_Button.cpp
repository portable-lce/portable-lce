#include "UIControl_Button.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "util/StringHelpers.h"

UIControl_Button::UIControl_Button() {}

bool UIControl_Button::setupControl(UIScene* scene, IggyValuePath* parent,
                                    const std::string& controlName) {
    UIControl::setControlType(UIControl::eButton);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // Button specific initialisers
    m_funcEnableButton = registerFastName("EnableButton");

    return success;
}

void UIControl_Button::init(UIString label, int id) {
    m_label = label;
    m_id = id;

    IggyDataValue result;
    IggyDataValue value[2];
    value[0].type = IGGY_DATATYPE_string_UTF8;
    IggyStringUTF8 stringVal;

    stringVal.string = const_cast<char*>(label.getString().c_str());
    stringVal.length = label.getString().length();
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = id;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_initFunc, 2, value);
}

void UIControl_Button::ReInit() {
    UIControl_Base::ReInit();

    init(m_label, m_id);
}

void UIControl_Button::setEnable(bool enable) {
    IggyDataValue result;
    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_boolean;
    value[0].boolval = enable;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcEnableButton, 1, value);
}
