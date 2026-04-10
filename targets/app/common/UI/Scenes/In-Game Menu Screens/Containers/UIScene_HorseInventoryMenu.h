#pragma once

#include <string>

#include "app/common/UI/All Platforms/IUIScene_HorseInventoryMenu.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_MinecraftHorse.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIScene_AbstractContainerMenu.h"

class InventoryMenu;
class UILayer;

class UIScene_HorseInventoryMenu : public UIScene_AbstractContainerMenu,
                                   public IUIScene_HorseInventoryMenu {
    friend class UIControl_MinecraftHorse;

public:
    UIScene_HorseInventoryMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_HorseMenu; }

protected:
    UIControl_SlotList m_slotSaddle, m_slotArmor, m_slotListChest;
    UIControl_Label m_labelHorse;

    IggyName m_funcSetIsDonkey, m_funcSetHasInventory;

    UIControl_MinecraftHorse m_horsePreview;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_slotSaddle, "SlotSaddle")
    UI_MAP_ELEMENT(m_slotArmor, "SlotArmor")
    UI_MAP_ELEMENT(m_slotListChest, "DonkeyInventoryList")
    UI_MAP_ELEMENT(m_labelHorse, "horseinventoryText")

    UI_MAP_ELEMENT(m_horsePreview, "iggy_horse")
    UI_END_MAP_CHILD_ELEMENTS()

    UI_MAP_NAME(m_funcSetIsDonkey, "SetIsDonkey")
    UI_MAP_NAME(m_funcSetHasInventory, "SetHasInventory")
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

    void SetHasInventory(bool bHasInventory);
    void SetIsDonkey(bool bSetIsDonkey);
};