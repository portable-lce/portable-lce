
#include "UIScene_ConnectingProgress.h"

#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Progress.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "java/System.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "platform/profile/profile.h"
#include "strings.h"

UIScene_ConnectingProgress::UIScene_ConnectingProgress(int iPad,
                                                       void* _initData,
                                                       UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    parentLayer->addComponent(iPad, eUIComponent_Panorama);
    parentLayer->addComponent(iPad, eUIComponent_Logo);

    m_progressBar.showBar(false);
    m_progressBar.setVisible(false);
    m_labelTip.setVisible(false);

    ConnectionProgressParams* param = (ConnectionProgressParams*)_initData;

    if (param->stringId >= 0) {
        m_labelTitle.init(app.GetString(param->stringId));
    } else {
        m_labelTitle.init("");
    }
    m_progressBar.init("", 0, 0, 100, 0);
    m_buttonConfirm.init(app.GetString(IDS_CONFIRM_OK), eControl_Confirm);
    m_buttonConfirm.setVisible(false);

    m_showTooltips = param->showTooltips;
    m_runFailTimer = param->setFailTimer;
    m_timerTime = param->timerTime;
    m_cancelFunc = param->cancelFunc;
    m_cancelFuncParam = param->cancelFuncParam;
    m_removeLocalPlayer = false;
    m_showingButton = false;
}

UIScene_ConnectingProgress::~UIScene_ConnectingProgress() {
    m_parentLayer->removeComponent(eUIComponent_Panorama);
    m_parentLayer->removeComponent(eUIComponent_Logo);
}

void UIScene_ConnectingProgress::updateTooltips() {
    // 4J-PB - removing the option of cancel join, since it didn't work anyway
    // ui.SetTooltips( m_iPad, -1, m_showTooltips?IDS_TOOLTIPS_CANCEL_JOIN:-1);
    ui.SetTooltips(m_iPad, -1, -1);
}

void UIScene_ConnectingProgress::tick() {
    UIScene::tick();

    if (m_removeLocalPlayer) {
        m_removeLocalPlayer = false;

        Minecraft* pMinecraft = Minecraft::GetInstance();
        pMinecraft->removeLocalPlayerIdx(m_iPad);
    }
}

std::string UIScene_ConnectingProgress::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1 && !m_parentLayer->IsFullscreenGroup()) {
        return "FullscreenProgressSplit";
    } else {
        return "FullscreenProgress";
    }
}

void UIScene_ConnectingProgress::handleGainFocus(bool navBack) {
    UIScene::handleGainFocus(navBack);
    if (!navBack && m_runFailTimer) addTimer(0, m_timerTime);
}

void UIScene_ConnectingProgress::handleLoseFocus() {
    int millisecsLeft = getTimer(0)->targetTime - System::currentTimeMillis();
    int millisecsTaken = getTimer(0)->duration - millisecsLeft;
    app.DebugPrintf("\n");
    app.DebugPrintf(
        "---------------------------------------------------------\n");
    app.DebugPrintf(
        "---------------------------------------------------------\n");
    app.DebugPrintf("UIScene_ConnectingProgress time taken = %d millisecs\n",
                    millisecsTaken);
    app.DebugPrintf(
        "---------------------------------------------------------\n");
    app.DebugPrintf(
        "---------------------------------------------------------\n");
    app.DebugPrintf("\n");

    killTimer(0);
}

