#include "SDL2Input.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <functional>
#include <string>

#include "../../PlatformTypes.h"
#include "../InputConstants.h"
#include "SDL.h"
#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "SDL_joystick.h"
#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "SDL_scancode.h"
#include "SDL_stdinc.h"
#include "SDL_video.h"
#include "begin_code.h"

#if defined(__APPLE__)
#include "SDL2CursorPatch.h"
#endif

namespace platform_internal {
IPlatformInput& PlatformInput_get() {
    static SDL2Input instance;
    return instance;
}
}  // namespace platform_internal

static const int KEY_COUNT = SDL_NUM_SCANCODES;
static const int BTN_COUNT = SDL_CONTROLLER_BUTTON_MAX;
static const int AXS_COUNT = SDL_CONTROLLER_AXIS_MAX;
static const float MOUSE_SCALE = 0.015f;
// Vars
static bool s_sdlInitialized = false;
static bool s_keysCurrent[KEY_COUNT] = {};
static bool s_keysPrev[KEY_COUNT] = {};
static bool s_btnsCurrent[BTN_COUNT] = {};
static bool s_btnsPrev[BTN_COUNT] = {};
static bool s_axisCurrent[AXS_COUNT] = {};
static bool s_axisPrev[AXS_COUNT] = {};
static float axisVal[AXS_COUNT] = {};
static bool s_mouseLeftCurrent = false, s_mouseLeftPrev = false;
static bool s_mouseRightCurrent = false, s_mouseRightPrev = false;
static bool s_menuDisplayed[4] = {};
static bool s_prevMenuDisplayed = false;
static bool s_snapTaken = false;
static float s_accumRelX = 0, s_accumRelY = 0;
static float s_snapRelX = 0, s_snapRelY = 0;
static int s_mouseX = 0, s_mouseY = 0;

static int s_scrollTicksForButtonPressed = 0;
static int s_scrollTicksForGetValue = 0;
static int s_scrollTicksSnap = 0;
static bool s_scrollSnapTaken = false;

// Text input state (non-blocking keyboard)
static bool s_keyboardActive = false;
static std::string s_textInputBuf;
static std::function<int(bool)> s_keyboardCallback;

// We set all the watched keys
// I don't know if I'll need to change this if we add chat support soon.
static const int s_watchedKeys[] = {
    SDL_SCANCODE_W,      SDL_SCANCODE_A,      SDL_SCANCODE_S,
    SDL_SCANCODE_D,      SDL_SCANCODE_SPACE,  SDL_SCANCODE_LSHIFT,
    SDL_SCANCODE_RSHIFT, SDL_SCANCODE_E,      SDL_SCANCODE_Q,
    SDL_SCANCODE_F,      SDL_SCANCODE_C,      SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_RETURN, SDL_SCANCODE_F3,     SDL_SCANCODE_F5,
    SDL_SCANCODE_UP,     SDL_SCANCODE_DOWN,   SDL_SCANCODE_LEFT,
    SDL_SCANCODE_RIGHT,  SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEDOWN,
    SDL_SCANCODE_TAB,    SDL_SCANCODE_LCTRL,  SDL_SCANCODE_RCTRL,
    SDL_SCANCODE_1,      SDL_SCANCODE_2,      SDL_SCANCODE_3,
    SDL_SCANCODE_4,      SDL_SCANCODE_5,      SDL_SCANCODE_6,
    SDL_SCANCODE_7,      SDL_SCANCODE_8,      SDL_SCANCODE_9,
    SDL_SCANCODE_Z,      SDL_SCANCODE_X,      SDL_SCANCODE_C,
    SDL_SCANCODE_V};
static const int s_watchedKeyCount =
    (int)(sizeof(s_watchedKeys) / sizeof(s_watchedKeys[0]));

