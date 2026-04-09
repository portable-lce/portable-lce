#pragma once

#include <string>

#include "app/common/UI/All Platforms/IUIScene_HUD.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/iggy.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/linux/Stubs/iggy_stubs.h"
#endif
#include "app/linux/Iggy/include/rrCore.h"

class UILayer;

#define CHAT_LINES_COUNT 10

class UIScene_HUD : public UIScene, public IUIScene_HUD {
private:
    bool m_bSplitscreen;

protected:
    UIControl_Label m_labelChatText[CHAT_LINES_COUNT];
    UIControl_Label m_labelJukebox;
    UIControl m_controlLabelBackground[CHAT_LINES_COUNT];
    UIControl_Label m_labelDisplayName;

    IggyName m_funcLoadHud, m_funcSetExpBarProgress, m_funcSetPlayerLevel,
        m_funcSetActiveSlot;
    IggyName m_funcSetHealth, m_funcSetFood, m_funcSetAir, m_funcSetArmour;
    IggyName m_funcShowHealth, m_funcShowHorseHealth, m_funcShowFood,
        m_funcShowAir, m_funcShowArmour, m_funcShowExpbar;
    IggyName m_funcSetRegenerationEffect, m_funcSetFoodSaturationLevel;
    IggyName m_funcSetDragonHealth, m_funcSetDragonLabel,
        m_funcShowDragonHealth;
    IggyName m_funcSetSelectedLabel, m_funcHideSelectedLabel;
    IggyName m_funcRepositionHud, m_funcSetDisplayName,
        m_funcSetTooltipsEnabled;
    IggyName m_funcSetRidingHorse, m_funcSetHorseHealth,
        m_funcSetHorseJumpBarProgress;
    IggyName m_funcSetHealthAbsorb;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_labelChatText[0], "Label1")
    UI_MAP_ELEMENT(m_labelChatText[1], "Label2")
    UI_MAP_ELEMENT(m_labelChatText[2], "Label3")
    UI_MAP_ELEMENT(m_labelChatText[3], "Label4")
    UI_MAP_ELEMENT(m_labelChatText[4], "Label5")
    UI_MAP_ELEMENT(m_labelChatText[5], "Label6")
    UI_MAP_ELEMENT(m_labelChatText[6], "Label7")
    UI_MAP_ELEMENT(m_labelChatText[7], "Label8")
    UI_MAP_ELEMENT(m_labelChatText[8], "Label9")
    UI_MAP_ELEMENT(m_labelChatText[9], "Label10")

    UI_MAP_ELEMENT(m_controlLabelBackground[0], "Label1Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[1], "Label2Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[2], "Label3Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[3], "Label4Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[4], "Label5Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[5], "Label6Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[6], "Label7Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[7], "Label8Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[8], "Label9Background")
    UI_MAP_ELEMENT(m_controlLabelBackground[9], "Label10Background")

    UI_MAP_ELEMENT(m_labelJukebox, "Jukebox")

    UI_MAP_ELEMENT(m_labelDisplayName, "LabelGamertag")

    UI_MAP_NAME(m_funcLoadHud, "LoadHud")
    UI_MAP_NAME(m_funcSetExpBarProgress, "SetExpBarProgress")
    UI_MAP_NAME(m_funcSetPlayerLevel, "SetPlayerLevel")
    UI_MAP_NAME(m_funcSetActiveSlot, "SetActiveSlot")

    UI_MAP_NAME(m_funcSetHealth, "SetHealth")
    UI_MAP_NAME(m_funcSetFood, "SetFood")
    UI_MAP_NAME(m_funcSetAir, "SetAir")
    UI_MAP_NAME(m_funcSetArmour, "SetArmour")

    UI_MAP_NAME(m_funcShowHealth, "ShowHealth")
    UI_MAP_NAME(m_funcShowHorseHealth, "ShowHorseHealth")
    UI_MAP_NAME(m_funcShowFood, "ShowFood")
    UI_MAP_NAME(m_funcShowAir, "ShowAir")
    UI_MAP_NAME(m_funcShowArmour, "ShowArmour")
    UI_MAP_NAME(m_funcShowExpbar, "ShowExpBar")

    UI_MAP_NAME(m_funcSetRegenerationEffect, "SetRegenerationEffect")
    UI_MAP_NAME(m_funcSetFoodSaturationLevel, "SetFoodSaturationLevel")

    UI_MAP_NAME(m_funcSetDragonHealth, "SetDragonHealth")
    UI_MAP_NAME(m_funcSetDragonLabel, "SetDragonLabel")
    UI_MAP_NAME(m_funcShowDragonHealth, "ShowDragonHealthBar")

    UI_MAP_NAME(m_funcSetSelectedLabel, "SetSelectedLabel")
    UI_MAP_NAME(m_funcHideSelectedLabel, "HideSelectedLabel")

    UI_MAP_NAME(m_funcRepositionHud, "RepositionHud")
    UI_MAP_NAME(m_funcSetDisplayName, "SetGamertag")

    UI_MAP_NAME(m_funcSetTooltipsEnabled, "SetTooltipsEnabled")

    UI_MAP_NAME(m_funcSetRidingHorse, "SetRidingHorse")
    UI_MAP_NAME(m_funcSetHorseHealth, "SetHorseHealth")
    UI_MAP_NAME(m_funcSetHorseJumpBarProgress, "SetHorseJumpBarProgress")

    UI_MAP_NAME(m_funcSetHealthAbsorb, "SetHealthAbsorb")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIScene_HUD(int iPad, void* initData, UILayer* parentLayer);

    virtual void tick();

    virtual void updateSafeZone();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    virtual EUIScene getSceneType() { return eUIScene_HUD; }

    // Returns true if this scene handles input
    virtual bool stealsFocus() { return false; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return false; }

    // Returns true if lower scenes in this scenes layer, or in any layer below
    // this scenes layers should be hidden
    virtual bool hidesLowerScenes() { return false; }

    virtual void customDraw(IggyCustomDrawCallbackRegion* region);

    virtual void handleReload();

