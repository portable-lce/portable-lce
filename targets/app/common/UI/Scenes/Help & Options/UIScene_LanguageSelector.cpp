#include "UIScene_LanguageSelector.h"

#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/client/Minecraft.h"
#include "app/common/Audio/SoundTypes.h"
#include "strings.h"

// strings for buttons in the list
const unsigned int UIScene_LanguageSelector::m_uiHTPButtonNameA[] = {
    IDS_LANG_SYSTEM,
    IDS_LANG_ENGLISH,
    IDS_LANG_GERMAN,
    IDS_LANG_SPANISH_SPAIN,
    IDS_LANG_SPANISH_LATIN_AMERICA,
    IDS_LANG_FRENCH,
    IDS_LANG_ITALIAN,
    IDS_LANG_PORTUGUESE_PORTUGAL,
    IDS_LANG_PORTUGUESE_BRAZIL,
    IDS_LANG_JAPANESE,
    IDS_LANG_KOREAN,
    IDS_LANG_CHINESE_TRADITIONAL,
    IDS_LANG_CHINESE_SIMPLIFIED,
    IDS_LANG_DANISH,
    IDS_LANG_FINISH,
    IDS_LANG_DUTCH,
    IDS_LANG_POLISH,
    IDS_LANG_RUSSIAN,
    IDS_LANG_SWEDISH,
    IDS_LANG_NORWEGIAN,
    // IDS_LANG_SLOVAK,
    // IDS_LANG_CZECH,
    IDS_LANG_GREEK,
    IDS_LANG_TURKISH,
};

UIScene_LanguageSelector::UIScene_LanguageSelector(int iPad, void* initData,
                                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_buttonListHowTo.init(eControl_Buttons);

    for (unsigned int i = 0; i < eLanguageSelector_MAX; ++i) {
        m_buttonListHowTo.addItem(m_uiHTPButtonNameA[i], i);
    }
}

std::string UIScene_LanguageSelector::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1)
        return "LanguagesMenuSplit";
    else
        return "LanguagesMenu";
}

void UIScene_LanguageSelector::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_LanguageSelector::updateComponents() {
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

void UIScene_LanguageSelector::handleReload() {
    for (unsigned int i = 0; i < eLanguageSelector_MAX; ++i) {
        m_buttonListHowTo.addItem(m_uiHTPButtonNameA[i], i);
    }
}

void UIScene_LanguageSelector::handleInput(int iPad, int key, bool repeat,
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
                // ui.NavigateToScene(m_iPad, eUIScene_SettingsOptionsMenu);
            }
            break;
        case ACTION_MENU_OK:
            sendInputToMovie(key, repeat, pressed, released);
            break;
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_LanguageSelector::handlePress(F64 controlId, F64 childId) {
    if ((int)controlId == eControl_Buttons) {
        // CD - Added for audio
        ui.PlayUISFX(eSFX_Press);

        int newLanguage, newLocale;
        newLanguage = uiLangMap[(int)childId];
        newLocale = uiLocaleMap[(int)childId];

        app.SetMinecraftLanguage(m_iPad, newLanguage);
        app.SetMinecraftLocale(m_iPad, newLocale);

        app.CheckGameSettingsChanged(true, m_iPad);
    }
}