static inline bool KDown(int sc) {
    return (sc > 0 && sc < KEY_COUNT) ? s_keysCurrent[sc] : false;
}
static inline bool KPressed(int sc) {
    return (sc > 0 && sc < KEY_COUNT) ? !s_keysPrev[sc] && s_keysCurrent[sc]
                                      : false;
}
static inline bool KReleased(int sc) {
    return (sc > 0 && sc < KEY_COUNT) ? s_keysPrev[sc] && !s_keysCurrent[sc]
                                      : false;
}

static inline bool MouseLDown() { return s_mouseLeftCurrent; }
static inline bool MouseLPressed() {
    return s_mouseLeftCurrent && !s_mouseLeftPrev;
}
static inline bool MouseLReleased() {
    return !s_mouseLeftCurrent && s_mouseLeftPrev;
}
static inline bool MouseRDown() { return s_mouseRightCurrent; }
static inline bool MouseRPressed() {
    return s_mouseRightCurrent && !s_mouseRightPrev;
}
static inline bool MouseRReleased() {
    return !s_mouseRightCurrent && s_mouseRightPrev;
}

// holds controller object
static SDL_GameController* controller = nullptr;

// Watched controller buttons set
static const SDL_GameControllerButton s_watchedBtns[] = {
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
static const int s_watchedBtnsCount =
    (int)(sizeof(s_watchedBtns) / sizeof(s_watchedBtns[0]));

static inline bool CDown(int cb) {
    return (cb >= 0 && cb < BTN_COUNT) ? s_btnsCurrent[cb] : false;
}
static inline bool CPressed(int cb) {
    return (cb >= 0 && cb < BTN_COUNT) ? !s_btnsPrev[cb] && s_btnsCurrent[cb]
                                       : false;
}
static inline bool CReleased(int cb) {
    return (cb >= 0 && cb < BTN_COUNT) ? s_btnsPrev[cb] && !s_btnsCurrent[cb]
                                       : false;
}

// Sets controller dead zone
static int deadZone = 8000;

// Watched controller axes set
static const SDL_GameControllerAxis s_watchedAxis[] = {
    SDL_CONTROLLER_AXIS_LEFTX,       SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,      SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT};
static const int s_watchedAxisCount =
    (int)(sizeof(s_watchedAxis) / sizeof(s_watchedAxis[0]));

static inline bool ADown(int ca) {
    return (ca >= 0 && ca < AXS_COUNT) ? s_axisCurrent[ca] : false;
}
static inline bool APressed(int ca) {
    return (ca >= 0 && ca < AXS_COUNT) ? !s_axisPrev[ca] && s_axisCurrent[ca]
                                       : false;
}
static inline bool AReleased(int ca) {
    return (ca >= 0 && ca < AXS_COUNT) ? s_axisPrev[ca] && !s_axisCurrent[ca]
                                       : false;
}

// get directly into SDL events before the game queue can steal them.
// this took me a while.
static int SDLCALL EventWatcher(void*, SDL_Event* e) {
    if (e->type == SDL_MOUSEWHEEL) {
        int y = e->wheel.y;
        if (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
            y = -y;
        }
        s_scrollTicksForGetValue += y;
        s_scrollTicksForButtonPressed += y;
    } else if (e->type == SDL_MOUSEBUTTONDOWN) {
        if (e->button.button == 4) {
            s_scrollTicksForGetValue++;
            s_scrollTicksForButtonPressed++;
        } else if (e->button.button == 5) {
            s_scrollTicksForGetValue--;
            s_scrollTicksForButtonPressed--;
        }
    } else if (e->type == SDL_MOUSEMOTION) {
        s_accumRelX += (float)e->motion.xrel;
        s_accumRelY += (float)e->motion.yrel;
    } else if (e->type == SDL_TEXTINPUT && s_keyboardActive) {
        s_textInputBuf += e->text.text;
    } else if (e->type == SDL_CONTROLLERDEVICEADDED) {  // Will search for
                                                        // controller if none
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                break;
            }
        }
    } else if (controller) {  // only checks when a controller exists
        if (e->type == SDL_CONTROLLERDEVICEREMOVED) {
            SDL_Joystick* joy = SDL_GameControllerGetJoystick(controller);
            if (SDL_JoystickInstanceID(joy) == e->cdevice.which) {
                SDL_GameControllerClose(controller);
                controller = nullptr;
            }
        } else if (e->type == SDL_CONTROLLERBUTTONDOWN) {
            if (e->cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                s_scrollTicksForGetValue++;
                s_scrollTicksForButtonPressed++;
            } else if (e->cbutton.button ==
                       SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                s_scrollTicksForGetValue--;
                s_scrollTicksForButtonPressed--;
            }
        }
    }
    return 1;
}

