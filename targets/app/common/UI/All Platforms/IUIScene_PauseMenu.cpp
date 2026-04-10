#include "IUIScene_PauseMenu.h"

#include <stdint.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/GameRules/GameRuleManager.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/ServerAction.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "platform/profile/profile.h"
#include "strings.h"

class TexturePack;

int IUIScene_PauseMenu::ExitGameDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    IUIScene_PauseMenu* pScene = dynamic_cast<IUIScene_PauseMenu*>(
        ui.GetSceneFromCallbackId((std::size_t)pParam));

    // Results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        if (pScene) pScene->SetIgnoreInput(true);
        app.SetAction(iPad, eAppAction_ExitWorld);
    }
    return 0;
}

int IUIScene_PauseMenu::ExitGameSaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    IUIScene_PauseMenu* pScene = dynamic_cast<IUIScene_PauseMenu*>(
        ui.GetSceneFromCallbackId((std::size_t)pParam));

    // Exit with or without saving
    // Decline means save in this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline ||
        result == IPlatformStorage::EMessage_ResultThirdOption) {
        if (result == IPlatformStorage::EMessage_ResultDecline)  // Save
        {
            // 4J-PB - Is the player trying to save but they are using a trial
            // texturepack ?
            if (!Minecraft::GetInstance()->skins->isUsingDefaultSkin()) {
                TexturePack* tPack =
                    Minecraft::GetInstance()->skins->getSelected();
                DLCTexturePack* pDLCTexPack = (DLCTexturePack*)tPack;

                DLCPack* pDLCPack =
                    pDLCTexPack
                        ->getDLCInfoParentPack();  // tPack->getDLCPack();
                if (!pDLCPack->hasPurchasedFile(DLCManager::e_DLCType_Texture,
                                                "")) {
                    unsigned int uiIDA[2];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    uiIDA[1] = IDS_CONFIRM_CANCEL;

                    // Give the player a warning about the trial version of the
                    // texture pack
                    ui.RequestAlertMessage(
                        IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE,
                        IDS_WARNING_DLC_TRIALTEXTUREPACK_TEXT, uiIDA, 2,
                        PlatformProfile.GetPrimaryPad(),
                        &IUIScene_PauseMenu::WarningTrialTexturePackReturned,
                        pParam);

                    return 0;
                }
            }

            // does the save exist?
            bool bSaveExists;
            PlatformStorage.DoesSaveExist(&bSaveExists);
            // 4J-PB - we check if the save exists inside the libs
            // we need to ask if they are sure they want to overwrite the
            // existing game
            if (bSaveExists) {
                unsigned int uiIDA[2];
                uiIDA[0] = IDS_CONFIRM_CANCEL;
                uiIDA[1] = IDS_CONFIRM_OK;
                ui.RequestAlertMessage(
                    IDS_TITLE_SAVE_GAME, IDS_CONFIRM_SAVE_GAME, uiIDA, 2,
                    PlatformProfile.GetPrimaryPad(),
                    &IUIScene_PauseMenu::ExitGameAndSaveReturned, pParam);
                return 0;
            } else {
                MinecraftServer::getInstance()->setSaveOnExit(true);
            }
        } else {
            // been a few requests for a confirm on exit without saving
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_CONFIRM_OK;
            ui.RequestAlertMessage(
                IDS_TITLE_DECLINE_SAVE_GAME, IDS_CONFIRM_DECLINE_SAVE_GAME,
                uiIDA, 2, PlatformProfile.GetPrimaryPad(),
                &IUIScene_PauseMenu::ExitGameDeclineSaveReturned, pParam);
            return 0;
        }

        if (pScene) pScene->SetIgnoreInput(true);

        app.SetAction(iPad, eAppAction_ExitWorld);
    }
    return 0;
}

