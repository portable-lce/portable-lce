
#include "UIScene_EULA.h"

#include <vector>

#include "app/common/Audio/SoundTypes.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_DynamicLabel.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "strings.h"

UIScene_EULA::UIScene_EULA(int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    parentLayer->addComponent(iPad, eUIComponent_Panorama);
    parentLayer->addComponent(iPad, eUIComponent_Logo);

    m_buttonConfirm.init(app.GetString(IDS_TOOLTIPS_ACCEPT), eControl_Confirm);

    std::string EULA = "";

    std::vector<std::string> paragraphs;
    int lastIndex = 0;
    for (int index = EULA.find("\r\n", lastIndex, 2);
         index != std::string::npos; index = EULA.find("\r\n", lastIndex, 2)) {
        paragraphs.push_back(EULA.substr(lastIndex, index - lastIndex) + " ");
        lastIndex = index + 2;
    }
    paragraphs.push_back(EULA.substr(lastIndex, EULA.length() - lastIndex));

    for (unsigned int i = 0; i < paragraphs.size(); ++i) {
        m_labelDescription.addText(paragraphs[i], i == (paragraphs.size() - 1));
    }

    // 4J-PB - If we have a signed in user connected, let's get the DLC now
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if ((PlatformInput.IsPadConnected(i) ||
             PlatformProfile.IsSignedIn(i))) {
            if (!app.DLCInstallProcessCompleted() && !app.DLCInstallPending()) {
                app.StartInstallDLCProcess(i);
                break;
            }
        }
    }

    m_bIgnoreInput = false;

    // ui.setFontCachingCalculationBuffer(20000);
}

UIScene_EULA::~UIScene_EULA() {
    m_parentLayer->removeComponent(eUIComponent_Panorama);
    m_parentLayer->removeComponent(eUIComponent_Logo);
}

std::string UIScene_EULA::getMoviePath() { return "EULA"; }

void UIScene_EULA::updateTooltips() {
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT);
}

void UIScene_EULA::handleInput(int iPad, int key, bool repeat, bool pressed,
                               bool released, bool& handled) {
    if (m_bIgnoreInput) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
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

void UIScene_EULA::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Confirm:
            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);
            app.SetGameSettings(0, eGameSetting_PS3_EULA_Read, 1);
            ui.NavigateToScene(0, eUIScene_SaveMessage);
            ui.setFontCachingCalculationBuffer(-1);
            break;
    };
}
