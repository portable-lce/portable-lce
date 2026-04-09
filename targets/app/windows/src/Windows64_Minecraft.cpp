// Minecraft.cpp : Defines the entry point for the application.
//

#include <assert.h>

#include <mutex>

#include "app/common/Network/Socket.h"
#include "minecraft/client/User.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/ConnectScreen.h"
#include "minecraft/client/player/LocalPlayer.h"
#include "minecraft/locale/Language.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/stats/StatsCounter.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/MapItem.h"
#include "minecraft/world/item/crafting/Recipes.h"
#include "minecraft/world/item/crafting/Recipy.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/Vec3.h"
#include "util/StringHelpers.h"
// #include "Social/SocialManager.h"
// #include "app/common/Leaderboards/LeaderboardManager.h"
// #include "../Common/XUI/XUI_Scene_Container.h"
// #include "NetworkManager.h"
#include "../Resource.h"
#include "Sentient/SentientManager.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/world/level/chunk/storage/OldChunkStorage.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"

HINSTANCE hMyInst;
LRESULT CALLBACK DlgProc(HWND hWndDlg, uint32_t Msg, WPARAM wParam,
                         LPARAM lParam);
char chGlobalText[256];
uint16_t ui16GlobalText[256];

#define THEME_NAME "584111F70AAAAAAA"
#define THEME_FILESIZE 2797568

// #define THREE_MB 3145728 // minimum save size (checking for this on a
// selected device) #define FIVE_MB 5242880 // minimum save size (checking for
// this on a selected device) #define FIFTY_TWO_MB (1024*1024*52) // Maximum TCR
// space required for a save (checking for this on a selected device)
#define FIFTY_ONE_MB \
    (1000000 * 51)  // Maximum TCR space required for a save is 52MB (checking
                    // for this on a selected device)

// #define PROFILE_VERSION 3 // new version for the interim bug fix 166 TU
#define NUM_PROFILE_VALUES 5
#define NUM_PROFILE_SETTINGS 4
uint32_t dwProfileSettingsA[NUM_PROFILE_VALUES] = {0, 0, 0, 0, 0};

//-------------------------------------------------------------------------------------
// Time             Since fAppTime is a float, we need to keep the quadword app
// time
//                  as a LARGE_INTEGER so that we don't lose precision after
//                  running for a long time.
//-------------------------------------------------------------------------------------

bool g_bWidescreen = true;

int g_iScreenWidth = 1920;
int g_iScreenHeight = 1080;

