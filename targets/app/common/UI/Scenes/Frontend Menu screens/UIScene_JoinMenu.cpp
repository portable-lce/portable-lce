
#include "UIScene_JoinMenu.h"

#include <stddef.h>
#include <stdint.h>

#include "minecraft/GameTypes.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Network/SessionInfo.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/world/Difficulty.h"
#include "minecraft/world/level/LevelSettings.h"
#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "strings.h"

#define UPDATE_PLAYERS_TIMER_ID 0
#define UPDATE_PLAYERS_TIMER_TIME 30000

UIScene_JoinMenu::UIScene_JoinMenu(int iPad, void* _initData,
                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    JoinMenuInitData* initData = (JoinMenuInitData*)_initData;
    m_selectedSession = initData->selectedSession;
    m_friendInfoUpdatedOK = false;
    m_friendInfoUpdatedERROR = false;
    m_friendInfoRequestIssued = false;
}

void UIScene_JoinMenu::updateTooltips() {
    int iA = -1;
    int iY = -1;
    if (getControlFocus() == eControl_GamePlayers) {
    } else {
        iA = IDS_TOOLTIPS_SELECT;
    }

    ui.SetTooltips(DEFAULT_XUI_MENU_USER, iA, IDS_TOOLTIPS_BACK, -1, iY);
}

