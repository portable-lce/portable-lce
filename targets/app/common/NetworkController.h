#pragma once

#include <cstdint>

#include "app/common/App_structs.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "platform/NetTypes.h"
#include "platform/XboxStubs.h"
#include "platform/storage/storage.h"

struct INVITE_INFO;

typedef struct _JoinFromInviteData {
    std::uint32_t dwUserIndex;
    std::uint32_t dwLocalUsersMask;
    const INVITE_INFO* pInviteInfo;
} JoinFromInviteData;

class NetworkController {
public:
    NetworkController();

    // Player info
    void updatePlayerInfo(std::uint8_t networkSmallId,
                          int16_t playerColourIndex,
                          unsigned int playerGamePrivileges);
    short getPlayerColour(std::uint8_t networkSmallId);
    unsigned int getPlayerPrivileges(std::uint8_t networkSmallId);

    // Sign-in change
    static void signInChangeCallback(void* pParam, bool bVal,
                                     unsigned int uiSignInData);
    static void clearSignInChangeUsersMask();
    static int signoutExitWorldThreadProc(void* lpParameter);
    static int primaryPlayerSignedOutReturned(
        void* pParam, int iPad, const IPlatformStorage::EMessageResult);
    static int ethernetDisconnectReturned(
        void* pParam, int iPad, const IPlatformStorage::EMessageResult);
    static void profileReadErrorCallback(void* pParam);

    // Notifications
    static void notificationsCallback(void* pParam,
                                      std::uint32_t dwNotification,
                                      unsigned int uiParam);

    // Ethernet/Live link
    static void liveLinkChangeCallback(void* pParam, bool bConnected);

    // Invites
    void processInvite(std::uint32_t dwUserIndex,
                       std::uint32_t dwLocalUsersMask,
                       const INVITE_INFO* pInviteInfo);
    static int exitAndJoinFromInvite(void* pParam, int iPad,
                                     IPlatformStorage::EMessageResult result);
    static int exitAndJoinFromInviteSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int exitAndJoinFromInviteAndSaveReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int exitAndJoinFromInviteDeclineSaveReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int warningTrialTexturePackReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);

    // Disconnect
    DisconnectPacket::eDisconnectReason getDisconnectReason() {
        return m_disconnectReason;
    }
    void setDisconnectReason(DisconnectPacket::eDisconnectReason bVal) {
        m_disconnectReason = bVal;
    }

    // Session type flags
    bool getChangingSessionType() { return m_bChangingSessionType; }
    void setChangingSessionType(bool bVal) { m_bChangingSessionType = bVal; }
    bool getReallyChangingSessionType() { return m_bReallyChangingSessionType; }
    void setReallyChangingSessionType(bool bVal) {
        m_bReallyChangingSessionType = bVal;
    }

    // Live link
    bool getLiveLinkRequired() { return m_bLiveLinkRequired; }
    void setLiveLinkRequired(bool required) { m_bLiveLinkRequired = required; }

    // Sign-in info
    XUSER_SIGNIN_INFO m_currentSigninInfo[XUSER_MAX_COUNT];

    // Invite data
    JoinFromInviteData m_InviteData;

    // Notifications
    typedef std::vector<PNOTIFICATION> VNOTIFICATIONS;
    VNOTIFICATIONS m_vNotifications;
    VNOTIFICATIONS* getNotifications() { return &m_vNotifications; }

    // Static sign-in data
    static unsigned int m_uiLastSignInData;

private:
    std::uint8_t m_playerColours[MINECRAFT_NET_MAX_PLAYERS];
    unsigned int m_playerGamePrivileges[MINECRAFT_NET_MAX_PLAYERS];

    DisconnectPacket::eDisconnectReason m_disconnectReason;
    bool m_bChangingSessionType;
    bool m_bReallyChangingSessionType;
    bool m_bLiveLinkRequired;
};
