#include "app/common/Audio/SoundEngine.h"
#include "platform/game/game.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/Game.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_PauseMenu.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/User.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/stats/StatsCounter.h"
#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"
#include "util/StringHelpers.h"

void Game::HandleXuiActions(void) {
    eXuiAction eAction;
    eTMSAction eTMS;
    void* param;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    std::shared_ptr<MultiplayerLocalPlayer> player;

    // are there any global actions to deal with?
    eAction = app.GetGlobalXuiAction();
    if (eAction != eAppAction_Idle) {
        switch (eAction) {
            case eAppAction_DisplayLavaMessage:
                // Display a warning about placing lava in the spawn area
                {
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    IPlatformStorage::EMessageResult result =
                        ui.RequestErrorMessage(IDS_CANT_PLACE_NEAR_SPAWN_TITLE,
                                               IDS_CANT_PLACE_NEAR_SPAWN_TEXT,
                                               uiIDA, 1, XUSER_INDEX_ANY);
                    if (result != IPlatformStorage::EMessage_Busy)
                        SetGlobalXuiAction(eAppAction_Idle);
                }
                break;
            default:
                break;
        }
    }

    // are there any app actions to deal with?
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        eAction = app.GetXuiAction(i);
        param = m_menuController.getXuiActionParam(i);

        if (eAction != eAppAction_Idle) {
            switch (eAction) {
                //     // the renderer will capture a screenshot
                // case eAppAction_SocialPost:
                //     if (PlatformProfile.IsFullVersion()) {
                //         // Facebook Share
                //         if (CSocialManager::Instance()
                //                 ->IsTitleAllowedToPostImages() &&
                //             CSocialManager::Instance()
                //                 ->AreAllUsersAllowedToPostImages()) {
                //             // disable character name tags for the shot
                //             // m_bwasHidingGui =
                //             pMinecraft->options->hideGui;
                //             // // 4J Stu - Removed 1.8.2 bug fix (TU6) as
                //             don't
                //             // need this
                //             pMinecraft->options->hideGui = true;

                //             SetAction(i, eAppAction_SocialPostScreenshot);
                //         } else {
                //             SetAction(i, eAppAction_Idle);
                //         }
                //     } else {
                //         SetAction(i, eAppAction_Idle);
                //     }
                //     break;
                // case eAppAction_SocialPostScreenshot: {
                //     SetAction(i, eAppAction_Idle);
                //     bool bKeepHiding = false;
                //     for (int j = 0; j < XUSER_MAX_COUNT; ++j) {
                //         if (app.GetXuiAction(j) ==
                //             eAppAction_SocialPostScreenshot) {
                //             bKeepHiding = true;
                //             break;
                //         }
                //     }
                //     pMinecraft->options->hideGui = bKeepHiding;

                //     // Facebook Share

                //     if (app.GetLocalPlayerCount() > 1) {
                //         ui.NavigateToScene(i, eUIScene_SocialPost);
                //     } else {
                //         ui.NavigateToScene(i, eUIScene_SocialPost);
                //     }
                // } break;
                case eAppAction_SaveGame:
                    SetAction(i, eAppAction_Idle);
                    if (!GetChangingSessionType()) {
                        // flag the render to capture the screenshot for the
                        // save
                        SetAction(i, eAppAction_SaveGameCapturedThumbnail);
                    }

                    break;
                case eAppAction_AutosaveSaveGame: {
                    // Need to run a check to see if the save exists in order to
                    // stop the dialog asking if we want to overwrite it coming
                    // up on an autosave
                    bool bSaveExists;
                    PlatformStorage.DoesSaveExist(&bSaveExists);

                    SetAction(i, eAppAction_Idle);
                    if (!GetChangingSessionType()) {
                        // flag the render to capture the screenshot for the
                        // save
                        SetAction(i,
                                  eAppAction_AutosaveSaveGameCapturedThumbnail);
                    }
                }

                break;

                case eAppAction_SaveGameCapturedThumbnail:
                    // reset the autosave timer
                    app.SetAutosaveTimerTime();
                    SetAction(i, eAppAction_Idle);
                    // Check that there is a name for the save - if we're saving
                    // from the tutorial and this is the first save from the
                    // tutorial, we'll not have a name
                    /*if(PlatformStorage.GetSaveName()==nullptr)
                    {
                    app.NavigateToScene(i,eUIScene_SaveWorld);
                    }
                    else*/
                    {
                        // turn off the gamertags in splitscreen for the primary
                        // player, since they are about to be made fullscreen
                        ui.HideAllGameUIElements();

                        // Hide the other players scenes
                        ui.ShowOtherPlayersBaseScene(
                            PlatformProfile.GetPrimaryPad(), false);

                        // int saveOrCheckpointId = 0;
                        // bool validSave =
                        // PlatformStorage.GetSaveUniqueNumber(&saveOrCheckpointId);
                        // SentientManager.RecordLevelSaveOrCheckpoint(PlatformProfile.GetPrimaryPad(),
                        // saveOrCheckpointId);

                        LoadingInputParams* loadingParams =
                            new LoadingInputParams();
                        loadingParams->func =
                            &UIScene_PauseMenu::SaveWorldThreadProc;
                        loadingParams->lpParam = (void*)false;

                        // 4J-JEV - PS4: Fix for #5708 - [ONLINE] - If the user
                        // pulls their network cable out while saving the title
                        // will hang.
                        loadingParams->waitForThreadToDelete = true;

                        UIFullscreenProgressCompletionData* completionData =
                            new UIFullscreenProgressCompletionData();
                        completionData->bShowBackground = true;
                        completionData->bShowLogo = true;
                        completionData->type =
                            e_ProgressCompletion_NavigateBackToScene;
                        completionData->iPad = PlatformProfile.GetPrimaryPad();

                        if (ui.IsSceneInStack(PlatformProfile.GetPrimaryPad(),
                                              eUIScene_EndPoem)) {
                            completionData->scene = eUIScene_EndPoem;
                        } else {
                            completionData->scene = eUIScene_PauseMenu;
                        }

                        loadingParams->completionData = completionData;

                        // 4J Stu - Xbox only

                        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                           eUIScene_FullscreenProgress,
                                           loadingParams, eUILayer_Fullscreen,
                                           eUIGroup_Fullscreen);
                    }
                    break;
                case eAppAction_AutosaveSaveGameCapturedThumbnail:

                {
                    app.SetAutosaveTimerTime();
                    SetAction(i, eAppAction_Idle);

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    // app.CloseAllPlayersXuiScenes();
                    //  Hide the other players scenes
                    ui.ShowOtherPlayersBaseScene(
                        PlatformProfile.GetPrimaryPad(), false);

                    // This just allows it to be shown
                    if (pMinecraft
                            ->localgameModes[PlatformProfile.GetPrimaryPad()] !=
                        nullptr)
                        pMinecraft
                            ->localgameModes[PlatformProfile.GetPrimaryPad()]
                            ->getTutorial()
                            ->showTutorialPopup(false);

                    // int saveOrCheckpointId = 0;
                    // bool validSave =
                    // PlatformStorage.GetSaveUniqueNumber(&saveOrCheckpointId);
                    // SentientManager.RecordLevelSaveOrCheckpoint(PlatformProfile.GetPrimaryPad(),
                    // saveOrCheckpointId);

                    LoadingInputParams* loadingParams =
                        new LoadingInputParams();
                    loadingParams->func =
                        &UIScene_PauseMenu::SaveWorldThreadProc;

                    loadingParams->lpParam = (void*)true;

                    UIFullscreenProgressCompletionData* completionData =
                        new UIFullscreenProgressCompletionData();
                    completionData->bShowBackground = true;
                    completionData->bShowLogo = true;
                    completionData->type =
                        e_ProgressCompletion_AutosaveNavigateBack;
                    completionData->iPad = PlatformProfile.GetPrimaryPad();
                    // completionData->bAutosaveWasMenuDisplayed=ui.GetMenuDisplayed(PlatformProfile.GetPrimaryPad());
                    loadingParams->completionData = completionData;

                    // 4J Stu - Xbox only

                    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_FullscreenProgress,
                                       loadingParams, eUILayer_Fullscreen,
                                       eUIGroup_Fullscreen);
                } break;
                case eAppAction_ExitPlayer:
                    // a secondary player has chosen to quit
                    {
                        int iPlayerC = g_NetworkManager.GetPlayerCount();

                        // Since the player is exiting, let's flush any profile
                        // writes for them, and hope we're not breaking TCR
                        // 136...
                        PlatformProfile.ForceQueuedProfileWrites(i);

                        // not required - it's done within the
                        // removeLocalPlayerIdx
                        // 				if(pMinecraft->level->isClientSide)
                        // 				{
                        // 					// we need to
                        // remove the qnetplayer, or this player won't be able
                        // to get back into the game until qnet times out and
                        // removes them
                        // 					g_NetworkManager.NotifyPlayerLeaving(g_NetworkManager.GetLocalPlayerByUserIndex(i));
                        // 				}

                        // if there are any tips showing, we need to close them

                        pMinecraft->gui->clearMessages(i);

                        // Make sure we've not got this player selected as
                        // current - this shouldn't be the case anyway
                        pMinecraft->setLocalPlayerIdx(
                            PlatformProfile.GetPrimaryPad());
                        pMinecraft->removeLocalPlayerIdx(i);

                        // Wipe out the tooltips
                        ui.SetTooltips(i, -1);

                        // Change the presence info
                        // Are we offline or online, and how many players are
                        // there
                        if (iPlayerC > 2)  // one player is about to leave here
                                           // - they'll be set to idle in the
                                           // qnet manager player leave
                        {
                            for (int iPlayer = 0; iPlayer < XUSER_MAX_COUNT;
                                 iPlayer++) {
                                if ((iPlayer != i) &&
                                    pMinecraft->localplayers[iPlayer]) {
                                    if (g_NetworkManager.IsLocalGame()) {
                                        PlatformProfile.SetCurrentGameActivity(
                                            iPlayer,
                                            CONTEXT_PRESENCE_MULTIPLAYEROFFLINE,
                                            false);
                                    } else {
                                        PlatformProfile.SetCurrentGameActivity(
                                            iPlayer,
                                            CONTEXT_PRESENCE_MULTIPLAYER,
                                            false);
                                    }
                                }
                            }
                        } else {
                            for (int iPlayer = 0; iPlayer < XUSER_MAX_COUNT;
                                 iPlayer++) {
                                if ((iPlayer != i) &&
                                    pMinecraft->localplayers[iPlayer]) {
                                    if (g_NetworkManager.IsLocalGame()) {
                                        PlatformProfile.SetCurrentGameActivity(
                                            iPlayer,
                                            CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE,
                                            false);
                                    } else {
                                        PlatformProfile.SetCurrentGameActivity(
                                            iPlayer,
                                            CONTEXT_PRESENCE_MULTIPLAYER_1P,
                                            false);
                                    }
                                }
                            }
                        }

                        SetAction(i, eAppAction_Idle);
                    }
                    break;
                case eAppAction_ExitPlayerPreLogin: {
                    int iPlayerC = g_NetworkManager.GetPlayerCount();
                    // Since the player is exiting, let's flush any profile
                    // writes for them, and hope we're not breaking TCR 136...
                    PlatformProfile.ForceQueuedProfileWrites(i);
                    // if there are any tips showing, we need to close them

                    pMinecraft->gui->clearMessages(i);

                    // Make sure we've not got this player selected as current -
                    // this shouldn't be the case anyway
                    pMinecraft->setLocalPlayerIdx(
                        PlatformProfile.GetPrimaryPad());
                    pMinecraft->removeLocalPlayerIdx(i);

                    // Wipe out the tooltips
                    ui.SetTooltips(i, -1);

                    // Change the presence info
                    // Are we offline or online, and how many players are there
                    if (iPlayerC >
                        2)  // one player is about to leave here - they'll be
                            // set to idle in the qnet manager player leave
                    {
                        for (int iPlayer = 0; iPlayer < XUSER_MAX_COUNT;
                             iPlayer++) {
                            if ((iPlayer != i) &&
                                pMinecraft->localplayers[iPlayer]) {
                                if (g_NetworkManager.IsLocalGame()) {
                                    PlatformProfile.SetCurrentGameActivity(
                                        iPlayer,
                                        CONTEXT_PRESENCE_MULTIPLAYEROFFLINE,
                                        false);
                                } else {
                                    PlatformProfile.SetCurrentGameActivity(
                                        iPlayer, CONTEXT_PRESENCE_MULTIPLAYER,
                                        false);
                                }
                            }
                        }
                    } else {
                        for (int iPlayer = 0; iPlayer < XUSER_MAX_COUNT;
                             iPlayer++) {
                            if ((iPlayer != i) &&
                                pMinecraft->localplayers[iPlayer]) {
                                if (g_NetworkManager.IsLocalGame()) {
                                    PlatformProfile.SetCurrentGameActivity(
                                        iPlayer,
                                        CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE,
                                        false);
                                } else {
                                    PlatformProfile.SetCurrentGameActivity(
                                        iPlayer,
                                        CONTEXT_PRESENCE_MULTIPLAYER_1P, false);
                                }
                            }
                        }
                    }
                    SetAction(i, eAppAction_Idle);
                } break;

                case eAppAction_ExitWorld:
                    pMinecraft->exitingWorldRightNow = true;

                    SetAction(i, eAppAction_Idle);

                    // If we're already leaving don't exit
                    if (g_NetworkManager.IsLeavingGame()) {
                        break;
                    }

                    pMinecraft->gui->clearMessages();

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    // reset the flag stopping new dlc message being shown if
                    // you've seen the message before
                    DisplayNewDLCTipAgain();

                    // clear the autosave timer that might be on screen
                    ui.ShowAutosaveCountdownTimer(false);

                    // Hide the selected item text
                    ui.HideAllGameUIElements();

                    // Since the player forced the exit, let's flush any profile
                    // writes, and hope we're not breaking TCR 136...

                    // 4J-PB - cancel any possible std::string verifications
                    // queued with LIVE
                    // PlatformInput.CancelAllVerifyInProgress();

                    // In a split screen, only the primary player actually
                    // quits the game, others just remove their players
                    if (i != PlatformProfile.GetPrimaryPad()) {
                        // Make sure we've not got this player selected as
                        // current - this shouldn't be the case anyway
                        pMinecraft->setLocalPlayerIdx(
                            PlatformProfile.GetPrimaryPad());
                        pMinecraft->removeLocalPlayerIdx(i);

                        SetAction(i, eAppAction_Idle);
                        return;
                    }
                    // flag to capture the save thumbnail
                    SetAction(i, eAppAction_ExitWorldCapturedThumbnail, param);

                    // Change the presence info
                    // Are we offline or online, and how many players are there

                    if (g_NetworkManager.GetPlayerCount() > 1) {
                        for (int j = 0; j < XUSER_MAX_COUNT; j++) {
                            if (pMinecraft->localplayers[j]) {
                                if (g_NetworkManager.IsLocalGame()) {
                                    PlatformGame.SetRichPresenceContext(
                                        j, CONTEXT_GAME_STATE_BLANK);
                                    PlatformProfile.SetCurrentGameActivity(
                                        j, CONTEXT_PRESENCE_MULTIPLAYEROFFLINE,
                                        false);
                                } else {
                                    PlatformGame.SetRichPresenceContext(
                                        j, CONTEXT_GAME_STATE_BLANK);
                                    PlatformProfile.SetCurrentGameActivity(
                                        j, CONTEXT_PRESENCE_MULTIPLAYER, false);
                                }
                            }
                        }
                    } else {
                        PlatformGame.SetRichPresenceContext(i, CONTEXT_GAME_STATE_BLANK);
                        if (g_NetworkManager.IsLocalGame()) {
                            PlatformProfile.SetCurrentGameActivity(
                                i, CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE,
                                false);
                        } else {
                            PlatformProfile.SetCurrentGameActivity(
                                i, CONTEXT_PRESENCE_MULTIPLAYER_1P, false);
                        }
                    }
                    break;
                case eAppAction_ExitWorldCapturedThumbnail: {
                    SetAction(i, eAppAction_Idle);
                    // Stop app running
                    SetGameStarted(false);
                    SetChangingSessionType(
                        true);  // Added to stop handling ethernet disconnects

                    ui.CloseAllPlayersScenes();

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    // 4J Stu - Fix for #12368 - Crash: Game crashes when saving
                    // then exiting and selecting to save
                    for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
                        // 4J Stu - Fix for #13257 - CRASH: Gameplay: Title
                        // crashed after exiting the tutorial It doesn't matter
                        // if they were in the tutorial already
                        pMinecraft->playerLeftTutorial(idx);
                    }

                    LoadingInputParams* loadingParams =
                        new LoadingInputParams();
                    loadingParams->func =
                        &UIScene_PauseMenu::ExitWorldThreadProc;
                    loadingParams->lpParam = param;

                    UIFullscreenProgressCompletionData* completionData =
                        new UIFullscreenProgressCompletionData();
                    // If param is non-null then this is a forced exit by the
                    // server, so make sure the player knows why 4J Stu -
                    // Changed - Don't use the FullScreenProgressScreen for
                    // action, use a dialog instead
                    completionData->bRequiresUserAction =
                        false;  //(param != nullptr) ? true : false;
                    completionData->bShowTips =
                        (param != nullptr) ? false : true;
                    completionData->bShowBackground = true;
                    completionData->bShowLogo = true;
                    completionData->type =
                        e_ProgressCompletion_NavigateToHomeMenu;
                    completionData->iPad = DEFAULT_XUI_MENU_USER;
                    loadingParams->completionData = completionData;

                    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_FullscreenProgress,
                                       loadingParams);
                } break;
                case eAppAction_ExitWorldTrial: {
                    SetAction(i, eAppAction_Idle);

                    pMinecraft->gui->clearMessages();

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    // Stop app running
                    SetGameStarted(false);

                    ui.CloseAllPlayersScenes();

                    // 4J Stu - Fix for #12368 - Crash: Game crashes when saving
                    // then exiting and selecting to save
                    for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
                        // 4J Stu - Fix for #13257 - CRASH: Gameplay: Title
                        // crashed after exiting the tutorial It doesn't matter
                        // if they were in the tutorial already
                        pMinecraft->playerLeftTutorial(idx);
                    }

                    LoadingInputParams* loadingParams =
                        new LoadingInputParams();
                    loadingParams->func =
                        &UIScene_PauseMenu::ExitWorldThreadProc;
                    loadingParams->lpParam = param;

                    UIFullscreenProgressCompletionData* completionData =
                        new UIFullscreenProgressCompletionData();
                    completionData->bShowBackground = true;
                    completionData->bShowLogo = true;
                    completionData->type =
                        e_ProgressCompletion_NavigateToHomeMenu;
                    completionData->iPad = DEFAULT_XUI_MENU_USER;
                    loadingParams->completionData = completionData;

                    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_FullscreenProgress,
                                       loadingParams);
                }

                break;
                case eAppAction_ExitTrial:
                    // XLaunchNewImage(XLAUNCH_KEYWORD_DASH_ARCADE, 0);
                    ExitGame();
                    break;

                case eAppAction_Respawn: {
                    ConnectionProgressParams* param =
                        new ConnectionProgressParams();
                    param->iPad = i;
                    param->stringId = IDS_PROGRESS_RESPAWNING;
                    param->showTooltips = false;
                    param->setFailTimer = false;
                    ui.NavigateToScene(i, eUIScene_ConnectingProgress, param);

                    // Need to reset this incase the player has already died and
                    // respawned
                    pMinecraft->localplayers[i]->SetPlayerRespawned(false);

                    SetAction(i, eAppAction_WaitForRespawnComplete);
                    if (app.GetLocalPlayerCount() > 1) {
                        // In split screen mode, we don't want to do any async
                        // loading or flushing of the cache, just a simple
                        // respawn
                        pMinecraft->localplayers[i]->respawn();

                        // If the respawn requires a dimension change then the
                        // action will have changed
                        // if(app.GetXuiAction(i) == eAppAction_Respawn)
                        //{
                        //	SetAction(i,eAppAction_Idle);
                        //	CloseXuiScenes(i);
                        //}
                    } else {
                        // SetAction(i,eAppAction_WaitForRespawnComplete);

                        // LoadingInputParams *loadingParams = new
                        // LoadingInputParams(); loadingParams->func =
                        // &CScene_Death::RespawnThreadProc;
                        // loadingParams->lpParam = (void*)i;

                        // Disable game & update thread whilst we do any of this
                        // app.SetGameStarted(false);
                        pMinecraft->gameRenderer->DisableUpdateThread();

                        // 4J Stu - We don't need this on a thread in
                        // multiplayer as respawning is asynchronous.
                        pMinecraft->localplayers[i]->respawn();

                        // app.SetGameStarted(true);
                        pMinecraft->gameRenderer->EnableUpdateThread();

                        // UIFullscreenProgressCompletionData *completionData =
                        // new UIFullscreenProgressCompletionData();
                        // completionData->bShowBackground=true;
                        // completionData->bShowLogo=true;
                        // completionData->type =
                        // e_ProgressCompletion_CloseUIScenes;
                        // completionData->iPad = i;
                        // loadingParams->completionData = completionData;

                        // app.NavigateToScene(i,eUIScene_FullscreenProgress,
                        // loadingParams, true);
                    }
                } break;
                case eAppAction_WaitForRespawnComplete:
                    player = pMinecraft->localplayers[i];
                    if (player != nullptr && player->GetPlayerRespawned()) {
                        SetAction(i, eAppAction_Idle);

                        if (ui.IsSceneInStack(i, eUIScene_EndPoem)) {
                            ui.NavigateBack(i, false, eUIScene_EndPoem);
                        } else {
                            ui.CloseUIScenes(i);
                        }

                        // clear the progress messages

                        // 					pMinecraft->progressRenderer->progressStart(-1);
                        // 					pMinecraft->progressRenderer->progressStage(-1);
                    } else if (!g_NetworkManager.IsInGameplay()) {
                        SetAction(i, eAppAction_Idle);
                    }
                    break;
                case eAppAction_WaitForDimensionChangeComplete:
                    player = pMinecraft->localplayers[i];
                    if (player != nullptr && player->connection &&
                        player->connection->isStarted()) {
                        SetAction(i, eAppAction_Idle);
                        ui.CloseUIScenes(i);
                    } else if (!g_NetworkManager.IsInGameplay()) {
                        SetAction(i, eAppAction_Idle);
                    }
                    break;
                case eAppAction_PrimaryPlayerSignedOut: {
                    // SetAction(i,eAppAction_Idle);

                    // clear the autosavetimer that might be displayed
                    ui.ShowAutosaveCountdownTimer(false);

                    // If the player signs out before the game started the
                    // server can be killed a bit earlier to stop the loading or
                    // saving of a new game continuing running while the
                    // UI/Guide is up
                    if (!app.GetGameStarted())
                        MinecraftServer::HaltServer(true);

                    // inform the player they are being returned to the menus
                    // because they signed out
                    PlatformStorage.SetSaveDeviceSelected(i, false);
                    // need to clear the player stats - can't assume it'll be
                    // done in setlevel - we may not be in the game
                    StatsCounter* pStats = Minecraft::GetInstance()->stats[i];
                    pStats->clear();

                    // 4J-PB - the libs will display the Returned to Title
                    // screen 					unsigned int
                    // uiIDA[1]; uiIDA[0]=IDS_CONFIRM_OK;
                    //
                    // 					ui.RequestMessageBox(IDS_RETURNEDTOMENU_TITLE,
                    // IDS_RETURNEDTOTITLESCREEN_TEXT, uiIDA, 1,
                    // i,&Game::PrimaryPlayerSignedOutReturned,this,app.GetStringTable());
                    if (g_NetworkManager.IsInSession()) {
                        app.SetAction(
                            i, eAppAction_PrimaryPlayerSignedOutReturned);
                    } else {
                        app.SetAction(
                            i, eAppAction_PrimaryPlayerSignedOutReturned_Menus);
                        MinecraftServer::resetFlags();
                    }
                } break;
                case eAppAction_EthernetDisconnected: {
                    app.DebugPrintf(
                        "Handling eAppAction_EthernetDisconnected\n");
                    SetAction(i, eAppAction_Idle);

                    // 4J Stu - Fix for #12530 -TCR 001 BAS Game Stability:
                    // Title will crash if the player disconnects while starting
                    // a new world and then opts to play the tutorial once they
                    // have been returned to the Main Menu.
                    if (!g_NetworkManager.IsLeavingGame()) {
                        app.DebugPrintf(
                            "Handling eAppAction_EthernetDisconnected - Not "
                            "leaving game\n");
                        // 4J-PB - not the same as a signout. We should only
                        // leave the game if this machine is not the host. We
                        // shouldn't get rid of the save device either.
                        if (g_NetworkManager.IsHost()) {
                            app.DebugPrintf(
                                "Handling eAppAction_EthernetDisconnected - Is "
                                "Host\n");
                            // If it's already a local game, then an ethernet
                            // disconnect should have no effect
                            if (!g_NetworkManager.IsLocalGame() &&
                                g_NetworkManager.IsInGameplay()) {
                                // Change the session to an offline session
                                SetAction(i, eAppAction_ChangeSessionType);
                            } else if (!g_NetworkManager.IsLocalGame() &&
                                       !g_NetworkManager.IsInGameplay()) {
                                // There are two cases here, either:
                                //	 1. We're early enough in the
                                // create/load game that we can do a really
                                // minimal shutdown or
                                //   2. We're far enough in (game has started
                                //   but the actual game started flag hasn't
                                //   been set) that we should just wait until
                                //   we're in the game and switch to offline
                                //   mode

                                // If there's a non-null level then, for our
                                // purposes, the game has started
                                bool gameStarted = false;
                                for (int j = 0; j < pMinecraft->levels.size();
                                     j++) {
                                    if (pMinecraft->levels.data()[j] !=
                                        nullptr) {
                                        gameStarted = true;
                                        break;
                                    }
                                }

                                if (!gameStarted) {
                                    // 1. Exit
                                    MinecraftServer::HaltServer();

                                    // Fix for #12530 - TCR 001 BAS Game
                                    // Stability: Title will crash if the player
                                    // disconnects while starting a new world
                                    // and then opts to play the tutorial once
                                    // they have been returned to the Main Menu.
                                    // 4J Stu - Leave the session
                                    g_NetworkManager.LeaveGame(false);

                                    // need to clear the player stats - can't
                                    // assume it'll be done in setlevel - we may
                                    // not be in the game
                                    StatsCounter* pStats =
                                        Minecraft::GetInstance()->stats[i];
                                    pStats->clear();
                                    unsigned int uiIDA[1];
                                    uiIDA[0] = IDS_CONFIRM_OK;

                                    ui.RequestErrorMessage(
                                        g_NetworkManager.CorrectErrorIDS(
                                            IDS_CONNECTION_LOST),
                                        g_NetworkManager.CorrectErrorIDS(
                                            IDS_CONNECTION_LOST_LIVE),
                                        uiIDA, 1, i,
                                        &Game::EthernetDisconnectReturned,
                                        this);
                                } else {
                                    // 2. Switch to offline
                                    SetAction(i, eAppAction_ChangeSessionType);
                                }
                            }
                        } else {
                            {
                                app.DebugPrintf(
                                    "Handling eAppAction_EthernetDisconnected "
                                    "- Not host\n");
                                // need to clear the player stats - can't assume
                                // it'll be done in setlevel - we may not be in
                                // the game
                                StatsCounter* pStats =
                                    Minecraft::GetInstance()->stats[i];
                                pStats->clear();
                                unsigned int uiIDA[1];
                                uiIDA[0] = IDS_CONFIRM_OK;

                                ui.RequestErrorMessage(
                                    g_NetworkManager.CorrectErrorIDS(
                                        IDS_CONNECTION_LOST),
                                    g_NetworkManager.CorrectErrorIDS(
                                        IDS_CONNECTION_LOST_LIVE),
                                    uiIDA, 1, i,
                                    &Game::EthernetDisconnectReturned, this);
                            }
                        }
                    }
                } break;
                    // We currently handle both these returns the same way.
                case eAppAction_EthernetDisconnectedReturned:
                case eAppAction_PrimaryPlayerSignedOutReturned: {
                    SetAction(i, eAppAction_Idle);

                    pMinecraft->gui->clearMessages();

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    // set the state back to pre-game
                    PlatformProfile.ResetProfileProcessState();

                    if (g_NetworkManager.IsLeavingGame()) {
                        // 4J Stu - If we are already leaving the game, then we
                        // just need to signal that the player signed out to
                        // stop saves
                        pMinecraft->progressRenderer->progressStartNoAbort(
                            IDS_EXITING_GAME);
                        pMinecraft->progressRenderer->progressStage(-1);
                        // This has no effect on client machines
                        MinecraftServer::HaltServer(true);
                    } else {
                        // Stop app running
                        SetGameStarted(false);

                        // turn off the gamertags in splitscreen for the primary
                        // player, since they are about to be made fullscreen
                        ui.HideAllGameUIElements();

                        ui.CloseAllPlayersScenes();

                        // 4J Stu - Fix for #12368 - Crash: Game crashes when
                        // saving then exiting and selecting to save
                        for (unsigned int idx = 0; idx < XUSER_MAX_COUNT;
                             ++idx) {
                            // 4J Stu - Fix for #13257 - CRASH: Gameplay: Title
                            // crashed after exiting the tutorial It doesn't
                            // matter if they were in the tutorial already
                            pMinecraft->playerLeftTutorial(idx);
                        }

                        LoadingInputParams* loadingParams =
                            new LoadingInputParams();
                        loadingParams->func = &Game::SignoutExitWorldThreadProc;

                        UIFullscreenProgressCompletionData* completionData =
                            new UIFullscreenProgressCompletionData();
                        completionData->bShowBackground = true;
                        completionData->bShowLogo = true;
                        completionData->iPad = DEFAULT_XUI_MENU_USER;
                        completionData->type =
                            e_ProgressCompletion_NavigateToHomeMenu;
                        loadingParams->completionData = completionData;

                        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                           eUIScene_FullscreenProgress,
                                           loadingParams);
                    }
                } break;
                case eAppAction_PrimaryPlayerSignedOutReturned_Menus:
                    SetAction(i, eAppAction_Idle);
                    // set the state back to pre-game
                    PlatformProfile.ResetProfileProcessState();
                    // clear the save device
                    PlatformStorage.SetSaveDeviceSelected(i, false);

                    ui.UpdatePlayerBasePositions();
                    // there are multiple layers in the help menu, so a navigate
                    // back isn't enough
                    ui.NavigateToHomeMenu();

                    break;
                case eAppAction_EthernetDisconnectedReturned_Menus:
                    SetAction(i, eAppAction_Idle);
                    // set the state back to pre-game
                    PlatformProfile.ResetProfileProcessState();

                    ui.UpdatePlayerBasePositions();

                    // there are multiple layers in the help menu, so a navigate
                    // back isn't enough
                    ui.NavigateToHomeMenu();

                    break;

                case eAppAction_TrialOver: {
                    SetAction(i, eAppAction_Idle);
                    unsigned int uiIDA[2];
                    uiIDA[0] = IDS_UNLOCK_TITLE;
                    uiIDA[1] = IDS_EXIT_GAME;

                    ui.RequestErrorMessage(IDS_TRIALOVER_TITLE,
                                           IDS_TRIALOVER_TEXT, uiIDA, 2, i,
                                           &Game::TrialOverReturned, this);
                } break;

                    // INVITES
                case eAppAction_DashboardTrialJoinFromInvite: {
                    SetAction(i, eAppAction_Idle);
                    unsigned int uiIDA[2];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    uiIDA[1] = IDS_CONFIRM_CANCEL;

                    ui.RequestErrorMessage(
                        IDS_UNLOCK_TITLE, IDS_UNLOCK_ACCEPT_INVITE, uiIDA, 2, i,
                        &Game::UnlockFullInviteReturned, this);
                } break;
                case eAppAction_ExitAndJoinFromInvite: {
                    unsigned int uiIDA[3];

                    SetAction(i, eAppAction_Idle);
                    // Check the player really wants to do this

                    if (!PlatformStorage.GetSaveDisabled() &&
                        i == PlatformProfile.GetPrimaryPad() &&
                        g_NetworkManager.IsHost() && GetGameStarted()) {
                        uiIDA[0] = IDS_CONFIRM_CANCEL;
                        uiIDA[1] = IDS_EXIT_GAME_SAVE;
                        uiIDA[2] = IDS_EXIT_GAME_NO_SAVE;

                        ui.RequestAlertMessage(
                            IDS_EXIT_GAME, IDS_CONFIRM_LEAVE_VIA_INVITE, uiIDA,
                            3, i,
                            &Game::ExitAndJoinFromInviteSaveDialogReturned,
                            this);
                    } else {
                        uiIDA[0] = IDS_CONFIRM_CANCEL;
                        uiIDA[1] = IDS_CONFIRM_OK;
                        ui.RequestAlertMessage(
                            IDS_EXIT_GAME, IDS_CONFIRM_LEAVE_VIA_INVITE, uiIDA,
                            2, i, &Game::ExitAndJoinFromInvite, this);
                    }
                } break;
                case eAppAction_ExitAndJoinFromInviteConfirmed: {
                    SetAction(i, eAppAction_Idle);

                    pMinecraft->gui->clearMessages();

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    // Stop app running
                    SetGameStarted(false);

                    ui.CloseAllPlayersScenes();

                    // 4J Stu - Fix for #12368 - Crash: Game crashes when saving
                    // then exiting and selecting to save
                    for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
                        // 4J Stu - Fix for #13257 - CRASH: Gameplay: Title
                        // crashed after exiting the tutorial It doesn't matter
                        // if they were in the tutorial already
                        pMinecraft->playerLeftTutorial(idx);
                    }

                    // 4J-PB - may have been using a texture pack with audio ,
                    // so clean up anything texture pack related here

                    // unload any texture pack audio
                    // if there is audio in use, clear out the audio, and
                    // unmount the pack
                    TexturePack* pTexPack =
                        Minecraft::GetInstance()->skins->getSelected();
                    DLCTexturePack* pDLCTexPack = nullptr;

                    if (pTexPack->hasAudio()) {
                        // get the dlc texture pack, and store it
                        pDLCTexPack = (DLCTexturePack*)pTexPack;
                    }

                    // change to the default texture pack
                    pMinecraft->skins->selectTexturePackById(
                        TexturePackRepository::DEFAULT_TEXTURE_PACK_ID);

                    if (pTexPack->hasAudio()) {
                        // need to stop the streaming audio - by playing
                        // streaming audio from the default texture pack now
                        // reset the streaming sounds back to the normal ones
                        static_cast<SoundEngine*>(pMinecraft->soundEngine)
                            ->SetStreamingSounds(eStream_Overworld_Calm1,
                                                 eStream_Overworld_piano3,
                                                 eStream_Nether1,
                                                 eStream_Nether4,
                                                 eStream_end_dragon,
                                                 eStream_end_end, eStream_CD_1);
                        pMinecraft->soundEngine->playStreaming("", 0, 0, 0, 1,
                                                               1);

                        const unsigned int result =
                            PlatformStorage.UnmountInstalledDLC("TPACK");
                        app.DebugPrintf("Unmount result is %d\n", result);
                    }

                    LoadingInputParams* loadingParams =
                        new LoadingInputParams();
                    loadingParams->func =
                        &CGameNetworkManager::ExitAndJoinFromInviteThreadProc;
                    loadingParams->lpParam = (void*)&m_InviteData;

                    UIFullscreenProgressCompletionData* completionData =
                        new UIFullscreenProgressCompletionData();
                    completionData->bShowBackground = true;
                    completionData->bShowLogo = true;
                    completionData->iPad = DEFAULT_XUI_MENU_USER;
                    completionData->type = e_ProgressCompletion_NoAction;
                    loadingParams->completionData = completionData;

                    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_FullscreenProgress,
                                       loadingParams);
                }

                break;
                case eAppAction_JoinFromInvite: {
                    SetAction(i, eAppAction_Idle);

                    // 4J Stu - Move this state block from
                    // IPlatformNetwork::ExitAndJoinFromInviteThreadProc,
                    // as g_NetworkManager.JoinGameFromInviteInfo ultimately can
                    // call NavigateToScene,
                    /// and we should only be calling that from the main thread
                    app.SetTutorialMode(false);

                    g_NetworkManager.SetLocalGame(false);

                    JoinFromInviteData* inviteData = (JoinFromInviteData*)param;
                    // 4J-PB - clear any previous connection errors
                    Minecraft::GetInstance()->clearConnectionFailed();

                    app.DebugPrintf(
                        "Changing Primary Pad on an invite accept - pad was "
                        "%d, and is now %d\n",
                        PlatformProfile.GetPrimaryPad(),
                        inviteData->dwUserIndex);
                    PlatformProfile.SetLockedProfile(inviteData->dwUserIndex);
                    PlatformProfile.SetPrimaryPad(inviteData->dwUserIndex);

                    // change the minecraft player name
                    Minecraft::GetInstance()->user->name =
                        PlatformProfile.GetGamertag(
                            PlatformProfile.GetPrimaryPad());

                    bool success = g_NetworkManager.JoinGameFromInviteInfo(
                        inviteData->dwUserIndex,       // dwUserIndex
                        inviteData->dwLocalUsersMask,  // dwUserMask
                        inviteData->pInviteInfo);      // pInviteInfo

                    if (!success) {
                        app.DebugPrintf("Failed joining game from invite\n");
                        // return hr;

                        // 4J Stu - Copied this from XUI_FullScreenProgress to
                        // properly handle the fail case, as the thread will no
                        // longer be failing
                        unsigned int uiIDA[1];
                        uiIDA[0] = IDS_CONFIRM_OK;
                        ui.RequestErrorMessage(
                            IDS_CONNECTION_FAILED, IDS_CONNECTION_LOST_SERVER,
                            uiIDA, 1, PlatformProfile.GetPrimaryPad());

                        ui.NavigateToHomeMenu();
                        ui.UpdatePlayerBasePositions();
                    }
                } break;
                case eAppAction_ChangeSessionType: {
                    // If we are not in gameplay yet, then wait until the server
                    // is setup before changing the session type
                    if (g_NetworkManager.IsInGameplay()) {
                        // This kicks off a thread that waits for the server to
                        // end, then closes the current session, starts a new
                        // one and joins the local players into it

                        SetAction(i, eAppAction_Idle);

                        if (!GetChangingSessionType() &&
                            !g_NetworkManager.IsLocalGame()) {
                            SetGameStarted(false);
                            SetChangingSessionType(true);
                            SetReallyChangingSessionType(true);

                            // turn off the gamertags in splitscreen for the
                            // primary player, since they are about to be made
                            // fullscreen
                            ui.HideAllGameUIElements();

                            if (!ui.IsSceneInStack(
                                    PlatformProfile.GetPrimaryPad(),
                                    eUIScene_EndPoem)) {
                                ui.CloseAllPlayersScenes();
                            }
                            ui.ShowOtherPlayersBaseScene(
                                PlatformProfile.GetPrimaryPad(), true);

                            // Remove this line to fix:
                            // #49084 - TU5: Code: Gameplay: The title crashes
                            // every time client navigates to 'Play game' menu
                            // and loads/creates new game after a "Connection to
                            // Xbox LIVE was lost" message has appeared.
                            // app.NavigateToScene(0,eUIScene_Main);

                            LoadingInputParams* loadingParams =
                                new LoadingInputParams();
                            loadingParams->func =
                                &CGameNetworkManager::
                                    ChangeSessionTypeThreadProc;
                            loadingParams->lpParam = nullptr;

                            UIFullscreenProgressCompletionData* completionData =
                                new UIFullscreenProgressCompletionData();
                            completionData->bRequiresUserAction = true;
                            completionData->bShowBackground = true;
                            completionData->bShowLogo = true;
                            completionData->iPad = DEFAULT_XUI_MENU_USER;
                            if (ui.IsSceneInStack(
                                    PlatformProfile.GetPrimaryPad(),
                                    eUIScene_EndPoem)) {
                                completionData->type =
                                    e_ProgressCompletion_NavigateBackToScene;
                                completionData->scene = eUIScene_EndPoem;
                            } else {
                                completionData->type =
                                    e_ProgressCompletion_CloseAllPlayersUIScenes;
                            }
                            loadingParams->completionData = completionData;

                            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                               eUIScene_FullscreenProgress,
                                               loadingParams);
                        }
                    } else if (g_NetworkManager.IsLeavingGame()) {
                        // If we are leaving the game, then ignore the state
                        // change
                        SetAction(i, eAppAction_Idle);
                    }
                } break;
                case eAppAction_SetDefaultOptions:
                    SetAction(i, eAppAction_Idle);
                    SetDefaultOptions((IPlatformProfile::PROFILESETTINGS*)param,
                                      i);

                    // if the profile data has been changed, then force a
                    // profile write It seems we're allowed to break the 5
                    // minute rule if it's the result of a user action
                    CheckGameSettingsChanged(true, i);

                    break;

                case eAppAction_RemoteServerSave: {
                    // If the remote server save has already finished, don't
                    // complete the action
                    if (GetGameStarted()) {
                        SetAction(PlatformProfile.GetPrimaryPad(),
                                  eAppAction_Idle);
                        break;
                    }

                    SetAction(i, eAppAction_WaitRemoteServerSaveComplete);

                    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                        ui.CloseUIScenes(i, true);
                    }

                    // turn off the gamertags in splitscreen for the primary
                    // player, since they are about to be made fullscreen
                    ui.HideAllGameUIElements();

                    LoadingInputParams* loadingParams =
                        new LoadingInputParams();
                    loadingParams->func = &Game::RemoteSaveThreadProc;
                    loadingParams->lpParam = nullptr;

                    UIFullscreenProgressCompletionData* completionData =
                        new UIFullscreenProgressCompletionData();
                    completionData->bRequiresUserAction = false;
                    completionData->bShowBackground = true;
                    completionData->bShowLogo = true;
                    completionData->iPad = DEFAULT_XUI_MENU_USER;
                    if (ui.IsSceneInStack(PlatformProfile.GetPrimaryPad(),
                                          eUIScene_EndPoem)) {
                        completionData->type =
                            e_ProgressCompletion_NavigateBackToScene;
                        completionData->scene = eUIScene_EndPoem;
                    } else {
                        completionData->type =
                            e_ProgressCompletion_CloseAllPlayersUIScenes;
                    }
                    loadingParams->completionData = completionData;

                    loadingParams->cancelFunc = &Game::ExitGameFromRemoteSave;
                    loadingParams->cancelText = IDS_TOOLTIPS_EXIT;

                    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_FullscreenProgress,
                                       loadingParams);
                } break;
                case eAppAction_WaitRemoteServerSaveComplete:
                    // Do nothing
                    break;
                case eAppAction_FailedToJoinNoPrivileges: {
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    IPlatformStorage::EMessageResult result =
                        ui.RequestErrorMessage(
                            IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                            IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT, uiIDA, 1,
                            PlatformProfile.GetPrimaryPad());
                    if (result != IPlatformStorage::EMessage_Busy)
                        SetAction(i, eAppAction_Idle);
                } break;
                case eAppAction_ProfileReadError:
                    // Return player to the main menu - code largely copied from
                    // that for handling eAppAction_PrimaryPlayerSignedOut,
                    // although I don't think we should have got as far as
                    // needing to halt the server, or running the game, before
                    // returning to the menu
                    if (!app.GetGameStarted())
                        MinecraftServer::HaltServer(true);

                    if (g_NetworkManager.IsInSession()) {
                        app.SetAction(
                            i, eAppAction_PrimaryPlayerSignedOutReturned);
                    } else {
                        app.SetAction(
                            i, eAppAction_PrimaryPlayerSignedOutReturned_Menus);
                        MinecraftServer::resetFlags();
                    }
                    break;

                case eAppAction_BanLevel: {
                    // It's possible that this state can get set after the game
                    // has been exited (e.g. by network disconnection) so we
                    // can't ban the level at that point
                    if (g_NetworkManager.IsInGameplay() &&
                        !g_NetworkManager.IsLeavingGame()) {
                        // primary player would exit the world, secondary would
                        // exit the player
                        if (PlatformProfile.GetPrimaryPad() == i) {
                            SetAction(i, eAppAction_ExitWorld);
                        } else {
                            SetAction(i, eAppAction_ExitPlayer);
                        }
                    }
                } break;
                case eAppAction_LevelInBanLevelList: {
                    unsigned int uiIDA[2];
                    uiIDA[0] = IDS_BUTTON_REMOVE_FROM_BAN_LIST;
                    uiIDA[1] = IDS_EXIT_GAME;

                    // pass in the gamertag format std::string
                    char wchFormat[40];
                    INetworkPlayer* player =
                        g_NetworkManager.GetLocalPlayerByUserIndex(i);

                    // If not the primary player, but the primary player has
                    // banned this level and decided not to unban then we may
                    // have left the game by now
                    if (player) {
                        snprintf(wchFormat, 40, "%s\n\n%%s",
                                 player->GetOnlineName());

                        IPlatformStorage::EMessageResult result =
                            ui.RequestErrorMessage(
                                IDS_BANNED_LEVEL_TITLE, IDS_PLAYER_BANNED_LEVEL,
                                uiIDA, 2, i, &Game::BannedLevelDialogReturned,
                                this, wchFormat);
                        if (result != IPlatformStorage::EMessage_Busy)
                            SetAction(i, eAppAction_Idle);
                    } else {
                        SetAction(i, eAppAction_Idle);
                    }
                } break;
                case eAppAction_DebugText:
                    // launch the xui for text entry
                    { SetAction(i, eAppAction_Idle); }
                    break;

                case eAppAction_ReloadTexturePack: {
                    SetAction(i, eAppAction_Idle);
                    Minecraft* pMinecraft = Minecraft::GetInstance();
                    pMinecraft->textures->reloadAll();
                    pMinecraft->skins->updateUI();

                    if (!pMinecraft->skins->isUsingDefaultSkin()) {
                        TexturePack* pTexturePack =
                            pMinecraft->skins->getSelected();

                        DLCPack* pDLCPack = pTexturePack->getDLCPack();

                        bool purchased = false;
                        // do we have a license?
                        if (pDLCPack &&
                            pDLCPack->hasPurchasedFile(
                                DLCManager::e_DLCType_Texture, "")) {
                            purchased = true;
                        }
                    }

                    // 4J-PB  - If the texture pack has audio, we need to switch
                    // to this
                    if (pMinecraft->skins->getSelected()->hasAudio()) {
                        Minecraft::GetInstance()->soundEngine->playStreaming(
                            "", 0, 0, 0, 1, 1);
                    }
                } break;

                case eAppAction_ReloadFont: {
                    app.DebugPrintf(
                        "[Consoles_App] eAppAction_ReloadFont, ingame='%s'.\n",
                        app.GetGameStarted() ? "Yes" : "No");

                    SetAction(i, eAppAction_Idle);

                    ui.SetTooltips(i, -1);

                    ui.ReloadSkin();
                    ui.StartReloadSkinThread();

                    ui.setCleanupOnReload();
                } break;

                case eAppAction_TexturePackRequired: {
                    unsigned int uiIDA[2];

                    uiIDA[0] = IDS_TEXTUREPACK_FULLVERSION;
                    uiIDA[1] = IDS_TEXTURE_PACK_TRIALVERSION;

                    // Give the player a warning about the texture pack missing
                    ui.RequestErrorMessage(
                        IDS_DLC_TEXTUREPACK_NOT_PRESENT_TITLE,
                        IDS_DLC_TEXTUREPACK_NOT_PRESENT, uiIDA, 2,
                        PlatformProfile.GetPrimaryPad(),
                        &Game::TexturePackDialogReturned, this);
                    SetAction(i, eAppAction_Idle);
                }

                break;
                default:
                    break;
            }
        }

        // Any TMS actions?

        eTMS = app.GetTMSAction(i);

        if (eTMS != eTMSAction_Idle) {
            switch (eTMS) {
                    // TMS++ actions
                case eTMSAction_TMSPP_RetrieveFiles_CreateLoad_SignInReturned:
                case eTMSAction_TMSPP_RetrieveFiles_RunPlayGame:
                    SetTMSAction(i, eTMSAction_TMSPP_UserFileList);
                    break;

                case eTMSAction_TMSPP_UserFileList:
                    // retrieve the file list first
                    SetTMSAction(i, eTMSAction_TMSPP_XUIDSFile);
                    break;
                case eTMSAction_TMSPP_XUIDSFile:
                    SetTMSAction(i, eTMSAction_TMSPP_DLCFile);

                    break;
                case eTMSAction_TMSPP_DLCFile:
                    SetTMSAction(i, eTMSAction_TMSPP_BannedListFile);
                    break;
                case eTMSAction_TMSPP_BannedListFile:
                    // If we have one in TMSPP, then we can assume we can ignore
                    // TMS
                    SetTMSAction(i, eTMSAction_TMS_RetrieveFiles_Complete);
                    break;

                    // SPECIAL CASE - where the user goes directly in to Help &
                    // Options from the main menu
                case eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions:
                case eTMSAction_TMSPP_RetrieveFiles_DLCMain:
                    // retrieve the file list first
                    SetTMSAction(i, eTMSAction_TMSPP_DLCFileOnly);
                    break;
                case eTMSAction_TMSPP_RetrieveUserFilelist_DLCFileOnly:
                    SetTMSAction(i, eTMSAction_TMSPP_DLCFileOnly);

                    break;

                case eTMSAction_TMSPP_DLCFileOnly:
                    SetTMSAction(i, eTMSAction_TMSPP_RetrieveFiles_Complete);
                    break;

                case eTMSAction_TMSPP_RetrieveFiles_Complete:
                    SetTMSAction(i, eTMSAction_Idle);
                    break;

                    // TMS files
                    /*			case
                    eTMSAction_TMS_RetrieveFiles_CreateLoad_SignInReturned: case
                    eTMSAction_TMS_RetrieveFiles_RunPlayGame: #ifdef 0
                    SetTMSAction(i,eTMSAction_TMS_XUIDSFile_Waiting);
                    // pass in the next app action on the call or callback
                    completing
                    app.ReadXuidsFileFromTMS(i,eTMSAction_TMS_DLCFile,true);
                    #else
                    SetTMSAction(i,eTMSAction_TMS_DLCFile);
                    #endif
                    break;

                    case eTMSAction_TMS_DLCFile:
                    SetTMSAction(i,eTMSAction_TMS_BannedListFile);

                    break;

                    case eTMSAction_TMS_RetrieveFiles_HelpAndOptions:
                    case eTMSAction_TMS_RetrieveFiles_DLCMain:
                    SetTMSAction(i,eTMSAction_Idle);

                    break;
                    case eTMSAction_TMS_BannedListFile:

                    break;

                    */
                case eTMSAction_TMS_RetrieveFiles_Complete:
                    SetTMSAction(i, eTMSAction_Idle);
                    // 				if(PlatformStorage.SetSaveDevice(&CScene_Main::DeviceSelectReturned,pClass))
                    // 				{
                    // 					// save device already
                    // selected
                    // 					// ensure we've applied
                    // this player's settings
                    // 					app.ApplyGameSettingsChanged(PlatformProfile.GetPrimaryPad());
                    // 					app.NavigateToScene(PlatformProfile.GetPrimaryPad(),eUIScene_MultiGameJoinLoad);
                    // 				}
                    break;
                default:
                    break;
            }
        }
    }
}

// loadMediaArchive and loadStringTable moved to
// ArchiveManager/LocalizationManager