void UIScene_ConnectingProgress::handleTimerComplete(int id) {
    // Check if the connection failed
    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (pMinecraft->m_connectionFailed[m_iPad] ||
        !g_NetworkManager.IsInSession()) {
        int exitReasonStringId;
        switch (pMinecraft->m_connectionFailedReason[m_iPad]) {
            case DisconnectPacket::eDisconnect_LoginTooLong:
                exitReasonStringId = IDS_DISCONNECTED_LOGIN_TOO_LONG;
                break;
            case DisconnectPacket::eDisconnect_ServerFull:
                exitReasonStringId = IDS_DISCONNECTED_SERVER_FULL;
                break;
            case DisconnectPacket::eDisconnect_Kicked:
                exitReasonStringId = IDS_DISCONNECTED_KICKED;
                break;
            case DisconnectPacket::eDisconnect_NoUGC_AllLocal:
                exitReasonStringId =
                    IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_ALL_LOCAL;
                break;
            case DisconnectPacket::eDisconnect_NoUGC_Single_Local:
                exitReasonStringId =
                    IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_SINGLE_LOCAL;
                break;
            case DisconnectPacket::eDisconnect_NoFlying:
                exitReasonStringId = IDS_DISCONNECTED_FLYING;
                break;
            case DisconnectPacket::eDisconnect_Quitting:
                exitReasonStringId = IDS_DISCONNECTED_SERVER_QUIT;
                break;
            case DisconnectPacket::eDisconnect_OutdatedServer:
                exitReasonStringId = IDS_DISCONNECTED_SERVER_OLD;
                break;
            case DisconnectPacket::eDisconnect_OutdatedClient:
                exitReasonStringId = IDS_DISCONNECTED_CLIENT_OLD;
                break;
            default:
                exitReasonStringId = IDS_CONNECTION_LOST_SERVER;
                break;
        }

        if (m_iPad != PlatformProfile.GetPrimaryPad() &&
            g_NetworkManager.IsInSession()) {
            m_buttonConfirm.setVisible(true);
            m_showingButton = true;

            // Set text
            m_labelTitle.setLabel(app.GetString(IDS_CONNECTION_FAILED));
            m_progressBar.setLabel(app.GetString(exitReasonStringId));
            m_progressBar.setVisible(true);
            m_controlTimer.setVisible(false);
        } else {
            unsigned int uiIDA[1];
            uiIDA[0] = IDS_CONFIRM_OK;
            ui.RequestErrorMessage(IDS_CONNECTION_FAILED, exitReasonStringId,
                                   uiIDA, 1, PlatformProfile.GetPrimaryPad());
            exitReasonStringId = -1;

            // app.NavigateToHomeMenu();
            app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_ExitWorld,
                          (void*)true);
        }
    }
}

void UIScene_ConnectingProgress::handleInput(int iPad, int key, bool repeat,
                                             bool pressed, bool released,
                                             bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");

    if (m_showTooltips) {
        ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

        switch (key) {
                // 4J-PB - Removed the option to cancel join - it didn't work
                // anyway 		case ACTION_MENU_CANCEL:
                // 			{
                // 				if(m_cancelFunc != nullptr)
                // 				{
                // 					m_cancelFunc(m_cancelFuncParam);
                // 				}
                // 				else
                // 				{
                // 					// Cancel the join
                // 					Minecraft *pMinecraft =
                // Minecraft::GetInstance();
                // 					pMinecraft->removeLocalPlayerIdx(m_iPad);
                // 				}
                // 				handled = true;
                // 			}
                // 			break;
            case ACTION_MENU_OK:
                if (pressed) {
                    sendInputToMovie(key, repeat, pressed, released);
                }
                break;
            case ACTION_MENU_UP:
            case ACTION_MENU_DOWN:
                if (pressed) {
                    sendInputToMovie(key, repeat, pressed, released);
                }
                break;
        }
    }
}

void UIScene_ConnectingProgress::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Confirm:
            if (m_showingButton) {
                if (m_iPad != PlatformProfile.GetPrimaryPad() &&
                    g_NetworkManager.IsInSession()) {
                    // The connection failed if we see the button, so the temp
                    // player should be removed and the viewports updated again
                    // This is actually done in the tick as we can't pull down
                    // the scene we are currently in from here
                    m_removeLocalPlayer = true;
                } else {
                    ui.NavigateToHomeMenu();
                    // app.NavigateBack( PlatformProfile.GetPrimaryPad() );
                }
            }
            break;
    }
}
