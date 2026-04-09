#include "StubProfile.h"

#include <cstdio>
#include <cstring>
#include <functional>

#include "../ProfileConstants.h"
#include "input/input.h"

namespace platform_internal {
IPlatformProfile& PlatformProfile_get() {
    static StubProfile instance;
    return instance;
}
}  // namespace platform_internal

namespace {
constexpr PlayerUID kFakeXuidBase = 0xe000d45248242f2eULL;

struct ProfileGameSettings {
    bool bSettingsChanged;
    unsigned char ucMusicVolume;
    unsigned char ucSoundFXVolume;
    unsigned char ucSensitivity;
    unsigned char ucGamma;
    unsigned char ucPad01;
    unsigned short usBitmaskValues;
    unsigned int uiDebugBitmask;
    union {
        struct {
            unsigned char ucTutorialCompletion[TUTORIAL_PROFILE_STORAGE_BYTES];
            std::uint32_t dwSelectedSkin;
            unsigned char ucMenuSensitivity;
            unsigned char ucInterfaceOpacity;
            unsigned char ucPad02;
            unsigned char usPad03;
            unsigned int uiBitmaskValues;
            unsigned int uiSpecialTutorialBitmask;
            std::uint32_t dwSelectedCape;
            unsigned int uiFavoriteSkinA[MAX_FAVORITE_SKINS];
            unsigned char ucCurrentFavoriteSkinPos;
            unsigned int uiMashUpPackWorldsDisplay;
            unsigned char ucLanguage;
        };

        unsigned char ucReservedSpace[192];
    };
};

static_assert(sizeof(ProfileGameSettings) == 204,
              "ProfileGameSettings must match GAME_SETTINGS profile storage");

void* s_profileData[XUSER_MAX_COUNT] = {};
IPlatformProfile::PROFILESETTINGS s_dashboardSettings[XUSER_MAX_COUNT] = {};
char s_gamertags[XUSER_MAX_COUNT][16] = {};
std::string s_displayNames[XUSER_MAX_COUNT];
int s_lockedProfile = 0;
std::function<int(IPlatformProfile::PROFILESETTINGS*, int)>
    s_defaultOptionsCallback;

bool isValidPad(int iPad) { return iPad >= 0 && iPad < XUSER_MAX_COUNT; }

void ensureFakeIdentity(int iPad) {
    if (!isValidPad(iPad) || s_gamertags[iPad][0] != '\0') {
        return;
    }

    std::snprintf(s_gamertags[iPad], sizeof(s_gamertags[iPad]), "Player%d",
                  iPad + 1);
    s_displayNames[iPad] = std::string("Player") + std::to_string(iPad + 1);
}

void initialiseDefaultGameSettings(ProfileGameSettings* gameSettings) {
    gameSettings->ucMenuSensitivity = 100;
    gameSettings->ucInterfaceOpacity = 80;
    gameSettings->usBitmaskValues |= 0x0200;
    gameSettings->usBitmaskValues |= 0x0400;
    gameSettings->usBitmaskValues |= 0x1000;
    gameSettings->usBitmaskValues |= 0x8000;
    gameSettings->uiBitmaskValues = 0L;
    gameSettings->uiBitmaskValues |= GAMESETTING_CLOUDS;
    gameSettings->uiBitmaskValues |= GAMESETTING_ONLINE;
    gameSettings->uiBitmaskValues |= GAMESETTING_FRIENDSOFFRIENDS;
    gameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
    gameSettings->uiBitmaskValues &= ~GAMESETTING_BEDROCKFOG;
    gameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYHUD;
    gameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYHAND;
    gameSettings->uiBitmaskValues |= GAMESETTING_CUSTOMSKINANIM;
    gameSettings->uiBitmaskValues |= GAMESETTING_DEATHMESSAGES;
    gameSettings->uiBitmaskValues |= (GAMESETTING_UISIZE & 0x00000800);
    gameSettings->uiBitmaskValues |=
        (GAMESETTING_UISIZE_SPLITSCREEN & 0x00004000);
    gameSettings->uiBitmaskValues |= GAMESETTING_ANIMATEDCHARACTER;

    for (int i = 0; i < MAX_FAVORITE_SKINS; ++i) {
        gameSettings->uiFavoriteSkinA[i] = 0xFFFFFFFF;
    }

    gameSettings->ucCurrentFavoriteSkinPos = 0;
    gameSettings->uiMashUpPackWorldsDisplay = 0xFFFFFFFF;
    gameSettings->uiBitmaskValues &= ~GAMESETTING_PS3EULAREAD;
    gameSettings->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
    gameSettings->uiBitmaskValues &= ~GAMESETTING_PSVITANETWORKMODEADHOC;
    gameSettings->ucTutorialCompletion[0] = 0xFF;
    gameSettings->ucTutorialCompletion[1] = 0xFF;
    gameSettings->ucTutorialCompletion[2] = 0x0F;
    gameSettings->ucTutorialCompletion[28] |= 1 << 0;
}
}  // namespace

