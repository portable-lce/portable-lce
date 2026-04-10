#include "UIControl_ButtonList.h"

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "util/StringHelpers.h"

UIControl_ButtonList::UIControl_ButtonList() {
    m_itemCount = 0;
    m_iCurrentSelection = 0;
}

bool UIControl_ButtonList::setupControl(UIScene* scene, IggyValuePath* parent,
                                        const std::string& controlName) {
    UIControl::setControlType(UIControl::eButtonList);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // SlotList specific initialisers
    m_addNewItemFunc = registerFastName("addNewItem");
    m_removeAllItemsFunc = registerFastName("removeAllItems");
    m_funcHighlightItem = registerFastName("HighlightItem");
    m_funcRemoveItem = registerFastName("RemoveItem");
    m_funcSetButtonLabel = registerFastName("SetButtonLabel");
    m_funcSetTouchFocus = registerFastName("SetTouchFocus");
    m_funcCanTouchTrigger = registerFastName("CanTouchTrigger");

    return success;
}

void UIControl_ButtonList::init(int id) {
    m_id = id;

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = id;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_initFunc, 1, value);
}

void UIControl_ButtonList::ReInit() {
    UIControl_Base::ReInit();
    init(m_id);
    m_itemCount = 0;
    m_iCurrentSelection = 0;
}

void UIControl_ButtonList::clearList() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_removeAllItemsFunc, 0, nullptr);

    m_itemCount = 0;
}

void UIControl_ButtonList::addItem(const std::string& label) {
    addItem(label, m_itemCount);
}

void UIControl_ButtonList::addItem(const std::string& label, int data) {
    IggyDataValue result;
    IggyDataValue value[2];

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>((char*)label.c_str());
    stringVal.length = (S32)label.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = data;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_addNewItemFunc, 2, value);

    ++m_itemCount;
}

void UIControl_ButtonList::removeItem(int index) {
    IggyDataValue result;
    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = index;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcRemoveItem, 1, value);

    --m_itemCount;
}

void UIControl_ButtonList::setCurrentSelection(int iSelection) {
    IggyDataValue result;
    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iSelection;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcHighlightItem, 1, value);
}

int UIControl_ButtonList::getCurrentSelection() { return m_iCurrentSelection; }

void UIControl_ButtonList::updateChildFocus(int iChild) {
    m_iCurrentSelection = iChild;
}

void UIControl_ButtonList::setButtonLabel(int iButtonId,
                                          const std::string& label) {
    IggyDataValue result;
    IggyDataValue value[2];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iButtonId;

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(label.c_str());
    stringVal.length = label.length();
    value[1].type = IGGY_DATATYPE_string_UTF8;
    value[1].string8 = stringVal;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcSetButtonLabel, 2, value);
}

void UIControl_DynamicButtonList::tick() {
    UIControl_ButtonList::tick();

    int buttonIndex = 0;
    std::vector<UIString>::iterator itr;
    for (itr = m_labels.begin(); itr != m_labels.end(); itr++) {
        if (itr->needsUpdating()) {
            setButtonLabel(buttonIndex, itr->getString());
            itr->setUpdated();
        }
        buttonIndex++;
    }
}

void UIControl_DynamicButtonList::addItem(UIString label, int data) {
    if (data < 0) data = m_itemCount;

    if (data < m_labels.size()) {
        m_labels[data] = label;
    } else {
        while (data > m_labels.size()) {
            m_labels.push_back(UIString());
        }
        m_labels.push_back(label);
    }

    UIControl_ButtonList::addItem(label.getString(), data);
}

void UIControl_DynamicButtonList::removeItem(int index) {
    m_labels.erase(m_labels.begin() + index);
    UIControl_ButtonList::removeItem(index);
}