int IUIScene_PauseMenu::ExitGameAndSaveReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    // 4J-PB - we won't come in here if we have a trial texture pack
    IUIScene_PauseMenu* pScene = dynamic_cast<IUIScene_PauseMenu*>(
        ui.GetSceneFromCallbackId((std::size_t)pParam));

    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        // int32_t saveOrCheckpointId = 0;
        // bool validSave =
        // PlatformStorage.GetSaveUniqueNumber(&saveOrCheckpointId);
        // SentientManager.RecordLevelSaveOrCheckpoint(PlatformProfile.GetPrimaryPad(),
        // saveOrCheckpointId);
        if (pScene) pScene->SetIgnoreInput(true);
        MinecraftServer::getInstance()->setSaveOnExit(true);
        // flag a app action of exit game
        app.SetAction(iPad, eAppAction_ExitWorld);
    } else {
        // has someone disconnected the ethernet here, causing the pause menu to
        // shut?
        if (ui.IsPauseMenuDisplayed(PlatformProfile.GetPrimaryPad())) {
            unsigned int uiIDA[3];
            // you cancelled the save on exit after choosing exit and save? You
            // go back to the Exit choices then.
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_EXIT_GAME_SAVE;
            uiIDA[2] = IDS_EXIT_GAME_NO_SAVE;

            if (g_NetworkManager.GetPlayerCount() > 1) {
                ui.RequestAlertMessage(
                    IDS_EXIT_GAME,
                    IDS_CONFIRM_EXIT_GAME_CONFIRM_DISCONNECT_SAVE, uiIDA, 3,
                    PlatformProfile.GetPrimaryPad(),
                    &IUIScene_PauseMenu::ExitGameSaveDialogReturned, pParam);
            } else {
                ui.RequestAlertMessage(
                    IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 3,
                    PlatformProfile.GetPrimaryPad(),
                    &IUIScene_PauseMenu::ExitGameSaveDialogReturned, pParam);
            }
        }
    }
    return 0;
}

int IUIScene_PauseMenu::ExitGameDeclineSaveReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    IUIScene_PauseMenu* pScene = dynamic_cast<IUIScene_PauseMenu*>(
        ui.GetSceneFromCallbackId((std::size_t)pParam));

    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        if (pScene) pScene->SetIgnoreInput(true);
        MinecraftServer::getInstance()->setSaveOnExit(false);
        // flag a app action of exit game
        app.SetAction(iPad, eAppAction_ExitWorld);
    } else {
        // has someone disconnected the ethernet here, causing the pause menu to
        // shut?
        if (ui.IsPauseMenuDisplayed(PlatformProfile.GetPrimaryPad())) {
            unsigned int uiIDA[3];
            // you cancelled the save on exit after choosing exit and save? You
            // go back to the Exit choices then.
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_EXIT_GAME_SAVE;
            uiIDA[2] = IDS_EXIT_GAME_NO_SAVE;

            if (g_NetworkManager.GetPlayerCount() > 1) {
                ui.RequestAlertMessage(
                    IDS_EXIT_GAME,
                    IDS_CONFIRM_EXIT_GAME_CONFIRM_DISCONNECT_SAVE, uiIDA, 3,
                    PlatformProfile.GetPrimaryPad(),
                    &IUIScene_PauseMenu::ExitGameSaveDialogReturned, pParam);
            } else {
                ui.RequestAlertMessage(
                    IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 3,
                    PlatformProfile.GetPrimaryPad(),
                    &IUIScene_PauseMenu::ExitGameSaveDialogReturned, pParam);
            }
        }
    }
    return 0;
}

int IUIScene_PauseMenu::WarningTrialTexturePackReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    return 0;
}

int IUIScene_PauseMenu::SaveWorldThreadProc(void* lpParameter) {
    bool bAutosave = (bool)lpParameter;
    MinecraftServer::getInstance()->queueServerAction(
        minecraft::server::SaveGame{bAutosave});

    // Share AABB & Vec3 pools with default (main thread) - should be ok as long
    // as we don't tick the main thread whilst this thread is running
    Compression::UseDefaultThreadStorage();

    Minecraft* pMinecraft = Minecraft::GetInstance();

    // printf("Loading world on thread\n");

    app.SetGameStarted(false);

    if (!MinecraftServer::serverHalted() && !app.GetChangingSessionType())
        app.SetGameStarted(true);

    int32_t hr = 0;
    if (app.GetChangingSessionType()) {
        // 4J Stu - This causes the fullscreenprogress scene to ignore the
        // action it was given
        hr = 1223;  // ERROR_CANCELLED
    }
    return hr;
}

