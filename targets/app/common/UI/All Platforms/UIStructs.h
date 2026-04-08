#pragma once

// #pragma message("UIStructs.h")

#include <cstdint>
#include <cstring>
#include <functional>

#include "platform/storage/storage.h"
#include "minecraft/GameHostOptions.h"
#include "UIEnums.h"
#include "platform/C4JThread.h"

class Container;
class Inventory;
class BrewingStandTileEntity;
class DispenserTileEntity;
class FurnaceTileEntity;
class SignTileEntity;
class LevelGenerationOptions;
class LocalPlayer;
class Merchant;
class EntityHorse;
class BeaconTileEntity;
class Slot;
class AbstractContainerMenu;
class Level;
class FriendSessionInfo;

// 4J Stu - Structs shared by Iggy and Xui scenes.
typedef struct _UIVec2D {
    float x;
    float y;

    _UIVec2D& operator+=(const _UIVec2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
} UIVec2D;

// Brewing
typedef struct _BrewingScreenInput {
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<BrewingStandTileEntity> brewingStand;
    int iPad;
    bool bSplitscreen;
} BrewingScreenInput;

// Chest
typedef struct _ContainerScreenInput {
    std::shared_ptr<Container> inventory;
    std::shared_ptr<Container> container;
    int iPad;
    bool bSplitscreen;
} ContainerScreenInput;

// Dispenser
typedef struct _TrapScreenInput {
    std::shared_ptr<Container> inventory;
    std::shared_ptr<DispenserTileEntity> trap;
    int iPad;
    bool bSplitscreen;
} TrapScreenInput;

// Inventory and creative inventory
typedef struct _InventoryScreenInput {
    std::shared_ptr<LocalPlayer> player;
    bool bNavigateBack;  // If we came here from the crafting screen, go back to
                         // it, rather than closing the xui menus
    int iPad;
    bool bSplitscreen;
} InventoryScreenInput;

// Enchanting
typedef struct _EnchantingScreenInput {
    std::shared_ptr<Inventory> inventory;
    Level* level;
    int x;
    int y;
    int z;
    int iPad;
    bool bSplitscreen;
    std::string name;
} EnchantingScreenInput;

// Furnace
typedef struct _FurnaceScreenInput {
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<FurnaceTileEntity> furnace;
    int iPad;
    bool bSplitscreen;
} FurnaceScreenInput;

// Crafting
typedef struct _CraftingPanelScreenInput {
    std::shared_ptr<LocalPlayer> player;
    int iContainerType;  // RECIPE_TYPE_2x2 or RECIPE_TYPE_3x3
    bool bSplitscreen;
    int iPad;
    int x;
    int y;
    int z;
} CraftingPanelScreenInput;

// Fireworks
typedef struct _FireworksScreenInput {
    std::shared_ptr<LocalPlayer> player;
    bool bSplitscreen;
    int iPad;
    int x;
    int y;
    int z;
} FireworksScreenInput;

// Trading
typedef struct _TradingScreenInput {
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<Merchant> trader;
    Level* level;
    int iPad;
    bool bSplitscreen;
} TradingScreenInput;

// Anvil
typedef struct _AnvilScreenInput {
    std::shared_ptr<Inventory> inventory;
    Level* level;
    int x;
    int y;
    int z;
    int iPad;
    bool bSplitscreen;
} AnvilScreenInput;

// Hopper
typedef struct _HopperScreenInput {
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<Container> hopper;
    int iPad;
    bool bSplitscreen;
} HopperScreenInput;

// Horse
typedef struct _HorseScreenInput {
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<Container> container;
    std::shared_ptr<EntityHorse> horse;
    int iPad;
    bool bSplitscreen;
} HorseScreenInput;

// Beacon
typedef struct _BeaconScreenInput {
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<BeaconTileEntity> beacon;
    int iPad;
    bool bSplitscreen;
} BeaconScreenInput;

// Sign
typedef struct _SignEntryScreenInput {
    std::shared_ptr<SignTileEntity> sign;
    int iPad;
} SignEntryScreenInput;

// Connecting progress
typedef struct _ConnectionProgressParams {
    int iPad;
    int stringId;
    bool showTooltips;
    bool setFailTimer;
    int timerTime;
    void (*cancelFunc)(void* param);
    void* cancelFuncParam;

    _ConnectionProgressParams() {
        iPad = 0;
        stringId = -1;
        showTooltips = false;
        setFailTimer = false;
        timerTime = 0;
        cancelFunc = nullptr;
        cancelFuncParam = nullptr;
    }
} ConnectionProgressParams;

// Fullscreen progress
typedef struct _UIFullscreenProgressCompletionData {
    bool bRequiresUserAction;
    bool bShowBackground;
    bool bShowLogo;
    bool bShowTips;
    ProgressionCompletionType type;
    int iPad;
    EUIScene scene;

    _UIFullscreenProgressCompletionData() {
        bRequiresUserAction = false;
        bShowBackground = true;
        bShowLogo = true;
        bShowTips = true;
        type = e_ProgressCompletion_NoAction;
    }
} UIFullscreenProgressCompletionData;

// Create world
typedef struct _CreateWorldMenuInitData {
    bool bOnline;
    bool bIsPrivate;
    int iPad;
} CreateWorldMenuInitData;

// Join/Load saves list
typedef struct _SaveListDetails {
    int saveId;
    std::uint8_t* pbThumbnailData;
    unsigned int dwThumbnailSize;
    char UTF8SaveName[128];
    char UTF8SaveFilename[MAX_SAVEFILENAME_LENGTH];

    _SaveListDetails() {
        saveId = 0;
        pbThumbnailData = nullptr;
        dwThumbnailSize = 0;
        std::memset(UTF8SaveName, 0, 128);
        std::memset(UTF8SaveFilename, 0, MAX_SAVEFILENAME_LENGTH);
    }

} SaveListDetails;

// Load world
typedef struct _LoadMenuInitData {
    int iPad;
    int iSaveGameInfoIndex;
    LevelGenerationOptions* levelGen;
    SaveListDetails* saveDetails;
} LoadMenuInitData;

// Join Games
typedef struct _JoinMenuInitData {
    FriendSessionInfo* selectedSession;
    int iPad;
} JoinMenuInitData;

// More Options
typedef struct _LaunchMoreOptionsMenuInitData {
    bool bOnlineGame;
    bool bInviteOnly;
    bool bAllowFriendsOfFriends;

    bool bGenerateOptions;
    bool bStructures;
    bool bFlatWorld;
    bool bBonusChest;

    bool bPVP;
    bool bTrust;
    bool bFireSpreads;
    bool bTNT;

    bool bHostPrivileges;
    bool bResetNether;

    bool bMobGriefing;
    bool bKeepInventory;
    bool bDoMobSpawning;
    bool bDoMobLoot;
    bool bDoTileDrops;
    bool bNaturalRegeneration;
    bool bDoDaylightCycle;

    bool bOnlineSettingChangedBySystem;

    int iPad;

    uint32_t dwTexturePack;

    std::string seed;
    int worldSize;
    bool bDisableSaving;

    EGameHostOptionWorldSize currentWorldSize;
    EGameHostOptionWorldSize newWorldSize;
    bool newWorldSizeOverwriteEdges;

    _LaunchMoreOptionsMenuInitData() {
        bOnlineGame = true;
        bInviteOnly = false;
        bAllowFriendsOfFriends = true;
        bGenerateOptions = false;
        bStructures = false;
        bFlatWorld = false;
        bBonusChest = false;
        bPVP = true;
        bTrust = false;
        bFireSpreads = true;
        bTNT = false;
        bHostPrivileges = false;
        bResetNether = false;
        bMobGriefing = true;
        bKeepInventory = false;
        bDoMobSpawning = false;
        bDoMobLoot = true;
        bDoTileDrops = true;
        bNaturalRegeneration = true;
        bDoDaylightCycle = true;
        bOnlineSettingChangedBySystem = false;

        iPad = 0;

        dwTexturePack = 0;

        worldSize = 3;
        seed = "";
        bDisableSaving = false;

        currentWorldSize = e_worldSize_Unknown;
        newWorldSize = e_worldSize_Unknown;
        newWorldSizeOverwriteEdges = false;
    }
} LaunchMoreOptionsMenuInitData;

typedef struct _LoadingInputParams {
    C4JThreadStartFunc* func;
    void* lpParam;
    UIFullscreenProgressCompletionData* completionData;

    int cancelText;
    void (*cancelFunc)(void* param);
    void (*completeFunc)(void* param);
    void* m_cancelFuncParam;
    void* m_completeFuncParam;
    bool waitForThreadToDelete;

    _LoadingInputParams() {
        func = nullptr;
        lpParam = nullptr;
        completionData = nullptr;

        cancelText = -1;
        cancelFunc = nullptr;
        completeFunc = nullptr;
        m_cancelFuncParam = nullptr;
        m_completeFuncParam = nullptr;
        waitForThreadToDelete = false;
    }
} LoadingInputParams;

// Tutorial
class UIScene;
class Tutorial;
typedef struct _TutorialPopupInfo {
    UIScene* interactScene;
    const char* desc;
    const char* title;
    int icon;
    int iAuxVal /* = 0 */;
    bool isFoil /* = false */;
    bool allowFade /* = true */;
    bool isReminder /*= false*/;
    Tutorial* tutorial;

    _TutorialPopupInfo() {
        interactScene = nullptr;
        desc = "";
        title = "";
        icon = -1;
        iAuxVal = 0;
        isFoil = false;
        allowFade = true;
        isReminder = false;
        tutorial = nullptr;
    }

} TutorialPopupInfo;

// Quadrant sign in
typedef struct _SignInInfo {
    std::function<int(bool, int)> Func;
    bool requireOnline;
} SignInInfo;

// Credits
struct SCreditTextItemDef {
    const char* m_Text;  // Should contain string, optionally with %s to add
                            // in translated string ... e.g. "Andy West - %s"
    int m_iStringID[2];  // May be NO_TRANSLATED_STRING if we do not require to
                         // add any translated string.
    ECreditTextTypes m_eType;
};

// Message box
typedef struct _MessageBoxInfo {
    uint32_t uiTitle;
    uint32_t uiText;
    uint32_t* uiOptionA;
    uint32_t uiOptionC;
    uint32_t dwPad;
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult);
    void* lpParam;
    // C4JStringTable *pStringTable; // 4J Stu - We don't need this for our
    // internal message boxes
    char* pwchFormatString;
    unsigned int dwFocusButton;
} MessageBoxInfo;

typedef struct _DLCOffersParam {
    int iPad;
    int iOfferC;
    int iType;
} DLCOffersParam;

typedef struct _InGamePlayerOptionsInitData {
    int iPad;
    std::uint8_t networkSmallId;
    unsigned int playerPrivileges;
} InGamePlayerOptionsInitData;

typedef struct _DebugSetCameraPosition {
    int player;
    double m_camX, m_camY, m_camZ, m_yRot, m_elev;
} DebugSetCameraPosition;

typedef struct _TeleportMenuInitData {
    int iPad;
    bool teleportToPlayer;
} TeleportMenuInitData;

typedef struct _CustomDrawData {
    float x0, y0, x1,
        y1;  // the bounding box of the original DisplayObject, in object space
    float mat[16];
} CustomDrawData;

typedef struct _ItemEditorInput {
    int iPad;
    Slot* slot;
    AbstractContainerMenu* menu;
} ItemEditorInput;