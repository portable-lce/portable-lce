
#include "UIScene_TrialExitUpsell.h"

#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameTypes.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/profile/profile.h"
#include "strings.h"

class UILayer;

UIScene_TrialExitUpsell::UIScene_TrialExitUpsell(int iPad, void* initData,
                                                 UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
}

std::string UIScene_TrialExitUpsell::getMoviePath() {
    return "TrialExitUpsell";
}

void UIScene_TrialExitUpsell::updateTooltips() {
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_EXIT_GAME, IDS_TOOLTIPS_BACK,
                   IDS_UNLOCK_TITLE);
}

void UIScene_TrialExitUpsell::handleInput(int iPad, int key, bool repeat,
                                          bool pressed, bool released,
                                          bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            navigateBack();
            break;
        case ACTION_MENU_OK:
            if (pressed) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Press);
                app.ExitGame();
            }
            break;
        case ACTION_MENU_X:
            if (PlatformProfile.IsSignedIn(iPad)) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Press);
            }
            break;
    }
}

void UIScene_TrialExitUpsell::handleAnimationEnd() {
    // ui.NavigateToHomeMenu();
    ui.NavigateToScene(0, eUIScene_SaveMessage);
}
