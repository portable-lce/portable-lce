#include "UIScene_Keyboard.h"

#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/GameTypes.h"
#include "strings.h"
#include "util/StringHelpers.h"

#define KEYBOARD_DONE_TIMER_ID 0
#define KEYBOARD_DONE_TIMER_TIME 100

UIScene_Keyboard::UIScene_Keyboard(int iPad, void* initData,
                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_EnterTextLabel.init("Enter Sign Text");

    m_KeyboardTextInput.init("", -1);
    m_KeyboardTextInput.SetCharLimit(15);

    m_ButtonSpace.init("Space", -1);
    m_ButtonCursorLeft.init("Cursor Left", -1);
    m_ButtonCursorRight.init("Cursor Right", -1);
    m_ButtonCaps.init("Caps", -1);
    m_ButtonDone.init("Done", 0);  // only the done button needs an id, the
                                   // others will never call back!
    m_ButtonSymbols.init("Symbols", -1);
    m_ButtonBackspace.init("Backspace", -1);

    // Initialise function keyboard Buttons and set alternative symbol button
    // string
    std::string label = "Abc";
    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(label.c_str());
    stringVal.length = label.length();

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    IggyResult out = IggyPlayerCallMethodRS(
        getMovie(), &result, IggyPlayerRootPath(getMovie()),
        m_funcInitFunctionButtons, 1, value);

    m_bKeyboardDonePressed = false;

    parentLayer->addComponent(iPad, eUIComponent_MenuBackground);
}

UIScene_Keyboard::~UIScene_Keyboard() {
    m_parentLayer->removeComponent(eUIComponent_MenuBackground);
}

std::string UIScene_Keyboard::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1 && !m_parentLayer->IsFullscreenGroup()) {
        return "KeyboardSplit";
    } else {
        return "Keyboard";
    }
}

void UIScene_Keyboard::updateTooltips() {
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT,
                   IDS_TOOLTIPS_BACK, -1, -1);
}

bool UIScene_Keyboard::allowRepeat(int key) {
    // 4J - TomK - we want to allow X and Y repeats!
    switch (key) {
        case ACTION_MENU_OK:
        case ACTION_MENU_CANCEL:
        case ACTION_MENU_A:
        case ACTION_MENU_B:
        case ACTION_MENU_PAUSEMENU:
            // case ACTION_MENU_X:
            // case ACTION_MENU_Y:
            return false;
    }
    return true;
}

void UIScene_Keyboard::handleInput(int iPad, int key, bool repeat, bool pressed,
                                   bool released, bool& handled) {
    IggyDataValue result;
    IggyResult out;

    if (repeat || pressed) {
        switch (key) {
            case ACTION_MENU_CANCEL:
                navigateBack();
                handled = true;
                break;
            case ACTION_MENU_X:  // X
                out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcBackspaceButtonPressed, 0, nullptr);
                handled = true;
                break;
            case ACTION_MENU_PAGEUP:  // LT
                out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcSymbolButtonPressed, 0, nullptr);
                handled = true;
                break;
            case ACTION_MENU_Y:  // Y
                out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcSpaceButtonPressed, 0, nullptr);
                handled = true;
                break;
            case ACTION_MENU_STICK_PRESS:  // LS
                out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcCapsButtonPressed, 0, nullptr);
                handled = true;
                break;
            case ACTION_MENU_LEFT_SCROLL:  // LB
                out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcCursorLeftButtonPressed, 0, nullptr);
                handled = true;
                break;
            case ACTION_MENU_RIGHT_SCROLL:  // RB
                out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcCursorRightButtonPressed, 0, nullptr);
                handled = true;
                break;
            case ACTION_MENU_PAUSEMENU:  // Start
                if (!m_bKeyboardDonePressed) {
                    out = IggyPlayerCallMethodRS(
                        getMovie(), &result, IggyPlayerRootPath(getMovie()),
                        m_funcDoneButtonPressed, 0, nullptr);

                    // kick off done timer
                    addTimer(KEYBOARD_DONE_TIMER_ID, KEYBOARD_DONE_TIMER_TIME);
                    m_bKeyboardDonePressed = true;
                }
                handled = true;
                break;
        }
    }

    switch (key) {
        case ACTION_MENU_OK:
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            sendInputToMovie(key, repeat, pressed, released);
            handled = true;
            break;
    }
}

void UIScene_Keyboard::handlePress(F64 controlId, F64 childId) {
    if ((int)controlId == 0) {
        // Done has been pressed. At this point we can query for the input
        // string and pass it on to wherever it is needed. we can not query for
        // m_KeyboardTextInput.getLabel() here because we're in an iggy callback
        // so we need to wait a frame.
        if (!m_bKeyboardDonePressed) {
            // kick off done timer
            addTimer(KEYBOARD_DONE_TIMER_ID, KEYBOARD_DONE_TIMER_TIME);
            m_bKeyboardDonePressed = true;
        }
    }
}

void UIScene_Keyboard::handleTimerComplete(int id) {
    if (id == KEYBOARD_DONE_TIMER_ID) {
        // remove timer
        killTimer(KEYBOARD_DONE_TIMER_ID);

        // we're done here!
        KeyboardDonePressed();
    }
}

void UIScene_Keyboard::KeyboardDonePressed() {
    // Debug
    app.DebugPrintf("UI Keyboard - DONE - [%s]\n",
                    m_KeyboardTextInput.getLabel());

    // ToDo: Keyboard can now pass on its final string value and close itself
    // down
    navigateBack();
}
