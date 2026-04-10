#pragma once

#include <string>

#include "app/common/UI/All Platforms/IUIScene_InventoryMenu.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_MinecraftPlayer.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIScene_AbstractContainerMenu.h"
#include "minecraft/world/effect/MobEffect.h"

class InventoryMenu;
class UILayer;

class UIScene_InventoryMenu : public UIScene_AbstractContainerMenu,
                              public IUIScene_InventoryMenu {
    friend class UIControl_MinecraftPlayer;

private:
    int m_bEffectTime[MobEffect::NUM_EFFECTS];

public:
    UIScene_InventoryMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_InventoryMenu; }

protected:
    UIControl_SlotList m_slotListArmor;
    UIControl_MinecraftPlayer m_playerPreview;
    IggyName m_funcUpdateEffects, m_funcAddEffect;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_slotListArmor, "armorList")
    UI_MAP_ELEMENT(m_playerPreview, "iggy_player")

    UI_MAP_NAME(m_funcUpdateEffects, "UpdateEffects")
    UI_MAP_NAME(m_funcAddEffect, "AddEffect")
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
    virtual void handleTimerComplete(int id);

private:
    void updateEffectsDisplay();
};