void StubProfile::Initialise(std::uint32_t, std::uint32_t, unsigned short,
                             unsigned int, unsigned int, std::uint32_t*,
                             int iGameDefinedDataSizeX4, unsigned int*) {
    s_lockedProfile = 0;
    std::memset(s_dashboardSettings, 0, sizeof(s_dashboardSettings));

    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        delete[] static_cast<unsigned char*>(s_profileData[i]);
        s_profileData[i] = new unsigned char[iGameDefinedDataSizeX4 / 4];
        std::memset(s_profileData[i], 0, iGameDefinedDataSizeX4 / 4);
        initialiseDefaultGameSettings(
            static_cast<ProfileGameSettings*>(s_profileData[i]));
        ensureFakeIdentity(i);
    }
}

int StubProfile::GetLockedProfile() { return s_lockedProfile; }
void StubProfile::SetLockedProfile(int iProf) { s_lockedProfile = iProf; }
bool StubProfile::IsSignedIn(int iQuadrant) { return iQuadrant == 0; }
bool StubProfile::IsSignedInLive(int iProf) { return IsSignedIn(iProf); }
bool StubProfile::IsGuest(int) { return false; }
bool StubProfile::QuerySigninStatus() { return true; }

void StubProfile::GetXUID(int iPad, PlayerUID* pXuid, bool) {
    if (pXuid)
        *pXuid =
            kFakeXuidBase + static_cast<PlayerUID>(isValidPad(iPad) ? iPad : 0);
}

bool StubProfile::AreXUIDSEqual(PlayerUID xuid1, PlayerUID xuid2) {
    return xuid1 == xuid2;
}

bool StubProfile::XUIDIsGuest(PlayerUID) { return false; }
bool StubProfile::AllowedToPlayMultiplayer(int) { return true; }

bool StubProfile::GetChatAndContentRestrictions(int, bool* pbChatRestricted,
                                                bool* pbContentRestricted,
                                                int* piAge) {
    if (pbChatRestricted) *pbChatRestricted = false;
    if (pbContentRestricted) *pbContentRestricted = false;
    if (piAge) *piAge = 18;
    return true;
}

char* StubProfile::GetGamertag(int iPad) {
    const int p = isValidPad(iPad) ? iPad : 0;
    ensureFakeIdentity(p);
    return s_gamertags[p];
}

std::string StubProfile::GetDisplayName(int iPad) {
    const int p = isValidPad(iPad) ? iPad : 0;
    ensureFakeIdentity(p);
    return s_displayNames[p];
}

int StubProfile::SetDefaultOptionsCallback(
    std::function<int(PROFILESETTINGS*, int)> callback) {
    s_defaultOptionsCallback = std::move(callback);
    return 0;
}

IPlatformProfile::PROFILESETTINGS* StubProfile::GetDashboardProfileSettings(
    int iPad) {
    return &s_dashboardSettings[isValidPad(iPad) ? iPad : 0];
}

void* StubProfile::GetGameDefinedProfileData(int iQuadrant) {
    return isValidPad(iQuadrant) ? s_profileData[iQuadrant] : nullptr;
}

void StubProfile::AllowedPlayerCreatedContent(int, bool, bool* allAllowed,
                                              bool* friendsAllowed) {
    if (allAllowed) *allAllowed = true;
    if (friendsAllowed) *friendsAllowed = true;
}

bool StubProfile::CanViewPlayerCreatedContent(int, bool, PlayerUID*,
                                              unsigned int) {
    return true;
}

// GetPrimaryPad/SetPrimaryPad — delegates to PlatformPlatft.
// Kept here temporarily for call sites that still use PlatformPlatfore.
// These forward to the canonical copies in SDL2Input.
int StubProfile::GetPrimaryPad() { return PlatformInput.GetPrimaryPad(); }
void StubProfile::SetPrimaryPad(int iPad) { PlatformInput.SetPrimaryPad(iPad); }
