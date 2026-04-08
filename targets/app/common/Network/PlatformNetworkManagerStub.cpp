#include "PlatformNetworkManagerStub.h"

#include <string.h>
#include <wchar.h>

#include <compare>

#include "app/common/Network/GameNetworkManager.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "platform/NetTypes.h"
#include "NetworkPlayerQNet.h"
#include "Socket.h"
#include "platform/C4JThread.h"

IPlatformNetworkStub* g_pPlatformNetworkManager;

void IPlatformNetworkStub::NotifyPlayerJoined(IQNetPlayer* pQNetPlayer) {
    const char* pszDescription;

    // 4J Stu - We create a fake socket for every where that we need an INBOUND
    // queue of game data. Outbound is all handled by QNet so we don't need
    // that. Therefore each client player has one, and the host has one for each
    // client player.
    bool createFakeSocket = false;
    bool localPlayer = false;

    NetworkPlayerQNet* networkPlayer =
        (NetworkPlayerQNet*)addNetworkPlayer(pQNetPlayer);

    if (pQNetPlayer->IsLocal()) {
        localPlayer = true;
        if (pQNetPlayer->IsHost()) {
            pszDescription = "local host";
            // 4J Stu - No socket for the localhost as it uses a special
            // loopback queue

            m_machineQNetPrimaryPlayers.push_back(pQNetPlayer);
        } else {
            pszDescription = "local";

            // We need an inbound queue on all local players to receive data
            // from the host
            createFakeSocket = true;
        }
    } else {
        if (pQNetPlayer->IsHost()) {
            pszDescription = "remote host";
        } else {
            pszDescription = "remote";

            // If we are the host, then create a fake socket for every remote
            // player
            if (m_pIQNet->IsHost()) {
                createFakeSocket = true;
            }
        }

        if (m_pIQNet->IsHost() && !m_bHostChanged) {
            // Do we already have a primary player for this system?
            bool systemHasPrimaryPlayer = false;
            for (auto it = m_machineQNetPrimaryPlayers.begin();
                 it < m_machineQNetPrimaryPlayers.end(); ++it) {
                IQNetPlayer* pQNetPrimaryPlayer = *it;
                if (pQNetPlayer->IsSameSystem(pQNetPrimaryPlayer)) {
                    systemHasPrimaryPlayer = true;
                    break;
                }
            }
            if (!systemHasPrimaryPlayer)
                m_machineQNetPrimaryPlayers.push_back(pQNetPlayer);
        }
    }
    g_NetworkManager.PlayerJoining(networkPlayer);

    if (createFakeSocket == true && !m_bHostChanged) {
        g_NetworkManager.CreateSocket(networkPlayer, localPlayer);
    }

    app.DebugPrintf("Player 0x%p \"%s\" joined; %s; voice %i; camera %i.\n",
                    pQNetPlayer, pQNetPlayer->GetGamertag(), pszDescription,
                    (int)pQNetPlayer->HasVoice(),
                    (int)pQNetPlayer->HasCamera());

    if (m_pIQNet->IsHost()) {
        // 4J-PB - only the host should do this
        //		g_NetworkManager.UpdateAndSetGameSessionData();
        SystemFlagAddPlayer(networkPlayer);
    }

    for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
        if (playerChangedCallback[idx])
            playerChangedCallback[idx](networkPlayer, false);
    }

    if (m_pIQNet->GetState() == QNET_STATE_GAME_PLAY) {
        int localPlayerCount = 0;
        for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
            if (m_pIQNet->GetLocalPlayerByUserIndex(idx) != nullptr)
                ++localPlayerCount;
        }

        float appTime = app.getAppTime();

        // Only record stats for the primary player here
        m_lastPlayerEventTimeStart = appTime;
    }
}

bool IPlatformNetworkStub::Initialise(
    CGameNetworkManager* pGameNetworkManager, int flagIndexSize) {
    m_pGameNetworkManager = pGameNetworkManager;
    m_flagIndexSize = flagIndexSize;
    g_pPlatformNetworkManager = this;
    // 4jcraft added this, as it was never called
    m_pIQNet = new IQNet();
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        playerChangedCallback[i] = nullptr;
    }

    m_bLeavingGame = false;
    m_bLeaveGameOnTick = false;
    m_bHostChanged = false;

    m_bSearchResultsReady = false;
    m_bSearchPending = false;

    m_bIsOfflineGame = false;
    m_SessionsUpdatedCallback = nullptr;

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        m_searchResultsCount[i] = 0;
        m_lastSearchStartTime[i] = 0;

        // The results that will be filled in with the current search
        m_pSearchResults[i] = nullptr;
        m_pQoSResult[i] = nullptr;
        m_pCurrentSearchResults[i] = nullptr;
        m_pCurrentQoSResult[i] = nullptr;
        m_currentSearchResultsCount[i] = 0;
    }

    // Success!
    return true;
}

