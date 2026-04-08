#include "UIScene_InGameInfoMenu.h"

#include <memory>

#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "minecraft/GameEnums.h"
#include "app/common/Console_Debug_enum.h"
#include "app/common/Network/GameNetworkManager.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_PlayerList.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/network/packet/KickPlayerPacket.h"
#include "minecraft/sounds/SoundTypes.h"
#include "strings.h"

UIScene_InGameInfoMenu::UIScene_InGameInfoMenu(int iPad, void* initData,
                                               UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_buttonGameOptions.init(app.GetString(IDS_HOST_OPTIONS),
                             eControl_GameOptions);
    m_labelTitle.init(app.GetString(IDS_PLAYERS_INVITE));
    m_playerList.init(eControl_GamePlayers);

    m_players = std::vector<PlayerInfo*>();

    int playerCount = g_NetworkManager.GetPlayerCount();

    for (int i = 0; i < playerCount; ++i) {
        INetworkPlayer* player = g_NetworkManager.GetPlayerByIndex(i);

        if (player != nullptr) {
            PlayerInfo* info = BuildPlayerInfo(player);

            m_players.push_back(info);
            m_playerList.addItem(info->m_name, info->m_colorState,
                                 info->m_voiceStatus);
        }
    }

    g_NetworkManager.RegisterPlayerChangedCallback(
        m_iPad, [this](INetworkPlayer* pPlayer, bool leaving) {
            OnPlayerChanged(this, pPlayer, leaving);
        });

    INetworkPlayer* thisPlayer =
        g_NetworkManager.GetLocalPlayerByUserIndex(m_iPad);
    m_isHostPlayer = false;
    if (thisPlayer != nullptr) m_isHostPlayer = thisPlayer->IsHost() == true;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    std::shared_ptr<MultiplayerLocalPlayer> localPlayer =
        pMinecraft->localplayers[m_iPad];
    if (!m_isHostPlayer && !localPlayer->isModerator()) {
        removeControl(&m_buttonGameOptions, false);
    }

    updateTooltips();

#if TO_BE_IMPLEMENTED
    SetTimer(TOOLTIP_TIMERID, INGAME_INFO_TOOLTIP_TIMER);
#endif

    // get rid of the quadrant display if it's on
    ui.HidePressStart();

#if TO_BE_IMPLEMENTED
    SetTimer(IGNORE_KEYPRESS_TIMERID, IGNORE_KEYPRESS_TIME);
#endif
}

UIScene_InGameInfoMenu::~UIScene_InGameInfoMenu() {
    // Delete player infos
    for (int i = 0; i < m_players.size(); i++) {
        delete m_players[i];
    }
}

std::string UIScene_InGameInfoMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "InGameInfoMenuSplit";
    } else {
        return "InGameInfoMenu";
    }
}

void UIScene_InGameInfoMenu::updateTooltips() {
    int keyX = IDS_TOOLTIPS_INVITE_FRIENDS;
    int ikeyY = -1;

    // XPARTY_USER_LIST partyList;
    // if ((XPartyGetUserList(&partyList) != XPARTY_E_NOT_IN_PARTY) &&
    //     (partyList.dwUserCount > 1)) {
    //     keyX = IDS_TOOLTIPS_INVITE_PARTY;
    // }

    if (g_NetworkManager.IsLocalGame()) keyX = -1;

    INetworkPlayer* selectedPlayer = g_NetworkManager.GetPlayerBySmallId(
        m_players[m_playerList.getCurrentSelection()]->m_smallId);

    int keyA = -1;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    std::shared_ptr<MultiplayerLocalPlayer> localPlayer =
        pMinecraft->localplayers[m_iPad];

    bool isOp = m_isHostPlayer || localPlayer->isModerator();
    bool cheats = app.GetGameHostOption(eGameHostOption_CheatsEnabled) != 0;
    bool trust = app.GetGameHostOption(eGameHostOption_TrustPlayers) != 0;

    if (isOp) {
        if (m_buttonGameOptions.hasFocus()) {
            keyA = IDS_TOOLTIPS_SELECT;
        } else if (selectedPlayer != nullptr) {
            bool editingHost = selectedPlayer->IsHost();
            if ((cheats && (m_isHostPlayer || !editingHost)) ||
                (!trust && (m_isHostPlayer || !editingHost))
#if !defined(_CONTENT_PACKAGE) && !defined(_FINAL_BUILD) && \
    defined(_DEBUG_MENUS_ENABLED)
                || (m_isHostPlayer && editingHost)
#endif
            ) {
                keyA = IDS_TOOLTIPS_PRIVILEGES;
            } else if (selectedPlayer->IsLocal() != true &&
                       selectedPlayer->IsSameSystem(
                           g_NetworkManager.GetHostPlayer()) != true) {
                // Only ops will hit this, can kick anyone not local and not
                // local to the host
                keyA = IDS_TOOLTIPS_KICK;
            }
        }
    }

    if (!m_buttonGameOptions.hasFocus()) {
        // if the player is me, then view gamer profile
        if (selectedPlayer != nullptr && selectedPlayer->IsLocal() &&
            selectedPlayer->GetUserIndex() == m_iPad) {
            ikeyY = IDS_TOOLTIPS_VIEW_GAMERPROFILE;
        } else {
            ikeyY = IDS_TOOLTIPS_VIEW_GAMERCARD;
        }
    }
    ui.SetTooltips(m_iPad, keyA, IDS_TOOLTIPS_BACK, keyX, ikeyY);
}

