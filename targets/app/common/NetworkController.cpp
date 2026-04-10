#include "app/common/NetworkController.h"

#include <chrono>
#include <cstring>
#include <thread>

#include "app/common/DLC/DLCPack.h"
#include "app/common/Game.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"

unsigned int NetworkController::m_uiLastSignInData = 0;

NetworkController::NetworkController() {
    m_disconnectReason = DisconnectPacket::eDisconnect_None;
    m_bLiveLinkRequired = false;
    m_bChangingSessionType = false;
    m_bReallyChangingSessionType = false;

    memset(&m_InviteData, 0, sizeof(JoinFromInviteData));
    memset(m_playerColours, 0, MINECRAFT_NET_MAX_PLAYERS);
    memset(m_playerGamePrivileges, 0, sizeof(m_playerGamePrivileges));

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        if (XUserGetSigninInfo(i, XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY,
                               &m_currentSigninInfo[i]) < 0) {
            m_currentSigninInfo[i].xuid = INVALID_XUID;
            m_currentSigninInfo[i].dwGuestNumber = 0;
        }
    }
}

void NetworkController::updatePlayerInfo(std::uint8_t networkSmallId,
                                         int16_t playerColourIndex,
                                         unsigned int playerGamePrivileges) {
    for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i) {
        if (m_playerColours[i] == networkSmallId) {
            m_playerColours[i] = 0;
            m_playerGamePrivileges[i] = 0;
        }
    }
    if (playerColourIndex >= 0 &&
        playerColourIndex < MINECRAFT_NET_MAX_PLAYERS) {
        m_playerColours[playerColourIndex] = networkSmallId;
        m_playerGamePrivileges[playerColourIndex] = playerGamePrivileges;
    }
}

short NetworkController::getPlayerColour(std::uint8_t networkSmallId) {
    short index = -1;
    for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i) {
        if (m_playerColours[i] == networkSmallId) {
            index = i;
            break;
        }
    }
    return index;
}

unsigned int NetworkController::getPlayerPrivileges(
    std::uint8_t networkSmallId) {
    unsigned int privileges = 0;
    for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i) {
        if (m_playerColours[i] == networkSmallId) {
            privileges = m_playerGamePrivileges[i];
            break;
        }
    }
    return privileges;
}

void NetworkController::processInvite(std::uint32_t dwUserIndex,
                                      std::uint32_t dwLocalUsersMask,
                                      const INVITE_INFO* pInviteInfo) {
    m_InviteData.dwUserIndex = dwUserIndex;
    m_InviteData.dwLocalUsersMask = dwLocalUsersMask;
    m_InviteData.pInviteInfo = pInviteInfo;
    app.SetAction(dwUserIndex, eAppAction_ExitAndJoinFromInvite);
}

int NetworkController::primaryPlayerSignedOutReturned(
    void* pParam, int iPad, const IPlatformStorage::EMessageResult) {
    if (g_NetworkManager.IsInSession()) {
        app.SetAction(iPad, eAppAction_PrimaryPlayerSignedOutReturned);
    } else {
        app.SetAction(iPad, eAppAction_PrimaryPlayerSignedOutReturned_Menus);
    }
    return 0;
}

int NetworkController::ethernetDisconnectReturned(
    void* pParam, int iPad, const IPlatformStorage::EMessageResult) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (Minecraft::GetInstance()->player != nullptr) {
        app.SetAction(pMinecraft->player->GetXboxPad(),
                      eAppAction_EthernetDisconnectedReturned);
    } else {
        app.SetAction(iPad, eAppAction_EthernetDisconnectedReturned_Menus);
    }
    return 0;
}

void NetworkController::profileReadErrorCallback(void* pParam) {
    Game* pApp = (Game*)pParam;
    int iPrimaryPlayer = PlatformProfile.GetPrimaryPad();
    pApp->SetAction(iPrimaryPlayer, eAppAction_ProfileReadError);
}

