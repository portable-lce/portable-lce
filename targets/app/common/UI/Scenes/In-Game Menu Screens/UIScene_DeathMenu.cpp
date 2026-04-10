
#include "UIScene_DeathMenu.h"

#include <memory>

#include "app/common/Game.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/common/UI/All Platforms/IUIScene_PauseMenu.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"
#include "strings.h"

class UILayer;

UIScene_DeathMenu::UIScene_DeathMenu(int iPad, void* initData,
                                     UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_buttonRespawn.init(app.GetString(IDS_RESPAWN), eControl_Respawn);
    m_buttonExitGame.init(app.GetString(IDS_EXIT_GAME), eControl_ExitGame);

    m_labelTitle.setLabel(app.GetString(IDS_YOU_DIED));

    m_bIgnoreInput = false;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft != nullptr && pMinecraft->localgameModes[iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[iPad];

        // This just allows it to be shown
        gameMode->getTutorial()->showTutorialPopup(false);
    }
}

UIScene_DeathMenu::~UIScene_DeathMenu() {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft != nullptr &&
        pMinecraft->localgameModes[m_iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[m_iPad];

        // This just allows it to be shown
        gameMode->getTutorial()->showTutorialPopup(true);
    }
}

std::string UIScene_DeathMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "DeathMenuSplit";
    } else {
        return "DeathMenu";
    }
}

void UIScene_DeathMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT);
}

void UIScene_DeathMenu::handleInput(int iPad, int key, bool repeat,
                                    bool pressed, bool released,
                                    bool& handled) {
    if (m_bIgnoreInput) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            handled = true;
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            sendInputToMovie(key, repeat, pressed, released);
            handled = true;
            break;
    }
}

void UIScene_DeathMenu::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Respawn:
            m_bIgnoreInput = true;
            app.SetAction(m_iPad, eAppAction_Respawn);
            break;
        case eControl_ExitGame: {
            Minecraft* pMinecraft = Minecraft::GetInstance();
            // 4J-PB - fix for #8333 - BLOCKER: If player decides to exit game,
            // then cancels the exit player becomes stuck at game over screen
            // m_bIgnoreInput = true;
            // is it the primary player exiting?
            if (m_iPad == PlatformProfile.GetPrimaryPad()) {
                unsigned int uiIDA[3];
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

                        ui.RequestAlertMessage(
                            IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 3,
                            m_iPad,
                            &IUIScene_PauseMenu::ExitGameSaveDialogReturned,
                            (void*)GetCallbackUniqueId());
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
                // just exit the player
                app.SetAction(m_iPad, eAppAction_ExitPlayer);
            }
        } break;
    }
}