void IPlatformNetworkStub::Terminate() {
    // TODO: 4jcraft, no release of ressources
}

int IPlatformNetworkStub::GetJoiningReadyPercentage() { return 100; }

int IPlatformNetworkStub::CorrectErrorIDS(int IDS) { return IDS; }

bool IPlatformNetworkStub::isSystemPrimaryPlayer(
    IQNetPlayer* pQNetPlayer) {
    return true;
}

// We call this twice a frame, either side of the render call so is a good place
// to "tick" things
void IPlatformNetworkStub::DoWork() {}

int IPlatformNetworkStub::GetPlayerCount() {
    return m_pIQNet->GetPlayerCount();
}

bool IPlatformNetworkStub::ShouldMessageForFullSession() {
    return false;
}

int IPlatformNetworkStub::GetOnlinePlayerCount() { return 1; }

int IPlatformNetworkStub::GetLocalPlayerMask(int playerIndex) {
    return 1 << playerIndex;
}

bool IPlatformNetworkStub::AddLocalPlayerByUserIndex(int userIndex) {
    NotifyPlayerJoined(m_pIQNet->GetLocalPlayerByUserIndex(userIndex));
    return (m_pIQNet->AddLocalPlayerByUserIndex(userIndex) == 0);
}

bool IPlatformNetworkStub::RemoveLocalPlayerByUserIndex(int userIndex) {
    return true;
}

bool IPlatformNetworkStub::IsInStatsEnabledSession() { return true; }

bool IPlatformNetworkStub::SessionHasSpace(
    unsigned int spaceRequired /*= 1*/) {
    return true;
}

void IPlatformNetworkStub::SendInviteGUI(int quadrant) {}

bool IPlatformNetworkStub::IsAddingPlayer() { return false; }

bool IPlatformNetworkStub::LeaveGame(bool bMigrateHost) {
    if (m_bLeavingGame) return true;

    m_bLeavingGame = true;

    // If we are the host wait for the game server to end
    if (m_pIQNet->IsHost() && g_NetworkManager.ServerStoppedValid()) {
        m_pIQNet->EndGame();
        g_NetworkManager.ServerStoppedWait();
        g_NetworkManager.ServerStoppedDestroy();
    }
    return true;
}

bool IPlatformNetworkStub::_LeaveGame(bool bMigrateHost,
                                             bool bLeaveRoom) {
    return true;
}

void IPlatformNetworkStub::HostGame(
    int localUsersMask, bool bOnlineGame, bool bIsPrivate,
    unsigned char publicSlots /*= MINECRAFT_NET_MAX_PLAYERS*/,
    unsigned char privateSlots /*= 0*/) {
    // #ifdef 0
    // 4J Stu - We probably did this earlier as well, but just to be sure!
    SetLocalGame(!bOnlineGame);
    SetPrivateGame(bIsPrivate);
    SystemFlagReset();

    // Make sure that the Primary Pad is in by default
    localUsersMask |= GetLocalPlayerMask(g_NetworkManager.GetPrimaryPad());

    m_bLeavingGame = false;

    m_pIQNet->HostGame();

    _HostGame(localUsersMask, publicSlots, privateSlots);
    // #endif
}

void IPlatformNetworkStub::_HostGame(
    int usersMask, unsigned char publicSlots /*= MINECRAFT_NET_MAX_PLAYERS*/,
    unsigned char privateSlots /*= 0*/) {}

bool IPlatformNetworkStub::_StartGame() { return true; }

int IPlatformNetworkStub::JoinGame(FriendSessionInfo* searchResult,
                                          int localUsersMask,
                                          int primaryUserIndex) {
    return CGameNetworkManager::JOINGAME_SUCCESS;
}

