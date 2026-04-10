#pragma once
#ifdef _DEBUG_MENUS_ENABLED
#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"
#include "minecraft/server/ServerAction.h"

class UILayer;

class UIScene_DebugCreateSchematic : public UIScene {
private:
    enum eControls {
        eControl_Name,
        eControl_StartX,
        eControl_StartY,
        eControl_StartZ,
        eControl_EndX,
        eControl_EndY,
        eControl_EndZ,
        eControl_SaveMobs,
        eControl_UseCompression,
        eControl_Create,
    };

    eControls m_keyboardCallbackControl;

    // Local UI state collected from the form. Sent to the server as an
    // ExportSchematic action when the user hits Create.
    minecraft::server::ExportSchematic m_data;

public:
    UIScene_DebugCreateSchematic(int iPad, void* initData,
                                 UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_DebugCreateSchematic; }

protected:
    UIControl_TextInput m_textInputStartX, m_textInputStartY, m_textInputStartZ,
        m_textInputEndX, m_textInputEndY, m_textInputEndZ, m_textInputName;
    UIControl_CheckBox m_checkboxSaveMobs, m_checkboxUseCompression;
    UIControl_Button m_buttonCreate;
    UIControl_Label m_labelStartX, m_labelStartY, m_labelStartZ, m_labelEndX,
        m_labelEndY, m_labelEndZ, m_labelTitle;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_textInputStartX, "StartX")
    UI_MAP_ELEMENT(m_textInputStartY, "StartY")
    UI_MAP_ELEMENT(m_textInputStartZ, "StartZ")
    UI_MAP_ELEMENT(m_textInputEndX, "EndX")
    UI_MAP_ELEMENT(m_textInputEndY, "EndY")
    UI_MAP_ELEMENT(m_textInputEndZ, "EndZ")
    UI_MAP_ELEMENT(m_textInputName, "Name")

    UI_MAP_ELEMENT(m_checkboxSaveMobs, "SaveMobs")
    UI_MAP_ELEMENT(m_checkboxUseCompression, "UseCompression")

    UI_MAP_ELEMENT(m_buttonCreate, "Create")

    UI_MAP_ELEMENT(m_labelStartX, "LabelStartX")
    UI_MAP_ELEMENT(m_labelStartY, "LabelStartY")
    UI_MAP_ELEMENT(m_labelStartZ, "LabelStartZ")
    UI_MAP_ELEMENT(m_labelEndX, "LabelEndX")
    UI_MAP_ELEMENT(m_labelEndY, "LabelEndY")
    UI_MAP_ELEMENT(m_labelEndZ, "LabelEndZ")
    UI_MAP_ELEMENT(m_labelTitle, "LabelTitle")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
    virtual void handleCheckboxToggled(F64 controlId, bool selected);

private:
    int handleKeyboardComplete(bool bRes);
};
#endif
