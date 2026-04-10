#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "platform/PlatformTypes.h"

class CXuiStringTable;

class IPlatformProfile {
public:
    struct PROFILESETTINGS {
        int iYAxisInversion;
        int iControllerSensitivity;
        int iVibration;
        bool bSwapSticks;
    };

    virtual ~IPlatformProfile() = default;

    // Lifecycle
    virtual void Initialise(std::uint32_t dwTitleID, std::uint32_t dwOfferID,
                            unsigned short usProfileVersion,
                            unsigned int uiProfileValuesC,
                            unsigned int uiProfileSettingsC,
                            std::uint32_t* pdwProfileSettingsA,
                            int iGameDefinedDataSizeX4,
                            unsigned int* puiGameDefinedDataChangedBitmask) = 0;
    virtual void Tick() = 0;

    // Sign-in
    [[nodiscard]] virtual int GetLockedProfile() = 0;
    virtual void SetLockedProfile(int iProf) = 0;
    [[nodiscard]] virtual bool IsSignedIn(int iQuadrant) = 0;
    [[nodiscard]] virtual bool IsSignedInLive(int iProf) = 0;
    [[nodiscard]] virtual bool IsGuest(int iQuadrant) = 0;
    virtual unsigned int RequestSignInUI(bool bFromInvite, bool bLocalGame,
                                         bool bNoGuestsAllowed,
                                         bool bMultiplayerSignIn, bool bAddUser,
                                         std::function<int(bool, int)> callback,
                                         int iQuadrant = XUSER_INDEX_ANY) = 0;
    virtual unsigned int DisplayOfflineProfile(
        std::function<int(bool, int)> callback,
        int iQuadrant = XUSER_INDEX_ANY) = 0;
    virtual unsigned int RequestConvertOfflineToGuestUI(
        std::function<int(bool, int)> callback,
        int iQuadrant = XUSER_INDEX_ANY) = 0;
    virtual void SetPrimaryPlayerChanged(bool bVal) = 0;
    [[nodiscard]] virtual bool QuerySigninStatus() = 0;
    virtual void GetXUID(int iPad, PlayerUID* pXuid, bool bOnlineXuid) = 0;
    [[nodiscard]] virtual bool AreXUIDSEqual(PlayerUID xuid1,
                                             PlayerUID xuid2) = 0;
    [[nodiscard]] virtual bool XUIDIsGuest(PlayerUID xuid) = 0;
    [[nodiscard]] virtual bool AllowedToPlayMultiplayer(int iProf) = 0;
    [[nodiscard]] virtual bool GetChatAndContentRestrictions(
        int iPad, bool* pbChatRestricted, bool* pbContentRestricted,
        int* piAge) = 0;

    // System
    [[nodiscard]] virtual int GetPrimaryPad() = 0;
    virtual void SetPrimaryPad(int iPad) = 0;
    [[nodiscard]] virtual char* GetGamertag(int iPad) = 0;
    [[nodiscard]] virtual std::string GetDisplayName(int iPad) = 0;
    virtual void SetSignInChangeCallback(
        std::function<void(bool, unsigned int)> callback) = 0;
    virtual void SetNotificationsCallback(
        std::function<void(std::uint32_t, unsigned int)> callback) = 0;
    [[nodiscard]] virtual bool RegionIsNorthAmerica() = 0;
    [[nodiscard]] virtual bool LocaleIsUSorCanada() = 0;
    [[nodiscard]] virtual int GetLiveConnectionStatus() = 0;
    [[nodiscard]] virtual bool IsSystemUIDisplayed() = 0;
    virtual void SetProfileReadErrorCallback(
        std::function<void()> callback) = 0;

    // Profile data
    virtual int SetDefaultOptionsCallback(
        std::function<int(PROFILESETTINGS*, int)> callback) = 0;
    virtual int SetOldProfileVersionCallback(
        std::function<int(unsigned char*, unsigned short, int)> callback) = 0;
    [[nodiscard]] virtual PROFILESETTINGS* GetDashboardProfileSettings(
        int iPad) = 0;
    virtual void WriteToProfile(int iQuadrant,
                                bool bGameDefinedDataChanged = false,
                                bool bOverrideTimeLimit = false) = 0;
    virtual void ForceQueuedProfileWrites(int iPad = XUSER_INDEX_ANY) = 0;
    [[nodiscard]] virtual void* GetGameDefinedProfileData(int iQuadrant) = 0;
    virtual void ResetProfileProcessState() = 0;

    // Content
    virtual void AllowedPlayerCreatedContent(int iPad, bool thisQuadrantOnly,
                                             bool* allAllowed,
                                             bool* friendsAllowed) = 0;
    [[nodiscard]] virtual bool CanViewPlayerCreatedContent(
        int iPad, bool thisQuadrantOnly, PlayerUID* pXuids,
        unsigned int xuidCount) = 0;
    virtual void ShowProfileCard(int iPad, PlayerUID targetUid) = 0;
    [[nodiscard]] virtual bool GetProfileAvatar(
        int iPad, std::function<int(std::uint8_t*, unsigned int)> callback) = 0;
    virtual void CancelProfileAvatarRequest() = 0;

    // Achievements
    virtual void RegisterAward(int iAwardNumber, int iGamerconfigID,
                               EAwardType eType,
                               bool bLeaderboardAffected = false,
                               CXuiStringTable* pStringTable = nullptr,
                               int iTitleStr = -1, int iTextStr = -1,
                               int iAcceptStr = -1,
                               char* pszThemeName = nullptr,
                               unsigned int uiThemeSize = 0L) = 0;
    [[nodiscard]] virtual int GetAwardId(int iAwardNumber) = 0;
    [[nodiscard]] virtual EAwardType GetAwardType(int iAwardNumber) = 0;
    [[nodiscard]] virtual bool CanBeAwarded(int iQuadrant,
                                            int iAwardNumber) = 0;
    virtual void Award(int iQuadrant, int iAwardNumber,
                       bool bForce = false) = 0;
    [[nodiscard]] virtual bool IsAwardsFlagSet(int iQuadrant, int iAward) = 0;

    // Rich presence
    virtual void RichPresenceInit(int iPresenceCount, int iContextCount) = 0;
    virtual void RegisterRichPresenceContext(int iGameConfigContextID) = 0;
    virtual void SetRichPresenceContextValue(int iPad, int iContextID,
                                             int iVal) = 0;
    virtual void SetCurrentGameActivity(int iPad, int iNewPresence,
                                        bool bSetOthersToIdle = false) = 0;

    // Debug
    virtual void SetDebugFullOverride(bool bVal) = 0;
};
