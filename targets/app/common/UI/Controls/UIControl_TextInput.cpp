#include "UIControl_TextInput.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "util/StringHelpers.h"

UIControl_TextInput::UIControl_TextInput() { m_bHasFocus = false; }

bool UIControl_TextInput::setupControl(UIScene* scene, IggyValuePath* parent,
                                       const std::string& controlName) {
    UIControl::setControlType(UIControl::eTextInput);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // TextInput specific initialisers
    m_textName = registerFastName("text");
    m_funcChangeState = registerFastName("ChangeState");
    m_funcSetCharLimit = registerFastName("SetCharLimit");

    return success;
}

void UIControl_TextInput::init(UIString label, int id) {
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

void UIControl_TextInput::ReInit() {
    UIControl_Base::ReInit();

    init(m_label, m_id);
}

void UIControl_TextInput::setFocus(bool focus) {
    if (m_bHasFocus != focus) {
        m_bHasFocus = focus;

        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_number;
        value[0].number = focus ? 0 : 1;
        IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(),
                                                &result, getIggyValuePath(),
                                                m_funcChangeState, 1, value);
    }
}

void UIControl_TextInput::SetCharLimit(int iLimit) {
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iLimit;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcSetCharLimit, 1, value);
}