void UIScene_JoinMenu::tick() {
    if (!m_friendInfoRequestIssued) {
        ui.NavigateToScene(m_iPad, eUIScene_Timer);
        g_NetworkManager.GetFullFriendSessionInfo(
            m_selectedSession,
            [this](bool success) { friendSessionUpdated(success, this); });
        m_friendInfoRequestIssued = true;
    }

    if (m_friendInfoUpdatedOK) {
        m_friendInfoUpdatedOK = false;

        m_buttonJoinGame.init(app.GetString(IDS_JOIN_GAME), eControl_JoinGame);

        m_buttonListPlayers.init(eControl_GamePlayers);

        m_labelLabels[eLabel_Difficulty].init(
            app.GetString(IDS_LABEL_DIFFICULTY));
        m_labelLabels[eLabel_GameType].init(app.GetString(IDS_LABEL_GAME_TYPE));
        m_labelLabels[eLabel_GamertagsOn].init(
            app.GetString(IDS_LABEL_GAMERTAGS));
        m_labelLabels[eLabel_Structures].init(
            app.GetString(IDS_LABEL_STRUCTURES));
        m_labelLabels[eLabel_LevelType].init(
            app.GetString(IDS_LABEL_LEVEL_TYPE));
        m_labelLabels[eLabel_PVP].init(app.GetString(IDS_LABEL_PvP));
        m_labelLabels[eLabel_Trust].init(app.GetString(IDS_LABEL_TRUST));
        m_labelLabels[eLabel_TNTOn].init(app.GetString(IDS_LABEL_TNT));
        m_labelLabels[eLabel_FireOn].init(
            app.GetString(IDS_LABEL_FIRE_SPREADS));

        unsigned int uiGameHostSettings =
            m_selectedSession->data.m_uiGameHostSettings;
        switch (app.GetGameHostOption(uiGameHostSettings,
                                      eGameHostOption_Difficulty)) {
            case Difficulty::EASY:
                m_labelValues[eLabel_Difficulty].init(
                    app.GetString(IDS_DIFFICULTY_TITLE_EASY));
                break;
            case Difficulty::NORMAL:
                m_labelValues[eLabel_Difficulty].init(
                    app.GetString(IDS_DIFFICULTY_TITLE_NORMAL));
                break;
            case Difficulty::HARD:
                m_labelValues[eLabel_Difficulty].init(
                    app.GetString(IDS_DIFFICULTY_TITLE_HARD));
                break;
            case Difficulty::PEACEFUL:
            default:
                m_labelValues[eLabel_Difficulty].init(
                    app.GetString(IDS_DIFFICULTY_TITLE_PEACEFUL));
                break;
        }

        int option =
            app.GetGameHostOption(uiGameHostSettings, eGameHostOption_GameType);
        if (option == GameType::CREATIVE->getId()) {
            m_labelValues[eLabel_GameType].init(app.GetString(IDS_CREATIVE));
        } else if (option == GameType::ADVENTURE->getId()) {
            m_labelValues[eLabel_GameType].init(app.GetString(IDS_ADVENTURE));
        } else {
            m_labelValues[eLabel_GameType].init(app.GetString(IDS_SURVIVAL));
        }

        if (app.GetGameHostOption(uiGameHostSettings,
                                  eGameHostOption_Gamertags))
            m_labelValues[eLabel_GamertagsOn].init(app.GetString(IDS_ON));
        else
            m_labelValues[eLabel_GamertagsOn].init(app.GetString(IDS_OFF));

        if (app.GetGameHostOption(uiGameHostSettings,
                                  eGameHostOption_Structures))
            m_labelValues[eLabel_Structures].init(app.GetString(IDS_ON));
        else
            m_labelValues[eLabel_Structures].init(app.GetString(IDS_OFF));

        if (app.GetGameHostOption(uiGameHostSettings,
                                  eGameHostOption_LevelType))
            m_labelValues[eLabel_LevelType].init(
                app.GetString(IDS_LEVELTYPE_SUPERFLAT));
        else
            m_labelValues[eLabel_LevelType].init(
                app.GetString(IDS_LEVELTYPE_NORMAL));

        if (app.GetGameHostOption(uiGameHostSettings, eGameHostOption_PvP))
            m_labelValues[eLabel_PVP].init(app.GetString(IDS_ON));
        else
            m_labelValues[eLabel_PVP].init(app.GetString(IDS_OFF));

        if (app.GetGameHostOption(uiGameHostSettings,
                                  eGameHostOption_TrustPlayers))
            m_labelValues[eLabel_Trust].init(app.GetString(IDS_ON));
        else
            m_labelValues[eLabel_Trust].init(app.GetString(IDS_OFF));

        if (app.GetGameHostOption(uiGameHostSettings, eGameHostOption_TNT))
            m_labelValues[eLabel_TNTOn].init(app.GetString(IDS_ON));
        else
            m_labelValues[eLabel_TNTOn].init(app.GetString(IDS_OFF));

        if (app.GetGameHostOption(uiGameHostSettings,
                                  eGameHostOption_FireSpreads))
            m_labelValues[eLabel_FireOn].init(app.GetString(IDS_ON));
        else
            m_labelValues[eLabel_FireOn].init(app.GetString(IDS_OFF));

        m_bIgnoreInput = false;

        // Alert the app the we want to be informed of ethernet connections
        app.SetLiveLinkRequired(true);

        addTimer(UPDATE_PLAYERS_TIMER_ID, UPDATE_PLAYERS_TIMER_TIME);
    }

    if (m_friendInfoUpdatedERROR) {
        m_buttonJoinGame.init(app.GetString(IDS_JOIN_GAME), eControl_JoinGame);

        m_buttonListPlayers.init(eControl_GamePlayers);

        m_labelLabels[eLabel_Difficulty].init(
            app.GetString(IDS_LABEL_DIFFICULTY));
        m_labelLabels[eLabel_GameType].init(app.GetString(IDS_LABEL_GAME_TYPE));
        m_labelLabels[eLabel_GamertagsOn].init(
            app.GetString(IDS_LABEL_GAMERTAGS));
        m_labelLabels[eLabel_Structures].init(
            app.GetString(IDS_LABEL_STRUCTURES));
        m_labelLabels[eLabel_LevelType].init(
            app.GetString(IDS_LABEL_LEVEL_TYPE));
        m_labelLabels[eLabel_PVP].init(app.GetString(IDS_LABEL_PvP));
        m_labelLabels[eLabel_Trust].init(app.GetString(IDS_LABEL_TRUST));
        m_labelLabels[eLabel_TNTOn].init(app.GetString(IDS_LABEL_TNT));
        m_labelLabels[eLabel_FireOn].init(
            app.GetString(IDS_LABEL_FIRE_SPREADS));

        m_labelValues[eLabel_Difficulty].init(
            app.GetString(IDS_DIFFICULTY_TITLE_PEACEFUL));
        m_labelValues[eLabel_GameType].init(app.GetString(IDS_CREATIVE));
        m_labelValues[eLabel_GamertagsOn].init(app.GetString(IDS_OFF));
        m_labelValues[eLabel_Structures].init(app.GetString(IDS_OFF));
        m_labelValues[eLabel_LevelType].init(
            app.GetString(IDS_LEVELTYPE_NORMAL));
        m_labelValues[eLabel_PVP].init(app.GetString(IDS_OFF));
        m_labelValues[eLabel_Trust].init(app.GetString(IDS_OFF));
        m_labelValues[eLabel_TNTOn].init(app.GetString(IDS_OFF));
        m_labelValues[eLabel_FireOn].init(app.GetString(IDS_OFF));

        m_friendInfoUpdatedERROR = false;

        // Show a generic network error message, not always safe to assume the
        // error was host quitting without bubbling more info up from the
        // network manager so this is the best we can do
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_CONFIRM_OK;
        ui.RequestErrorMessage(IDS_ERROR_NETWORK_TITLE, IDS_ERROR_NETWORK,
                               uiIDA, 1, m_iPad, ErrorDialogReturned, this);
    }

    UIScene::tick();
}

