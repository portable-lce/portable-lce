#include "UIControl_TexturePackList.h"

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "util/StringHelpers.h"

UIControl_TexturePackList::UIControl_TexturePackList() {}

bool UIControl_TexturePackList::setupControl(UIScene* scene,
                                             IggyValuePath* parent,
                                             const std::string& controlName) {
    UIControl::setControlType(UIControl::eTexturePackList);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // SlotList specific initialisers
    m_addPackFunc = registerFastName("addPack");
    m_clearSlotsFunc = registerFastName("removeAllItems");
    m_funcSelectSlot = registerFastName("SelectSlot");
    m_funcEnableSelector = registerFastName("EnableSelector");
    m_funcSetTouchFocus = registerFastName("SetTouchFocus");
    m_funcCanTouchTrigger = registerFastName("CanTouchTrigger");
    m_funcGetRealHeight = registerFastName("GetRealHeight");

    return success;
}

void UIControl_TexturePackList::init(const std::string& label, int id) {
    m_label = label;
    m_id = id;

    IggyDataValue result;
    IggyDataValue value[2];
    value[0].type = IGGY_DATATYPE_string_UTF8;
    IggyStringUTF8 stringVal;

    stringVal.string = const_cast<char*>(label.c_str());
    stringVal.length = label.length();
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = id;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_initFunc, 2, value);
}

void UIControl_TexturePackList::addPack(int id,
                                        const std::string& textureName) {
    IggyDataValue result;
    IggyDataValue value[2];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = id;

    value[1].type = IGGY_DATATYPE_string_UTF8;
    IggyStringUTF8 stringVal;

    stringVal.string = const_cast<char*>(textureName.c_str());
    stringVal.length = textureName.length();
    value[1].string8 = stringVal;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_addPackFunc, 2, value);
}

void UIControl_TexturePackList::selectSlot(int id) {
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = id;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcSelectSlot, 1, value);
}

void UIControl_TexturePackList::clearSlots() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_clearSlotsFunc, 0, nullptr);
}

void UIControl_TexturePackList::setEnabled(bool enable) {
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_boolean;
    value[0].number = enable;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcEnableSelector, 1, value);
}

void UIControl_TexturePackList::SetTouchFocus(S32 iX, S32 iY, bool bRepeat) {
    IggyDataValue result;
    IggyDataValue value[3];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iX;
    value[1].type = IGGY_DATATYPE_number;
    value[1].number = iY;
    value[2].type = IGGY_DATATYPE_boolean;
    value[2].boolval = bRepeat;

    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcSetTouchFocus, 3, value);
}

bool UIControl_TexturePackList::CanTouchTrigger(S32 iX, S32 iY) {
    IggyDataValue result;
    IggyDataValue value[2];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iX;
    value[1].type = IGGY_DATATYPE_number;
    value[1].number = iY;

    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcCanTouchTrigger, 2, value);

    S32 bCanTouchTrigger = false;
    if (result.type == IGGY_DATATYPE_boolean) {
        bCanTouchTrigger = (bool)result.boolval;
    }
    return bCanTouchTrigger;
}

S32 UIControl_TexturePackList::GetRealHeight() {
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
