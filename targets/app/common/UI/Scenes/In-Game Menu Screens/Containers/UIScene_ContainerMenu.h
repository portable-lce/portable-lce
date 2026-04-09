#pragma once

#include <string>

#include "UIScene_AbstractContainerMenu.h"
#include "app/common/UI/All Platforms/IUIScene_ContainerMenu.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/UIScene.h"

class InventoryMenu;
class UILayer;

class UIScene_ContainerMenu : public UIScene_AbstractContainerMenu,
                              public IUIScene_ContainerMenu {
private:
    bool m_bLargeChest;

public:
    UIScene_ContainerMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_ContainerMenu; }

protected:
    UIControl_SlotList m_slotListContainer;
    UIControl_Label m_labelChest;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_slotListContainer, "containerList")
    UI_MAP_ELEMENT(m_labelChest, "chestLabel")
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
};