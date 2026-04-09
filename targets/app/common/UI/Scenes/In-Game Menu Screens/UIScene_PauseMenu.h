#pragma once

#include <string>

#include "app/common/UI/All Platforms/IUIScene_PauseMenu.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "platform/storage/storage.h"

class UILayer;

#define BUTTON_PAUSE_RESUMEGAME 0
#define BUTTON_PAUSE_HELPANDOPTIONS 1
#define BUTTON_PAUSE_LEADERBOARDS 2
#define BUTTON_PAUSE_ACHIEVEMENTS 3

#define BUTTON_PAUSE_SAVEGAME 4
#define BUTTON_PAUSE_EXITGAME 5
#define BUTTONS_PAUSE_MAX BUTTON_PAUSE_EXITGAME + 1

class UIScene_PauseMenu : public UIScene, public IUIScene_PauseMenu {
private:
    bool m_savesDisabled;
    bool m_bTrialTexturePack;
    bool m_bErrorDialogRunning;

    enum eActions {
        eAction_None = 0,

    };
    eActions m_eAction;

    UIControl_Button m_buttons[BUTTONS_PAUSE_MAX];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttons[BUTTON_PAUSE_RESUMEGAME], "Button1")
    UI_MAP_ELEMENT(m_buttons[BUTTON_PAUSE_HELPANDOPTIONS], "Button2")
    UI_MAP_ELEMENT(m_buttons[BUTTON_PAUSE_LEADERBOARDS], "Button3")
    UI_MAP_ELEMENT(m_buttons[BUTTON_PAUSE_ACHIEVEMENTS], "Button4")
    UI_MAP_ELEMENT(m_buttons[BUTTON_PAUSE_SAVEGAME], "Button5")
    UI_MAP_ELEMENT(m_buttons[BUTTON_PAUSE_EXITGAME], "Button6")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    virtual void HandleDLCMountingComplete();
    virtual void HandleDLCInstalled();
    static int UnlockFullSaveReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result);
    static int SaveGame_SignInReturned(void* pParam, bool bContinue, int iPad);

public:
    UIScene_PauseMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_PauseMenu();

    virtual EUIScene getSceneType() { return eUIScene_PauseMenu; }

    virtual void tick();

    virtual void updateTooltips();
    virtual void updateComponents();
    virtual void handlePreReload();
    virtual void handleReload();

protected:
    void updateControlsVisibility();

    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
    virtual void ShowScene(bool show);
    virtual void SetIgnoreInput(bool ignoreInput);
    bool m_bIgnoreInput;

private:
    void PerformActionSaveGame();

protected:
};
