#include "StubPlatformNetwork.h"

#include <string.h>
#include <wchar.h>

#include <compare>

#include "StubNetworkPlayer.h"
#include "app/common/Network/GameNetworkManager.h"
#include "minecraft/network/Socket.h"
#include "platform/C4JThread.h"
#include "platform/network/NetTypes.h"
#include "platform/network/network.h"

namespace platform_internal {
IPlatformNetwork& PlatformNetwork_get() {
    static StubPlatformNetwork instance;
    return instance;
}
}  // namespace platform_internal

static bool s_gameRunning = false;
StubNetworkPlayer StubPlatformNetwork::m_players[4];

void StubPlatformNetwork::NotifyPlayerJoined(INetworkPlayer* pQNetPlayer) {
    const char* pszDescription;

    // 4J Stu - We create a fake socket for every where that we need an INBOUND
    // queue of game data. Outbound is all handled by QNet so we don't need
    // that. Therefore each client player has one, and the host has one for each
    // client player.
    bool createFakeSocket = false;
    bool localPlayer = false;

    INetworkPlayer* networkPlayer =
        (INetworkPlayer*)addNetworkPlayer(pQNetPlayer);

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
            if (IsHost()) {
                createFakeSocket = true;
            }
        }

        if (IsHost() && !m_bHostChanged) {
            // Do we already have a primary player for this system?
            bool systemHasPrimaryPlayer = false;
            for (auto it = m_machineQNetPrimaryPlayers.begin();
                 it < m_machineQNetPrimaryPlayers.end(); ++it) {
                INetworkPlayer* pQNetPrimaryPlayer = *it;
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

    fprintf(stderr, "Player 0x%p \"%s\" joined; %s; voice %i; camera %i.\n",
            pQNetPlayer, pQNetPlayer->GetOnlineName(), pszDescription,
            (int)pQNetPlayer->HasVoice(), (int)pQNetPlayer->HasCamera());

    if (IsHost()) {
        // 4J-PB - only the host should do this
        //		g_NetworkManager.UpdateAndSetGameSessionData();
        SystemFlagAddPlayer(networkPlayer);
    }

    for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
        if (playerChangedCallback[idx])
            playerChangedCallback[idx](networkPlayer, false);
    }

    if (s_gameRunning) {
        int localPlayerCount = 0;
        for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
            if (GetLocalPlayerByUserIndex(idx) != nullptr) ++localPlayerCount;
        }
    }
}

bool StubPlatformNetwork::Initialise(CGameNetworkManager* pGameNetworkManager,
                                     int flagIndexSize) {
    m_pGameNetworkManager = pGameNetworkManager;
    m_flagIndexSize = flagIndexSize;

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        playerChangedCallback[i] = nullptr;
    }

    m_bLeavingGame = false;
    m_bLeaveGameOnTick = false;
    m_bHostChanged = false;

    m_bIsOfflineGame = false;
    m_SessionsUpdatedCallback = nullptr;

    // Success!
    return true;
}

void StubPlatformNetwork::Terminate() {
    // TODO: 4jcraft, no release of ressources
}

int StubPlatformNetwork::GetJoiningReadyPercentage() { return 100; }

int StubPlatformNetwork::CorrectErrorIDS(int IDS) { return IDS; }

bool StubPlatformNetwork::isSystemPrimaryPlayer(INetworkPlayer* pQNetPlayer) {
    return true;
}

// We call this twice a frame, either side of the render call so is a good place
// to "tick" things
void StubPlatformNetwork::DoWork() {}

int StubPlatformNetwork::GetPlayerCount() { return 1; }

bool StubPlatformNetwork::ShouldMessageForFullSession() { return false; }

int StubPlatformNetwork::GetOnlinePlayerCount() { return 1; }

int StubPlatformNetwork::GetLocalPlayerMask(int playerIndex) {
    return 1 << playerIndex;
}

bool StubPlatformNetwork::AddLocalPlayerByUserIndex(int userIndex) {
    NotifyPlayerJoined(GetLocalPlayerByUserIndex(userIndex));
    return true;
}

bool StubPlatformNetwork::RemoveLocalPlayerByUserIndex(int userIndex) {
    return true;
}

