#pragma once

#include <string>

#include "UIScene_AbstractContainerMenu.h"
#include "app/common/UI/All Platforms/IUIScene_EnchantingMenu.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_EnchantmentBook.h"
#include "app/common/UI/Controls/UIControl_EnchantmentButton.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/iggy.h"

class InventoryMenu;
class UILayer;

class UIScene_EnchantingMenu : public UIScene_AbstractContainerMenu,
                               public IUIScene_EnchantingMenu {
private:
    enum EControls {
        eControl_UNKNOWN,
        eControl_Button1,
        eControl_Button2,
        eControl_Button3,
    };

public:
    UIScene_EnchantingMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_EnchantingMenu; }

protected:
    UIControl_SlotList m_slotListIngredient;
    UIControl_Label m_labelEnchant;
    UIControl_EnchantmentButton m_enchantButton[3];
    UIControl_EnchantmentBook m_enchantBook;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_slotListIngredient, "ingredient")
    UI_MAP_ELEMENT(m_enchantButton[0], "Button1")
    UI_MAP_ELEMENT(m_enchantButton[1], "Button2")
    UI_MAP_ELEMENT(m_enchantButton[2], "Button3")
    UI_MAP_ELEMENT(m_labelEnchant, "enchantLabel")

    UI_MAP_ELEMENT(m_enchantBook, "iggy_EnchantmentBook")
    UI_END_MAP_CHILD_ELEMENTS()
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual std::string getMoviePath();
    virtual void handleReload();

    virtual int getSectionColumns(ESceneSection eSection);
    virtual int getSectionRows(ESceneSection eSection);
    virtual void GetPositionOfSection(ESceneSection eSection,
                                      UIVec2D* pPosition);
    virtual void GetItemScreenData(ESceneSection eSection, int iItemIndex,
                                   UIVec2D* pPosition, UIVec2D* pSize);
    virtual void handleSectionClick(ESceneSection eSection) {}
    virtual void setSectionSelectedSlot(ESceneSection eSection, int x, int y);

    virtual UIControl* getSection(ESceneSection eSection);

    virtual void customDraw(IggyCustomDrawCallbackRegion* region);
};