#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/IUIScene_AbstractContainerMenu.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Cursor.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/UIScene.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class AbstractContainerMenu;
class UILayer;

class UIScene_AbstractContainerMenu
    : public UIScene,
      public virtual IUIScene_AbstractContainerMenu {
private:
    ESceneSection m_focusSection;
    bool m_bIgnoreInput;

protected:
    UIControl m_controlMainPanel;
    UIControl_SlotList m_slotListHotbar, m_slotListInventory;
    UIControl_Cursor m_cursorPath;
    UIControl_Label m_labelInventory, m_labelBrewingStand;
    UIControl m_controlBackgroundPanel;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_controlMainPanel, "MainPanel")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_controlBackgroundPanel, "BackgroundPanel")
    UI_MAP_ELEMENT(m_slotListHotbar, "hotbarList")
    UI_MAP_ELEMENT(m_slotListInventory, "inventoryList")
    UI_MAP_ELEMENT(m_cursorPath, "cursor")
    UI_MAP_ELEMENT(m_labelInventory, "inventoryLabel")
    UI_END_MAP_CHILD_ELEMENTS()
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIScene_AbstractContainerMenu(int iPad, UILayer* parentLayer);
    ~UIScene_AbstractContainerMenu();

    virtual void handleDestroy();

    int getPad() { return m_iPad; }
    bool getIgnoreInput() { return m_bIgnoreInput; }
    void setIgnoreInput(bool bVal) { m_bIgnoreInput = bVal; }

protected:
    virtual void PlatformInitialize(int iPad, int startIndex);
    virtual void InitDataAssociations(int iPad, AbstractContainerMenu* menu,
                                      int startIndex = 0);
    virtual bool doesSectionTreeHaveFocus(ESceneSection eSection) {
        return false;
    }
    virtual void setSectionFocus(ESceneSection eSection, int iPad);
    void setFocusToPointer(int iPad);
    void SetPointerText(std::vector<HtmlString>* description, bool newSlot);
    virtual std::shared_ptr<ItemInstance> getSlotItem(ESceneSection eSection,
                                                      int iSlot);
    virtual Slot* getSlot(ESceneSection eSection, int iSlot);
    virtual bool isSlotEmpty(ESceneSection eSection, int iSlot);
    virtual void adjustPointerForSafeZone();

    virtual UIControl* getSection(ESceneSection eSection) { return nullptr; }
    virtual int GetBaseSlotCount() { return 0; }

public:
    virtual void tick();

    // 4J - TomK If update tooltips is called then make sure the correct parent
    // is invoked! (both UIScene AND IUIScene_AbstractContainerMenu have an
    // instance of said function!)
    virtual void updateTooltips() {
        IUIScene_AbstractContainerMenu::UpdateTooltips();
    }

    virtual void render(S32 width, S32 height,
                        IPlatformRenderer::eViewportType viewpBort);
    virtual void customDraw(IggyCustomDrawCallbackRegion* region);

    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
};
