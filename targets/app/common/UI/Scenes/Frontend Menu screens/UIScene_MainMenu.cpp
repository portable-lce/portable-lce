
#include "UIScene_MainMenu.h"
#include "platform/game/game.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <format>
#include <functional>
#include <numbers>

#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "java/InputOutputStream/BufferedReader.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/InputStreamReader.h"
#include "java/Random.h"
#include "java/System.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/User.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/ScreenSizeCalculator.h"
#include "minecraft/server/MinecraftServer.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/NetTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "strings.h"
#include "util/StringHelpers.h"

class LevelGenerationOptions;

Random* UIScene_MainMenu::random = new Random();

int UIScene_MainMenu::eNavigateWhenReady = -1;

UIScene_MainMenu::UIScene_MainMenu(int iPad, void* initData,
                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    m_bRunGameChosen = false;
    m_bErrorDialogRunning = false;

    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    parentLayer->addComponent(iPad, eUIComponent_Panorama);
    parentLayer->addComponent(iPad, eUIComponent_Logo);

    m_eAction = eAction_None;
    m_bIgnorePress = false;

    m_buttons[(int)eControl_PlayGame].init(IDS_PLAY_GAME, eControl_PlayGame);

    m_buttons[(int)eControl_Leaderboards].init(IDS_LEADERBOARDS,
                                               eControl_Leaderboards);
    m_buttons[(int)eControl_Achievements].init((UIString)IDS_ACHIEVEMENTS,
                                               eControl_Achievements);
    m_buttons[(int)eControl_HelpAndOptions].init(IDS_HELP_AND_OPTIONS,
                                                 eControl_HelpAndOptions);
    m_bTrialVersion = false;
    m_buttons[(int)eControl_UnlockOrDLC].init(IDS_DOWNLOADABLECONTENT,
                                              eControl_UnlockOrDLC);

    m_buttons[(int)eControl_Exit].init(app.GetString(IDS_EXIT_GAME),
                                       eControl_Exit);

    doHorizontalResizeCheck();

    m_splash = "";

    std::string filename = "splashes.txt";
    if (app.hasArchiveFile(filename)) {
        std::vector<uint8_t> splashesArray = app.getArchiveFile(filename);
        ByteArrayInputStream bais(splashesArray);
        InputStreamReader isr(&bais);
        BufferedReader br(&isr);

        std::string line = "";
        while (!(line = br.readLine()).empty()) {
            line = trimString(line);
            if (line.length() > 0) {
                m_splashes.push_back(line);
            }
        }

        br.close();
    }

    m_bIgnorePress = false;
    m_bLoadTrialOnNetworkManagerReady = false;

    // 4J Stu - Clear out any loaded game rules
    app.setLevelGenerationOptions(nullptr);

    // 4J Stu - Reset the leaving game flag so that we correctly handle signouts
    // while in the menus
    g_NetworkManager.ResetLeavingGame();

#if TO_BE_IMPLEMENTED
    // Fix for #45154 - Frontend: DLC: Content can only be downloaded from the
    // frontend if you have not joined/exited multiplayer
    XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
#endif
}

UIScene_MainMenu::~UIScene_MainMenu() {
    m_parentLayer->removeComponent(eUIComponent_Panorama);
    m_parentLayer->removeComponent(eUIComponent_Logo);
}

void UIScene_MainMenu::updateTooltips() {
    int iX = -1;
    int iA = -1;
    if (!m_bIgnorePress) {
        iA = IDS_TOOLTIPS_SELECT;
    }
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, iA, -1, iX);
}

void UIScene_MainMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
}

