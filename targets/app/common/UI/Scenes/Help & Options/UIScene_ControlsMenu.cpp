#include "UIScene_ControlsMenu.h"

#include <wchar.h>

#include <memory>

#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/BuildVer.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "platform/input/input.h"
#include "strings.h"

class UILayer;

UIScene_ControlsMenu::UIScene_ControlsMenu(int iPad, void* initData,
                                           UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
#if defined(_WIN64)
    value[0].number = (F64)0;
#endif
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetPlatform, 1, value);

    bool bNotInGame = (Minecraft::GetInstance()->level == nullptr);

    if (bNotInGame) {
        char* layoutString = new char[128];
        snprintf(layoutString, 128, "%s", VER_PRODUCTVERSION_STR_W);
        m_labelVersion.init(layoutString);
        delete[] layoutString;
    }
    // 4J-PB - stop the label showing in the in-game controls menu
    else {
        m_labelVersion.init(" ");
    }
    m_bCreativeMode =
        !bNotInGame && Minecraft::GetInstance()->localplayers[m_iPad] &&
        Minecraft::GetInstance()->localplayers[m_iPad]->abilities.mayfly;

    {
        m_buttonLayouts[0].init("1", eControl_Button0);
        m_buttonLayouts[1].init("2", eControl_Button1);
        m_buttonLayouts[2].init("3", eControl_Button2);
    }

    m_checkboxInvert.init(
        app.GetString(IDS_INVERT_LOOK), eControl_InvertLook,
        app.GetGameSettings(m_iPad, eGameSetting_ControlInvertLook));
    m_checkboxSouthpaw.init(
        app.GetString(IDS_SOUTHPAW), eControl_Southpaw,
        app.GetGameSettings(m_iPad, eGameSetting_ControlSouthPaw));

    m_iSchemeTextA[0] = IDS_CONTROLS_SCHEME0;
    m_iSchemeTextA[1] = IDS_CONTROLS_SCHEME1;
    m_iSchemeTextA[2] = IDS_CONTROLS_SCHEME2;

    int iSelected = app.GetGameSettings(m_iPad, eGameSetting_ControlScheme);

    char* layoutString = new char[128];
    snprintf(layoutString, 128, "%s : %s", app.GetString(IDS_CURRENT_LAYOUT),
             app.GetString(m_iSchemeTextA[iSelected]));
    { m_labelCurrentLayout.init(layoutString); }

    m_iCurrentNavigatedControlsLayout = iSelected;

    {
        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_number;
        value[0].number = (F64)m_iCurrentNavigatedControlsLayout;
        IggyResult out = IggyPlayerCallMethodRS(
            getMovie(), &result, IggyPlayerRootPath(getMovie()),
            m_funcSetControllerLayout, 1, value);
    }

    for (unsigned int i = 0; i < e_PadCOUNT; ++i) {
        m_labelsPad[i].init("");
        m_controlLines[i].setVisible(false);
    }
    m_bLayoutChanged = false;

    PositionAllText(m_iPad);
}

std::string UIScene_ControlsMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "ControlsSplit";
    } else {
        return "Controls";
    }
}

void UIScene_ControlsMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_ControlsMenu::tick() {
    if (m_bLayoutChanged) PositionAllText(m_iPad);
    UIScene::tick();
}

void UIScene_ControlsMenu::handleInput(int iPad, int key, bool repeat,
                                       bool pressed, bool released,
                                       bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");
    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                app.CheckGameSettingsChanged(true, iPad);
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
            if (pressed) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Press);
            }
            sendInputToMovie(key, repeat, pressed, released);
            break;
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_ControlsMenu::handleCheckboxToggled(F64 controlId, bool selected) {
    switch ((int)controlId) {
        case eControl_InvertLook:
            app.SetGameSettings(m_iPad, eGameSetting_ControlInvertLook,
                                (unsigned char)(selected));
            break;
        case eControl_Southpaw:
            app.SetGameSettings(m_iPad, eGameSetting_ControlSouthPaw,
                                (unsigned char)(selected));
            PositionAllText(m_iPad);
            break;
    };
}