int NetworkController::signoutExitWorldThreadProc(void* lpParameter) {
    Compression::UseDefaultThreadStorage();

    Minecraft* pMinecraft = Minecraft::GetInstance();

    int exitReasonStringId = -1;

    bool saveStats = false;
    if (pMinecraft->isClientSide() || g_NetworkManager.IsInSession()) {
        if (lpParameter != nullptr) {
            switch (app.GetDisconnectReason()) {
                case DisconnectPacket::eDisconnect_Kicked:
                    exitReasonStringId = IDS_DISCONNECTED_KICKED;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_AllLocal:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_Single_Local:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
                    break;
                case DisconnectPacket::eDisconnect_NoFlying:
                    exitReasonStringId = IDS_DISCONNECTED_FLYING;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedServer:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_OLD;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedClient:
                    exitReasonStringId = IDS_DISCONNECTED_CLIENT_OLD;
                    break;
                default:
                    exitReasonStringId = IDS_DISCONNECTED;
            }
            pMinecraft->progressRenderer->progressStartNoAbort(
                exitReasonStringId);
            if (pMinecraft->levels[0] != nullptr)
                pMinecraft->levels[0]->disconnect(false);
            if (pMinecraft->levels[1] != nullptr)
                pMinecraft->levels[1]->disconnect(false);
        } else {
            exitReasonStringId = IDS_EXITING_GAME;
            pMinecraft->progressRenderer->progressStartNoAbort(
                IDS_EXITING_GAME);

            if (pMinecraft->levels[0] != nullptr)
                pMinecraft->levels[0]->disconnect();
            if (pMinecraft->levels[1] != nullptr)
                pMinecraft->levels[1]->disconnect();
        }

        MinecraftServer::HaltServer(true);
        saveStats = false;
        g_NetworkManager.LeaveGame(false);
    } else {
        if (lpParameter != nullptr) {
            switch (app.GetDisconnectReason()) {
                case DisconnectPacket::eDisconnect_Kicked:
                    exitReasonStringId = IDS_DISCONNECTED_KICKED;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_AllLocal:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;
                    break;
                case DisconnectPacket::eDisconnect_NoUGC_Single_Local:
                    exitReasonStringId =
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedServer:
                    exitReasonStringId = IDS_DISCONNECTED_SERVER_OLD;
                    break;
                case DisconnectPacket::eDisconnect_OutdatedClient:
                    exitReasonStringId = IDS_DISCONNECTED_CLIENT_OLD;
                default:
                    exitReasonStringId = IDS_DISCONNECTED;
            }
            pMinecraft->progressRenderer->progressStartNoAbort(
                exitReasonStringId);
        }
    }
    pMinecraft->setLevel(nullptr, exitReasonStringId, nullptr, saveStats, true);

    app.m_gameRules.unloadCurrentGameRules();

    MinecraftServer::resetFlags();

    while (g_NetworkManager.IsInSession()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

void NetworkController::clearSignInChangeUsersMask() {
    int iPrimaryPlayer = PlatformProfile.GetPrimaryPad();

    if (m_uiLastSignInData != 0) {
        if (iPrimaryPlayer >= 0) {
            m_uiLastSignInData = 1 << iPrimaryPlayer;
        } else {
            m_uiLastSignInData = 0;
        }
    }
}

void NetworkController::signInChangeCallback(void* pParam,
                                             bool bPrimaryPlayerChanged,
                                             unsigned int uiSignInData) {
    Game* pApp = (Game*)pParam;
    int iPrimaryPlayer = PlatformProfile.GetPrimaryPad();

    if ((PlatformProfile.GetLockedProfile() != -1) && iPrimaryPlayer != -1) {
        if (((uiSignInData & (1 << iPrimaryPlayer)) == 0) ||
            bPrimaryPlayerChanged) {
            pApp->SetAction(iPrimaryPlayer, eAppAction_PrimaryPlayerSignedOut);
            pApp->InvalidateBannedList(iPrimaryPlayer);
            PlatformStorage.ClearDLCOffers();
            pApp->ClearAndResetDLCDownloadQueue();
            pApp->ClearDLCInstalled();
        } else {
            unsigned int uiChangedPlayers = uiSignInData ^ m_uiLastSignInData;

            if (g_NetworkManager.IsInSession()) {
                bool hasGuestIdChanged = false;
                for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                    unsigned int guestNumber = 0;
                    if (PlatformProfile.IsSignedIn(i)) {
                        XUSER_SIGNIN_INFO info;
                        XUserGetSigninInfo(
                            i, XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY, &info);
                        pApp->DebugPrintf(
                            "Player at index %d has guest number %d\n", i,
                            info.dwGuestNumber);
                        guestNumber = info.dwGuestNumber;
                    }
                    if (pApp->m_networkController.m_currentSigninInfo[i]
                                .dwGuestNumber != 0 &&
                        guestNumber != 0 &&
                        pApp->m_networkController.m_currentSigninInfo[i]
                                .dwGuestNumber != guestNumber) {
                        hasGuestIdChanged = true;
                    }
                }

                if (hasGuestIdChanged) {
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    ui.RequestErrorMessage(IDS_GUEST_ORDER_CHANGED_TITLE,
                                           IDS_GUEST_ORDER_CHANGED_TEXT, uiIDA,
                                           1, PlatformProfile.GetPrimaryPad());
                }

                bool switchToOffline = false;
                if (!PlatformProfile.IsSignedInLive(
                        PlatformProfile.GetLockedProfile()) &&
                    !g_NetworkManager.IsLocalGame()) {
                    switchToOffline = true;
                }

                for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                    if (i == iPrimaryPlayer) continue;

                    if (hasGuestIdChanged &&
                        pApp->m_networkController.m_currentSigninInfo[i]
                                .dwGuestNumber != 0 &&
                        g_NetworkManager.GetLocalPlayerByUserIndex(i) !=
                            nullptr) {
                        pApp->DebugPrintf(
                            "Recommending removal of player at index %d "
                            "because their guest id changed\n",
                            i);
                        pApp->SetAction(i, eAppAction_ExitPlayer);
                    } else {
                        XUSER_SIGNIN_INFO info;
                        XUserGetSigninInfo(
                            i, XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY, &info);

                        bool bPlayerChanged =
                            (uiChangedPlayers & (1 << i)) == (1 << i);
                        bool bPlayerSignedIn = ((uiSignInData & (1 << i)) != 0);

                        if (bPlayerChanged &&
                            (!bPlayerSignedIn ||
                             (bPlayerSignedIn && !PlatformProfile.AreXUIDSEqual(
                                                     pApp->m_networkController
                                                         .m_currentSigninInfo[i]
                                                         .xuid,
                                                     info.xuid)))) {
                            pApp->DebugPrintf(
                                "Player at index %d Left - invalidating their "
                                "banned list\n",
                                i);
                            pApp->InvalidateBannedList(i);

                            if (g_NetworkManager.GetLocalPlayerByUserIndex(i) !=
                                    nullptr ||
                                Minecraft::GetInstance()->localplayers[i] !=
                                    nullptr) {
                                pApp->DebugPrintf("Player %d signed out\n", i);
                                pApp->SetAction(i, eAppAction_ExitPlayer);
                            }
                        }
                    }
                }

                if (switchToOffline) {
                    pApp->SetAction(iPrimaryPlayer,
                                    eAppAction_EthernetDisconnected);
                }

                g_NetworkManager.HandleSignInChange();
            } else if (pApp->GetLiveLinkRequired() &&
                       !PlatformProfile.IsSignedInLive(
                           PlatformProfile.GetLockedProfile())) {
                {
                    pApp->SetAction(iPrimaryPlayer,
                                    eAppAction_EthernetDisconnected);
                }
            }
        }
        m_uiLastSignInData = uiSignInData;
    } else if (iPrimaryPlayer != -1) {
        pApp->InvalidateBannedList(iPrimaryPlayer);
        PlatformStorage.ClearDLCOffers();
        pApp->ClearAndResetDLCDownloadQueue();
        pApp->ClearDLCInstalled();
    }

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (XUserGetSigninInfo(
                i, XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY,
                &pApp->m_networkController.m_currentSigninInfo[i]) < 0) {
            pApp->m_networkController.m_currentSigninInfo[i].xuid =
                INVALID_XUID;
            pApp->m_networkController.m_currentSigninInfo[i].dwGuestNumber = 0;
        }
        app.DebugPrintf(
            "Player at index %d has guest number %d\n", i,
            pApp->m_networkController.m_currentSigninInfo[i].dwGuestNumber);
    }
}