void DefineActions(void) {
    // The app needs to define the actions required, and the possible mappings
    // for these

    // Split into Menu actions, and in-game actions

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_A,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_B,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_X,
                                    _360_JOY_BUTTON_X);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_Y,
                                    _360_JOY_BUTTON_Y);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OK,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_CANCEL,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_0, ACTION_MENU_UP,
        _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_0, ACTION_MENU_DOWN,
        _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_0, ACTION_MENU_LEFT,
        _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_0, ACTION_MENU_RIGHT,
        _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEUP,
                                    _360_JOY_BUTTON_LT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEDOWN,
                                    _360_JOY_BUTTON_RT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_RIGHT_SCROLL,
                                    _360_JOY_BUTTON_RB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_LEFT_SCROLL,
                                    _360_JOY_BUTTON_LB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAUSEMENU,
                                    _360_JOY_BUTTON_START);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_STICK_PRESS,
                                    _360_JOY_BUTTON_LTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_PRESS,
                                    _360_JOY_BUTTON_RTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_UP,
                                    _360_JOY_BUTTON_RSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_DOWN,
                                    _360_JOY_BUTTON_RSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_LEFT,
                                    _360_JOY_BUTTON_RSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_RIGHT,
                                    _360_JOY_BUTTON_RSTICK_RIGHT);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_JUMP,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_FORWARD,
                                    _360_JOY_BUTTON_LSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_BACKWARD,
                                    _360_JOY_BUTTON_LSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LEFT,
                                    _360_JOY_BUTTON_LSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RIGHT,
                                    _360_JOY_BUTTON_LSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_LEFT,
                                    _360_JOY_BUTTON_RSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_RIGHT,
                                    _360_JOY_BUTTON_RSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_UP,
                                    _360_JOY_BUTTON_RSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_DOWN,
                                    _360_JOY_BUTTON_RSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_USE,
                                    _360_JOY_BUTTON_LT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_ACTION,
                                    _360_JOY_BUTTON_RT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RIGHT_SCROLL,
                                    _360_JOY_BUTTON_RB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LEFT_SCROLL,
                                    _360_JOY_BUTTON_LB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_INVENTORY,
                                    _360_JOY_BUTTON_Y);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_PAUSEMENU,
                                    _360_JOY_BUTTON_START);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DROP,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_SNEAK_TOGGLE,
                                    _360_JOY_BUTTON_RTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_CRAFTING,
                                    _360_JOY_BUTTON_X);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0,
                                    MINECRAFT_ACTION_RENDER_THIRD_PERSON,
                                    _360_JOY_BUTTON_LTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_GAME_INFO,
                                    _360_JOY_BUTTON_BACK);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_LEFT,
                                    _360_JOY_BUTTON_DPAD_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_RIGHT,
                                    _360_JOY_BUTTON_DPAD_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_UP,
                                    _360_JOY_BUTTON_DPAD_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_DOWN,
                                    _360_JOY_BUTTON_DPAD_DOWN);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_A,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_B,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_X,
                                    _360_JOY_BUTTON_X);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_Y,
                                    _360_JOY_BUTTON_Y);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OK,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_CANCEL,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_1, ACTION_MENU_UP,
        _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_1, ACTION_MENU_DOWN,
        _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_1, ACTION_MENU_LEFT,
        _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_1, ACTION_MENU_RIGHT,
        _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_PAGEUP,
                                    _360_JOY_BUTTON_LB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_PAGEDOWN,
                                    _360_JOY_BUTTON_RT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_RIGHT_SCROLL,
                                    _360_JOY_BUTTON_RB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_LEFT_SCROLL,
                                    _360_JOY_BUTTON_LB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_PAUSEMENU,
                                    _360_JOY_BUTTON_START);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_STICK_PRESS,
                                    _360_JOY_BUTTON_LTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_PRESS,
                                    _360_JOY_BUTTON_RTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_UP,
                                    _360_JOY_BUTTON_RSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_DOWN,
                                    _360_JOY_BUTTON_RSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_LEFT,
                                    _360_JOY_BUTTON_RSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, ACTION_MENU_OTHER_STICK_RIGHT,
                                    _360_JOY_BUTTON_RSTICK_RIGHT);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_JUMP,
                                    _360_JOY_BUTTON_RB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_FORWARD,
                                    _360_JOY_BUTTON_LSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_BACKWARD,
                                    _360_JOY_BUTTON_LSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LEFT,
                                    _360_JOY_BUTTON_LSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_RIGHT,
                                    _360_JOY_BUTTON_LSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_LEFT,
                                    _360_JOY_BUTTON_RSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_RIGHT,
                                    _360_JOY_BUTTON_RSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_UP,
                                    _360_JOY_BUTTON_RSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LOOK_DOWN,
                                    _360_JOY_BUTTON_RSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_USE,
                                    _360_JOY_BUTTON_RT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_ACTION,
                                    _360_JOY_BUTTON_LT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_RIGHT_SCROLL,
                                    _360_JOY_BUTTON_DPAD_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_LEFT_SCROLL,
                                    _360_JOY_BUTTON_DPAD_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_INVENTORY,
                                    _360_JOY_BUTTON_Y);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_PAUSEMENU,
                                    _360_JOY_BUTTON_START);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DROP,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_SNEAK_TOGGLE,
                                    _360_JOY_BUTTON_LTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_CRAFTING,
                                    _360_JOY_BUTTON_X);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1,
                                    MINECRAFT_ACTION_RENDER_THIRD_PERSON,
                                    _360_JOY_BUTTON_RTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_GAME_INFO,
                                    _360_JOY_BUTTON_BACK);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_LEFT,
                                    _360_JOY_BUTTON_DPAD_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_RIGHT,
                                    _360_JOY_BUTTON_DPAD_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_UP,
                                    _360_JOY_BUTTON_DPAD_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_1, MINECRAFT_ACTION_DPAD_DOWN,
                                    _360_JOY_BUTTON_DPAD_DOWN);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_A,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_B,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_X,
                                    _360_JOY_BUTTON_X);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_Y,
                                    _360_JOY_BUTTON_Y);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OK,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_CANCEL,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_2, ACTION_MENU_UP,
        _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_2, ACTION_MENU_DOWN,
        _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_2, ACTION_MENU_LEFT,
        _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_2, ACTION_MENU_RIGHT,
        _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(
        MAP_STYLE_2, ACTION_MENU_PAGEUP,
        _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_PAGEDOWN,
                                    _360_JOY_BUTTON_RT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_RIGHT_SCROLL,
                                    _360_JOY_BUTTON_RB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_LEFT_SCROLL,
                                    _360_JOY_BUTTON_LB);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_JUMP,
                                    _360_JOY_BUTTON_LT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_FORWARD,
                                    _360_JOY_BUTTON_LSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_BACKWARD,
                                    _360_JOY_BUTTON_LSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LEFT,
                                    _360_JOY_BUTTON_LSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_RIGHT,
                                    _360_JOY_BUTTON_LSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_LEFT,
                                    _360_JOY_BUTTON_RSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_RIGHT,
                                    _360_JOY_BUTTON_RSTICK_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_UP,
                                    _360_JOY_BUTTON_RSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LOOK_DOWN,
                                    _360_JOY_BUTTON_RSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_USE,
                                    _360_JOY_BUTTON_RT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_ACTION,
                                    _360_JOY_BUTTON_A);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_RIGHT_SCROLL,
                                    _360_JOY_BUTTON_DPAD_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_LEFT_SCROLL,
                                    _360_JOY_BUTTON_DPAD_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_INVENTORY,
                                    _360_JOY_BUTTON_Y);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_PAUSEMENU,
                                    _360_JOY_BUTTON_START);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DROP,
                                    _360_JOY_BUTTON_B);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_SNEAK_TOGGLE,
                                    _360_JOY_BUTTON_LB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_CRAFTING,
                                    _360_JOY_BUTTON_X);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2,
                                    MINECRAFT_ACTION_RENDER_THIRD_PERSON,
                                    _360_JOY_BUTTON_LTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_GAME_INFO,
                                    _360_JOY_BUTTON_BACK);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_PAUSEMENU,
                                    _360_JOY_BUTTON_START);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_STICK_PRESS,
                                    _360_JOY_BUTTON_LTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_PRESS,
                                    _360_JOY_BUTTON_RTHUMB);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_UP,
                                    _360_JOY_BUTTON_RSTICK_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_DOWN,
                                    _360_JOY_BUTTON_RSTICK_DOWN);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_LEFT,
                                    _360_JOY_BUTTON_RSTICK_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, ACTION_MENU_OTHER_STICK_RIGHT,
                                    _360_JOY_BUTTON_RSTICK_RIGHT);

    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_LEFT,
                                    _360_JOY_BUTTON_DPAD_LEFT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_RIGHT,
                                    _360_JOY_BUTTON_DPAD_RIGHT);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_UP,
                                    _360_JOY_BUTTON_DPAD_UP);
    PlatformInput.SetGameJoypadMaps(MAP_STYLE_2, MINECRAFT_ACTION_DPAD_DOWN,
                                    _360_JOY_BUTTON_DPAD_DOWN);
}

HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;

//
//  FUNCTION: WndProc(HWND, uint32_t, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, uint32_t message, WPARAM wParam,
                         LPARAM lParam) {
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
        case WM_COMMAND:
            wmId = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, "Minecraft");
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = "Minecraft";
    wcex.lpszClassName = "MinecraftClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
bool InitInstance(HINSTANCE hInstance, int nCmdShow) {
    g_hInst = hInstance;  // Store instance handle in our global variable

    RECT wr = {0, 0, g_iScreenWidth,
               g_iScreenHeight};  // set the size, but not the position
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);  // adjust the size

    g_hWnd = CreateWindow("MinecraftClass", "Minecraft", WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, 0,
                          wr.right - wr.left,  // width of the window
                          wr.bottom - wr.top,  // height of the window
                          nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) {
        return false;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return true;
}

// 4J Stu - These functions are referenced from the Windows Input library
void ClearGlobalText() {
    // clear the global text
    memset(chGlobalText, 0, 256);
    memset(ui16GlobalText, 0, 512);
}

uint16_t* GetGlobalText() {
    // copy the ch text to ui16
    char* pchBuffer = (char*)ui16GlobalText;
    for (int i = 0; i < 256; i++) {
        pchBuffer[i * 2] = chGlobalText[i];
    }
    return ui16GlobalText;
}
void SeedEditBox() {
    DialogBox(hMyInst, MAKEINTRESOURCE(IDD_SEED), g_hWnd,
              reinterpret_cast<DLGPROC>(DlgProc));
}