void UIScene_InGameInfoMenu::handleDestroy() {
    g_NetworkManager.UnRegisterPlayerChangedCallback(m_iPad);

    m_parentLayer->removeComponent(eUIComponent_MenuBackground);
}

void UIScene_InGameInfoMenu::handleGainFocus(bool navBack) {
    UIScene::handleGainFocus(navBack);
    if (navBack)
        g_NetworkManager.RegisterPlayerChangedCallback(
            m_iPad, [this](INetworkPlayer* pPlayer, bool leaving) {
                OnPlayerChanged(this, pPlayer, leaving);
            });
}

void UIScene_InGameInfoMenu::handleReload() {
    int playerCount = g_NetworkManager.GetPlayerCount();

    // Remove all player info
    for (int i = 0; i < m_players.size(); i++) {
        delete m_players[i];
    }
    m_players.clear();

    for (uint32_t i = 0; i < playerCount; ++i) {
        INetworkPlayer* player = g_NetworkManager.GetPlayerByIndex(i);

        if (player != nullptr) {
            PlayerInfo* info = BuildPlayerInfo(player);

            m_players.push_back(info);
            m_playerList.addItem(info->m_name, info->m_colorState,
                                 info->m_voiceStatus);
        }
    }

    INetworkPlayer* thisPlayer =
        g_NetworkManager.GetLocalPlayerByUserIndex(m_iPad);
    m_isHostPlayer = false;
    if (thisPlayer != nullptr) m_isHostPlayer = thisPlayer->IsHost() == true;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    std::shared_ptr<MultiplayerLocalPlayer> localPlayer =
        pMinecraft->localplayers[m_iPad];
    if (!m_isHostPlayer && !localPlayer->isModerator()) {
        removeControl(&m_buttonGameOptions, false);
    }

    updateTooltips();

    if (controlHasFocus(eControl_GamePlayers)) {
        m_playerList.setCurrentSelection(getControlChildFocus());
    }
}

void UIScene_InGameInfoMenu::tick() {
    UIScene::tick();

    // Update players by index
    for (uint32_t i = 0; i < m_players.size(); ++i) {
        INetworkPlayer* player = g_NetworkManager.GetPlayerByIndex(i);

        if (player != nullptr) {
            PlayerInfo* info = BuildPlayerInfo(player);

            m_players[i]->m_smallId = info->m_smallId;

            if (info->m_voiceStatus != m_players[i]->m_voiceStatus) {
                m_players[i]->m_voiceStatus = info->m_voiceStatus;
                m_playerList.setVOIPIcon(i, info->m_voiceStatus);
            }

            if (info->m_colorState != m_players[i]->m_colorState) {
                m_players[i]->m_colorState = info->m_colorState;
                m_playerList.setPlayerIcon(i, info->m_colorState);
            }

            if (info->m_name.compare(m_players[i]->m_name) != 0) {
                m_playerList.setButtonLabel(i, info->m_name);
                m_players[i]->m_name = info->m_name;
            }

            delete info;
        }
    }
}

