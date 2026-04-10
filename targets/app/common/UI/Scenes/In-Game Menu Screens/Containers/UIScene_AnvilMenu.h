#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/common/UI/All Platforms/IUIScene_AnvilMenu.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UIScene.h"
#include "platform/input/input.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIScene_AbstractContainerMenu.h"
#include "minecraft/world/inventory/MerchantMenu.h"

class InventoryMenu;
class UILayer;

class UIScene_AnvilMenu : public UIScene_AbstractContainerMenu,
                          public IUIScene_AnvilMenu {
private:
    bool m_showingCross;

    enum EControls {
        eControl_TextInput,
    };

public:
    UIScene_AnvilMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_AnvilMenu; }

protected:
    UIControl_SlotList m_slotListItem1, m_slotListItem2, m_slotListResult;
    UIControl_Label m_labelAnvil;
    UIControl_TextInput m_textInputAnvil;

    IggyName m_funcShowRedCross, m_funcSetCostLabel;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_slotListItem1, "Ingredient")
    UI_MAP_ELEMENT(m_slotListItem2, "Ingredient2")
    UI_MAP_ELEMENT(m_slotListResult, "Result")
    UI_MAP_ELEMENT(m_labelAnvil, "AnvilText")
    UI_MAP_ELEMENT(m_textInputAnvil, "AnvilTextInput")
    UI_END_MAP_CHILD_ELEMENTS()

    UI_MAP_NAME(m_funcShowRedCross, "ShowRedCross")
    UI_MAP_NAME(m_funcSetCostLabel, "SetCostLabel")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual std::string getMoviePath();
    virtual void handleReload();

    virtual void tick();

    virtual int getSectionColumns(ESceneSection eSection);
    virtual int getSectionRows(ESceneSection eSection);
    virtual void GetPositionOfSection(ESceneSection eSection,
                                      UIVec2D* pPosition);
    virtual void GetItemScreenData(ESceneSection eSection, int iItemIndex,
                                   UIVec2D* pPosition, UIVec2D* pSize);
    virtual void handleSectionClick(ESceneSection eSection) {}
    virtual void setSectionSelectedSlot(ESceneSection eSection, int x, int y);

    virtual UIControl* getSection(ESceneSection eSection);

    virtual void handleEditNamePressed();
    virtual void setEditNameValue(const std::string& name);
    virtual void setEditNameEditable(bool enabled);
    virtual void handleDestroy();

    void setCostLabel(const std::string& label, bool canAfford);
    void showCross(bool show);
};
