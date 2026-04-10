#include "UIScene_Intro.h"

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/ConsoleUIController.h"

class UILayer;

#if !defined(_ENABLEIGGY)
static int s_introTickCount = 0;
#endif

UIScene_Intro::UIScene_Intro(int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
    m_bIgnoreNavigate = false;
    m_bAnimationEnded = false;
#if !defined(_ENABLEIGGY)
    s_introTickCount = 0;
#endif

    bool bSkipESRB = false;
    bool bChina = false;

    // 4J Stu - These map to values in the Actionscript
#if defined(_WINDOWS64) || defined(__linux__)
    int platformIdx = 0;
#endif

    IggyDataValue result;
    IggyDataValue value[3];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = platformIdx;

    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = bChina ? true : bSkipESRB;

    value[2].type = IGGY_DATATYPE_boolean;
    value[2].boolval = bChina;

    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetIntroPlatform, 3, value);
}

std::string UIScene_Intro::getMoviePath() { return "Intro"; }

void UIScene_Intro::handleInput(int iPad, int key, bool repeat, bool pressed,
                                bool released, bool& handled) {
    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_OK:
            if (!m_bIgnoreNavigate) {
                m_bIgnoreNavigate = true;
                // ui.NavigateToHomeMenu();
                ui.NavigateToScene(0, eUIScene_SaveMessage);
            }
            break;
    }
}

void UIScene_Intro::handleAnimationEnd() {
    if (!m_bIgnoreNavigate) {
        m_bIgnoreNavigate = true;
        // ui.NavigateToHomeMenu();
        ui.NavigateToScene(0, eUIScene_SaveMessage);
    }
}

void UIScene_Intro::handleGainFocus(bool navBack) {
    // Only relevant on xbox one - if we didn't navigate to the main menu at
    // animation end due to the timer or quadrant sign-in being up, then we'll
    // need to do it now in case the user has cancelled or joining a game failed
    if (m_bAnimationEnded) {
        ui.NavigateToScene(0, eUIScene_MainMenu);
    }
}

#if !defined(_ENABLEIGGY)
void UIScene_Intro::tick() {
    // Call base tick first (processes Iggy ticking)
    UIScene::tick();

    // Auto-skip the intro after 60 ticks (~2 seconds at 30fps)
    // since we have no SWF renderer to play the intro animation
    s_introTickCount++;
    if (s_introTickCount == 60 && !m_bIgnoreNavigate) {
        fprintf(stderr,
                "[Linux] Auto-skipping intro -> MainMenu after %d ticks\n",
                s_introTickCount);
        m_bIgnoreNavigate = true;
        // Skip straight to MainMenu, bypassing SaveMessage (no SWF interaction
        // possible)
        ui.NavigateToScene(0, eUIScene_MainMenu);
    }
}
#endif
