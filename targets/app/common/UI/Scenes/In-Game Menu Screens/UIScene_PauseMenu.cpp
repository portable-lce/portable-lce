
#include "UIScene_PauseMenu.h"

#include <stddef.h>

#include <memory>

#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/common/UI/All Platforms/IUIScene_PauseMenu.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/ServerAction.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/profile/profile.h"
#include "strings.h"

class TexturePack;

UIScene_PauseMenu::UIScene_PauseMenu(int iPad, void* initData,
                                     UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
    m_bIgnoreInput = false;
    m_eAction = eAction_None;

    m_buttons[BUTTON_PAUSE_RESUMEGAME].init(app.GetString(IDS_RESUME_GAME),
                                            BUTTON_PAUSE_RESUMEGAME);
    m_buttons[BUTTON_PAUSE_HELPANDOPTIONS].init(
        app.GetString(IDS_HELP_AND_OPTIONS), BUTTON_PAUSE_HELPANDOPTIONS);
    m_buttons[BUTTON_PAUSE_LEADERBOARDS].init(app.GetString(IDS_LEADERBOARDS),
                                              BUTTON_PAUSE_LEADERBOARDS);
    m_buttons[BUTTON_PAUSE_ACHIEVEMENTS].init(app.GetString(IDS_ACHIEVEMENTS),
                                              BUTTON_PAUSE_ACHIEVEMENTS);
    m_buttons[BUTTON_PAUSE_SAVEGAME].init(app.GetString(IDS_SAVE_GAME),
                                          BUTTON_PAUSE_SAVEGAME);
    m_buttons[BUTTON_PAUSE_EXITGAME].init(app.GetString(IDS_EXIT_GAME),
                                          BUTTON_PAUSE_EXITGAME);

    updateControlsVisibility();

    doHorizontalResizeCheck();

    // get rid of the quadrant display if it's on
    ui.HidePressStart();

#if TO_BE_IMPLEMENTED
    XuiSetTimer(m_hObj, IGNORE_KEYPRESS_TIMERID, IGNORE_KEYPRESS_TIME);
#endif

    // TODO: proper fix for pausing
    // 4jcraft: replace IsLocalGame() with GetPlayerCount() == 1 due to
    // IsLocalGame() issues on Iggy
    if (/*g_NetworkManager.IsLocalGame() &&*/ g_NetworkManager
            .GetPlayerCount() == 1) {
        MinecraftServer::getInstance()->queueServerAction(
            minecraft::server::PauseServer{true});
    }

    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft != nullptr && pMinecraft->localgameModes[iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[iPad];

        // This just allows it to be shown
        gameMode->getTutorial()->showTutorialPopup(false);
    }
    m_bErrorDialogRunning = false;
}

UIScene_PauseMenu::~UIScene_PauseMenu() {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft != nullptr &&
        pMinecraft->localgameModes[m_iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[m_iPad];

        // This just allows it to be shown
        gameMode->getTutorial()->showTutorialPopup(true);
    }

    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, false);
    m_parentLayer->showComponent(m_iPad, eUIComponent_MenuBackground, false);
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

std::string UIScene_PauseMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "PauseMenuSplit";
    } else {
        return "PauseMenu";
    }
}

void UIScene_PauseMenu::tick() { UIScene::tick(); }

void UIScene_PauseMenu::updateTooltips() {
    // bool bUserisClientSide = PlatformProfile.IsSignedInLive(m_iPad);
    // bool bIsisPrimaryHost =
    //     g_NetworkManager.IsHost() && (PlatformProfile.GetPrimaryPad() ==
    //     m_iPad);

    int iY = -1;
    int iRB = -1;
    int iX = -1;

    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK, iX, iY, -1,
                   -1, -1, iRB);
}

void UIScene_PauseMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, false);
    m_parentLayer->showComponent(m_iPad, eUIComponent_MenuBackground, true);

    if (app.GetLocalPlayerCount() == 1)
        m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
    else
        m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

void UIScene_PauseMenu::handlePreReload() {}

void UIScene_PauseMenu::handleReload() {
    updateTooltips();
    updateControlsVisibility();

    doHorizontalResizeCheck();
}

