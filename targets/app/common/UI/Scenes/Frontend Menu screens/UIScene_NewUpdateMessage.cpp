
#include "UIScene_NewUpdateMessage.h"

#include <vector>

#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_DynamicLabel.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "app/common/Audio/SoundTypes.h"
#include "strings.h"

UIScene_NewUpdateMessage::UIScene_NewUpdateMessage(int iPad, void* initData,
                                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    parentLayer->addComponent(iPad, eUIComponent_Panorama);
    parentLayer->addComponent(iPad, eUIComponent_Logo);

    m_buttonConfirm.init(app.GetString(IDS_TOOLTIPS_ACCEPT), eControl_Confirm);

    std::string message = app.GetString(IDS_TITLEUPDATE);
    message.append("\r\n");

    message = app.FormatHTMLString(m_iPad, message);

    std::vector<std::string> paragraphs;
    int lastIndex = 0;
    for (int index = message.find("\r\n", lastIndex, 2);
         index != std::string::npos;
         index = message.find("\r\n", lastIndex, 2)) {
        paragraphs.push_back(message.substr(lastIndex, index - lastIndex) +
                             " ");
        lastIndex = index + 2;
    }
    paragraphs.push_back(
        message.substr(lastIndex, message.length() - lastIndex));

    for (unsigned int i = 0; i < paragraphs.size(); ++i) {
        m_labelDescription.addText(paragraphs[i], i == (paragraphs.size() - 1));
    }

    m_bIgnoreInput = false;
}

UIScene_NewUpdateMessage::~UIScene_NewUpdateMessage() {
    m_parentLayer->removeComponent(eUIComponent_Panorama);
    m_parentLayer->removeComponent(eUIComponent_Logo);
}

std::string UIScene_NewUpdateMessage::getMoviePath() { return "EULA"; }

void UIScene_NewUpdateMessage::updateTooltips() {
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT);
}

void UIScene_NewUpdateMessage::handleInput(int iPad, int key, bool repeat,
                                           bool pressed, bool released,
                                           bool& handled) {
    if (m_bIgnoreInput) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_B: {
            int iVal =
                app.GetGameSettings(m_iPad, eGameSetting_DisplayUpdateMessage);
            if (iVal > 0) iVal--;

            // set the update text as seen, by clearing the flag
            app.SetGameSettings(m_iPad, eGameSetting_DisplayUpdateMessage,
                                iVal);
            // force a profile write
            app.CheckGameSettingsChanged(true, m_iPad);
            ui.NavigateBack(m_iPad);
        } break;
        case ACTION_MENU_OK:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_UP:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
        case ACTION_MENU_OTHER_STICK_DOWN:
        case ACTION_MENU_OTHER_STICK_UP:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_NewUpdateMessage::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Confirm: {
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            int iVal =
                app.GetGameSettings(m_iPad, eGameSetting_DisplayUpdateMessage);
            if (iVal > 0) iVal--;

            // set the update text as seen, by clearing the flag
            app.SetGameSettings(m_iPad, eGameSetting_DisplayUpdateMessage,
                                iVal);
            // force a profile write
            app.CheckGameSettingsChanged(true, m_iPad);
            ui.NavigateBack(m_iPad);
        } break;
    };
}
