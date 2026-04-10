#include "UIScene_DebugOptions.h"

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/Console_Debug_enum.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Game.h"
#include "platform/input/InputConstants.h"

class UILayer;

const char*
    UIScene_DebugOptionsMenu::m_DebugCheckboxTextA[eDebugSetting_Max + 1] = {
        "Load Saves From Local Folder Mode",
        "Write Saves To Local Folder Mode",
        "Freeze Players",  // "Not Used",
        "Display Safe Area",
        "Mobs don't attack",
        "Freeze Time",
        "Disable Weather",
        "Craft Anything",
        "Use DPad for debug",
        "Mobs don't tick",
        "Art tools",  // "Instant Mine",
        "Show UI Console",
        "Distributable Save",
        "Debug Leaderboards",
        "Height-Water Maps",
        "Superflat Nether",
        // "Light/Dark background",
        "More lightning when thundering",
        "Biome override",
        // "Go To End",
        "Go To Overworld",
        "Unlock All DLC",  // "Toggle Font",
        "Show Marketing Guide",
};

UIScene_DebugOptionsMenu::UIScene_DebugOptionsMenu(int iPad, void* initData,
                                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    unsigned int uiDebugBitmask = app.GetGameSettingsDebugMask(iPad);

    IggyValuePath* root = IggyPlayerRootPath(getMovie());
    for (m_iTotalCheckboxElements = 0;
         m_iTotalCheckboxElements < eDebugSetting_Max &&
         m_iTotalCheckboxElements < 21;
         ++m_iTotalCheckboxElements) {
        std::string label(m_DebugCheckboxTextA[m_iTotalCheckboxElements]);
        m_checkboxes[m_iTotalCheckboxElements].init(
            label, m_iTotalCheckboxElements,
            (uiDebugBitmask & (1 << m_iTotalCheckboxElements)) ? true : false);
    }
}

std::string UIScene_DebugOptionsMenu::getMoviePath() {
    return "DebugOptionsMenu";
}

void UIScene_DebugOptionsMenu::handleInput(int iPad, int key, bool repeat,
                                           bool pressed, bool released,
                                           bool& handled) {
    // app.DebugPrintf("UIScene_DebugOptionsMenu handling input for pad %d, key
    // %d, repeat- %s, pressed- %s, released- %s\n", iPad, key,
    // repeat?"true":"false", pressed?"true":"false", released?"true":"false");

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                int iCurrentBitmaskIndex = 0;
                unsigned int uiDebugBitmask = 0L;
                for (int i = 0; i < m_iTotalCheckboxElements; i++) {
                    uiDebugBitmask |= m_checkboxes[i].IsChecked()
                                          ? (1 << iCurrentBitmaskIndex)
                                          : 0;
                    iCurrentBitmaskIndex++;
                }

                if (uiDebugBitmask != app.GetGameSettingsDebugMask(iPad)) {
                    app.SetGameSettingsDebugMask(iPad, uiDebugBitmask);
                    if (app.DebugSettingsOn()) {
                        app.ActionDebugMask(iPad);
                    } else {
                        // force debug mask off
                        app.ActionDebugMask(iPad, true);
                    }

                    app.CheckGameSettingsChanged(true, iPad);
                }

                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}
