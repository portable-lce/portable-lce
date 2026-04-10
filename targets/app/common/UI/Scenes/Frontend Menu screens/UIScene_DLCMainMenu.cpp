
#include "UIScene_DLCMainMenu.h"

#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/GameEnums.h"
#include "strings.h"

class UILayer;

#define PLAYER_ONLINE_TIMER_ID 0
#define PLAYER_ONLINE_TIMER_TIME 100

UIScene_DLCMainMenu::UIScene_DLCMainMenu(int iPad, void* initData,
                                         UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
    // Alert the app the we want to be informed of ethernet connections
    app.SetLiveLinkRequired(true);

    m_labelOffers.init(IDS_DOWNLOADABLE_CONTENT_OFFERS);
    m_buttonListOffers.init(eControl_OffersList);

    if (m_loadedResolution == eSceneResolution_1080) {
        m_labelXboxStore.init("");
    }
}

UIScene_DLCMainMenu::~UIScene_DLCMainMenu() {
    // Alert the app the we no longer want to be informed of ethernet
    // connections
    app.SetLiveLinkRequired(false);
}

std::string UIScene_DLCMainMenu::getMoviePath() { return "DLCMainMenu"; }

void UIScene_DLCMainMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_DLCMainMenu::handleInput(int iPad, int key, bool repeat,
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
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_DLCMainMenu::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_OffersList: {
            int iIndex = (int)childId;
            DLCOffersParam* param = new DLCOffersParam();
            param->iPad = m_iPad;

            param->iType = iIndex;
            // promote the DLC content request type

            // Xbox One will have requested the marketplace content - there is
            // only that type
            app.AddDLCRequest((eDLCMarketplaceType)iIndex, true);
            killTimer(PLAYER_ONLINE_TIMER_ID);
            ui.NavigateToScene(m_iPad, eUIScene_DLCOffersMenu, param);
            break;
        }
    };
}

void UIScene_DLCMainMenu::handleTimerComplete(int id) {}

int UIScene_DLCMainMenu::ExitDLCMainMenu(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_DLCMainMenu* pClass = (UIScene_DLCMainMenu*)pParam;

    pClass->navigateBack();

    return 0;
}

void UIScene_DLCMainMenu::handleGainFocus(bool navBack) {
    UIScene::handleGainFocus(navBack);

    updateTooltips();

    if (navBack) {
        // add the timer back in
    }
}

void UIScene_DLCMainMenu::tick() { UIScene::tick(); }