//---------------------------------------------------------------------------
LRESULT CALLBACK DlgProc(HWND hWndDlg, uint32_t Msg, WPARAM wParam,
                         LPARAM lParam) {
    switch (Msg) {
        case WM_INITDIALOG:
            return true;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    // Set the text
                    GetDlgItemText(hWndDlg, IDC_EDIT, chGlobalText, 256);
                    EndDialog(hWndDlg, 0);
                    return true;
            }
            break;
    }

    return false;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
int32_t InitDevice() {
    int32_t hr = 0;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    uint32_t width = rc.right - rc.left;
    uint32_t height = rc.bottom - rc.top;
    // app.DebugPrintf("width: %d, height: %d\n", width, height);
    width = g_iScreenWidth;
    height = g_iScreenHeight;
    app.DebugPrintf("width: %d, height: %d\n", width, height);

    uint32_t createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    uint32_t numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    uint32_t numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    memset(&sd, 0, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = true;

    for (uint32_t driverTypeIndex = 0; driverTypeIndex < numDriverTypes;
         driverTypeIndex++) {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels,
            numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        if (HRESULT_SUCCEEDED(hr)) break;
    }
    if (FAILED(hr)) return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                 (void**)&pBackBuffer);
    if (FAILED(hr)) return hr;

    // Create a depth stencil buffer
    D3D11_TEXTURE2D_DESC descDepth;

    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr,
                                       &g_pDepthStencilBuffer);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSView;
    descDSView.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSView.Texture2D.MipSlice = 0;

    hr = g_pd3dDevice->CreateDepthStencilView(
        g_pDepthStencilBuffer, &descDSView, &g_pDepthStencilView);

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                              &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView,
                                            g_pDepthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

    PlatformRenderer.Initialise(g_pd3dDevice, g_pSwapChain);

    return 0;
}