void UIScene_PauseMenu::updateControlsVisibility() {
    // are we the primary player?
    // 4J-PB - fix for 7844 & 7845 -
    // TCR # 128:  XLA Pause Menu:   When in a multiplayer game as a client the
    // Pause Menu does not have a Leaderboards option. TCR # 128:  XLA Pause
    // Menu:   When in a multiplayer game as a client the Pause Menu does not
    // have an Achievements option.
    if (PlatformProfile.GetPrimaryPad() ==
        m_iPad)  // && g_NetworkManager.IsHost())
    {
        // are we in splitscreen?
        // how many local players do we have?
        if (app.GetLocalPlayerCount() > 1) {
            // Hide the BUTTON_PAUSE_LEADERBOARDS and BUTTON_PAUSE_ACHIEVEMENTS
            removeControl(&m_buttons[BUTTON_PAUSE_LEADERBOARDS], false);
            removeControl(&m_buttons[BUTTON_PAUSE_ACHIEVEMENTS], false);
        }

        if (!g_NetworkManager.IsHost()) {
            // Hide the BUTTON_PAUSE_SAVEGAME
            removeControl(&m_buttons[BUTTON_PAUSE_SAVEGAME], false);
        }
    } else {
        // Hide the BUTTON_PAUSE_LEADERBOARDS, BUTTON_PAUSE_ACHIEVEMENTS and
        // BUTTON_PAUSE_SAVEGAME
        removeControl(&m_buttons[BUTTON_PAUSE_LEADERBOARDS], false);
        removeControl(&m_buttons[BUTTON_PAUSE_ACHIEVEMENTS], false);
        removeControl(&m_buttons[BUTTON_PAUSE_SAVEGAME], false);
    }

    // is saving disabled?
    if (PlatformStorage.GetSaveDisabled()) {
    }
}

void UIScene_PauseMenu::handleInput(int iPad, int key, bool repeat,
                                    bool pressed, bool released,
                                    bool& handled) {
    if (m_bIgnoreInput) {
        return;
    }

    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");
    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                // TODO: proper fix for pausing
                // 4jcraft: replace IsLocalGame() with GetPlayerCount() == 1 due
                // to IsLocalGame() issues on Iggy
                if (iPad == PlatformProfile.GetPrimaryPad() &&
                    /*g_NetworkManager.IsLocalGame()*/ g_NetworkManager
                            .GetPlayerCount() == 1) {
                    MinecraftServer::getInstance()->queueServerAction(
                        minecraft::server::PauseServer{false});
                }

                ui.PlayUISFX(eSFX_Back);
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            if (pressed) {
                sendInputToMovie(key, repeat, pressed, released);
            }
            break;

#if TO_BE_IMPLEMENTED
        case VK_PAD_X:
            // Change device
            if (bIsisPrimaryHost) {
                // we need a function to deal with the return from this - if it
                // changes, we need to update the pause menu and tooltips Fix
                // for #12531 - TCR 001: BAS Game Stability: When a player
                // selects to change a storage device, and repeatedly backs out
                // of the SD screen, disconnects from LIVE, and then selects a
                // SD, the title crashes.
                m_bIgnoreInput = true;

                PlatformStorage.SetSaveDevice(
                    &UIScene_PauseMenu::DeviceSelectReturned, this, true);
            }
            rfHandled = true;
            break;
#endif

        case ACTION_MENU_Y: {
#if TO_BE_IMPLEMENTED
            if (bUserisClientSide) {
                // 4J Stu - Added check in 1.8.2 bug fix (TU6) to stop repeat
                // key presses
                bool bCanScreenshot = true;
                for (int j = 0; j < XUSER_MAX_COUNT; ++j) {
                    if (app.GetXuiAction(j) ==
                        eAppAction_SocialPostScreenshot) {
                        bCanScreenshot = false;
                        break;
                    }
                }
                if (bCanScreenshot)
                    app.SetAction(pInputData->UserIndex, eAppAction_SocialPost);
            }
            rfHandled = true;
#endif
        } break;
    }
}

