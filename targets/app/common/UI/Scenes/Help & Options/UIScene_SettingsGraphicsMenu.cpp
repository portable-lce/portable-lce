#include "UIScene_SettingsGraphicsMenu.h"

#include <wchar.h>

#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "platform/profile/profile.h"
#include "strings.h"

UIScene_SettingsGraphicsMenu::UIScene_SettingsGraphicsMenu(int iPad,
                                                           void* initData,
                                                           UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_bNotInGame = (Minecraft::GetInstance()->level == nullptr);

    m_checkboxClouds.init(
        app.GetString(IDS_CHECKBOX_RENDER_CLOUDS), eControl_Clouds,
        (app.GetGameSettings(m_iPad, eGameSetting_Clouds) != 0));
    m_checkboxBedrockFog.init(
        app.GetString(IDS_CHECKBOX_RENDER_BEDROCKFOG), eControl_BedrockFog,
        (app.GetGameSettings(m_iPad, eGameSetting_BedrockFog) != 0));
    m_checkboxCustomSkinAnim.init(
        app.GetString(IDS_CHECKBOX_CUSTOM_SKIN_ANIM), eControl_CustomSkinAnim,
        (app.GetGameSettings(m_iPad, eGameSetting_CustomSkinAnim) != 0));

    char TempString[256];

    snprintf(TempString, 256, "%s: %d%%", app.GetString(IDS_SLIDER_GAMMA),
             app.GetGameSettings(m_iPad, eGameSetting_Gamma));
    m_sliderGamma.init(TempString, eControl_Gamma, 0, 100,
                       app.GetGameSettings(m_iPad, eGameSetting_Gamma));

    snprintf(TempString, 256, "%s: %d%%",
             app.GetString(IDS_SLIDER_INTERFACEOPACITY),
             app.GetGameSettings(m_iPad, eGameSetting_InterfaceOpacity));
    m_sliderInterfaceOpacity.init(
        TempString, eControl_InterfaceOpacity, 0, 100,
        app.GetGameSettings(m_iPad, eGameSetting_InterfaceOpacity));

    doHorizontalResizeCheck();

    bool bInGame = (Minecraft::GetInstance()->level != nullptr);
    bool bIsPrimaryPad = (PlatformProfile.GetPrimaryPad() == m_iPad);
    // if we're not in the game, we need to use basescene 0
    if (bInGame) {
        // If the game has started, then you need to be the host to change the
        // in-game gamertags
        if (bIsPrimaryPad) {
            // we are the primary player on this machine, but not the game host
            // are we the game host? If not, we need to remove the bedrockfog
            // setting
            if (!g_NetworkManager.IsHost()) {
                // hide the in-game bedrock fog setting
                removeControl(&m_checkboxBedrockFog, true);
            }
        } else {
            // We shouldn't have the bedrock fog option, or the m_CustomSkinAnim
            // option
            removeControl(&m_checkboxBedrockFog, true);
            removeControl(&m_checkboxCustomSkinAnim, true);
        }
    }

    if (app.GetLocalPlayerCount() > 1) {
#if TO_BE_IMPLEMENTED
        app.AdjustSplitscreenScene(m_hObj, &m_OriginalPosition, m_iPad);
#endif
    }
}

UIScene_SettingsGraphicsMenu::~UIScene_SettingsGraphicsMenu() {}

std::string UIScene_SettingsGraphicsMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "SettingsGraphicsMenuSplit";
    } else {
        return "SettingsGraphicsMenu";
    }
}

void UIScene_SettingsGraphicsMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsGraphicsMenu::updateComponents() {
    bool bNotInGame = (Minecraft::GetInstance()->level == nullptr);
    if (bNotInGame) {
        m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
        m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
    } else {
        m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, false);

        if (app.GetLocalPlayerCount() == 1)
            m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
        else
            m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
    }
}

void UIScene_SettingsGraphicsMenu::handleInput(int iPad, int key, bool repeat,
                                               bool pressed, bool released,
                                               bool& handled) {
    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                // check the checkboxes
                app.SetGameSettings(m_iPad, eGameSetting_Clouds,
                                    m_checkboxClouds.IsChecked() ? 1 : 0);
                app.SetGameSettings(m_iPad, eGameSetting_BedrockFog,
                                    m_checkboxBedrockFog.IsChecked() ? 1 : 0);
                app.SetGameSettings(
                    m_iPad, eGameSetting_CustomSkinAnim,
                    m_checkboxCustomSkinAnim.IsChecked() ? 1 : 0);

                navigateBack();
                handled = true;
            }
            break;
        case ACTION_MENU_OK:
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

void UIScene_SettingsGraphicsMenu::handleSliderMove(F64 sliderId,
                                                    F64 currentValue) {
    char TempString[256];
    int value = (int)currentValue;
    switch ((int)sliderId) {
        case eControl_Gamma:
            m_sliderGamma.handleSliderMove(value);

            app.SetGameSettings(m_iPad, eGameSetting_Gamma, value);
            snprintf(TempString, 256, "%s: %d%%",
                     app.GetString(IDS_SLIDER_GAMMA), value);
            m_sliderGamma.setLabel(TempString);

            break;
        case eControl_InterfaceOpacity:
            m_sliderInterfaceOpacity.handleSliderMove(value);

            app.SetGameSettings(m_iPad, eGameSetting_InterfaceOpacity, value);
            snprintf(TempString, 256, "%s: %d%%",
                     app.GetString(IDS_SLIDER_INTERFACEOPACITY), value);
            m_sliderInterfaceOpacity.setLabel(TempString);

            break;
    }
}