bool IPlatformNetworkStub::SetLocalGame(bool isLocal) {
    m_bIsOfflineGame = isLocal;

    return true;
}

void IPlatformNetworkStub::SetPrivateGame(bool isPrivate) {
    app.DebugPrintf("Setting as private game: %s\n", isPrivate ? "yes" : "no");
    m_bIsPrivateGame = isPrivate;
}

void IPlatformNetworkStub::RegisterPlayerChangedCallback(
    int iPad,
    std::function<void(INetworkPlayer* pPlayer, bool leaving)> callback) {
    playerChangedCallback[iPad] = std::move(callback);
}

void IPlatformNetworkStub::UnRegisterPlayerChangedCallback(int iPad) {
    playerChangedCallback[iPad] = nullptr;
}

void IPlatformNetworkStub::HandleSignInChange() { return; }

bool IPlatformNetworkStub::_RunNetworkGame() { return true; }

void IPlatformNetworkStub::UpdateAndSetGameSessionData(
    INetworkPlayer* pNetworkPlayerLeaving /*= nullptr*/) {
    // 	uint32_t playerCount = m_pIQNet->GetPlayerCount();
    //
    // 	if( this->m_bLeavingGame )
    // 		return;
    //
    // 	if( GetHostPlayer() == nullptr )
    // 		return;
    //
    // 	for(unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i)
    // 	{
    // 		if( i < playerCount )
    // 		{
    // 			INetworkPlayer *pNetworkPlayer = GetPlayerByIndex(i);
    //
    // 			// We can call this from NotifyPlayerLeaving but at that
    // point the player is still considered in the session
    // if( pNetworkPlayer != pNetworkPlayerLeaving )
    // 			{
    // 				m_hostGameSessionData.players[i] =
    // ((NetworkPlayerXbox *)pNetworkPlayer)->GetUID();
    //
    // 				char *temp;
    // 				temp = (char *)wstring_to_string(
    // pNetworkPlayer->GetOnlineName() );
    // 				memcpy(m_hostGameSessionData.szPlayers[i],temp,XUSER_NAME_SIZE);
    // 			}
    // 			else
    // 			{
    // 				m_hostGameSessionData.players[i] = nullptr;
    // 				memset(m_hostGameSessionData.szPlayers[i],0,XUSER_NAME_SIZE);
    // 			}
    // 		}
    // 		else
    // 		{
    // 			m_hostGameSessionData.players[i] = nullptr;
    // 			memset(m_hostGameSessionData.szPlayers[i],0,XUSER_NAME_SIZE);
    // 		}
    // 	}
    //
    // 	m_hostGameSessionData.hostPlayerUID = ((NetworkPlayerXbox
    // *)GetHostPlayer())->GetQNetPlayer()->GetXuid();
    // 	m_hostGameSessionData.m_uiGameHostSettings =
    // app.GetGameHostOption(eGameHostOption_All);
}

int IPlatformNetworkStub::RemovePlayerOnSocketClosedThreadProc(
    void* lpParam) {
    INetworkPlayer* pNetworkPlayer = (INetworkPlayer*)lpParam;

    Socket* socket = pNetworkPlayer->GetSocket();

    if (socket != nullptr) {
        // printf("Waiting for socket closed event\n");
        socket->m_socketClosedEvent->waitForSignal(C4JThread::kInfiniteTimeout);

        // printf("Socket closed event has fired\n");
        //  4J Stu - Clear our reference to this socket
        pNetworkPlayer->SetSocket(nullptr);
        delete socket;
    }

    return g_pPlatformNetworkManager->RemoveLocalPlayer(pNetworkPlayer);
}

bool IPlatformNetworkStub::RemoveLocalPlayer(
    INetworkPlayer* pNetworkPlayer) {
    return true;
}

IPlatformNetworkStub::PlayerFlags::PlayerFlags(
    INetworkPlayer* pNetworkPlayer, unsigned int count) {
    // 4J Stu - Don't assert, just make it a multiple of 8! This count is
    // calculated from a load of separate values, and makes tweaking
    // world/render sizes a pain if we hit an assert here
    count = (count + 8 - 1) & ~(8 - 1);
    // assert( ( count % 8 ) == 0 );
    this->m_pNetworkPlayer = pNetworkPlayer;
    this->flags = new unsigned char[count / 8];
    memset(this->flags, 0, count / 8);
    this->count = count;
}
IPlatformNetworkStub::PlayerFlags::~PlayerFlags() { delete[] flags; }