void UIScene_MainMenu::handleGainFocus(bool navBack) {
    UIScene::handleGainFocus(navBack);
    ui.ShowPlayerDisplayname(false);
    m_bIgnorePress = false;

    if (eNavigateWhenReady >= 0) {
        return;
    }

    // 4J-JEV: This needs to come before SetLockedProfile(-1) as it wipes the
    // XbLive contexts.
    if (!navBack) {
        for (int iPad = 0; iPad < MAX_LOCAL_PLAYERS; iPad++) {
            // For returning to menus after exiting a game.
            if (PlatformProfile.IsSignedIn(iPad)) {
                PlatformProfile.SetCurrentGameActivity(
                    iPad, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }
    PlatformProfile.SetLockedProfile(-1);

    m_bIgnorePress = false;
    updateTooltips();

    if (navBack) {
        // Replace the Unlock Full Game with Downloadable Content
        m_buttons[(int)eControl_UnlockOrDLC].setLabel(IDS_DOWNLOADABLECONTENT);
    }

#if TO_BE_IMPLEMENTED
    // Fix for #45154 - Frontend: DLC: Content can only be downloaded from the
    // frontend if you have not joined/exited multiplayer
    XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
    m_Timer.SetShow(false);
#endif
    m_controlTimer.setVisible(false);

    // 4J-PB - remove the "hobo humping" message legal say we can't have, and
    // the 1080p one for Vita
    int splashIndex =
        eSplashRandomStart + 1 +
        random->nextInt((int)m_splashes.size() - (eSplashRandomStart + 1));

    // Override splash text on certain dates
    const std::time_t now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime;
    localtime_r(&now, &localTime);
    const int month = localTime.tm_mon + 1;  // tm_mon is 0-based
    const int day = localTime.tm_mday;
    if (month == 11 && day == 9) {
        splashIndex = eSplashHappyBirthdayEx;
    } else if (month == 6 && day == 1) {
        splashIndex = eSplashHappyBirthdayNotch;
    } else if (month == 12 && day == 24)  // the Java game shows this on
                                          // Christmas Eve, so we will too
    {
        splashIndex = eSplashMerryXmas;
    } else if (month == 1 && day == 1) {
        splashIndex = eSplashHappyNewYear;
    }
    // splashIndex = 47; // Very short string
    // splashIndex = 194; // Very long string
    // splashIndex = 295; // Coloured
    // splashIndex = 296; // Noise
    m_splash = m_splashes.at(splashIndex);
}

std::string UIScene_MainMenu::getMoviePath() { return "MainMenu"; }

void UIScene_MainMenu::handleReload() {}

void UIScene_MainMenu::handleInput(int iPad, int key, bool repeat, bool pressed,
                                   bool released, bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");

    if (m_bIgnorePress || (eNavigateWhenReady >= 0)) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_OK:
            if (pressed) {
                PlatformProfile.SetPrimaryPad(iPad);
                PlatformProfile.SetLockedProfile(-1);
                sendInputToMovie(key, repeat, pressed, released);
            }
            break;

        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_MainMenu::handlePress(F64 controlId, F64 childId) {
    int primaryPad = PlatformProfile.GetPrimaryPad();

    std::function<int(bool, int)> signInReturnedFunc;

    switch ((int)controlId) {
        case eControl_PlayGame:
            m_eAction = eAction_RunGame;
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            signInReturnedFunc = [this](bool bContinue, int pad) {
                return CreateLoad_SignInReturned(this, bContinue, pad);
            };
            break;
        case eControl_Leaderboards:
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);
            m_eAction = eAction_RunLeaderboards;
            signInReturnedFunc = [this](bool bContinue, int pad) {
                return Leaderboards_SignInReturned(this, bContinue, pad);
            };
            break;
        case eControl_Achievements:
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            m_eAction = eAction_RunAchievements;
            signInReturnedFunc = [this](bool bContinue, int pad) {
                return Achievements_SignInReturned(this, bContinue, pad);
            };
            break;
        case eControl_HelpAndOptions:
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            m_eAction = eAction_RunHelpAndOptions;
            signInReturnedFunc = [this](bool bContinue, int pad) {
                return HelpAndOptions_SignInReturned(this, bContinue, pad);
            };
            break;
        case eControl_UnlockOrDLC:
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            m_eAction = eAction_RunUnlockOrDLC;
            signInReturnedFunc = [this](bool bContinue, int pad) {
                return UnlockFullGame_SignInReturned(this, bContinue, pad);
            };
            break;
        case eControl_Exit: {
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_CONFIRM_OK;
            ui.RequestErrorMessage(IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA,
                                   2, XUSER_INDEX_ANY,
                                   &UIScene_MainMenu::ExitGameReturned, this);
        } break;

        default:
            assert(0);
    }

    bool confirmUser = false;

    // Note: if no sign in returned func, assume this isn't required
    if (signInReturnedFunc) {
        if (PlatformProfile.IsSignedIn(primaryPad)) {
            if (confirmUser) {
                PlatformProfile.RequestSignInUI(false, false, true, false, true,
                                                signInReturnedFunc, primaryPad);
            } else {
                RunAction(primaryPad);
            }
        } else {
            // Ask user to sign in
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_OK;
            uiIDA[1] = IDS_CONFIRM_CANCEL;
            ui.RequestErrorMessage(IDS_MUST_SIGN_IN_TITLE,
                                   IDS_MUST_SIGN_IN_TEXT, uiIDA, 2, primaryPad,
                                   &UIScene_MainMenu::MustSignInReturned, this);
        }
    }
}

// Run current action
void UIScene_MainMenu::RunAction(int iPad) {
    switch (m_eAction) {
        case eAction_RunGame:
            RunPlayGame(iPad);
            break;
        case eAction_RunLeaderboards:
            RunLeaderboards(iPad);
            break;
        case eAction_RunAchievements:
            RunAchievements(iPad);
            break;
        case eAction_RunHelpAndOptions:
            RunHelpAndOptions(iPad);
            break;
        case eAction_RunUnlockOrDLC:
            RunUnlockOrDLC(iPad);
            break;
        default:
            break;
    }
}

void UIScene_MainMenu::customDraw(IggyCustomDrawCallbackRegion* region) {
    if (std::char_traits<char16_t>::compare(region->name, u"Splash", 6) == 0) {
        customDrawSplash(region);
    }
}

void UIScene_MainMenu::customDrawSplash(IggyCustomDrawCallbackRegion* region) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    // 4J Stu - Move this to the ctor when the main menu is not the first scene
    // we navigate to
    ScreenSizeCalculator ssc(pMinecraft->options, pMinecraft->width_phys,
                             pMinecraft->height_phys);
    m_fScreenWidth = (float)pMinecraft->width_phys;
    m_fRawWidth = (float)ssc.rawWidth;
    m_fScreenHeight = (float)pMinecraft->height_phys;
    m_fRawHeight = (float)ssc.rawHeight;

    // Setup GDraw, normal game render states and matrices
    CustomDrawData* customDrawRegion = ui.setupCustomDraw(this, region);
    delete customDrawRegion;

    Font* font = pMinecraft->font;

    // build and render with the game call
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glPushMatrix();

    float width = region->x1 - region->x0;
    float height = region->y1 - region->y0;
    float xo = width / 2;
    float yo = height;

    glTranslatef(xo, yo, 0);

    glRotatef(-17, 0, 0, 1);
    float sss = 1.8f - std::abs(sinf(System::currentTimeMillis() % 1000 /
                                     1000.0f * std::numbers::pi * 2) *
                                0.1f);
    sss *= (m_fScreenWidth / m_fRawWidth);

    sss = sss * 100 / (font->width(m_splash) + 8 * 4);
    glScalef(sss, sss, sss);
    // drawCenteredString(font, splash, 0, -8, 0xffff00);
    font->drawShadow(m_splash, 0 - (font->width(m_splash)) / 2, -8, 0xffff00);
    glPopMatrix();

    glDisable(GL_RESCALE_NORMAL);

    glEnable(GL_DEPTH_TEST);

    // Finish GDraw and anything else that needs to be finalised
    ui.endCustomDraw(region);
}

int UIScene_MainMenu::MustSignInReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
        // we need to specify local game here to display local and LIVE profiles
        // in the list
        switch (pClass->m_eAction) {
            case eAction_RunGame:
                PlatformProfile.RequestSignInUI(
                    false, true, false, false, true,
                    [pClass](bool b, int p) {
                        return CreateLoad_SignInReturned(pClass, b, p);
                    },
                    iPad);
                break;
            case eAction_RunHelpAndOptions:
                PlatformProfile.RequestSignInUI(
                    false, false, true, false, true,
                    [pClass](bool b, int p) {
                        return HelpAndOptions_SignInReturned(pClass, b, p);
                    },
                    iPad);
                break;
            case eAction_RunLeaderboards:
                PlatformProfile.RequestSignInUI(
                    false, false, true, false, true,
                    [pClass](bool b, int p) {
                        return Leaderboards_SignInReturned(pClass, b, p);
                    },
                    iPad);
                break;
            case eAction_RunAchievements:
                PlatformProfile.RequestSignInUI(
                    false, false, true, false, true,
                    [pClass](bool b, int p) {
                        return Achievements_SignInReturned(pClass, b, p);
                    },
                    iPad);
                break;
            case eAction_RunUnlockOrDLC:
                PlatformProfile.RequestSignInUI(
                    false, false, true, false, true,
                    [pClass](bool b, int p) {
                        return UnlockFullGame_SignInReturned(pClass, b, p);
                    },
                    iPad);
                break;
            default:
                break;
        }
    } else {
        pClass->m_bIgnorePress = false;
        // unlock the profile
        PlatformProfile.SetLockedProfile(-1);
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            // if the user is valid, we should set the presence
            if (PlatformProfile.IsSignedIn(i)) {
                PlatformProfile.SetCurrentGameActivity(
                    i, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }

    return 0;
}

int UIScene_MainMenu::HelpAndOptions_SignInReturned(void* pParam,
                                                    bool bContinue, int iPad) {
    UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    if (bContinue) {
        // 4J-JEV: Don't we only need to update rich-presence if the sign-in
        // status changes.
        PlatformProfile.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS,
                                               false);

#if TO_BE_IMPLEMENTED
        if (app.GetTMSDLCInfoRead())
#endif
        {
            PlatformProfile.SetLockedProfile(PlatformProfile.GetPrimaryPad());
            proceedToScene(iPad, eUIScene_HelpAndOptionsMenu);
        }
#if TO_BE_IMPLEMENTED
        else {
            // Changing to async TMS calls
            app.SetTMSAction(iPad,
                             eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions);

            // block all input
            pClass->m_bIgnorePress = true;
            // We want to hide everything in this scene and display a timer
            // until we get a completion for the TMS files
            for (int i = 0; i < BUTTONS_MAX; i++) {
                pClass->m_Buttons[i].SetShow(false);
            }

            pClass->updateTooltips();

            pClass->m_Timer.SetShow(true);
        }
#endif
    } else {
        pClass->m_bIgnorePress = false;
        // unlock the profile
        PlatformProfile.SetLockedProfile(-1);
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            // if the user is valid, we should set the presence
            if (PlatformProfile.IsSignedIn(i)) {
                PlatformProfile.SetCurrentGameActivity(
                    i, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }

    return 0;
}

int UIScene_MainMenu::CreateLoad_SignInReturned(void* pParam, bool bContinue,
                                                int iPad) {
    UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    if (bContinue) {
        // 4J-JEV: We only need to update rich-presence if the sign-in status
        // changes.
        PlatformProfile.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS,
                                               false);

        unsigned int uiIDA[1] = {IDS_OK};

        if (PlatformProfile.IsGuest(PlatformProfile.GetPrimaryPad())) {
            pClass->m_bIgnorePress = false;
            ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                   IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
        } else {
            PlatformProfile.SetLockedProfile(PlatformProfile.GetPrimaryPad());

            // change the minecraft player name
            Minecraft::GetInstance()->user->name =
                PlatformProfile.GetGamertag(PlatformProfile.GetPrimaryPad());

            {
                bool bSignedInLive = PlatformProfile.IsSignedInLive(iPad);

                // Check if we're signed in to LIVE
                if (bSignedInLive) {
                    // 4J-PB - Need to check for installed DLC
                    if (!app.DLCInstallProcessCompleted())
                        app.StartInstallDLCProcess(iPad);

                    if (PlatformProfile.IsGuest(iPad)) {
                        pClass->m_bIgnorePress = false;
                        ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                               IDS_PRO_GUESTPROFILE_TEXT, uiIDA,
                                               1);
                    } else {
                        // 4J Stu - Not relevant to PS3
#if TO_BE_IMPLEMENTED
                        // check if all the TMS files are loaded
                        if (app.GetTMSDLCInfoRead() &&
                            app.GetTMSXUIDsFileRead() &&
                            app.GetBanListRead(iPad)) {
                            if (PlatformStorage.SetSaveDevice(
                                    &UIScene_MainMenu::DeviceSelectReturned,
                                    pClass) == true) {
                                // save device already selected

                                // ensure we've applied this player's settings
                                app.ApplyGameSettingsChanged(
                                    PlatformProfile.GetPrimaryPad());
                                // check for DLC
                                // start timer to track DLC check finished
                                pClass->m_Timer.SetShow(true);
                                XuiSetTimer(pClass->m_hObj,
                                            DLC_INSTALLED_TIMER_ID,
                                            DLC_INSTALLED_TIMER_TIME);
                                // app.NavigateToScene(PlatformProfile.GetPrimaryPad(),eUIScene_MultiGameJoinLoad);
                            }
                        } else {
                            // Changing to async TMS calls
                            app.SetTMSAction(
                                iPad,
                                eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

                            // block all input
                            pClass->m_bIgnorePress = true;
                            // We want to hide everything in this scene and
                            // display a timer until we get a completion for the
                            // TMS files
                            for (int i = 0; i < BUTTONS_MAX; i++) {
                                pClass->m_Buttons[i].SetShow(false);
                            }

                            updateTooltips();

                            pClass->m_Timer.SetShow(true);
                        }
#else
                        Minecraft* pMinecraft = Minecraft::GetInstance();
                        pMinecraft->user->name = PlatformProfile.GetGamertag(
                            PlatformProfile.GetPrimaryPad());

                        // ensure we've applied this player's settings
                        app.ApplyGameSettingsChanged(iPad);

                        proceedToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_LoadOrJoinMenu);
#endif
                    }
                } else {
#if TO_BE_IMPLEMENTED
                    // offline
                    PlatformProfile.DisplayOfflineProfile(
                        [pClass](bool b, int p) {
                            return CScene_Main::
                                CreateLoad_OfflineProfileReturned(pClass, b, p);
                        },
                        PlatformProfile.GetPrimaryPad());
#else
                    app.DebugPrintf(
                        "Offline Profile returned not implemented\n");
                    proceedToScene(PlatformProfile.GetPrimaryPad(),
                                   eUIScene_LoadOrJoinMenu);
#endif
                }
            }
        }
    } else {
        pClass->m_bIgnorePress = false;

        // unlock the profile
        PlatformProfile.SetLockedProfile(-1);
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            // if the user is valid, we should set the presence
            if (PlatformProfile.IsSignedIn(i)) {
                PlatformProfile.SetCurrentGameActivity(
                    i, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }
    return 0;
}

int UIScene_MainMenu::Leaderboards_SignInReturned(void* pParam, bool bContinue,
                                                  int iPad) {
    UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    if (bContinue) {
        // 4J-JEV: We only need to update rich-presence if the sign-in status
        // changes.
        PlatformProfile.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS,
                                               false);

        unsigned int uiIDA[1] = {IDS_OK};

        // guests can't look at leaderboards
        if (PlatformProfile.IsGuest(PlatformProfile.GetPrimaryPad())) {
            pClass->m_bIgnorePress = false;
            ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                   IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
        } else if (!PlatformProfile.IsSignedInLive(
                       PlatformProfile.GetPrimaryPad())) {
            pClass->m_bIgnorePress = false;
            ui.RequestErrorMessage(IDS_PRO_NOTONLINE_TITLE,
                                   IDS_PRO_NOTONLINE_TEXT, uiIDA, 1);
        } else {
            bool bContentRestricted = false;
            if (bContentRestricted) {
                pClass->m_bIgnorePress = false;
#if !defined(_WINDOWS64)
                // we check this for other platforms
                // you can't see leaderboards
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(IDS_ONLINE_SERVICE_TITLE,
                                       IDS_CONTENT_RESTRICTION, uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
#endif
            } else {
                PlatformProfile.SetLockedProfile(
                    PlatformProfile.GetPrimaryPad());
                proceedToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_LeaderboardsMenu);
            }
        }
    } else {
        pClass->m_bIgnorePress = false;
        // unlock the profile
        PlatformProfile.SetLockedProfile(-1);
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            // if the user is valid, we should set the presence
            if (PlatformProfile.IsSignedIn(i)) {
                PlatformProfile.SetCurrentGameActivity(
                    i, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }
    return 0;
}

int UIScene_MainMenu::Achievements_SignInReturned(void* pParam, bool bContinue,
                                                  int iPad) {
    UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    if (bContinue) {
        pClass->m_bIgnorePress = false;
        // 4J-JEV: We only need to update rich-presence if the sign-in status
        // changes.
        PlatformProfile.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS,
                                               false);

        // XShowAchievementsUI(PlatformProfile.GetPrimaryPad());
    } else {
        pClass->m_bIgnorePress = false;
        // unlock the profile
        PlatformProfile.SetLockedProfile(-1);
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            // if the user is valid, we should set the presence
            if (PlatformProfile.IsSignedIn(i)) {
                PlatformProfile.SetCurrentGameActivity(
                    i, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }
    return 0;
}

int UIScene_MainMenu::UnlockFullGame_SignInReturned(void* pParam,
                                                    bool bContinue, int iPad) {
    UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    if (bContinue) {
        // 4J-JEV: We only need to update rich-presence if the sign-in status
        // changes.
        PlatformProfile.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS,
                                               false);

        pClass->RunUnlockOrDLC(iPad);
    } else {
        pClass->m_bIgnorePress = false;
        // unlock the profile
        PlatformProfile.SetLockedProfile(-1);
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            // if the user is valid, we should set the presence
            if (PlatformProfile.IsSignedIn(i)) {
                PlatformProfile.SetCurrentGameActivity(
                    i, CONTEXT_PRESENCE_MENUS, false);
            }
        }
    }

    return 0;
}

int UIScene_MainMenu::ExitGameReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    // UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

    // buttons reversed on this
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        // XLaunchNewImage(XLAUNCH_KEYWORD_DASH_ARCADE, 0);
        app.ExitGame();
    }

    return 0;
}

void UIScene_MainMenu::RunPlayGame(int iPad) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    // clear the remembered signed in users so their profiles get read again
    app.ClearSignInChangeUsersMask();

    PlatformGame.ReleaseSaveThumbnail();

    if (PlatformProfile.IsGuest(iPad)) {
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_OK;

        m_bIgnorePress = false;
        ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                               IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
    } else {
        PlatformProfile.SetLockedProfile(iPad);

        // If the player was signed in before selecting play, we'll not have
        // read the profile yet, so query the sign-in status to get this to
        // happen
        (void)PlatformProfile.QuerySigninStatus();

        // 4J-PB - Need to check for installed DLC
        if (!app.DLCInstallProcessCompleted()) app.StartInstallDLCProcess(iPad);

        {
            // are we offline?
            bool bSignedInLive = PlatformProfile.IsSignedInLive(iPad);

            if (!bSignedInLive) {
                PlatformProfile.SetLockedProfile(iPad);
                proceedToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_LoadOrJoinMenu);
            } else {
#if TO_BE_IMPLEMENTED
                // Check if there is any new DLC
                app.ClearNewDLCAvailable();
                PlatformStorage.GetAvailableDLCCount(iPad);

                // check if all the TMS files are loaded
                if (app.GetTMSDLCInfoRead() && app.GetTMSXUIDsFileRead() &&
                    app.GetBanListRead(iPad)) {
                    if (PlatformStorage.SetSaveDevice(
                            &CScene_Main::DeviceSelectReturned, this) == true) {
                        // change the minecraft player name
                        pMinecraft->user->name = PlatformProfile.GetGamertag(
                            PlatformProfile.GetPrimaryPad());
                        // save device already selected

                        // ensure we've applied this player's settings
                        app.ApplyGameSettingsChanged(iPad);
                        // check for DLC
                        // start timer to track DLC check finished
                        m_Timer.SetShow(true);
                        XuiSetTimer(m_hObj, DLC_INSTALLED_TIMER_ID,
                                    DLC_INSTALLED_TIMER_TIME);
                        // app.NavigateToScene(iPad,eUIScene_MultiGameJoinLoad);
                    }
                } else {
                    // Changing to async TMS calls
                    app.SetTMSAction(
                        iPad, eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

                    // block all input
                    m_bIgnorePress = true;
                    // We want to hide everything in this scene and display a
                    // timer until we get a completion for the TMS files
                    for (int i = 0; i < BUTTONS_MAX; i++) {
                        m_Buttons[i].SetShow(false);
                    }

                    updateTooltips();

                    m_Timer.SetShow(true);
                }
#else
                pMinecraft->user->name = PlatformProfile.GetGamertag(
                    PlatformProfile.GetPrimaryPad());

                // ensure we've applied this player's settings
                app.ApplyGameSettingsChanged(iPad);

                proceedToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_LoadOrJoinMenu);
#endif
            }
        }
    }
}

