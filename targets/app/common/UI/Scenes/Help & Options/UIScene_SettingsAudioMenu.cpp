
#include "UIScene_SettingsAudioMenu.h"

#include <wchar.h>

#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "strings.h"

UIScene_SettingsAudioMenu::UIScene_SettingsAudioMenu(int iPad, void* initData,
                                                     UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    char TempString[256];
    snprintf(TempString, 256, "%s: %d%%", app.GetString(IDS_SLIDER_MUSIC),
             app.GetGameSettings(m_iPad, eGameSetting_MusicVolume));
    m_sliderMusic.init(TempString, eControl_Music, 0, 100,
                       app.GetGameSettings(m_iPad, eGameSetting_MusicVolume));

    snprintf(TempString, 256, "%s: %d%%", app.GetString(IDS_SLIDER_SOUND),
             app.GetGameSettings(m_iPad, eGameSetting_SoundFXVolume));
    m_sliderSound.init(TempString, eControl_Sound, 0, 100,
                       app.GetGameSettings(m_iPad, eGameSetting_SoundFXVolume));

    doHorizontalResizeCheck();

    if (app.GetLocalPlayerCount() > 1) {
#if TO_BE_IMPLEMENTED
        app.AdjustSplitscreenScene(m_hObj, &m_OriginalPosition, m_iPad);
#endif
    }
}

UIScene_SettingsAudioMenu::~UIScene_SettingsAudioMenu() {}

std::string UIScene_SettingsAudioMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "SettingsAudioMenuSplit";
    } else {
        return "SettingsAudioMenu";
    }
}

void UIScene_SettingsAudioMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsAudioMenu::updateComponents() {
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

void UIScene_SettingsAudioMenu::handleInput(int iPad, int key, bool repeat,
                                            bool pressed, bool released,
                                            bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");
    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
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

void UIScene_SettingsAudioMenu::handleSliderMove(F64 sliderId,
                                                 F64 currentValue) {
    char TempString[256];
    int value = (int)currentValue;
    switch ((int)sliderId) {
        case eControl_Music:
            m_sliderMusic.handleSliderMove(value);

            app.SetGameSettings(m_iPad, eGameSetting_MusicVolume, value);
            snprintf(TempString, 256, "%s: %d%%",
                     app.GetString(IDS_SLIDER_MUSIC), value);
            m_sliderMusic.setLabel(TempString);

            break;
        case eControl_Sound:
            m_sliderSound.handleSliderMove(value);

            app.SetGameSettings(m_iPad, eGameSetting_SoundFXVolume, value);
            snprintf(TempString, 256, "%s: %d%%",
                     app.GetString(IDS_SLIDER_SOUND), value);
            m_sliderSound.setLabel(TempString);

            break;
    }
}