void UIScene_InGameInfoMenu::handleInput(int iPad, int key, bool repeat,
                                         bool pressed, bool released,
                                         bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");
    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed && !repeat) {
                ui.PlayUISFX(eSFX_Back);
                navigateBack();
            }
            break;
        case ACTION_MENU_Y:

            if (pressed && m_playerList.hasFocus() &&
                (m_playerList.getItemCount() > 0) &&
                (m_playerList.getCurrentSelection() < m_players.size())) {
                INetworkPlayer* player = g_NetworkManager.GetPlayerBySmallId(
                    m_players[m_playerList.getCurrentSelection()]->m_smallId);
                if (player != nullptr) {
                    PlayerUID uid = player->GetUID();
                    if (uid != INVALID_XUID) {
                        PlatformProfile.ShowProfileCard(iPad, uid);
                    }
                }
            }

            break;
        case ACTION_MENU_X:

            if (pressed && !repeat && !g_NetworkManager.IsLocalGame()) {
                g_NetworkManager.SendInviteGUI(iPad);
            }

            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_InGameInfoMenu::handlePress(F64 controlId, F64 childId) {
    app.DebugPrintf("Pressed = %d, %d\n", (int)controlId, (int)childId);
    switch ((int)controlId) {
        case eControl_GameOptions:
            ui.NavigateToScene(m_iPad, eUIScene_InGameHostOptionsMenu);
            break;
        case eControl_GamePlayers:
            int currentSelection = (int)childId;
            INetworkPlayer* selectedPlayer =
                g_NetworkManager.GetPlayerBySmallId(
                    m_players[currentSelection]->m_smallId);

            Minecraft* pMinecraft = Minecraft::GetInstance();
            std::shared_ptr<MultiplayerLocalPlayer> localPlayer =
                pMinecraft->localplayers[m_iPad];

            bool isOp = m_isHostPlayer || localPlayer->isModerator();
            bool cheats =
                app.GetGameHostOption(eGameHostOption_CheatsEnabled) != 0;
            bool trust =
                app.GetGameHostOption(eGameHostOption_TrustPlayers) != 0;

            if (isOp && selectedPlayer != nullptr) {
                bool editingHost = selectedPlayer->IsHost();
                if ((cheats && (m_isHostPlayer || !editingHost)) ||
                    (!trust && (m_isHostPlayer || !editingHost))
#if !defined(_CONTENT_PACKAGE) && !defined(_FINAL_BUILD) && \
    defined(_DEBUG_MENUS_ENABLED)
                    || (m_isHostPlayer && editingHost)
#endif
                ) {
                    InGamePlayerOptionsInitData* pInitData =
                        new InGamePlayerOptionsInitData();
                    pInitData->iPad = m_iPad;
                    pInitData->networkSmallId =
                        m_players[currentSelection]->m_smallId;
                    pInitData->playerPrivileges = app.GetPlayerPrivileges(
                        m_players[currentSelection]->m_smallId);
                    pInitData->playerPrivileges = app.GetPlayerPrivileges(
                        m_players[currentSelection]->m_smallId);
                    ui.NavigateToScene(m_iPad, eUIScene_InGamePlayerOptionsMenu,
                                       pInitData);
                } else if (selectedPlayer->IsLocal() != true &&
                           selectedPlayer->IsSameSystem(
                               g_NetworkManager.GetHostPlayer()) != true) {
                    // Only ops will hit this, can kick anyone not local and not
                    // local to the host
                    std::uint8_t* smallId = new std::uint8_t();
                    *smallId = m_players[currentSelection]->m_smallId;
                    unsigned int uiIDA[2];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    uiIDA[1] = IDS_CONFIRM_CANCEL;

                    ui.RequestAlertMessage(
                        IDS_UNLOCK_KICK_PLAYER_TITLE, IDS_UNLOCK_KICK_PLAYER,
                        uiIDA, 2, m_iPad,
                        &UIScene_InGameInfoMenu::KickPlayerReturned, smallId);
                }
            }
            break;
    }
}

void UIScene_InGameInfoMenu::handleFocusChange(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_GamePlayers:
            m_playerList.updateChildFocus((int)childId);
    };
    updateTooltips();
}