//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render() {
    // Just clear the backbuffer
    float ClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f};  // red,green,blue,alpha

    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
    g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice() {
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine,
                       _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (lpCmdLine) {
        if (lpCmdLine[0] == '1') {
            g_iScreenWidth = 1280;
            g_iScreenHeight = 720;
        } else if (lpCmdLine[0] == '2') {
            g_iScreenWidth = 640;
            g_iScreenHeight = 480;
        } else if (lpCmdLine[0] == '3') {
            // Vita
            g_iScreenWidth = 720;
            g_iScreenHeight = 408;

            // Vita native
            // g_iScreenWidth = 960;
            // g_iScreenHeight = 544;
        }
    }

    // Initialize global strings
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return false;
    }

    hMyInst = hInstance;

    if (FAILED(InitDevice())) {
        CleanupDevice();
        return 0;
    }

    static bool bTrialTimerDisplayed = true;

    app.loadMediaArchive();

    PlatformRenderer.Initialise(g_pd3dDevice, g_pSwapChain);

    app.loadStringTable();
    ui.init(g_pd3dDevice, g_pImmediateContext, g_pRenderTargetView,
            g_pDepthStencilView, g_iScreenWidth, g_iScreenHeight);

    ////////////////
    // Initialise //
    ////////////////

    // Set the number of possible joypad layouts that the user can switch
    // between, and the number of actions
    PlatformInput.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);

    // Set the default joypad action mappings for Minecraft
    DefineActions();
    PlatformInput.SetJoypadMapVal(0, 0);
    PlatformInput.SetKeyRepeatRate(0.3f, 0.2f);

    // Initialise the profile manager with the game Title ID, Offer ID, a
    // profile version number, and the number of profile values and settings
    PlatformProfile.Initialise(
        TITLEID_MINECRAFT, app.m_dwOfferID, PROFILE_VERSION_10,
        NUM_PROFILE_VALUES, NUM_PROFILE_SETTINGS, dwProfileSettingsA,
        app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
        &app.uiGameDefinedDataChangedBitmask);
    // Set a callback for the default player options to be set - when there is
    // no profile data for the player
    PlatformProfile.SetDefaultOptionsCallback(
        [](IPlatformProfile::PROFILESETTINGS* pSettings, int iPad) {
            return Game::DefaultOptionsCallback(&app, pSettings, iPad);
        });
    // QNet needs to be setup after profile manager, as we do not want its
    // Notify listener to handle XN_SYS_SIGNINCHANGED notifications. This does
    // mean that we need to have a callback in the PlatformProfile for
    // XN_LIVE_INVITE_ACCEPTED for QNet.
    g_NetworkManager.Initialise();

    // 4J-PB moved further down
    // app.InitGameSettings();

    // debug switch to trial version
    PlatformProfile.SetDebugFullOverride(true);

    // Initialise TLS for tesselator, for this main thread
    Tesselator::CreateNewThreadStorage(1024 * 1024);
    // Initialise TLS for AABB and Vec3 pools, for this main thread
    Compression::CreateNewThreadStorage();
    OldChunkStorage::CreateNewThreadStorage();
    Level::enableLightingCache();
    Tile::CreateNewThreadStorage();

    Minecraft::main();
    Minecraft* pMinecraft = Minecraft::GetInstance();

    app.InitGameSettings();

    app.InitialiseTips();

    // Set the default sound levels
    pMinecraft->options->set(Options::Option::MUSIC, 1.0f);
    pMinecraft->options->set(Options::Option::SOUND, 1.0f);

    // app.TemporaryCreateGameStart();

    // Sleep(10000);
    MSG msg = {0};
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        PlatformRenderer.StartFrame();

        // 		static bool bPlay=false;
        // 		if(bPlay)
        // 		{
        // 			bPlay=false;
        // 			app.audio.PlaySound();
        // 		}

        app.UpdateTime();
        PlatformInput.Tick();

        //		PlatformProfile.Tick();

        PlatformStorage.Tick();

        PlatformRenderer.Tick();

        // Tick the social networking manager.
        //		CSocialManager::Instance()->Tick();

        // Tick sentient.
        //		SentientManager.Tick();

        //		g_NetworkManager.DoWork();

        //		LeaderboardManager::Instance()->Tick();
        // Render game graphics.
        if (app.GetGameStarted()) {
            pMinecraft->run_middle();
            app.SetAppPaused(
                g_NetworkManager.IsLocalGame() &&
                g_NetworkManager.GetPlayerCount() == 1 &&
                ui.IsPauseMenuDisplayed(PlatformProfile.GetPrimaryPad()));
        } else {
            pMinecraft->soundEngine->tick(nullptr, 0.0f);
            pMinecraft->textures->tick(true, false);
            if (app.GetReallyChangingSessionType()) {
                pMinecraft
                    ->tickAllConnections();  // Added to stop timing out when we
                                             // are waiting after converting to
                                             // an offline game
            }
        }

        pMinecraft->soundEngine->playMusicTick();

        ui.tick();
        ui.render();
        // Present the frame.
        PlatformRenderer.Present();

        ui.CheckMenuDisplayed();
        // Any threading type things to deal with from the xui side?
        app.HandleXuiActions();

        // need to turn off the trial timer if it was on
        if (bTrialTimerDisplayed) {
            ui.ShowTrialTimer(false);
            bTrialTimerDisplayed = false;
        }

        // Fix for #7318 - Title crashes after short soak in the leaderboards
    }

    // Free resources, unregister custom classes, and exit.
    //	app.Uninit();
    g_pd3dDevice->Release();
}