void UIScene_JoinMenu::friendSessionUpdated(bool success, void* pParam) {
    UIScene_JoinMenu* scene = (UIScene_JoinMenu*)pParam;
    ui.NavigateBack(scene->m_iPad);
    if (success) {
        scene->m_friendInfoUpdatedOK = true;
    } else {
        scene->m_friendInfoUpdatedERROR = true;
    }
}

int UIScene_JoinMenu::ErrorDialogReturned(
    void* pParam, int iPad, const IPlatformStorage::EMessageResult) {
    UIScene_JoinMenu* scene = (UIScene_JoinMenu*)pParam;
    ui.NavigateBack(scene->m_iPad);

    return 0;
}

void UIScene_JoinMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
}

std::string UIScene_JoinMenu::getMoviePath() { return "JoinMenu"; }

void UIScene_JoinMenu::handleInput(int iPad, int key, bool repeat, bool pressed,
                                   bool released, bool& handled) {
    if (m_bIgnoreInput) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
                handled = true;
            }
            break;
        case ACTION_MENU_OK:
            if (getControlFocus() != eControl_GamePlayers) {
                sendInputToMovie(key, repeat, pressed, released);
            }
            handled = true;
            break;
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            handled = true;
            break;
    }
}

void UIScene_JoinMenu::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_JoinGame: {
            m_bIgnoreInput = true;

            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            StartSharedLaunchFlow();
        } break;
        case eControl_GamePlayers:
            break;
    };
}

void UIScene_JoinMenu::handleFocusChange(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_GamePlayers:
            m_buttonListPlayers.updateChildFocus((int)childId);
    };
    updateTooltips();
}

void UIScene_JoinMenu::StartSharedLaunchFlow() {
    if (!app.IsLocalMultiplayerAvailable()) {
        JoinGame(this);
    } else {
        // PlatformProfile.RequestSignInUI(false, false, false, true,
        // false,&UIScene_JoinMenu::StartGame_SignInReturned,
        // this,PlatformProfile.GetPrimaryPad());
        SignInInfo info;
        info.Func = [this](bool bContinue, int pad) {
            return StartGame_SignInReturned(this, bContinue, pad);
        };
        info.requireOnline = true;
        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                           eUIScene_QuadrantSignin, &info);
    }
}

int UIScene_JoinMenu::StartGame_SignInReturned(void* pParam, bool bContinue,
                                               int iPad) {
    UIScene_JoinMenu* pClass = (UIScene_JoinMenu*)ui.GetSceneFromCallbackId(
        reinterpret_cast<size_t>(pParam));
    if (pClass == nullptr) {
        pClass = (UIScene_JoinMenu*)pParam;
    }

    if (bContinue == true && pClass != nullptr &&
        PlatformProfile.IsSignedIn(iPad)) {
        JoinGame(pClass);
    }

    if (pClass != nullptr) {
        pClass->m_bIgnoreInput = false;
    }

    return 0;
}

