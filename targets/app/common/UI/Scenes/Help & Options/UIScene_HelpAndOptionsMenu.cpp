
#include "UIScene_HelpAndOptionsMenu.h"

#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/client/Minecraft.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/profile/profile.h"
#include "strings.h"

UIScene_HelpAndOptionsMenu::UIScene_HelpAndOptionsMenu(int iPad, void* initData,
                                                       UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_bNotInGame = (Minecraft::GetInstance()->level == nullptr);

    m_buttons[BUTTON_HAO_CHANGESKIN].init(IDS_CHANGE_SKIN,
                                          BUTTON_HAO_CHANGESKIN);
    m_buttons[BUTTON_HAO_HOWTOPLAY].init(IDS_HOW_TO_PLAY, BUTTON_HAO_HOWTOPLAY);
    m_buttons[BUTTON_HAO_CONTROLS].init(IDS_CONTROLS, BUTTON_HAO_CONTROLS);
    m_buttons[BUTTON_HAO_SETTINGS].init(IDS_SETTINGS, BUTTON_HAO_SETTINGS);
    m_buttons[BUTTON_HAO_CREDITS].init(IDS_CREDITS, BUTTON_HAO_CREDITS);
    // m_buttons[BUTTON_HAO_REINSTALL].init(app.GetString(IDS_REINSTALL_CONTENT),BUTTON_HAO_REINSTALL);
    m_buttons[BUTTON_HAO_DEBUG].init(IDS_DEBUG_SETTINGS, BUTTON_HAO_DEBUG);

    /* 4J-TomK - we should never remove a control before the other buttons
    controls are initialised! (because vita touchboxes are rebuilt on remove
    since the remaining positions might change) */
    // We don't have a reinstall content, so remove the button
    removeControl(&m_buttons[BUTTON_HAO_REINSTALL], false);

#if defined(_FINAL_BUILD)
    removeControl(&m_buttons[BUTTON_HAO_DEBUG], false);
#else
    if (!app.DebugSettingsOn())
        removeControl(&m_buttons[BUTTON_HAO_DEBUG], false);
#endif

    // 4J-PB - do not need a storage device to see this menu - just need one
    // when you choose to re-install them
    bool bNotInGame = (Minecraft::GetInstance()->level == nullptr);

    // any content to be re-installed?
    if (m_iPad == PlatformProfile.GetPrimaryPad() && bNotInGame) {
        // We should show the reinstall menu
        app.DebugPrintf("Reinstall Menu required...\n");
    } else {
        removeControl(&m_buttons[BUTTON_HAO_REINSTALL], false);
    }

    if (app.GetLocalPlayerCount() > 1) {
        // no credits in splitscreen
        removeControl(&m_buttons[BUTTON_HAO_CREDITS], false);

#if TO_BE_IMPLEMENTED
        app.AdjustSplitscreenScene(m_hObj, &m_OriginalPosition, m_iPad, false);
#endif
        if (PlatformProfile.GetPrimaryPad() != m_iPad) {
            removeControl(&m_buttons[BUTTON_HAO_REINSTALL], false);
        }
    }

    // 4J-TomK Moved horizontal resize check to the end to prevent horizontal
    // scaling for buttons that might get removed anyways (debug options for
    // example)
    doHorizontalResizeCheck();

    // PlatformStorage.TMSPP_GetUserQuotaInfo(IPlatformStorage::eGlobalStorage_TitleUser,iPad);
    // PlatformStorage.WebServiceRequestGetFriends(iPad);
}

UIScene_HelpAndOptionsMenu::~UIScene_HelpAndOptionsMenu() {}

std::string UIScene_HelpAndOptionsMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "HelpAndOptionsMenuSplit";
    } else {
        return "HelpAndOptionsMenu";
    }
}

void UIScene_HelpAndOptionsMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_HelpAndOptionsMenu::updateComponents() {
    bool bNotInGame = (Minecraft::GetInstance()->level == nullptr);
    if (bNotInGame) {
        m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
        m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
    } else {
        m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, false);

        if (app.GetLocalPlayerCount() == 1)
            m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
        else
            m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
    }
}

void UIScene_HelpAndOptionsMenu::handleReload() {
#if defined(_FINAL_BUILD)
    removeControl(&m_buttons[BUTTON_HAO_DEBUG], false);
#else
    if (!app.DebugSettingsOn())
        removeControl(&m_buttons[BUTTON_HAO_DEBUG], false);
#endif

    // 4J-PB - do not need a storage device to see this menu - just need one
    // when you choose to re-install them
    bool bNotInGame = (Minecraft::GetInstance()->level == nullptr);

    // any content to be re-installed?
    if (m_iPad == PlatformProfile.GetPrimaryPad() && bNotInGame) {
        // We should show the reinstall menu
        app.DebugPrintf("Reinstall Menu required...\n");
    } else {
        removeControl(&m_buttons[BUTTON_HAO_REINSTALL], false);
    }

    if (app.GetLocalPlayerCount() > 1) {
        // no credits in splitscreen
        removeControl(&m_buttons[BUTTON_HAO_CREDITS], false);

#if TO_BE_IMPLEMENTED
        app.AdjustSplitscreenScene(m_hObj, &m_OriginalPosition, m_iPad, false);
#endif
        if (PlatformProfile.GetPrimaryPad() != m_iPad) {
            removeControl(&m_buttons[BUTTON_HAO_REINSTALL], false);
        }
    }

    doHorizontalResizeCheck();
}

void UIScene_HelpAndOptionsMenu::handleInput(int iPad, int key, bool repeat,
                                             bool pressed, bool released,
                                             bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed && !repeat) {
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
            // CD - Added for audio
            if (pressed) {
                ui.PlayUISFX(eSFX_Press);
            }

        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_HelpAndOptionsMenu::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case BUTTON_HAO_CHANGESKIN:
            ui.NavigateToScene(m_iPad, eUIScene_SkinSelectMenu);
            break;
        case BUTTON_HAO_HOWTOPLAY:
            ui.NavigateToScene(m_iPad, eUIScene_HowToPlayMenu);
            break;
        case BUTTON_HAO_CONTROLS:
            ui.NavigateToScene(m_iPad, eUIScene_ControlsMenu);
            break;
        case BUTTON_HAO_SETTINGS:
            ui.NavigateToScene(m_iPad, eUIScene_SettingsMenu);
            break;
        case BUTTON_HAO_CREDITS:
            ui.NavigateToScene(m_iPad, eUIScene_Credits);
            break;
        case BUTTON_HAO_REINSTALL:
            ui.NavigateToScene(m_iPad, eUIScene_ReinstallMenu);
            break;
        case BUTTON_HAO_DEBUG:
            ui.NavigateToScene(m_iPad, eUIScene_DebugOptions);
            break;
    }
}