void UIScene_MainMenu::RunLeaderboards(int iPad) {
    unsigned int uiIDA[1];
    uiIDA[0] = IDS_OK;

    // guests can't look at leaderboards
    if (PlatformProfile.IsGuest(iPad)) {
        ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                               IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
    } else if (!PlatformProfile.IsSignedInLive(iPad)) {
        ui.RequestErrorMessage(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_NOTONLINE_TEXT,
                               uiIDA, 1);
    } else {
        // we're supposed to check for parental control restrictions before
        // showing leaderboards The title enforces the user's NP parental
        // control setting for age-based content
        // restriction in network communications.
        // If age restrictions are in place and the user's age does not meet
        // the age restriction of the title's online service content rating
        // (CERO, ESRB, PEGI, etc.), then the title must
        // display a message such as the following and disallow online service
        // for this user.

        bool bContentRestricted = false;
        if (bContentRestricted) {
#if !defined(_WINDOWS64)
            // we check this for other platforms
            // you can't see leaderboards
            unsigned int uiIDA[1];
            uiIDA[0] = IDS_CONFIRM_OK;
            ui.RequestErrorMessage(
                IDS_ONLINE_SERVICE_TITLE, IDS_CONTENT_RESTRICTION, uiIDA, 1,
                PlatformProfile.GetPrimaryPad(), nullptr, this);
#endif
        } else {
            PlatformProfile.SetLockedProfile(iPad);
            // If the player was signed in before selecting play, we'll not have
            // read the profile yet, so query the sign-in status to get this to
            // happen
            (void)PlatformProfile.QuerySigninStatus();

            proceedToScene(iPad, eUIScene_LeaderboardsMenu);
        }
    }
}
void UIScene_MainMenu::RunUnlockOrDLC(int iPad) {
    unsigned int uiIDA[1];
    uiIDA[0] = IDS_OK;

    // downloadable content
    if (PlatformProfile.IsSignedInLive(iPad)) {
        if (PlatformProfile.IsGuest(iPad)) {
            m_bIgnorePress = false;
            ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                   IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
        } else {
            // If the player was signed in before selecting play, we'll not
            // have read the profile yet, so query the sign-in status to get
            // this to happen
            (void)PlatformProfile.QuerySigninStatus();

            {
                bool bContentRestricted = false;
                if (bContentRestricted) {
                    m_bIgnorePress = false;
#if !defined(_WINDOWS64)
                    // we check this for other platforms
                    // you can't see the store
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    ui.RequestErrorMessage(IDS_ONLINE_SERVICE_TITLE,
                                           IDS_CONTENT_RESTRICTION, uiIDA, 1,
                                           PlatformProfile.GetPrimaryPad(),
                                           nullptr, this);
#endif
                } else {
                    PlatformProfile.SetLockedProfile(iPad);
                    proceedToScene(PlatformProfile.GetPrimaryPad(),
                                   eUIScene_DLCMainMenu);
                }
            }

            // read the DLC info from TMS
            /*app.ReadDLCFileFromTMS(iPad);*/

            // We want to navigate to the DLC scene, but block input until
            // we get the DLC file in from TMS Don't navigate - we might
            // have an uplink disconnect
            // app.NavigateToScene(PlatformProfile.GetPrimaryPad(),eUIScene_DLCMainMenu);
        }
    } else {
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_OK;
        ui.RequestErrorMessage(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_NOTONLINE_TEXT,
                               uiIDA, 1);
    }
}