static int ScrollSnap() {
    if (!s_scrollSnapTaken) {
        s_scrollTicksSnap = s_scrollTicksForButtonPressed;
        s_scrollTicksForButtonPressed = 0;
        s_scrollSnapTaken = true;
    }
    return s_scrollTicksSnap;
}

static void TakeSnapIfNeeded() {
    if (!s_snapTaken) {
        s_snapRelX = s_accumRelX;
        s_accumRelX = 0;
        s_snapRelY = s_accumRelY;
        s_accumRelY = 0;
        s_snapTaken = true;
    }
}
// We initialize the SDL input
void SDL2Input::Initialise(int, unsigned char, unsigned char, unsigned char) {
    if (!s_sdlInitialized) {
#if defined(__APPLE__)
        PatchSDLInvisibleCursor();
#endif
        if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
            SDL_Init(SDL_INIT_VIDEO);
        }
        if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
            SDL_Init(SDL_INIT_GAMECONTROLLER);
        }
        SDL_AddEventWatch(EventWatcher, NULL);
        s_sdlInitialized = true;
    }

    memset(s_keysCurrent, 0, sizeof(s_keysCurrent));
    memset(s_keysPrev, 0, sizeof(s_keysPrev));
    memset(s_btnsCurrent, 0, sizeof(s_btnsCurrent));
    memset(s_btnsPrev, 0, sizeof(s_btnsPrev));
    memset(s_axisCurrent, 0, sizeof(s_axisCurrent));
    memset(s_axisPrev, 0, sizeof(s_axisPrev));
    memset(s_menuDisplayed, 0, sizeof(s_menuDisplayed));

    s_mouseLeftCurrent = s_mouseLeftPrev = s_mouseRightCurrent =
        s_mouseRightPrev = false;
    s_accumRelX = s_accumRelY = s_snapRelX = s_snapRelY = 0;
    // i really gotta name these vars better..
    s_scrollTicksForButtonPressed = s_scrollTicksForGetValue =
        s_scrollTicksSnap = 0;
    s_snapTaken = s_scrollSnapTaken = s_prevMenuDisplayed = false;

    if (s_sdlInitialized) {
        SDL_SetRelativeMouseMode(SDL_TRUE);

        // looks for controller
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                break;
            }
        }
    }
}
// Erase one UTF-8 codepoint from the end of a string.
static void utf8_pop_back(std::string& str) {
    if (str.empty()) return;
    size_t i = str.size() - 1;
    while (i > 0 && (str[i] & 0xC0) == 0x80) --i;
    str.erase(i);
}

