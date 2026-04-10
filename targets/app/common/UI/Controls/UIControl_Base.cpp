#include "app/common/UI/Controls/UIControl_Base.h"

#include <string>
#include <vector>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "util/StringHelpers.h"

UIControl_Base::UIControl_Base() {
    m_bLabelChanged = false;
    m_id = 0;
}

bool UIControl_Base::setupControl(UIScene* scene, IggyValuePath* parent,
                                  const std::string& controlName) {
    bool success = UIControl::setupControl(scene, parent, controlName);

    m_setLabelFunc = registerFastName("SetLabel");
    m_initFunc = registerFastName("Init");
    m_funcGetLabel = registerFastName("GetLabel");
    m_funcCheckLabelWidths = registerFastName("CheckLabelWidths");

    return success;
}

void UIControl_Base::tick() {
    UIControl::tick();

    if (m_label.needsUpdating() || m_bLabelChanged) {
        // app.DebugPrintf("Calling SetLabel - '%s'\n", m_label.c_str());
        m_bLabelChanged = false;

        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_string_UTF8;
        IggyStringUTF8 stringVal;

        stringVal.string = const_cast<char*>(m_label.getString().c_str());
        stringVal.length = m_label.getString().length();
        value[0].string8 = stringVal;

        IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(),
                                                &result, getIggyValuePath(),
                                                m_setLabelFunc, 1, value);

        m_label.setUpdated();
    }
}

void UIControl_Base::setLabel(UIString label, bool instant, bool force) {
    if (force ||
        ((!m_label.empty() || !label.empty()) && m_label.compare(label) != 0))
        m_bLabelChanged = true;
    m_label = label;

    if (m_bLabelChanged && instant) {
        m_bLabelChanged = false;

        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_string_UTF8;
        IggyStringUTF8 stringVal;

        stringVal.string = const_cast<char*>(m_label.getString().c_str());
        stringVal.length = m_label.getString().length();
        value[0].string8 = stringVal;

        IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(),
                                                &result, getIggyValuePath(),
                                                m_setLabelFunc, 1, value);
    }
}

const char* UIControl_Base::getLabel() {
    IggyDataValue result;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcGetLabel, 0, nullptr);

    if (result.type == IGGY_DATATYPE_string_UTF8) {
        m_label = result.string8.string;
    }

    return m_label.c_str();
}

void UIControl_Base::setAllPossibleLabels(int labelCount, char labels[][256]) {
    IggyDataValue result;
    IggyDataValue* value = new IggyDataValue[labelCount];
    IggyStringUTF8* stringVal = new IggyStringUTF8[labelCount];

    for (int i = 0; i < labelCount; ++i) {
        stringVal[i].string = const_cast<char*>(labels[i]);
        stringVal[i].length = strlen(labels[i]);
        value[i].type = IGGY_DATATYPE_string_UTF8;
        value[i].string8 = stringVal[i];
    }

    IggyResult out = IggyPlayerCallMethodRS(
        m_parentScene->getMovie(), &result, getIggyValuePath(),
        m_funcCheckLabelWidths, labelCount, value);

    delete[] value;
    delete[] stringVal;
}

bool UIControl_Base::hasFocus() { return m_parentScene->controlHasFocus(this); }