void UIScene_MainMenu::tick() {
    UIScene::tick();

#if !defined(_ENABLEIGGY) && !defined(ENABLE_JAVA_GUIS)
    // 4jcraft
    {
        static int s_mainMenuTickCount = 0;
        s_mainMenuTickCount++;
        if (s_mainMenuTickCount % 60 == 1) {
            fprintf(stderr, "[MM] tick %d\n", s_mainMenuTickCount);
            fflush(stderr);
        }
        // ~3 seconds at 30fps
        if (s_mainMenuTickCount == 90) {
            fprintf(stderr,
                    "[Linux] Auto-starting trial world from MainMenu after %d "
                    "ticks\n",
                    s_mainMenuTickCount);
            LoadTrial();
            return;
        }
    }
#endif

    if ((eNavigateWhenReady >= 0)) {
        int lockedProfile = PlatformProfile.GetLockedProfile();

        {
            app.DebugPrintf("[MainMenu] Navigating away from MainMenu.\n");
            ui.NavigateToScene(lockedProfile, (EUIScene)eNavigateWhenReady);
            eNavigateWhenReady = -1;
        }
    }
}

void UIScene_MainMenu::RunAchievements(int iPad) {
#if TO_BE_IMPLEMENTED
    unsigned int uiIDA[1];
    uiIDA[0] = IDS_OK;

    // guests can't look at achievements
    if (PlatformProfile.IsGuest(iPad)) {
        ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                               IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
    } else {
        XShowAchievementsUI(iPad);
    }
#endif
}