bool StubPlatformNetwork::IsInStatsEnabledSession() { return true; }

bool StubPlatformNetwork::SessionHasSpace(unsigned int spaceRequired /*= 1*/) {
    return true;
}

void StubPlatformNetwork::SendInviteGUI(int quadrant) {}

bool StubPlatformNetwork::IsAddingPlayer() { return false; }

bool StubPlatformNetwork::LeaveGame(bool bMigrateHost) {
    if (m_bLeavingGame) return true;

    m_bLeavingGame = true;

    // If we are the host wait for the game server to end
    if (IsHost() && g_NetworkManager.ServerStoppedValid()) {
        s_gameRunning = false;
        g_NetworkManager.ServerStoppedWait();
        g_NetworkManager.ServerStoppedDestroy();
    }
    return true;
}

bool StubPlatformNetwork::_LeaveGame(bool bMigrateHost, bool bLeaveRoom) {
    return true;
}

void StubPlatformNetwork::HostGame(
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
    s_gameRunning = true;

    _HostGame(localUsersMask, publicSlots, privateSlots);
    // #endif
}

void StubPlatformNetwork::_HostGame(
    int usersMask, unsigned char publicSlots /*= MINECRAFT_NET_MAX_PLAYERS*/,
    unsigned char privateSlots /*= 0*/) {}

bool StubPlatformNetwork::_StartGame() { return true; }

int StubPlatformNetwork::JoinGame(FriendSessionInfo* searchResult,
                                  int localUsersMask, int primaryUserIndex) {
    return CGameNetworkManager::JOINGAME_SUCCESS;
}

bool StubPlatformNetwork::SetLocalGame(bool isLocal) {
    m_bIsOfflineGame = isLocal;

    return true;
}

void StubPlatformNetwork::SetPrivateGame(bool isPrivate) {
    fprintf(stderr, "Setting as private game: %s\n", isPrivate ? "yes" : "no");
    m_bIsPrivateGame = isPrivate;
}

void StubPlatformNetwork::RegisterPlayerChangedCallback(
    int iPad,
    std::function<void(INetworkPlayer* pPlayer, bool leaving)> callback) {
    playerChangedCallback[iPad] = std::move(callback);
}

void StubPlatformNetwork::UnRegisterPlayerChangedCallback(int iPad) {
    playerChangedCallback[iPad] = nullptr;
}

void StubPlatformNetwork::HandleSignInChange() { return; }

bool StubPlatformNetwork::_RunNetworkGame() { return true; }