void UIScene_ControlsMenu::handlePress(F64 controlId, F64 childId) {
    int control = (int)controlId;
    switch (control) {
        case eControl_Button0:
        case eControl_Button1:
        case eControl_Button2:
            app.SetGameSettings(m_iPad, eGameSetting_ControlScheme,
                                (unsigned char)control);
            char* layoutString = new char[128];
            snprintf(layoutString, 128, "%s : %s",
                     app.GetString(IDS_CURRENT_LAYOUT),
                     app.GetString(m_iSchemeTextA[control]));
            { m_labelCurrentLayout.setLabel(layoutString); }

            break;
    };
}

void UIScene_ControlsMenu::handleFocusChange(F64 controlId, F64 childId) {
    int control = (int)controlId;
    switch (control) {
        case eControl_Button0:
        case eControl_Button1:
        case eControl_Button2:
            m_iCurrentNavigatedControlsLayout = control;
            m_bLayoutChanged = true;
            break;
    };
}

void UIScene_ControlsMenu::PositionAllText(int iPad) {
    for (unsigned int i = 0; i < e_PadCOUNT; ++i) {
        m_labelsPad[i].setLabel("");
        m_controlLines[i].setVisible(false);
    }

    if (m_bCreativeMode) {
        PositionText(iPad, IDS_CONTROLS_JUMPFLY, MINECRAFT_ACTION_JUMP);
    } else {
        PositionText(iPad, IDS_CONTROLS_JUMP, MINECRAFT_ACTION_JUMP);
    }
    PositionText(iPad, IDS_CONTROLS_INVENTORY, MINECRAFT_ACTION_INVENTORY);
    PositionText(iPad, IDS_CONTROLS_PAUSE, MINECRAFT_ACTION_PAUSEMENU);
    if (m_bCreativeMode) {
        PositionText(iPad, IDS_CONTROLS_SNEAKFLY,
                     MINECRAFT_ACTION_SNEAK_TOGGLE);
    } else {
        PositionText(iPad, IDS_CONTROLS_SNEAK, MINECRAFT_ACTION_SNEAK_TOGGLE);
    }
    PositionText(iPad, IDS_CONTROLS_USE, MINECRAFT_ACTION_USE);
    PositionText(iPad, IDS_CONTROLS_ACTION, MINECRAFT_ACTION_ACTION);
    PositionText(iPad, IDS_CONTROLS_HELDITEM, MINECRAFT_ACTION_RIGHT_SCROLL);
    PositionText(iPad, IDS_CONTROLS_HELDITEM, MINECRAFT_ACTION_LEFT_SCROLL);
    PositionText(iPad, IDS_CONTROLS_DROP, MINECRAFT_ACTION_DROP);
    PositionText(iPad, IDS_CONTROLS_CRAFTING, MINECRAFT_ACTION_CRAFTING);
    PositionText(iPad, IDS_CONTROLS_THIRDPERSON,
                 MINECRAFT_ACTION_RENDER_THIRD_PERSON);
    PositionText(iPad, IDS_CONTROLS_PLAYERS, MINECRAFT_ACTION_GAME_INFO);

    // Swap for southpaw.
    if (app.GetGameSettings(m_iPad, eGameSetting_ControlSouthPaw)) {
        // Move
        PositionText(iPad, IDS_CONTROLS_LOOK, MINECRAFT_ACTION_RIGHT);
        // Look
        PositionText(iPad, IDS_CONTROLS_MOVE, MINECRAFT_ACTION_LOOK_RIGHT);
    } else  // Normal right handed.
    {
        // Move
        PositionText(iPad, IDS_CONTROLS_MOVE, MINECRAFT_ACTION_RIGHT);
        // Look
        PositionText(iPad, IDS_CONTROLS_LOOK, MINECRAFT_ACTION_LOOK_RIGHT);
    }

    bool layoutHasDpadFly;
    layoutHasDpadFly = m_iCurrentNavigatedControlsLayout == 0;

    // If we're in controls mode 1, and creative mode show the dpad for Creative
    // Mode
    if (m_bCreativeMode && layoutHasDpadFly) {
        PositionText(iPad, IDS_CONTROLS_DPAD, MINECRAFT_ACTION_DPAD_LEFT);
    }
    m_bLayoutChanged = false;
}

