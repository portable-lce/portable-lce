#include "UIControl.h"

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Game.h"
#include "app/common/Iggy/include/rrCore.h"
#include "java/JavaMath.h"

UIControl::UIControl() {
    m_parentScene = nullptr;
    m_lastOpacity = 1.0f;
    m_controlName = "";
    m_isVisible = true;
    m_bHidden = false;
    m_isValid = false;
    m_eControlType = eNoControl;
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
}

bool UIControl::setupControl(UIScene* scene, IggyValuePath* parent,
                             const std::string& controlName) {
    m_parentScene = scene;
    m_controlName = controlName;

    rrbool res =
        IggyValuePathMakeNameRef(&m_iggyPath, parent, controlName.c_str());
    m_isValid = res ? true : false;

    m_nameXPos = registerFastName("x");
    m_nameYPos = registerFastName("y");
    m_nameWidth = registerFastName("width");
    m_nameHeight = registerFastName("height");
    m_funcSetAlpha = registerFastName("SetControlAlpha");
    m_nameVisible = registerFastName("visible");

    if (m_isValid) {
        IggyDatatype controlType = IGGY_DATATYPE__invalid_request;
        IggyResult typeResult =
            IggyValueGetTypeRS(getIggyValuePath(), 0, nullptr, &controlType);
        m_isValid = typeResult == IGGY_RESULT_SUCCESS &&
                    controlType != IGGY_DATATYPE__invalid_request &&
                    controlType != IGGY_DATATYPE_undefined;
    }

    if (m_isValid) {
        F64 fx, fy, fwidth, fheight;
        IggyValueGetF64RS(getIggyValuePath(), m_nameXPos, nullptr, &fx);
        IggyValueGetF64RS(getIggyValuePath(), m_nameYPos, nullptr, &fy);
        IggyValueGetF64RS(getIggyValuePath(), m_nameWidth, nullptr, &fwidth);
        IggyValueGetF64RS(getIggyValuePath(), m_nameHeight, nullptr, &fheight);

        m_x = (S32)fx;
        m_y = (S32)fy;
        m_width = (S32)Math::round(fwidth);
        m_height = (S32)Math::round(fheight);
    } else {
        m_x = 0;
        m_y = 0;
        m_width = 0;
        m_height = 0;
    }

    return res;
}

void UIControl::ReInit() {
    if (!m_isValid) return;

    if (m_lastOpacity != 1.0f) {
        IggyDataValue result;
        IggyDataValue value[2];
        IggyStringUTF8 stringVal;

        stringVal.string = const_cast<char*>((char*)m_controlName.c_str());
        stringVal.length = m_controlName.length();
        value[0].type = IGGY_DATATYPE_string_UTF8;
        value[0].string8 = stringVal;

        value[1].type = IGGY_DATATYPE_number;
        value[1].number = m_lastOpacity;

        IggyResult out = IggyPlayerCallMethodRS(
            m_parentScene->getMovie(), &result, m_parentScene->m_rootPath,
            m_funcSetAlpha, 2, value);
    }

    IggyValueSetBooleanRS(getIggyValuePath(), m_nameVisible, nullptr,
                          m_isVisible);
}

IggyValuePath* UIControl::getIggyValuePath() { return &m_iggyPath; }

S32 UIControl::getXPos() { return m_x; }

S32 UIControl::getYPos() { return m_y; }

S32 UIControl::getWidth() { return m_width; }

S32 UIControl::getHeight() { return m_height; }

void UIControl::setOpacity(float percent) {
    if (percent != m_lastOpacity) {
        m_lastOpacity = percent;
        if (!m_isValid) return;

        IggyDataValue result;
        IggyDataValue value[2];
        IggyStringUTF8 stringVal;

        stringVal.string = const_cast<char*>((char*)m_controlName.c_str());
        stringVal.length = m_controlName.length();
        value[0].type = IGGY_DATATYPE_string_UTF8;
        value[0].string8 = stringVal;

        value[1].type = IGGY_DATATYPE_number;
        value[1].number = m_lastOpacity;

        IggyResult out = IggyPlayerCallMethodRS(
            m_parentScene->getMovie(), &result, m_parentScene->m_rootPath,
            m_funcSetAlpha, 2, value);
    }
}

void UIControl::setVisible(bool visible) {
    if (visible != m_isVisible) {
        if (!m_isValid) {
            m_isVisible = visible;
            return;
        }

        rrbool succ = IggyValueSetBooleanRS(getIggyValuePath(), m_nameVisible,
                                            nullptr, visible);
        if (succ)
            m_isVisible = visible;
        else
            app.DebugPrintf("Failed to set visibility for control\n");
    }
}

bool UIControl::getVisible() {
    if (!m_isValid) return m_isVisible;

    rrbool bVisible = false;

    IggyResult result = IggyValueGetBooleanRS(getIggyValuePath(), m_nameVisible,
                                              nullptr, &bVisible);

    m_isVisible = bVisible;

    return bVisible;
}

IggyName UIControl::registerFastName(const std::string& name) {
    return m_parentScene->registerFastName(name);
}