void UIScene_PauseMenu::handlePress(F64 controlId, F64 childId) {
    if (m_bIgnoreInput) return;

    switch ((int)controlId) {
        case BUTTON_PAUSE_RESUMEGAME:
            // TODO: proper fix for pausing
            // 4jcraft: replace IsLocalGame() with GetPlayerCount() == 1 due to
            // IsLocalGame() issues on Iggy
            if (m_iPad == PlatformProfile.GetPrimaryPad() &&
                /*g_NetworkManager.IsLocalGame()*/ g_NetworkManager
                        .GetPlayerCount() == 1) {
                MinecraftServer::getInstance()->queueServerAction(
                    minecraft::server::PauseServer{false});
            }
            navigateBack();
            break;
        case BUTTON_PAUSE_LEADERBOARDS: {
            unsigned int uiIDA[1];
            uiIDA[0] = IDS_OK;

            // 4J Gordon: Being used for the leaderboards proper now
            //  guests can't look at leaderboards
            if (PlatformProfile.IsGuest(m_iPad)) {
                ui.RequestAlertMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                       IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
            } else if (!PlatformProfile.IsSignedInLive(m_iPad)) {
                unsigned int uiIDA[1] = {IDS_OK};
                ui.RequestErrorMessage(IDS_PRO_NOTONLINE_TITLE,
                                       IDS_PRO_NOTONLINE_TEXT, uiIDA, 1,
                                       m_iPad);
            } else {
                bool bContentRestricted = false;
                if (bContentRestricted) {
#if !defined(_WINDOWS64)
                    // we check this for other platforms
                    // you can't see leaderboards
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    ui.RequestAlertMessage(IDS_ONLINE_SERVICE_TITLE,
                                           IDS_CONTENT_RESTRICTION, uiIDA, 1,
                                           m_iPad);
#endif
                } else {
                    ui.NavigateToScene(m_iPad, eUIScene_LeaderboardsMenu);
                }
            }
        } break;
        case BUTTON_PAUSE_ACHIEVEMENTS:
            // guests can't look at achievements
            if (PlatformProfile.IsGuest(m_iPad)) {
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_OK;
                ui.RequestAlertMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                       IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
            } else {
                // XShowAchievementsUI(m_iPad);
            }
            break;

        case BUTTON_PAUSE_HELPANDOPTIONS:
            ui.NavigateToScene(m_iPad, eUIScene_HelpAndOptionsMenu);
            break;
        case BUTTON_PAUSE_SAVEGAME:
            PerformActionSaveGame();
            break;
        case BUTTON_PAUSE_EXITGAME: {
            Minecraft* pMinecraft = Minecraft::GetInstance();
            unsigned int uiIDA[3];

            // is it the primary player exiting?
            if (m_iPad == PlatformProfile.GetPrimaryPad()) {
                int playTime = -1;
                if (pMinecraft->localplayers[m_iPad] != nullptr) {
                    playTime = (int)pMinecraft->localplayers[m_iPad]
                                   ->getSessionTimer();
                }

                if (PlatformStorage.GetSaveDisabled()) {
                    uiIDA[0] = IDS_CONFIRM_CANCEL;
                    uiIDA[1] = IDS_CONFIRM_OK;
                    ui.RequestAlertMessage(
                        IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME_PROGRESS_LOST,
                        uiIDA, 2, m_iPad,
                        &IUIScene_PauseMenu::ExitGameDialogReturned,
                        (void*)GetCallbackUniqueId());
                } else {
                    if (g_NetworkManager.IsHost()) {
                        uiIDA[0] = IDS_CONFIRM_CANCEL;
                        uiIDA[1] = IDS_EXIT_GAME_SAVE;
                        uiIDA[2] = IDS_EXIT_GAME_NO_SAVE;

                        if (g_NetworkManager.GetPlayerCount() > 1) {
                            ui.RequestAlertMessage(
                                IDS_EXIT_GAME,
                                IDS_CONFIRM_EXIT_GAME_CONFIRM_DISCONNECT_SAVE,
                                uiIDA, 3, m_iPad,
                                &UIScene_PauseMenu::ExitGameSaveDialogReturned,
                                (void*)GetCallbackUniqueId());
                        } else {
                            ui.RequestAlertMessage(
                                IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 3,
                                m_iPad,
                                &UIScene_PauseMenu::ExitGameSaveDialogReturned,
                                (void*)GetCallbackUniqueId());
                        }
                    } else {
                        uiIDA[0] = IDS_CONFIRM_CANCEL;
                        uiIDA[1] = IDS_CONFIRM_OK;

                        ui.RequestAlertMessage(
                            IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 2,
                            m_iPad, &IUIScene_PauseMenu::ExitGameDialogReturned,
                            (void*)GetCallbackUniqueId());
                    }
                }
            } else {
                int playTime = -1;
                if (pMinecraft->localplayers[m_iPad] != nullptr) {
                    playTime = (int)pMinecraft->localplayers[m_iPad]
                                   ->getSessionTimer();
                }

                // just exit the player
                app.SetAction(m_iPad, eAppAction_ExitPlayer);
            }
        } break;
    }
}