void UIScene_ControlsMenu::PositionText(int iPad, int iTextID,
                                        unsigned char ucAction) {
    unsigned int uiVal = PlatformInput.GetGameJoypadMaps(
        m_iCurrentNavigatedControlsLayout, ucAction);

    if (uiVal & _360_JOY_BUTTON_A)
        PositionTextDirect(iPad, iTextID, e_PadA, true);
    if (uiVal & _360_JOY_BUTTON_B)
        PositionTextDirect(iPad, iTextID, e_PadB, true);
    if (uiVal & _360_JOY_BUTTON_X)
        PositionTextDirect(iPad, iTextID, e_PadX, true);
    if (uiVal & _360_JOY_BUTTON_Y)
        PositionTextDirect(iPad, iTextID, e_PadY, true);
    if (uiVal & _360_JOY_BUTTON_BACK) {
        PositionTextDirect(iPad, iTextID, e_PadBack, true);
    }
    if (uiVal & _360_JOY_BUTTON_START)
        PositionTextDirect(iPad, iTextID, e_PadStart, true);
    if (uiVal & _360_JOY_BUTTON_RB)
        PositionTextDirect(iPad, iTextID, e_PadRB, true);
    if (uiVal & _360_JOY_BUTTON_LB)
        PositionTextDirect(iPad, iTextID, e_PadLB, true);
    if (uiVal & _360_JOY_BUTTON_RTHUMB)
        PositionTextDirect(iPad, iTextID, e_PadRS_1, true);
    if (uiVal & _360_JOY_BUTTON_LTHUMB)
        PositionTextDirect(iPad, iTextID, e_PadLS_1, true);
    // Look
    if (uiVal & _360_JOY_BUTTON_RSTICK_RIGHT)
        PositionTextDirect(iPad, iTextID, e_PadRS_2, true);
    // Move
    if (uiVal & _360_JOY_BUTTON_LSTICK_RIGHT)
        PositionTextDirect(iPad, iTextID, e_PadLS_2, true);
    if (uiVal & _360_JOY_BUTTON_RT)
        PositionTextDirect(iPad, iTextID, e_PadRT, true);
    if (uiVal & _360_JOY_BUTTON_LT)
        PositionTextDirect(iPad, iTextID, e_PadLT, true);
    if (uiVal & _360_JOY_BUTTON_DPAD_RIGHT)
        PositionTextDirect(iPad, iTextID, e_PadDPadRight, true);
    if (uiVal & _360_JOY_BUTTON_DPAD_LEFT)
        PositionTextDirect(iPad, iTextID, e_PadDPadLeft, true);
    if (uiVal & _360_JOY_BUTTON_DPAD_UP)
        PositionTextDirect(iPad, iTextID, e_PadDPadUp, true);
    if (uiVal & _360_JOY_BUTTON_DPAD_DOWN)
        PositionTextDirect(iPad, iTextID, e_PadDPadDown, true);
}

void UIScene_ControlsMenu::PositionTextDirect(int iPad, int iTextID,
                                              int iControlDetailsIndex,
                                              bool bShow) {
    const char* text = app.GetString(iTextID);

    m_labelsPad[iControlDetailsIndex].setLabel(text);
    m_controlLines[iControlDetailsIndex].setVisible(bShow);
}