private:
    virtual int getPad();
    virtual void SetOpacity(float opacity);
    virtual void SetVisible(bool visible);

    void SetHudSize(int scale);
    void SetExpBarProgress(float progress, int xpNeededForNextLevel);
    void SetExpLevel(int level);
    void SetActiveSlot(int slot);

    void SetHealth(int iHealth, int iLastHealth, bool bBlink, bool bPoison,
                   bool bWither);
    void SetFood(int iFood, int iLastFood, bool bPoison);
    void SetAir(int iAir, int extra);
    void SetArmour(int iArmour);

    void ShowHealth(bool show);
    void ShowHorseHealth(bool show);
    void ShowFood(bool show);
    void ShowAir(bool show);
    void ShowArmour(bool show);
    void ShowExpBar(bool show);

    void SetRegenerationEffect(bool bEnabled);
    void SetFoodSaturationLevel(int iSaturation);

    void SetDragonHealth(float health);
    void SetDragonLabel(const std::string& label);
    void ShowDragonHealth(bool show);

    void HideSelectedLabel();

    void SetDisplayName(const std::string& displayName);

    void SetTooltipsEnabled(bool bEnabled);

    void SetRidingHorse(bool ridingHorse, bool bIsJumpable, int maxHorseHealth);
    void SetHorseHealth(int health, bool blink = false);
    void SetHorseJumpBarProgress(float progress);

    void SetHealthAbsorb(int healthAbsorb);

public:
    void SetSelectedLabel(const std::string& label);
    void ShowDisplayName(bool show);

    void handleGameTick();

    // RENDERING
    virtual void render(S32 width, S32 height,
                        IPlatformRenderer::eViewportType viewport);

protected:
    void handleTimerComplete(int id);

private:
    void repositionHud();
};
