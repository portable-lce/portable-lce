#pragma once

#include <cstdint>

#include "app/common/Tutorial/TutorialEnum.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/model/SkinBox.h"
#include "platform/NetTypes.h"
#include "platform/XboxStubs.h"
#include "platform/profile/ProfileConstants.h"
#include "platform/storage/storage.h"

typedef struct {
    char* wchFilename;
    eFileExtensionType eEXT;
    eTMSFileType eTMSType;
    std::uint8_t* pbData;
    unsigned int uiSize;
    int iConfig;  // used for texture pack data files
} TMS_FILE;

typedef struct {
    std::uint8_t* pbData;
    unsigned int byteCount;
    std::uint8_t ucRefCount;
} MEMDATA, *PMEMDATA;

typedef struct {
    unsigned int dwNotification;
    unsigned int uiParam;
} NOTIFICATION, *PNOTIFICATION;

typedef struct {
    bool bSettingsChanged;
    unsigned char ucMusicVolume;
    unsigned char ucSoundFXVolume;
    unsigned char ucSensitivity;
    unsigned char ucGamma;
    unsigned char ucPad01;           // 1 byte of padding inserted here
    unsigned short usBitmaskValues;  // bit 0,1 -	difficulty
    // bit 2 -		view bob
    // bit 3 -		player visible in a map
    // bit 4,5 -	control scheme
    // bit 6 -		invert look
    // bit 7 -		southpaw
    // bit 8 -		splitscreen vertical

    // 4J-PB - Adding new values for interim TU for 1.6.6
    // bit 9 -				Display gamertags in splitscreen
    // bit 10 -				Disable/Enable hints
    // bit 11,12,13,14 -	Autosave frequency - 0 = Off, 8 = (8*15 minutes)
    // = 2 hours bit 15				Tooltips

    // debug values
    unsigned int uiDebugBitmask;

    // block off space to use for whatever we want (e.g bitflags for storing
    // things the player has done in the game, so we can flag the first time
    // they do things, such as sleep)
    union {
        struct {
            unsigned char ucTutorialCompletion[TUTORIAL_PROFILE_STORAGE_BYTES];
            // adding new flags for interim TU to 1.6.6

            // A value that encodes the skin that the player has set as their
            // default
            std::uint32_t dwSelectedSkin;

            // In-Menu sensitivity
            unsigned char ucMenuSensitivity;
            unsigned char ucInterfaceOpacity;
            unsigned char ucPad02;  // 2 bytes of padding added here
            unsigned char usPad03;

            // Adding another bitmask flag for more settings for 1.8.2
            unsigned int
                uiBitmaskValues;  // 0x00000001 - eGameSetting_Clouds - on
            // 0x00000002 - eGameSetting_GameSetting_Online - on
            // 0x00000004 - eGameSetting_GameSetting_Invite - off
            // 0x00000008 - eGameSetting_GameSetting_FriendsOfFriends - on
            // 0x00000010 - eGameSetting_PSVita_NetworkModeAdhoc - on

            // TU 5
            // 0x00000030 - eGameSetting_DisplayUpdateMessage - 3  - counts down
            // to zero TU 6 0x00000040 - eGameSetting_BedrockFog - off
            // 0x00000080 - eGameSetting_DisplayHUD - on
            // 0x00000100 - eGameSetting_DisplayHand - on
            // TU 7
            // 0x00000200 - eGameSetting_CustomSkinAnim - on

            // TU9
            // // 0x00000400 - eGameSetting_DeathMessages - on

            // Adding another bitmask to store "special" completion tasks for
            // the tutorial
            unsigned int uiSpecialTutorialBitmask;

            // A value that encodes the cape that the player has set
            std::uint32_t dwSelectedCape;

            unsigned int uiFavoriteSkinA[MAX_FAVORITE_SKINS];
            unsigned char ucCurrentFavoriteSkinPos;

            // TU13
            unsigned int
                uiMashUpPackWorldsDisplay;  // bitmask to enable/disable the
                                            // display of the individual mash-up
                                            // pack worlds

            // PS3 1.05 - Adding Greek, so need a language
            unsigned char ucLanguage;

            // 29/Oct/2014 - Language selector.
            unsigned char ucLocale;

            // 4J Stu - See comment for GAME_SETTINGS_PROFILE_DATA_BYTES below
            // was 192
            // unsigned char
            // ucUnused[192-TUTORIAL_PROFILE_STORAGE_BYTES-sizeof(uint32_t)-sizeof(char)-sizeof(char)-sizeof(char)-sizeof(char)-sizeof(int32_t)-sizeof(int32_t)-sizeof(uint32_t)];
            // 4J-PB - don't need to define the padded space, the union with
            // ucReservedSpace will make the sizeof GAME_SETTINGS correct
        };

        unsigned char ucReservedSpace[192];
    };
} GAME_SETTINGS;

typedef struct {
    PlayerUID xuid;
    char pszLevelName[14];
} BANNEDLISTDATA, *PBANNEDLISTDATA;

typedef std::vector<PBANNEDLISTDATA> VBANNEDLIST;

typedef struct {
    int iPad;
    eXuiAction action;
} XuiActionParam;

// tips
typedef struct {
    int iSortValue;
    int uiStringID;
} TIPSTRUCT;

typedef struct {
    eDLCContentType eDLCType;

    uint64_t ullOfferID_Full;
    uint64_t ullOfferID_Trial;
    char wchBanner[MAX_BANNERNAME_SIZE];
    char wchDataFile[MAX_BANNERNAME_SIZE];
    int iGender;
    int iConfig;
    unsigned int uiSortIndex;
} DLC_INFO;

// banned list
typedef struct {
    std::uint8_t* pBannedList;
    unsigned int byteCount;
} BANNEDLIST;

typedef struct _DLCRequest {
    std::uint32_t dwType;
    eDLCContentState eState;
} DLCRequest;

typedef struct _TMSPPRequest {
    eTMSContentState eState;
    eDLCContentType eType;
    IPlatformStorage::eGlobalStorage eStorageFacility;
    IPlatformStorage::eTMS_FILETYPEVAL eFileTypeVal;
    // char szFilename[MAX_TMSFILENAME_SIZE];
    int (*CallbackFunc)(void*, int, int, IPlatformStorage::PTMSPP_FILEDATA,
                        const char* szFilename);
    char wchFilename[MAX_TMSFILENAME_SIZE];

    void* lpCallbackParam;
} TMSPPRequest;

typedef std::pair<EUIScene, HXUIOBJ> SceneStackPair;