void UIScene_PauseMenu::PerformActionSaveGame() {
    // 4J-PB - Is the player trying to save but they are using a trial
    // texturepack ?
    if (!Minecraft::GetInstance()->skins->isUsingDefaultSkin()) {
        TexturePack* tPack = Minecraft::GetInstance()->skins->getSelected();
        DLCTexturePack* pDLCTexPack = (DLCTexturePack*)tPack;

        m_pDLCPack =
            pDLCTexPack->getDLCInfoParentPack();  // tPack->getDLCPack();

        if (!m_pDLCPack->hasPurchasedFile(DLCManager::e_DLCType_Texture, "")) {
            // upsell
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_OK;
            uiIDA[1] = IDS_CONFIRM_CANCEL;

            // Give the player a warning about the trial version of the texture
            // pack
            {
                ui.RequestAlertMessage(
                    IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE,
                    IDS_WARNING_DLC_TRIALTEXTUREPACK_TEXT, uiIDA, 2, m_iPad,
                    &UIScene_PauseMenu::WarningTrialTexturePackReturned,
                    (void*)GetCallbackUniqueId());
            }

            return;
        } else {
            m_bTrialTexturePack = false;
        }
    }

    // does the save exist?
    bool bSaveExists;
    IPlatformStorage::ESaveGameState result =
        PlatformStorage.DoesSaveExist(&bSaveExists);

    {
        // we need to ask if they are sure they want to overwrite the
        // existing game
        if (bSaveExists) {
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_CONFIRM_OK;
            ui.RequestAlertMessage(IDS_TITLE_SAVE_GAME, IDS_CONFIRM_SAVE_GAME,
                                   uiIDA, 2, m_iPad,
                                   &IUIScene_PauseMenu::SaveGameDialogReturned,
                                   (void*)GetCallbackUniqueId());
        } else {
            // flag a app action of save game
            app.SetAction(m_iPad, eAppAction_SaveGame);
        }
    }
}

void UIScene_PauseMenu::ShowScene(bool show) {
    app.DebugPrintf("UIScene_PauseMenu::ShowScene is not implemented\n");
}

void UIScene_PauseMenu::HandleDLCInstalled() {
    // mounted DLC may have changed
    if (app.StartInstallDLCProcess(m_iPad) == false) {
        // not doing a mount, so re-enable input
        // m_bIgnoreInput=false;
        app.DebugPrintf(
            "UIScene_PauseMenu::HandleDLCInstalled - m_bIgnoreInput false\n");
    } else {
        // 4J-PB - Somehow, on th edisc build, we get in here, but don't call
        // HandleDLCMountingComplete, so input locks up
        // m_bIgnoreInput=true;
        app.DebugPrintf(
            "UIScene_PauseMenu::HandleDLCInstalled - m_bIgnoreInput true\n");
    }
    // this will send a CustomMessage_DLCMountingComplete when done
}

void UIScene_PauseMenu::HandleDLCMountingComplete() {
    // check if we should display the save option

    // m_bIgnoreInput=false;
    app.DebugPrintf(
        "UIScene_PauseMenu::HandleDLCMountingComplete - m_bIgnoreInput false "
        "\n");
}

int UIScene_PauseMenu::UnlockFullSaveReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    return 0;
}

int UIScene_PauseMenu::SaveGame_SignInReturned(void* pParam, bool bContinue,
                                               int iPad) {
    UIScene_PauseMenu* pClass =
        (UIScene_PauseMenu*)ui.GetSceneFromCallbackId((size_t)pParam);
    if (pClass) pClass->SetIgnoreInput(false);

    if (bContinue == true) {
        if (pClass) pClass->PerformActionSaveGame();
    }

    return 0;
}

void UIScene_PauseMenu::SetIgnoreInput(bool ignoreInput) {
    m_bIgnoreInput = ignoreInput;
}