void NetworkController::notificationsCallback(void* pParam,
                                              std::uint32_t dwNotification,
                                              unsigned int uiParam) {
    Game* pClass = (Game*)pParam;

    PNOTIFICATION pNotification = new NOTIFICATION;
    pNotification->dwNotification = dwNotification;
    pNotification->uiParam = uiParam;

    switch (dwNotification) {
        case XN_SYS_SIGNINCHANGED: {
            pClass->DebugPrintf("Signing changed - %d\n", uiParam);
        } break;
        case XN_SYS_INPUTDEVICESCHANGED:
            if (app.GetGameStarted() && g_NetworkManager.IsInSession()) {
                for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                    if (!PlatformInput.IsPadConnected(i) &&
                        Minecraft::GetInstance()->localplayers[i] != nullptr &&
                        !ui.IsPauseMenuDisplayed(i) &&
                        !ui.IsSceneInStack(i, eUIScene_EndPoem)) {
                        ui.CloseUIScenes(i);
                        ui.NavigateToScene(i, eUIScene_PauseMenu);
                    }
                }
            }
            break;
        case XN_LIVE_CONTENT_INSTALLED: {
            app.ClearDLCInstalled();
            ui.HandleDLCInstalled(PlatformProfile.GetPrimaryPad());
        } break;
        case XN_SYS_STORAGEDEVICESCHANGED: {
        } break;
    }

    pClass->m_networkController.m_vNotifications.push_back(pNotification);
}

