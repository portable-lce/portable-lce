#pragma once

#include <cstdint>

#include "app/common/App_structs.h"
#include "platform/XboxStubs.h"
#include "platform/profile/profile.h"

class GameSettingsManager {
public:
    GameSettingsManager();

    void initGameSettings();
    static int oldProfileVersionCallback(void* pParam, unsigned char* pucData,
                                         const unsigned short usVersion,
                                         const int iPad);
    static int defaultOptionsCallback(
        void* pParam, IPlatformProfile::PROFILESETTINGS* pSettings,
        const int iPad);
    int setDefaultOptions(IPlatformProfile::PROFILESETTINGS* pSettings,
                          const int iPad);

    void setGameSettings(int iPad, eGameSetting eVal, unsigned char ucVal);
    unsigned char getGameSettings(int iPad, eGameSetting eVal);
    unsigned char getGameSettings(eGameSetting eVal);

    void checkGameSettingsChanged(bool bOverride5MinuteTimer = false,
                                  int iPad = XUSER_INDEX_ANY);
    void applyGameSettingsChanged(int iPad);
    void clearGameSettingsChangedFlag(int iPad);
    void actionGameSettings(int iPad, eGameSetting eVal);

    unsigned int getGameSettingsDebugMask(int iPad = -1,
                                          bool bOverridePlayer = false);
    void setGameSettingsDebugMask(int iPad, unsigned int uiVal);
    void actionDebugMask(int iPad, bool bSetAllClear = false);

    void setSpecialTutorialCompletionFlag(int iPad, int index);

    // Mash-up pack worlds
    void hideMashupPackWorld(int iPad, unsigned int iMashupPackID);
    void enableMashupPackWorlds(int iPad);
    unsigned int getMashupPackWorlds(int iPad);

    // Language/locale
    void setMinecraftLanguage(int iPad, unsigned char ucLanguage);
    unsigned char getMinecraftLanguage(int iPad);
    void setMinecraftLocale(int iPad, unsigned char ucLocale);
    unsigned char getMinecraftLocale(int iPad);

    // Game host options (bitfield versions)
    void setGameHostOption(unsigned int& uiHostSettings, eGameHostOption eVal,
                           unsigned int uiVal);
    unsigned int getGameHostOption(unsigned int uiHostSettings,
                                   eGameHostOption eVal);

    bool canRecordStatsAndAchievements();

    // HandleXuiActions and HandleButtonPresses
    void handleXuiActions();
    void handleButtonPresses();

    // Action-related
    static void setActionConfirmed(void* param);

    // Saving message
    int displaySavingMessage(const IPlatformStorage::ESavingMessage eMsg,
                             int iPad);

    // Game settings array - public, referenced by Game via alias
    GAME_SETTINGS* GameSettingsA[XUSER_MAX_COUNT];

    // Game host settings bitfield
    unsigned int m_uiGameHostSettings;

private:
    void handleButtonPresses(int iPad);
};