// Add a player to the per system flag storage - if we've already got a player
// from that system, copy its flags over
void IPlatformNetworkStub::SystemFlagAddPlayer(
    INetworkPlayer* pNetworkPlayer) {
    PlayerFlags* newPlayerFlags =
        new PlayerFlags(pNetworkPlayer, m_flagIndexSize);
    // If any of our existing players are on the same system, then copy over
    // flags from that one
    for (unsigned int i = 0; i < m_playerFlags.size(); i++) {
        if (pNetworkPlayer->IsSameSystem(m_playerFlags[i]->m_pNetworkPlayer)) {
            memcpy(newPlayerFlags->flags, m_playerFlags[i]->flags,
                   m_playerFlags[i]->count / 8);
            break;
        }
    }
    m_playerFlags.push_back(newPlayerFlags);
}

// Remove a player from the per system flag storage - just maintains the
// m_playerFlags vector without any gaps in it
void IPlatformNetworkStub::SystemFlagRemovePlayer(
    INetworkPlayer* pNetworkPlayer) {
    for (unsigned int i = 0; i < m_playerFlags.size(); i++) {
        if (m_playerFlags[i]->m_pNetworkPlayer == pNetworkPlayer) {
            delete m_playerFlags[i];
            m_playerFlags[i] = m_playerFlags.back();
            m_playerFlags.pop_back();
            return;
        }
    }
}

void IPlatformNetworkStub::SystemFlagReset() {
    for (unsigned int i = 0; i < m_playerFlags.size(); i++) {
        delete m_playerFlags[i];
    }
    m_playerFlags.clear();
}

// Set a per system flag - this is done by setting the flag on every player that
// shares that system
void IPlatformNetworkStub::SystemFlagSet(INetworkPlayer* pNetworkPlayer,
                                                int index) {
    if ((index < 0) || (index >= m_flagIndexSize)) return;
    if (pNetworkPlayer == nullptr) return;

    for (unsigned int i = 0; i < m_playerFlags.size(); i++) {
        if (pNetworkPlayer->IsSameSystem(m_playerFlags[i]->m_pNetworkPlayer)) {
            m_playerFlags[i]->flags[index / 8] |= (128 >> (index % 8));
        }
    }
}

// Get value of a per system flag - can be read from the flags of the passed in
// player as anything else sent to that system should also have been duplicated
// here
bool IPlatformNetworkStub::SystemFlagGet(INetworkPlayer* pNetworkPlayer,
                                                int index) {
    if ((index < 0) || (index >= m_flagIndexSize)) return false;
    if (pNetworkPlayer == nullptr) {
        return false;
    }

    for (unsigned int i = 0; i < m_playerFlags.size(); i++) {
        if (m_playerFlags[i]->m_pNetworkPlayer == pNetworkPlayer) {
            return ((m_playerFlags[i]->flags[index / 8] &
                     (128 >> (index % 8))) != 0);
        }
    }
    return false;
}

std::string IPlatformNetworkStub::GatherStats() { return ""; }

std::string IPlatformNetworkStub::GatherRTTStats() {
    std::string stats("Rtt: ");

    char stat[32];

    for (unsigned int i = 0; i < GetPlayerCount(); ++i) {
        IQNetPlayer* pQNetPlayer =
            ((NetworkPlayerQNet*)GetPlayerByIndex(i))->GetQNetPlayer();

        if (!pQNetPlayer->IsLocal()) {
            memset(stat, 0, 32 * sizeof(char));
            snprintf(stat, 32, "%d: %d/", i, pQNetPlayer->GetCurrentRtt());
            stats.append(stat);
        }
    }
    return stats;
}

void IPlatformNetworkStub::TickSearch() {}

void IPlatformNetworkStub::SearchForGames() {}

int IPlatformNetworkStub::SearchForGamesThreadProc(void* lpParameter) {
    return 0;
}

void IPlatformNetworkStub::SetSearchResultsReady(int resultCount) {
    m_bSearchResultsReady = true;
    m_searchResultsCount[m_lastSearchPad] = resultCount;
}

std::vector<FriendSessionInfo*>* IPlatformNetworkStub::GetSessionList(
    int iPad, int localPlayers, bool partyOnly) {
    std::vector<FriendSessionInfo*>* filteredList =
        new std::vector<FriendSessionInfo*>();
    ;
    return filteredList;
}

