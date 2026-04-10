#include "UIControl_Touch.h"

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/ConsoleUIController.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif

UIControl_Touch::UIControl_Touch() {}

bool UIControl_Touch::setupControl(UIScene* scene, IggyValuePath* parent,
                                   const std::string& controlName) {
    UIControl::setControlType(UIControl::eTouchControl);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    return success;
}

void UIControl_Touch::init(int iId) {
    m_id = iId;

    switch (m_parentScene->GetParentLayer()->m_iLayer) {
        case eUILayer_Error:
        case eUILayer_Fullscreen:
        case eUILayer_Scene:
        case eUILayer_HUD:
            ui.TouchBoxAdd(this, m_parentScene);
            break;
    }
}

void UIControl_Touch::ReInit() {
    UIControl_Base::ReInit();

    init(m_id);
}