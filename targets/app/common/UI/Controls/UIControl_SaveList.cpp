#include "UIControl_SaveList.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "util/StringHelpers.h"

bool UIControl_SaveList::setupControl(UIScene* scene, IggyValuePath* parent,
                                      const std::string& controlName) {
    UIControl::setControlType(UIControl::eSaveList);
    bool success =
        UIControl_ButtonList::setupControl(scene, parent, controlName);

    // SlotList specific initialisers
    m_funcSetTextureName = registerFastName("SetTextureName");

    return success;
}

void UIControl_SaveList::addItem(const std::string& label) {
    addItem(label, "");
}

void UIControl_SaveList::addItem(const std::string& label, int data) {
    addItem(label, "", data);
}

void UIControl_SaveList::addItem(const std::string& label,
                                 const std::string& iconName) {
    addItem(label, iconName, m_itemCount);
    ++m_itemCount;
}

void UIControl_SaveList::addItem(const std::string& label,
                                 const std::string& iconName, int data) {
    IggyDataValue result;
    IggyDataValue value[3];

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>((char*)label.c_str());
    stringVal.length = (S32)label.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = m_itemCount;

    IggyStringUTF8 stringVal2;
    stringVal2.string = const_cast<char*>(iconName.c_str());
    stringVal2.length = iconName.length();
    value[2].type = IGGY_DATATYPE_string_UTF8;
    value[2].string8 = stringVal2;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_addNewItemFunc, 3, value);
}

void UIControl_SaveList::setTextureName(int iId, const std::string& iconName) {
    IggyDataValue result;
    IggyDataValue value[2];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iId;

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(iconName.c_str());
    stringVal.length = iconName.length();
    value[1].type = IGGY_DATATYPE_string_UTF8;
    value[1].string8 = stringVal;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcSetTextureName, 2, value);
}