// Shared function to join the game that is the same whether we used the
// sign-in UI or not
void UIScene_JoinMenu::JoinGame(UIScene_JoinMenu* pClass) {
    bool noPrivileges = false;
    int signedInUsers = 0;
    int localUsersMask = 0;
    uint32_t dwLocalUsersMask = 0;
    bool isSignedInLive = true;
    int iPadNotSignedInLive = -1;

    PlatformProfile.SetLockedProfile(0);  // TEMP!

    // If we're in SD mode, then only the primary player gets to play
    if (app.IsLocalMultiplayerAvailable()) {
        for (unsigned int index = 0; index < XUSER_MAX_COUNT; ++index) {
            if (PlatformProfile.IsSignedIn(index)) {
                if (isSignedInLive && !PlatformProfile.IsSignedInLive(index)) {
                    // Record the first non signed in live pad
                    iPadNotSignedInLive = index;
                }

                if (!PlatformProfile.AllowedToPlayMultiplayer(index))
                    noPrivileges = true;
                dwLocalUsersMask |=
                    CGameNetworkManager::GetLocalPlayerMask(index);
                isSignedInLive =
                    isSignedInLive && PlatformProfile.IsSignedInLive(index);
            }
        }
    } else {
        if (PlatformProfile.IsSignedIn(PlatformProfile.GetPrimaryPad())) {
            if (!PlatformProfile.AllowedToPlayMultiplayer(
                    PlatformProfile.GetPrimaryPad()))
                noPrivileges = true;
            dwLocalUsersMask |= CGameNetworkManager::GetLocalPlayerMask(
                PlatformProfile.GetPrimaryPad());

            isSignedInLive =
                PlatformProfile.IsSignedInLive(PlatformProfile.GetPrimaryPad());
        }
    }

    // If this is an online game but not all players are signed in to Live,
    // stop!
    if (!isSignedInLive) {
        {
            pClass->m_bIgnoreInput = false;
            unsigned int uiIDA[1];
            uiIDA[0] = IDS_CONFIRM_OK;
            ui.RequestErrorMessage(IDS_PRO_NOTONLINE_TITLE,
                                   IDS_PRO_NOTONLINE_TEXT, uiIDA, 1,
                                   PlatformProfile.GetPrimaryPad());
        }
        return;
    }

    // Check if user-created content is allowed, as we cannot play
    // multiplayer if it's not
    bool noUGC = false;
    bool pccAllowed = true;
    bool pccFriendsAllowed = true;

    PlatformProfile.AllowedPlayerCreatedContent(PlatformProfile.GetPrimaryPad(),
                                                false, &pccAllowed,
                                                &pccFriendsAllowed);
    if (!pccAllowed && !pccFriendsAllowed) noUGC = true;

    if (noUGC) {
        pClass->setVisible(true);
        pClass->m_bIgnoreInput = false;

        int messageText = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
        if (signedInUsers > 1)
            messageText = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;

        ui.RequestUGCMessageBox(IDS_CONNECTION_FAILED, messageText);
    } else if (noPrivileges) {
        pClass->setVisible(true);
        pClass->m_bIgnoreInput = false;
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_CONFIRM_OK;
        ui.RequestErrorMessage(IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                               IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT, uiIDA, 1,
                               PlatformProfile.GetPrimaryPad());
    } else {
        CGameNetworkManager::eJoinGameResult result = g_NetworkManager.JoinGame(
            pClass->m_selectedSession, dwLocalUsersMask);

        // Alert the app the we no longer want to be informed of ethernet
        // connections
        app.SetLiveLinkRequired(false);

        if (result != CGameNetworkManager::JOINGAME_SUCCESS) {
            int exitReasonStringId = -1;
            switch (result) {
                case CGameNetworkManager::JOINGAME_FAIL_SERVER_FULL:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_FULL;
                    break;
                default:
                    break;
            }

            if (exitReasonStringId == -1) {
                ui.NavigateBack(pClass->m_iPad);
            } else {
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(IDS_CONNECTION_FAILED,
                                       exitReasonStringId, uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
                exitReasonStringId = -1;

                ui.NavigateToHomeMenu();
            }
        }
    }
}

void UIScene_JoinMenu::handleTimerComplete(int id) {
    switch (id) {
        case UPDATE_PLAYERS_TIMER_ID: {
#if TO_BE_IMPLEMENTED
            PlayerUID selectedPlayerXUID =
                m_selectedSession->data.players[playersList.GetCurSel()];

            bool success = g_NetworkManager.GetGameSessionInfo(
                m_iPad, m_selectedSession->sessionId, m_selectedSession);

            if (success) {
                playersList.DeleteItems(0, playersList.GetItemCount());
                int selectedIndex = 0;
                for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i) {
                    if (m_selectedSession->data.players[i] != nullptr) {
                        if (m_selectedSession->data.players[i] ==
                            selectedPlayerXUID)
                            selectedIndex = i;
                        playersList.InsertItems(i, 1);
#if !defined(_CONTENT_PACKAGE)
                        if (app.DebugSettingsOn() &&
                            (app.GetGameSettingsDebugMask() &
                             (1L << eDebugSetting_DebugLeaderboards))) {
                            playersList.SetText(i, "WWWWWWWWWWWWWWWW");
                        } else
#endif
                        {
                            playersList
                                .SetText(i,
                                         m_selectedSession->data.szPlayers[i])
                                .c_str();
                        }
                    } else {
                        // Leave the loop when we hit the first nullptr player
                        break;
                    }
                }
                playersList.SetCurSel(selectedIndex);
            }
#endif
        } break;
    };
}