// Each tick we update the input state by polling SDL, this is where we get the
// kbd and mouse state.
void SDL2Input::Tick() {
    if (!s_sdlInitialized) return;

    memcpy(s_keysPrev, s_keysCurrent, sizeof(s_keysCurrent));
    memcpy(s_btnsPrev, s_btnsCurrent, sizeof(s_btnsCurrent));
    memcpy(s_axisPrev, s_axisCurrent, sizeof(s_axisCurrent));
    s_mouseLeftPrev = s_mouseLeftCurrent;
    s_mouseRightPrev = s_mouseRightCurrent;
    s_snapTaken = false;
    s_scrollSnapTaken = false;
    s_snapRelX = s_snapRelY = 0;
    s_scrollTicksSnap = 0;

    SDL_PumpEvents();

    if (s_menuDisplayed[0]) {
        s_scrollTicksForGetValue = 0;
    }

    const Uint8* state = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < s_watchedKeyCount; ++i) {
        int sc = s_watchedKeys[i];
        if (sc > 0 && sc < KEY_COUNT) s_keysCurrent[sc] = state[sc] != 0;
    }

    Uint32 btns = SDL_GetMouseState(&s_mouseX, &s_mouseY);
    s_mouseLeftCurrent = (btns & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    s_mouseRightCurrent = (btns & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

    if (!SDL_GetRelativeMouseMode()) {
        s_accumRelX = 0;
        s_accumRelY = 0;
    }

    if (!SDL_GetKeyboardFocus()) {
        SDL_Window* mf = SDL_GetMouseFocus();
        if (mf) {
            SDL_RaiseWindow(mf);
            SDL_SetWindowGrab(mf, SDL_TRUE);
        }
    }

    // If there is a controller update the buttons and sticks
    if (controller) {
        for (int i = 0; i < s_watchedBtnsCount; ++i) {
            int cb = s_watchedBtns[i];
            if (cb >= 0 && cb < BTN_COUNT)
                s_btnsCurrent[cb] =
                    SDL_GameControllerGetButton(controller, s_watchedBtns[i]);
        }
        for (int i = 0; i < s_watchedAxisCount; ++i) {
            int ca = s_watchedAxis[i];
            if (ca >= 0 && ca < AXS_COUNT) {
                int aVal =
                    SDL_GameControllerGetAxis(controller, s_watchedAxis[i]);
                if (s_watchedAxis[i] == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
                    s_watchedAxis[i] == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                    s_axisCurrent[ca] = aVal > deadZone;
                else {
                    s_axisCurrent[ca] = (aVal > deadZone || aVal < -deadZone);
                    axisVal[ca] = aVal / 32768.0f;
                }
            }
        }
    }

    // Handle non-blocking keyboard input completion
    if (s_keyboardActive) {
        if (KPressed(SDL_SCANCODE_BACKSPACE)) {
            utf8_pop_back(s_textInputBuf);
        }
        if (KPressed(SDL_SCANCODE_RETURN) || KPressed(SDL_SCANCODE_KP_ENTER)) {
            s_keyboardActive = false;
            SDL_StopTextInput();
            // Consume the key so it doesn't also trigger ACTION_MENU_OK
            s_keysCurrent[SDL_SCANCODE_RETURN] = false;
            s_keysCurrent[SDL_SCANCODE_KP_ENTER] = false;
            if (s_keyboardCallback) {
                s_keyboardCallback(true);
                s_keyboardCallback = nullptr;
            }
        } else if (KPressed(SDL_SCANCODE_ESCAPE)) {
            s_keyboardActive = false;
            s_textInputBuf.clear();
            SDL_StopTextInput();
            // Consume the key so it doesn't also trigger ACTION_MENU_CANCEL
            s_keysCurrent[SDL_SCANCODE_ESCAPE] = false;
            if (s_keyboardCallback) {
                s_keyboardCallback(false);
                s_keyboardCallback = nullptr;
            }
        }
    }
}

int SDL2Input::GetHotbarSlotPressed(int iPad) {
    if (iPad != 0) return -1;

    constexpr size_t NUM_HOTBAR_SLOTS = 9;

    static const int sc[NUM_HOTBAR_SLOTS] = {
        SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
        SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6,
        SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9,
    };
    static bool s_wasDown[NUM_HOTBAR_SLOTS] = {};

    for (int i = 0; i < NUM_HOTBAR_SLOTS; ++i) {
        bool down = KDown(sc[i]);
        bool pressed = down && !s_wasDown[i];
        s_wasDown[i] = down;
        if (pressed) return i;
    }
    return -1;
}

// KFN = Keyboard functions, CFN = Controller functions, AFN = Axis functions
#define ACTION_CASES(KFN, CFN, AFN)                                            \
    case ACTION_MENU_UP:                                                       \
        return KFN(SDL_SCANCODE_UP) || CFN(SDL_CONTROLLER_BUTTON_DPAD_UP);     \
    case ACTION_MENU_DOWN:                                                     \
        return KFN(SDL_SCANCODE_DOWN) || CFN(SDL_CONTROLLER_BUTTON_DPAD_DOWN); \
    case ACTION_MENU_LEFT:                                                     \
        return KFN(SDL_SCANCODE_LEFT) || CFN(SDL_CONTROLLER_BUTTON_DPAD_LEFT); \
    case ACTION_MENU_RIGHT:                                                    \
        return KFN(SDL_SCANCODE_RIGHT) ||                                      \
               CFN(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);                          \
    case ACTION_MENU_PAGEUP:                                                   \
        return KFN(SDL_SCANCODE_PAGEUP);                                       \
    case ACTION_MENU_PAGEDOWN:                                                 \
        return KFN(SDL_SCANCODE_PAGEDOWN);                                     \
    case ACTION_MENU_OK:                                                       \
        return KFN(SDL_SCANCODE_RETURN) || KFN(SDL_SCANCODE_Z) ||              \
               CFN(SDL_CONTROLLER_BUTTON_A);                                   \
    case ACTION_MENU_CANCEL:                                                   \
        return KFN(SDL_SCANCODE_ESCAPE) || KFN(SDL_SCANCODE_X) ||              \
               CFN(SDL_CONTROLLER_BUTTON_B);                                   \
    case ACTION_MENU_A:                                                        \
        return KFN(SDL_SCANCODE_Z) || KFN(SDL_SCANCODE_RETURN) ||              \
               CFN(SDL_CONTROLLER_BUTTON_A);                                   \
    case ACTION_MENU_B:                                                        \
        return KFN(SDL_SCANCODE_X) || KFN(SDL_SCANCODE_ESCAPE) ||              \
               CFN(SDL_CONTROLLER_BUTTON_B);                                   \
    case ACTION_MENU_X:                                                        \
        return KFN(SDL_SCANCODE_C) || CFN(SDL_CONTROLLER_BUTTON_X);            \
    case ACTION_MENU_Y:                                                        \
        return KFN(SDL_SCANCODE_V) || CFN(SDL_CONTROLLER_BUTTON_Y);            \
    case MINECRAFT_ACTION_JUMP:                                                \
        return KFN(SDL_SCANCODE_SPACE) || CFN(SDL_CONTROLLER_BUTTON_A);        \
    case MINECRAFT_ACTION_FORWARD:                                             \
        return KFN(SDL_SCANCODE_W) || AFN(SDL_CONTROLLER_AXIS_LEFTY);          \
    case MINECRAFT_ACTION_BACKWARD:                                            \
        return KFN(SDL_SCANCODE_S) || AFN(SDL_CONTROLLER_AXIS_LEFTY);          \
    case MINECRAFT_ACTION_LEFT:                                                \
        return KFN(SDL_SCANCODE_A) || AFN(SDL_CONTROLLER_AXIS_LEFTX);          \
    case MINECRAFT_ACTION_RIGHT:                                               \
        return KFN(SDL_SCANCODE_D) || AFN(SDL_CONTROLLER_AXIS_LEFTX);          \
    case MINECRAFT_ACTION_INVENTORY:                                           \
        return KFN(SDL_SCANCODE_E) || CFN(SDL_CONTROLLER_BUTTON_Y);            \
    case MINECRAFT_ACTION_PAUSEMENU:                                           \
        return KFN(SDL_SCANCODE_ESCAPE) || CFN(SDL_CONTROLLER_BUTTON_START);   \
    case MINECRAFT_ACTION_DROP:                                                \
        return KFN(SDL_SCANCODE_Q) || CFN(SDL_CONTROLLER_BUTTON_B);            \
    case MINECRAFT_ACTION_CRAFTING:                                            \
        return KFN(SDL_SCANCODE_C) || CFN(SDL_CONTROLLER_BUTTON_X);            \
    case MINECRAFT_ACTION_RENDER_THIRD_PERSON:                                 \
        return KFN(SDL_SCANCODE_F5) || CFN(SDL_CONTROLLER_BUTTON_LEFTSTICK);   \
    case MINECRAFT_ACTION_GAME_INFO:                                           \
        return KFN(SDL_SCANCODE_F3);                                           \
    case MINECRAFT_ACTION_DPAD_LEFT:                                           \
        return KFN(SDL_SCANCODE_LEFT) || CFN(SDL_CONTROLLER_BUTTON_DPAD_LEFT); \
    case MINECRAFT_ACTION_DPAD_RIGHT:                                          \
        return KFN(SDL_SCANCODE_RIGHT) ||                                      \
               CFN(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);                          \
    case MINECRAFT_ACTION_DPAD_UP:                                             \
        return KFN(SDL_SCANCODE_UP) || CFN(SDL_CONTROLLER_BUTTON_DPAD_UP);     \
    case MINECRAFT_ACTION_DPAD_DOWN:                                           \
        return KFN(SDL_SCANCODE_DOWN) || CFN(SDL_CONTROLLER_BUTTON_DPAD_DOWN); \
    default:                                                                   \
        return false;

bool SDL2Input::ButtonDown(int iPad, unsigned char ucAction) {
    if (iPad != 0) return false;
    if (s_keyboardActive) return false;
    if (ucAction == 255) {
        for (int i = 0; i < s_watchedKeyCount; ++i)
            if (s_keysCurrent[s_watchedKeys[i]]) return true;
        return s_mouseLeftCurrent || s_mouseRightCurrent;
    }
    switch (ucAction) {
        case MINECRAFT_ACTION_ACTION:
            return MouseLDown() || KDown(SDL_SCANCODE_RETURN) ||
                   ADown(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        case MINECRAFT_ACTION_USE:
            return MouseRDown() || KDown(SDL_SCANCODE_F) ||
                   ADown(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        case MINECRAFT_ACTION_SNEAK_TOGGLE:
            return KDown(SDL_SCANCODE_LSHIFT) || KDown(SDL_SCANCODE_RSHIFT) ||
                   CDown(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
        case MINECRAFT_ACTION_SPRINT:
            return KDown(SDL_SCANCODE_LCTRL) || KDown(SDL_SCANCODE_RCTRL);
        case MINECRAFT_ACTION_LEFT_SCROLL:
        case ACTION_MENU_LEFT_SCROLL:
            return ScrollSnap() > 0;
        case MINECRAFT_ACTION_RIGHT_SCROLL:
        case ACTION_MENU_RIGHT_SCROLL:
            return ScrollSnap() < 0;
            ACTION_CASES(KDown, CDown, ADown)
    }
}
// The part that handles completing the action of pressing a button.
bool SDL2Input::ButtonPressed(int iPad, unsigned char ucAction) {
    if (iPad != 0 || ucAction == 255) return false;
    if (s_keyboardActive) return false;
    switch (ucAction) {
        case MINECRAFT_ACTION_ACTION:
            return MouseLPressed() || KPressed(SDL_SCANCODE_RETURN) ||
                   APressed(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        case MINECRAFT_ACTION_USE:
            return MouseRPressed() || KPressed(SDL_SCANCODE_F) ||
                   APressed(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        case MINECRAFT_ACTION_SNEAK_TOGGLE:
            return KPressed(SDL_SCANCODE_LSHIFT) ||
                   KPressed(SDL_SCANCODE_RSHIFT) ||
                   CPressed(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
        case MINECRAFT_ACTION_SPRINT:
            return KPressed(SDL_SCANCODE_LCTRL) || KPressed(SDL_SCANCODE_RCTRL);
        case MINECRAFT_ACTION_LEFT_SCROLL:
        case ACTION_MENU_LEFT_SCROLL:
            return ScrollSnap() > 0;
        case MINECRAFT_ACTION_RIGHT_SCROLL:
        case ACTION_MENU_RIGHT_SCROLL:
            return ScrollSnap() < 0;
            ACTION_CASES(KPressed, CPressed, APressed)
    }
}
// The part that handles Releasing a button.
bool SDL2Input::ButtonReleased(int iPad, unsigned char ucAction) {
    if (iPad != 0 || ucAction == 255) return false;
    if (s_keyboardActive) return false;
    switch (ucAction) {
        case MINECRAFT_ACTION_ACTION:
            return MouseLReleased() || KReleased(SDL_SCANCODE_RETURN) ||
                   AReleased(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        case MINECRAFT_ACTION_USE:
            return MouseRReleased() || KReleased(SDL_SCANCODE_F) ||
                   AReleased(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        case MINECRAFT_ACTION_SNEAK_TOGGLE:
            return KReleased(SDL_SCANCODE_LSHIFT) ||
                   KReleased(SDL_SCANCODE_RSHIFT) ||
                   CReleased(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
        case MINECRAFT_ACTION_SPRINT:
            KReleased(SDL_SCANCODE_LCTRL) || KReleased(SDL_SCANCODE_RCTRL);
        case MINECRAFT_ACTION_LEFT_SCROLL:
        case ACTION_MENU_LEFT_SCROLL:
        case MINECRAFT_ACTION_RIGHT_SCROLL:
        case ACTION_MENU_RIGHT_SCROLL:
            return false;
            ACTION_CASES(KReleased, CReleased, AReleased)
    }
}

unsigned int SDL2Input::GetValue(int iPad, unsigned char ucAction, bool) {
    if (iPad != 0) return 0;
    if (ucAction == MINECRAFT_ACTION_LEFT_SCROLL) {
        if (s_scrollTicksForGetValue > 0) {
            unsigned int v = (unsigned int)s_scrollTicksForGetValue;
            s_scrollTicksForGetValue = 0;
            return v;
        }
        return 0u;
    }
    if (ucAction == MINECRAFT_ACTION_RIGHT_SCROLL) {
        if (s_scrollTicksForGetValue < 0) {
            unsigned int v = (unsigned int)(-s_scrollTicksForGetValue);
            s_scrollTicksForGetValue = 0;
            return v;
        }
        return 0u;
    }
    return ButtonDown(iPad, ucAction) ? 1u : 0u;
}
// Left stick movement, the one that moves the player around or selects menu
// options. (Soon be tested.)
float SDL2Input::GetJoypadStick_LX(int, bool) {
    if (ADown(SDL_CONTROLLER_AXIS_LEFTX))
        return axisVal[SDL_CONTROLLER_AXIS_LEFTX];
    return (KDown(SDL_SCANCODE_D) ? 1.f : 0.f) -
           (KDown(SDL_SCANCODE_A) ? 1.f : 0.f);
}
float SDL2Input::GetJoypadStick_LY(int, bool) {
    if (ADown(SDL_CONTROLLER_AXIS_LEFTY))
        return -axisVal[SDL_CONTROLLER_AXIS_LEFTY];
    return (KDown(SDL_SCANCODE_W) ? 1.f : 0.f) -
           (KDown(SDL_SCANCODE_S) ? 1.f : 0.f);
}
// We use mouse movement and convert it into a Right Stick output using
// logarithmic scaling This is the most important mouse part. Yet it's so small.
static float MouseAxis(float raw) {
    if (fabsf(raw) < 0.0001f) return 0.f;  // from 4j previous code
    return (raw >= 0.f ? 1.f : -1.f) * sqrtf(fabsf(raw));
}
// We apply the Stick movement on the R(Right) X(2D Position)
float SDL2Input::GetJoypadStick_RX(int, bool) {
    if (ADown(SDL_CONTROLLER_AXIS_RIGHTX))
        return axisVal[SDL_CONTROLLER_AXIS_RIGHTX];
    if (!SDL_GetRelativeMouseMode()) return 0.f;
    TakeSnapIfNeeded();
    return MouseAxis(s_snapRelX * MOUSE_SCALE);
}
// Bis. but with Y(2D Position)
float SDL2Input::GetJoypadStick_RY(int, bool) {
    if (ADown(SDL_CONTROLLER_AXIS_RIGHTY))
        return -axisVal[SDL_CONTROLLER_AXIS_RIGHTY];
    if (!SDL_GetRelativeMouseMode()) return 0.f;
    TakeSnapIfNeeded();
    return MouseAxis(-s_snapRelY * MOUSE_SCALE);
}

unsigned char SDL2Input::GetJoypadLTrigger(int, bool) {
    return (s_mouseRightCurrent ||
            s_axisCurrent[SDL_CONTROLLER_AXIS_TRIGGERLEFT])
               ? 255
               : 0;
}
unsigned char SDL2Input::GetJoypadRTrigger(int, bool) {
    return (s_mouseLeftCurrent ||
            s_axisCurrent[SDL_CONTROLLER_AXIS_TRIGGERRIGHT])
               ? 255
               : 0;
}

int SDL2Input::GetMouseX() { return s_mouseX; }
int SDL2Input::GetMouseY() { return s_mouseY; }

// We detect if a Menu is visible on the player's screen to the mouse being
// stuck.
void SDL2Input::SetMenuDisplayed(int iPad, bool bVal) {
    if (iPad >= 0 && iPad < 4) s_menuDisplayed[iPad] = bVal;
    if (!s_sdlInitialized || bVal == s_prevMenuDisplayed) return;
    SDL_SetRelativeMouseMode(bVal ? SDL_FALSE : SDL_TRUE);
    s_prevMenuDisplayed = bVal;
}

int SDL2Input::GetScrollDelta() {
    int v = s_scrollTicksForButtonPressed;
    s_scrollTicksForButtonPressed = 0;
    return v;
}

void SDL2Input::SetDeadzoneAndMovementRange(unsigned int, unsigned int) {}
void SDL2Input::SetGameJoypadMaps(unsigned char, unsigned char, unsigned int) {}
unsigned int SDL2Input::GetGameJoypadMaps(unsigned char, unsigned char) {
    return 0;
}
void SDL2Input::SetJoypadMapVal(int, unsigned char) {}
unsigned char SDL2Input::GetJoypadMapVal(int) { return 0; }
void SDL2Input::SetJoypadSensitivity(int, float) {}
void SDL2Input::SetJoypadStickAxisMap(int, unsigned int, unsigned int) {}
void SDL2Input::SetJoypadStickTriggerMap(int, unsigned int, unsigned int) {}
void SDL2Input::SetKeyRepeatRate(float, float) {}
void SDL2Input::SetDebugSequence(const char*, std::function<int()>) {}
float SDL2Input::GetIdleSeconds(int) { return 0.f; }
bool SDL2Input::IsPadConnected(int iPad) { return iPad == 0; }

EKeyboardResult SDL2Input::RequestKeyboard(const char*, const char*, int,
                                           unsigned int,
                                           std::function<int(bool)> callback,
                                           SDL2Input::EKeyboardMode) {
    s_keyboardActive = true;
    s_textInputBuf.clear();
    s_keyboardCallback = std::move(callback);
    SDL_StartTextInput();
    return EKeyboardResult::Pending;
}
bool SDL2Input::GetMenuDisplayed(int iPad) {
    if (iPad >= 0 && iPad < 4) return s_menuDisplayed[iPad];
    return false;
}
const char* SDL2Input::GetText() { return s_textInputBuf.c_str(); }
bool SDL2Input::VerifyStrings(char**, int,
                              std::function<int(STRING_VERIFY_RESPONSE*)>) {
    return true;
}
void SDL2Input::CancelQueuedVerifyStrings(
    std::function<int(STRING_VERIFY_RESPONSE*)>) {}
void SDL2Input::CancelAllVerifyInProgress() {}

// Primary pad (moved from Profile)
namespace {
int s_inputPrimaryPad = 0;
}
int SDL2Input::GetPrimaryPad() { return s_inputPrimaryPad; }
void SDL2Input::SetPrimaryPad(int iPad) { s_inputPrimaryPad = iPad; }