void NetworkController::liveLinkChangeCallback(void* pParam, bool bConnected) {
    // Implementation is platform-specific, stub here
}

int NetworkController::exitAndJoinFromInvite(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    Game* pApp = (Game*)pParam;

    if (result == IPlatformStorage::EMessage_ResultDecline) {
        pApp->SetAction(iPad, eAppAction_ExitAndJoinFromInviteConfirmed);
    }

    return 0;
}

int NetworkController::exitAndJoinFromInviteSaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    Game* pClass = (Game*)pParam;
    if (result == IPlatformStorage::EMessage_ResultDecline ||
        result == IPlatformStorage::EMessage_ResultThirdOption) {
        if (result == IPlatformStorage::EMessage_ResultDecline) {
            if (!Minecraft::GetInstance()->skins->isUsingDefaultSkin()) {
                TexturePack* tPack =
                    Minecraft::GetInstance()->skins->getSelected();
                DLCPack* pDLCPack = tPack->getDLCPack();
                if (!pDLCPack->hasPurchasedFile(DLCManager::e_DLCType_Texture,
                                                "")) {
                    unsigned int uiIDA[2];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    uiIDA[1] = IDS_CONFIRM_CANCEL;

                    ui.RequestErrorMessage(
                        IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE,
                        IDS_WARNING_DLC_TRIALTEXTUREPACK_TEXT, uiIDA, 2, iPad,
                        &NetworkController::warningTrialTexturePackReturned,
                        pClass);

                    return 0;
                }
            }
            bool bSaveExists;
            PlatformStorage.DoesSaveExist(&bSaveExists);
            if (bSaveExists) {
                unsigned int uiIDA[2];
                uiIDA[0] = IDS_CONFIRM_CANCEL;
                uiIDA[1] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(
                    IDS_TITLE_SAVE_GAME, IDS_CONFIRM_SAVE_GAME, uiIDA, 2,
                    PlatformProfile.GetPrimaryPad(),
                    &NetworkController::exitAndJoinFromInviteAndSaveReturned,
                    pClass);
                return 0;
            } else {
                MinecraftServer::getInstance()->setSaveOnExit(true);
            }
        } else {
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_CONFIRM_OK;
            ui.RequestErrorMessage(
                IDS_TITLE_DECLINE_SAVE_GAME, IDS_CONFIRM_DECLINE_SAVE_GAME,
                uiIDA, 2, PlatformProfile.GetPrimaryPad(),
                &NetworkController::exitAndJoinFromInviteDeclineSaveReturned,
                pClass);
            return 0;
        }

        app.SetAction(PlatformProfile.GetPrimaryPad(),
                      eAppAction_ExitAndJoinFromInviteConfirmed);
    }
    return 0;
}

int NetworkController::warningTrialTexturePackReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    return 0;
}

int NetworkController::exitAndJoinFromInviteAndSaveReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        if (!Minecraft::GetInstance()->skins->isUsingDefaultSkin()) {
            TexturePack* tPack = Minecraft::GetInstance()->skins->getSelected();
            DLCPack* pDLCPack = tPack->getDLCPack();
            if (!pDLCPack->hasPurchasedFile(DLCManager::e_DLCType_Texture,
                                            "")) {
                unsigned int uiIDA[2];
                uiIDA[0] = IDS_CONFIRM_OK;
                uiIDA[1] = IDS_CONFIRM_CANCEL;
                ui.RequestErrorMessage(
                    IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE,
                    IDS_WARNING_DLC_TRIALTEXTUREPACK_TEXT, uiIDA, 2, iPad,
                    &NetworkController::warningTrialTexturePackReturned,
                    nullptr);
                return 0;
            }
        }
        MinecraftServer::getInstance()->setSaveOnExit(true);
        app.SetAction(iPad, eAppAction_ExitAndJoinFromInviteConfirmed);
    }
    return 0;
}

int NetworkController::exitAndJoinFromInviteDeclineSaveReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        MinecraftServer::getInstance()->setSaveOnExit(false);
        app.SetAction(iPad, eAppAction_ExitAndJoinFromInviteConfirmed);
    }
    return 0;
}
