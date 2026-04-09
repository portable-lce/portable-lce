#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "../IPlatformProfile.h"
#include "PlatformTypes.h"

class StubProfile : public IPlatformProfile {
public:
    // --- Methods with real logic (implemented in .cpp) ---

    void Initialise(std::uint32_t dwTitleID, std::uint32_t dwOfferID,
                    unsigned short usProfileVersion,
                    unsigned int uiProfileValuesC,
                    unsigned int uiProfileSettingsC,
                    std::uint32_t* pdwProfileSettingsA,
                    int iGameDefinedDataSizeX4,
                    unsigned int* puiGameDefinedDataChangedBitmask);

    int GetLockedProfile();
    void SetLockedProfile(int iProf);
    bool IsSignedIn(int iQuadrant);
    bool IsSignedInLive(int iProf);
    bool IsGuest(int iQuadrant);
    bool QuerySigninStatus();
    void GetXUID(int iPad, PlayerUID* pXuid, bool bOnlineXuid);
    bool AreXUIDSEqual(PlayerUID xuid1, PlayerUID xuid2);
    bool XUIDIsGuest(PlayerUID xuid);
    bool AllowedToPlayMultiplayer(int iProf);
    bool GetChatAndContentRestrictions(int iPad, bool* pbChatRestricted,
                                       bool* pbContentRestricted, int* piAge);
    char* GetGamertag(int iPad);
    std::string GetDisplayName(int iPad);
    int SetDefaultOptionsCallback(
        std::function<int(PROFILESETTINGS*, int)> callback);
    PROFILESETTINGS* GetDashboardProfileSettings(int iPad);
    void* GetGameDefinedProfileData(int iQuadrant);
    void AllowedPlayerCreatedContent(int iPad, bool thisQuadrantOnly,
                                     bool* allAllowed, bool* friendsAllowed);
    bool CanViewPlayerCreatedContent(int iPad, bool thisQuadrantOnly,
                                     PlayerUID* pXuids, unsigned int xuidCount);

    // --- Dead stubs (inline no-ops, kept for call-site compat) ---

    void Tick() {}
    unsigned int RequestSignInUI(bool, bool, bool, bool, bool,
                                 std::function<int(bool, int)>,
                                 int = XUSER_INDEX_ANY) {
        return 0;
    }
    unsigned int DisplayOfflineProfile(std::function<int(bool, int)>,
                                       int = XUSER_INDEX_ANY) {
        return 0;
    }
    unsigned int RequestConvertOfflineToGuestUI(std::function<int(bool, int)>,
                                                int = XUSER_INDEX_ANY) {
        return 0;
    }
    void SetPrimaryPlayerChanged(bool) {}
    void ShowProfileCard(int, PlayerUID) {}
    bool GetProfileAvatar(int,
                          std::function<int(std::uint8_t*, unsigned int)>) {
        return false;
    }
    void CancelProfileAvatarRequest() {}
    void SetSignInChangeCallback(std::function<void(bool, unsigned int)>) {}
    void SetNotificationsCallback(
        std::function<void(std::uint32_t, unsigned int)>) {}
    bool RegionIsNorthAmerica() { return false; }
    bool LocaleIsUSorCanada() { return false; }
    int GetLiveConnectionStatus() { return 0; }
    bool IsSystemUIDisplayed() { return false; }
    void SetProfileReadErrorCallback(std::function<void()>) {}
    int SetOldProfileVersionCallback(
        std::function<int(unsigned char*, unsigned short, int)>) {
        return 0;
    }
    void WriteToProfile(int, bool = false, bool = false) {}
    void ForceQueuedProfileWrites(int = XUSER_INDEX_ANY) {}
    void ResetProfileProcessState() {}
    void RegisterAward(int, int, EAwardType, bool = false,
                       CXuiStringTable* = nullptr, int = -1, int = -1, int = -1,
                       char* = nullptr, unsigned int = 0L) {}
    int GetAwardId(int) { return 0; }
    EAwardType GetAwardType(int) { return EAwardType::Achievement; }
    bool CanBeAwarded(int, int) { return false; }
    void Award(int, int, bool = false) {}
    bool IsAwardsFlagSet(int, int) { return false; }
    void RichPresenceInit(int, int) {}
    void RegisterRichPresenceContext(int) {}
    void SetRichPresenceContextValue(int, int, int) {}
    void SetCurrentGameActivity(int, int, bool = false) {}
    void SetDebugFullOverride(bool) {}

    // GetPrimaryPad/SetPrimaryPad moved to InputManager
    int GetPrimaryPad();
    void SetPrimaryPad(int iPad);
};