void UIScene_InGameInfoMenu::OnPlayerChanged(void* callbackParam,
                                             INetworkPlayer* pPlayer,
                                             bool leaving) {
    app.DebugPrintf(
        "<UIScene_InGameInfoMenu::OnPlayerChanged> Player \"%s\" %s (smallId: "
        "%d)\n",
        pPlayer->GetOnlineName(), leaving ? "leaving" : "joining",
        pPlayer->GetSmallId());

    UIScene_InGameInfoMenu* scene = (UIScene_InGameInfoMenu*)callbackParam;
    bool playerFound = false;
    int foundIndex = 0;
    for (int i = 0; i < scene->m_players.size(); ++i) {
        if (!playerFound &&
            scene->m_players[i]->m_smallId == pPlayer->GetSmallId()) {
            if (scene->m_playerList.getCurrentSelection() ==
                scene->m_playerList.getItemCount() - 1) {
                scene->m_playerList.setCurrentSelection(
                    scene->m_playerList.getItemCount() - 2);
            }

            // Player found
            playerFound = true;
            foundIndex = i;
        }
    }

    if (leaving && !playerFound)
        app.DebugPrintf(
            "<UIScene_InGameInfoMenu::OnPlayerChanged> Error: Player \"%s\" "
            "leaving but not found in list\n",
            pPlayer->GetOnlineName());
    if (!leaving && playerFound)
        app.DebugPrintf(
            "<UIScene_InGameInfoMenu::OnPlayerChanged> Error: Player \"%s\" "
            "joining but already in list\n",
            pPlayer->GetOnlineName());

    // If the player was found remove them (even if they're joining, they'll be
    // added again later)
    if (playerFound) {
        app.DebugPrintf(
            "<UIScene_InGameInfoMenu::OnPlayerChanged> Player \"%s\" found, "
            "removing\n",
            pPlayer->GetOnlineName());

        // Remove player info
        delete scene->m_players[foundIndex];
        scene->m_players.erase(scene->m_players.begin() + foundIndex);

        // Remove player from list
        scene->m_playerList.removeItem(foundIndex);
    }

    // If the player is joining
    if (!leaving) {
        app.DebugPrintf(
            "<UIScene_InGameInfoMenu::OnPlayerChanged> Player \"%s\" not "
            "found, adding\n",
            pPlayer->GetOnlineName());

        PlayerInfo* info = scene->BuildPlayerInfo(pPlayer);
        scene->m_players.push_back(info);

        // Note that the tick updates buttons every tick so it's only really
        // important that we add the button (not the order or content)
        scene->m_playerList.addItem(info->m_name, info->m_colorState,
                                    info->m_voiceStatus);
    }
}

int UIScene_InGameInfoMenu::KickPlayerReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    std::uint8_t smallId = *(std::uint8_t*)pParam;
    delete (std::uint8_t*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
        Minecraft* pMinecraft = Minecraft::GetInstance();
        std::shared_ptr<MultiplayerLocalPlayer> localPlayer =
            pMinecraft->localplayers[iPad];
        if (localPlayer->connection) {
            localPlayer->connection->send(std::shared_ptr<KickPlayerPacket>(
                new KickPlayerPacket(smallId)));
        }
    }

    return 0;
}

UIScene_InGameInfoMenu::PlayerInfo* UIScene_InGameInfoMenu::BuildPlayerInfo(
    INetworkPlayer* player) {
    PlayerInfo* info = new PlayerInfo();
    info->m_smallId = player->GetSmallId();

    std::string playerName = "";
#if !defined(_CONTENT_PACKAGE)
    if (app.DebugSettingsOn() && (app.GetGameSettingsDebugMask() &
                                  (1L << eDebugSetting_DebugLeaderboards))) {
        playerName = "WWWWWWWWWWWWWWWW";
    } else
#endif
    {
        playerName = player->GetDisplayName();
    }

    int voiceStatus = 0;
    if (player != nullptr && player->HasVoice()) {
        if (player->IsMutedByLocalUser(m_iPad)) {
            // Muted image
            voiceStatus = 3;
        } else if (player->IsTalking()) {
            // Talking image
            voiceStatus = 2;
        } else {
            // Not talking image
            voiceStatus = 1;
        }
    }

    info->m_voiceStatus = voiceStatus;
    info->m_colorState = app.GetPlayerColour(info->m_smallId);
    info->m_name = playerName;

    return info;
}