bool IPlatformNetworkStub::GetGameSessionInfo(
    int iPad, SessionID sessionId, FriendSessionInfo* foundSessionInfo) {
    return false;
}

void IPlatformNetworkStub::SetSessionsUpdatedCallback(
    std::function<void()> callback) {
    m_SessionsUpdatedCallback = std::move(callback);
}

void IPlatformNetworkStub::GetFullFriendSessionInfo(
    FriendSessionInfo* foundSession,
    std::function<void(bool success)> callback) {
    callback(true);
}

void IPlatformNetworkStub::ForceFriendsSessionRefresh() {
    app.DebugPrintf("Resetting friends session search data\n");

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        m_searchResultsCount[i] = 0;
        m_lastSearchStartTime[i] = 0;
        delete m_pSearchResults[i];
        m_pSearchResults[i] = nullptr;
    }
}

INetworkPlayer* IPlatformNetworkStub::addNetworkPlayer(
    IQNetPlayer* pQNetPlayer) {
    NetworkPlayerQNet* pNetworkPlayer = new NetworkPlayerQNet(pQNetPlayer);
    pQNetPlayer->SetCustomDataValue((uintptr_t)pNetworkPlayer);
    currentNetworkPlayers.push_back(pNetworkPlayer);
    return pNetworkPlayer;
}

void IPlatformNetworkStub::removeNetworkPlayer(
    IQNetPlayer* pQNetPlayer) {
    INetworkPlayer* pNetworkPlayer = getNetworkPlayer(pQNetPlayer);
    for (auto it = currentNetworkPlayers.begin();
         it != currentNetworkPlayers.end(); it++) {
        if (*it == pNetworkPlayer) {
            currentNetworkPlayers.erase(it);
            return;
        }
    }
}

INetworkPlayer* IPlatformNetworkStub::getNetworkPlayer(
    IQNetPlayer* pQNetPlayer) {
    return pQNetPlayer ? (INetworkPlayer*)(pQNetPlayer->GetCustomDataValue())
                       : nullptr;
}

INetworkPlayer* IPlatformNetworkStub::GetLocalPlayerByUserIndex(
    int userIndex) {
    return getNetworkPlayer(m_pIQNet->GetLocalPlayerByUserIndex(userIndex));
}

INetworkPlayer* IPlatformNetworkStub::GetPlayerByIndex(int playerIndex) {
    return getNetworkPlayer(m_pIQNet->GetPlayerByIndex(playerIndex));
}

INetworkPlayer* IPlatformNetworkStub::GetPlayerByXuid(PlayerUID xuid) {
    return getNetworkPlayer(m_pIQNet->GetPlayerByXuid(xuid));
}

INetworkPlayer* IPlatformNetworkStub::GetPlayerBySmallId(
    unsigned char smallId) {
    return getNetworkPlayer(m_pIQNet->GetPlayerBySmallId(smallId));
}

INetworkPlayer* IPlatformNetworkStub::GetHostPlayer() {
    return getNetworkPlayer(m_pIQNet->GetHostPlayer());
}

bool IPlatformNetworkStub::IsHost() {
    return m_pIQNet->IsHost() && !m_bHostChanged;
}

bool IPlatformNetworkStub::JoinGameFromInviteInfo(
    int userIndex, int userMask, const INVITE_INFO* pInviteInfo) {
    return (m_pIQNet->JoinGameFromInviteInfo(userIndex, userMask,
                                             pInviteInfo) == 0);
}

void IPlatformNetworkStub::SetSessionTexturePackParentId(int id) {
    m_hostGameSessionData.texturePackParentId = id;
}

void IPlatformNetworkStub::SetSessionSubTexturePackId(int id) {
    m_hostGameSessionData.subTexturePackId = id;
}

void IPlatformNetworkStub::Notify(int ID, uintptr_t Param) {}

bool IPlatformNetworkStub::IsInSession() {
    return m_pIQNet->GetState() != QNET_STATE_IDLE;
}

bool IPlatformNetworkStub::IsInGameplay() {
    return m_pIQNet->GetState() == QNET_STATE_GAME_PLAY;
}

bool IPlatformNetworkStub::IsReadyToPlayOrIdle() { return true; }