void UIScene_MainMenu::RunHelpAndOptions(int iPad) {
    if (PlatformProfile.IsGuest(iPad)) {
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_OK;
        ui.RequestErrorMessage(IDS_PRO_GUESTPROFILE_TITLE,
                               IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
    } else {
        // If the player was signed in before selecting play, we'll not have
        // read the profile yet, so query the sign-in status to get this to
        // happen
        (void)PlatformProfile.QuerySigninStatus();

#if TO_BE_IMPLEMENTED
        // 4J-PB - You can be offline and still can go into help and options
        if (app.GetTMSDLCInfoRead() || !PlatformProfile.IsSignedInLive(iPad))
#endif
        {
            PlatformProfile.SetLockedProfile(iPad);
            proceedToScene(iPad, eUIScene_HelpAndOptionsMenu);
        }
#if TO_BE_IMPLEMENTED
        else {
            // Changing to async TMS calls
            app.SetTMSAction(iPad,
                             eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions);

            // block all input
            m_bIgnorePress = true;
            // We want to hide everything in this scene and display a timer
            // until we get a completion for the TMS files
            for (int i = 0; i < BUTTONS_MAX; i++) {
                m_Buttons[i].SetShow(false);
            }

            updateTooltips();

            m_Timer.SetShow(true);
        }
#endif
    }
}

void UIScene_MainMenu::LoadTrial(void) {
    app.SetTutorialMode(true);

    // clear out the app's terrain features list
    app.ClearTerrainFeaturePosition();

    PlatformStorage.ResetSaveData();

    // No saving in the trial
    PlatformStorage.SetSaveDisabled(true);
    app.SetGameHostOption(eGameHostOption_WasntSaveOwner, false);

    // Set the global flag, so that we don't disable saving again once the save
    // is complete
    app.SetGameHostOption(eGameHostOption_DisableSaving, 1);

    PlatformStorage.SetSaveTitle("Tutorial");

    // Reset the autosave time
    app.SetAutosaveTimerTime();

    // not online for the trial game
    g_NetworkManager.HostGame(0, false, true, MINECRAFT_NET_MAX_PLAYERS, 0);

    g_NetworkManager.FakeLocalPlayerJoined();

    NetworkGameInitData* param = new NetworkGameInitData();
    param->seed = 0;
    param->saveData = nullptr;
    param->settings = app.GetGameHostOption(eGameHostOption_Tutorial) |
                      app.GetGameHostOption(eGameHostOption_DisableSaving);

    std::vector<LevelGenerationOptions*>* generators = app.getLevelGenerators();
    param->levelGen = generators->at(0);

    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
    loadingParams->lpParam = (void*)param;

    UIFullscreenProgressCompletionData* completionData =
        new UIFullscreenProgressCompletionData();
    completionData->bShowBackground = true;
    completionData->bShowLogo = true;
    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
    completionData->iPad = PlatformProfile.GetPrimaryPad();
    loadingParams->completionData = completionData;

    ui.ShowTrialTimer(true);

    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                       eUIScene_FullscreenProgress, loadingParams);
}

void UIScene_MainMenu::handleUnlockFullVersion() {
    m_buttons[(int)eControl_UnlockOrDLC].setLabel(IDS_DOWNLOADABLECONTENT,
                                                  true);
}