void StubPlatformNetwork::UpdateAndSetGameSessionData(
    INetworkPlayer* pNetworkPlayerLeaving /*= nullptr*/) {
    // 	uint32_t playerCount = m_player->GetPlayerCount();
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

bool StubPlatformNetwork::RemoveLocalPlayer(INetworkPlayer* pNetworkPlayer) {
    return true;
}

StubPlatformNetwork::PlayerFlags::PlayerFlags(INetworkPlayer* pNetworkPlayer,
                                              unsigned int count) {
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
StubPlatformNetwork::PlayerFlags::~PlayerFlags() { delete[] flags; }

// Add a player to the per system flag storage - if we've already got a player
// from that system, copy its flags over
void StubPlatformNetwork::SystemFlagAddPlayer(INetworkPlayer* pNetworkPlayer) {
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

void StubPlatformNetwork::SystemFlagReset() {
    for (unsigned int i = 0; i < m_playerFlags.size(); i++) {
        delete m_playerFlags[i];
    }
    m_playerFlags.clear();
}

// Set a per system flag - this is done by setting the flag on every player that
// shares that system
void StubPlatformNetwork::SystemFlagSet(INetworkPlayer* pNetworkPlayer,
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
bool StubPlatformNetwork::SystemFlagGet(INetworkPlayer* pNetworkPlayer,
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

std::string StubPlatformNetwork::GatherStats() { return ""; }

std::string StubPlatformNetwork::GatherRTTStats() {
    std::string stats("Rtt: ");

    char stat[32];

    for (unsigned int i = 0; i < GetPlayerCount(); ++i) {
        INetworkPlayer* pQNetPlayer = GetPlayerByIndex(i);

        if (!pQNetPlayer->IsLocal()) {
            memset(stat, 0, 32 * sizeof(char));
            snprintf(stat, 32, "%d: %d/", i, pQNetPlayer->GetCurrentRtt());
            stats.append(stat);
        }
    }
    return stats;
}

void StubPlatformNetwork::TickSearch() {}

void StubPlatformNetwork::SearchForGames() {}

void StubPlatformNetwork::SetSearchResultsReady(int resultCount) {}

std::vector<FriendSessionInfo*>* StubPlatformNetwork::GetSessionList(
    int iPad, int localPlayers, bool partyOnly) {
    std::vector<FriendSessionInfo*>* filteredList =
        new std::vector<FriendSessionInfo*>();
    ;
    return filteredList;
}

bool StubPlatformNetwork::GetGameSessionInfo(
    int iPad, SessionID sessionId, FriendSessionInfo* foundSessionInfo) {
    return false;
}

void StubPlatformNetwork::SetSessionsUpdatedCallback(
    std::function<void()> callback) {
    m_SessionsUpdatedCallback = std::move(callback);
}

void StubPlatformNetwork::GetFullFriendSessionInfo(
    FriendSessionInfo* foundSession,
    std::function<void(bool success)> callback) {
    callback(true);
}

void StubPlatformNetwork::ForceFriendsSessionRefresh() {
    fprintf(stderr, "Resetting friends session search data\n");
}

INetworkPlayer* StubPlatformNetwork::addNetworkPlayer(
    INetworkPlayer* pQNetPlayer) {
    StubNetworkPlayer* pNetworkPlayer = new StubNetworkPlayer();
    currentNetworkPlayers.push_back(pNetworkPlayer);
    return pNetworkPlayer;
}

void StubPlatformNetwork::removeNetworkPlayer(INetworkPlayer* pQNetPlayer) {
    INetworkPlayer* pNetworkPlayer = getNetworkPlayer(pQNetPlayer);
    for (auto it = currentNetworkPlayers.begin();
         it != currentNetworkPlayers.end(); it++) {
        if (*it == pNetworkPlayer) {
            currentNetworkPlayers.erase(it);
            return;
        }
    }
}

INetworkPlayer* StubPlatformNetwork::getNetworkPlayer(
    INetworkPlayer* pQNetPlayer) {
    return pQNetPlayer;
}

INetworkPlayer* StubPlatformNetwork::GetLocalPlayerByUserIndex(int userIndex) {
    if (userIndex != 0) return nullptr;  // 4jcraft: hack
    return getNetworkPlayer(&m_players[userIndex]);
}

INetworkPlayer* StubPlatformNetwork::GetPlayerByIndex(int playerIndex) {
    return getNetworkPlayer(&m_players[0]);
}

INetworkPlayer* StubPlatformNetwork::GetPlayerByXuid(PlayerUID xuid) {
    return getNetworkPlayer(&m_players[0]);
}

INetworkPlayer* StubPlatformNetwork::GetPlayerBySmallId(unsigned char smallId) {
    return getNetworkPlayer(&m_players[0]);
}

INetworkPlayer* StubPlatformNetwork::GetHostPlayer() {
    return getNetworkPlayer(&m_players[0]);
}

bool StubPlatformNetwork::IsHost() { return !m_bHostChanged; }

bool StubPlatformNetwork::JoinGameFromInviteInfo(
    int userIndex, int userMask, const INVITE_INFO* pInviteInfo) {
    return 0;
}

void StubPlatformNetwork::SetSessionTexturePackParentId(int id) {
    m_hostGameSessionData.texturePackParentId = id;
}

void StubPlatformNetwork::SetSessionSubTexturePackId(int id) {
    m_hostGameSessionData.subTexturePackId = id;
}

void StubPlatformNetwork::Notify(int ID, uintptr_t Param) {}

bool StubPlatformNetwork::IsInSession() { return s_gameRunning; }
bool StubPlatformNetwork::IsInGameplay() { return s_gameRunning; }
bool StubPlatformNetwork::IsReadyToPlayOrIdle() { return true; }