int IUIScene_PauseMenu::ExitWorldThreadProc(void* lpParameter) {
    // Share AABB & Vec3 pools with default (main thread) - should be ok as long
    // as we don't tick the main thread whilst this thread is running
    Compression::UseDefaultThreadStorage();

    // app.SetGameStarted(false);

    _ExitWorld(lpParameter);

    return 0;
}

// This function performs the meat of exiting from a level. It should be called
// from a thread other than the main thread.
void IUIScene_PauseMenu::_ExitWorld(void* lpParameter) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    int exitReasonStringId = pMinecraft->progressRenderer->getCurrentTitle();
    int exitReasonTitleId = IDS_CONNECTION_LOST;

    bool saveStats = true;
    if (pMinecraft->isClientSide() || g_NetworkManager.IsInSession()) {
        if (lpParameter != nullptr) {
            // 4J-PB - check if we have lost connection to Live
            // if (PlatformProfile.GetLiveConnectionStatus() !=
            //     XONLINE_S_LOGON_CONNECTION_ESTABLISHED) {
            //     exitReasonStringId = IDS_CONNECTION_LOST_LIVE;
            // } else {
            switch (app.GetDisconnectReason()) {
                case DisconnectPacket::eDisconnect_Kicked:
                    exitReasonStringId = IDS_DISCONNECTED_KICKED;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_AllLocal:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;
                    exitReasonTitleId = IDS_CONNECTION_FAILED;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_Single_Local:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
                    exitReasonTitleId = IDS_CONNECTION_FAILED;
                    break;
                case DisconnectPacket::eDisconnect_NoFlying:
                    exitReasonStringId = IDS_DISCONNECTED_FLYING;
                    break;
                case DisconnectPacket::eDisconnect_Quitting:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_QUIT;
                    break;
                case DisconnectPacket::eDisconnect_NoFriendsInGame:
                    exitReasonStringId = IDS_DISCONNECTED_NO_FRIENDS_IN_GAME;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_Banned:
                    exitReasonStringId = IDS_DISCONNECTED_BANNED;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_NotFriendsWithHost:
                    exitReasonStringId = IDS_NOTALLOWED_FRIENDSOFFRIENDS;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedServer:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_OLD;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedClient:
                    exitReasonStringId = IDS_DISCONNECTED_CLIENT_OLD;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_ServerFull:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_FULL;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;

                default:
                    exitReasonStringId = IDS_CONNECTION_LOST_SERVER;
            }
            // }
            // pMinecraft->progressRenderer->progressStartNoAbort(
            // exitReasonStringId );

            unsigned int uiIDA[1];
            uiIDA[0] = IDS_CONFIRM_OK;
            // 4J Stu - Fix for #48669 - TU5: Code: Compliance: TCR #15:
            // Incorrect/misleading messages after signing out a profile during
            // online game session. If the primary player is signed out, then
            // that is most likely the cause of the disconnection so don't
            // display a message box. This will allow the message box requested
            // by the libraries to be brought up
            if (PlatformProfile.IsSignedIn(PlatformProfile.GetPrimaryPad()))
                ui.RequestErrorMessage(exitReasonTitleId, exitReasonStringId,
                                       uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
            exitReasonStringId = -1;

            // 4J - Force a disconnection, this handles the situation that the
            // server has already disconnected
            if (pMinecraft->levels[0] != nullptr)
                pMinecraft->levels[0]->disconnect(false);
            if (pMinecraft->levels[1] != nullptr)
                pMinecraft->levels[1]->disconnect(false);
            if (pMinecraft->levels[2] != nullptr)
                pMinecraft->levels[2]->disconnect(false);
        } else {
            exitReasonStringId = IDS_EXITING_GAME;
            pMinecraft->progressRenderer->progressStartNoAbort(
                IDS_EXITING_GAME);
            if (pMinecraft->levels[0] != nullptr)
                pMinecraft->levels[0]->disconnect();
            if (pMinecraft->levels[1] != nullptr)
                pMinecraft->levels[1]->disconnect();
            if (pMinecraft->levels[2] != nullptr)
                pMinecraft->levels[2]->disconnect();
        }

        // 4J Stu - This only does something if we actually have a server, so
        // don't need to do any other checks
        MinecraftServer::HaltServer();

        // We need to call the stats & leaderboards save before we exit the
        // session 4J We need to do this in a QNet callback where it is safe
        // pMinecraft->forceStatsSave();
        saveStats = false;

        // 4J Stu - Leave the session once the disconnect packet has been sent
        g_NetworkManager.LeaveGame(false);
    } else {
        if (lpParameter != nullptr &&
            PlatformProfile.IsSignedIn(PlatformProfile.GetPrimaryPad())) {
            switch (app.GetDisconnectReason()) {
                case DisconnectPacket::eDisconnect_Kicked:
                    exitReasonStringId = IDS_DISCONNECTED_KICKED;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_AllLocal:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;
                    exitReasonTitleId = IDS_CONNECTION_FAILED;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_Single_Local:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
                    exitReasonTitleId = IDS_CONNECTION_FAILED;
                    break;
                case DisconnectPacket::eDisconnect_Quitting:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_QUIT;
                    break;
                case DisconnectPacket::eDisconnect_NoMultiplayerPrivilegesJoin:
                    exitReasonStringId = IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedServer:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_OLD;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedClient:
                    exitReasonStringId = IDS_DISCONNECTED_CLIENT_OLD;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                case DisconnectPacket::eDisconnect_ServerFull:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_FULL;
                    exitReasonTitleId = IDS_CANTJOIN_TITLE;
                    break;
                default:
                    exitReasonStringId = IDS_DISCONNECTED;
            }
            // pMinecraft->progressRenderer->progressStartNoAbort(
            // exitReasonStringId );

            unsigned int uiIDA[1];
            uiIDA[0] = IDS_CONFIRM_OK;
            ui.RequestErrorMessage(exitReasonTitleId, exitReasonStringId, uiIDA,
                                   1, PlatformProfile.GetPrimaryPad());
            exitReasonStringId = -1;
        }
    }
    // Fix for #93148 - TCR 001: BAS Game Stability: Title will crash for the
    // multiplayer client if host of the game will exit during the clients
    // loading to created world.
    while (g_NetworkManager.IsNetworkThreadRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    pMinecraft->setLevel(nullptr, exitReasonStringId, nullptr, saveStats);

    app.m_gameRules.unloadCurrentGameRules();
    // app.m_Audio.unloadCurrentAudioDetails();

    MinecraftServer::resetFlags();

    // Fix for #48385 - BLACK OPS :TU5: Functional: Client becomes pseudo
    // soft-locked when returned to the main menu after a remote disconnect Make
    // sure there is text explaining why the player is waiting
    pMinecraft->progressRenderer->progressStart(IDS_EXITING_GAME);

    // Fix for #13259 - CRASH: Gameplay: loading process is halted when player
    // loads saved data We can't start/join a new game until the session is
    // destroyed, so wait for it to be idle again
    while (g_NetworkManager.IsInSession()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    app.SetChangingSessionType(false);
    app.SetReallyChangingSessionType(false);
    pMinecraft->exitingWorldRightNow = false;
}

int IUIScene_PauseMenu::SaveGameDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        // flag a app action of save game
        app.SetAction(iPad, eAppAction_SaveGame);
    }
    return 0;
}

int IUIScene_PauseMenu::EnableAutosaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        // Set the global flag, so that we don't disable saving again once the
        // save is complete
        app.SetGameHostOption(eGameHostOption_DisableSaving, 0);
    } else {
        // Set the global flag, so that we do disable saving again once the save
        // is complete We need to set this on as we may have only disabled it
        // due to having a trial texture pack
        app.SetGameHostOption(eGameHostOption_DisableSaving, 1);
    }
    // Re-enable saving temporarily
    PlatformStorage.SetSaveDisabled(false);

    // flag a app action of save game
    app.SetAction(iPad, eAppAction_SaveGame);
    return 0;
}

int IUIScene_PauseMenu::DisableAutosaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        // Set the global flag, so that we disable saving again once the save is
        // complete
        app.SetGameHostOption(eGameHostOption_DisableSaving, 1);
        PlatformStorage.SetSaveDisabled(false);

        // flag a app action of save game
        app.SetAction(iPad, eAppAction_SaveGame);
    }
    return 0;
}