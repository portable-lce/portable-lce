
#include "UIScene_FullscreenProgress.h"

#include <stdint.h>
#include <wchar.h>

#include "app/common/Game.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Progress.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "platform/thread/C4JThread.h"
#include "strings.h"

UIScene_FullscreenProgress::UIScene_FullscreenProgress(int iPad, void* initData,
                                                       UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    parentLayer->addComponent(iPad, eUIComponent_Panorama);
    parentLayer->addComponent(iPad, eUIComponent_Logo);
    parentLayer->showComponent(iPad, eUIComponent_Logo, true);
    parentLayer->showComponent(iPad, eUIComponent_MenuBackground, false);

    m_controlTimer.setVisible(false);

    m_titleText = "";
    m_statusText = "";

    m_lastTitle = -1;
    m_lastStatus = -1;
    m_lastProgress = 0;

    m_buttonConfirm.init(app.GetString(IDS_CONFIRM_OK), eControl_Confirm);
    m_buttonConfirm.setVisible(false);

    LoadingInputParams* params = (LoadingInputParams*)initData;

    m_CompletionData = params->completionData;
    m_iPad = params->completionData->iPad;
    m_cancelFunc = params->cancelFunc;
    m_cancelFuncParam = params->m_cancelFuncParam;
    m_completeFunc = params->completeFunc;
    m_completeFuncParam = params->m_completeFuncParam;

    m_cancelText = params->cancelText;
    m_bWasCancelled = false;
    m_bWaitForThreadToDelete = params->waitForThreadToDelete;

    // Clear the progress text
    Minecraft* pMinecraft = Minecraft::GetInstance();
    pMinecraft->progressRenderer->progressStart(-1);
    pMinecraft->progressRenderer->progressStage(-1);
    m_progressBar.init("", 0, 0, 100, 0);

    // set the tip
    std::string wsText =
        app.FormatHTMLString(m_iPad, app.GetString(app.GetNextTip()));

    char startTags[64];
    snprintf(startTags, 64, "<font color=\"#%08x\"><p align=center>",
             app.GetHTMLColour(eHTMLColor_White));
    wsText = startTags + wsText + "</p>";
    m_labelTip.init(wsText);

    addTimer(TIMER_FULLSCREEN_TIPS, TIMER_FULLSCREEN_TIPS_TIME);

    m_labelTitle.init("");

    m_labelTip.setVisible(m_CompletionData->bShowTips);

    thread = new C4JThread(params->func, params->lpParam, "FullscreenProgress");

    m_threadCompleted = false;
    thread->run();
    threadStarted = true;
}

UIScene_FullscreenProgress::~UIScene_FullscreenProgress() {
    m_parentLayer->removeComponent(eUIComponent_Panorama);
    m_parentLayer->removeComponent(eUIComponent_Logo);

    delete thread;

    delete m_CompletionData;
}

std::string UIScene_FullscreenProgress::getMoviePath() {
    return "FullscreenProgress";
}

void UIScene_FullscreenProgress::updateTooltips() {
    ui.SetTooltips(
        m_parentLayer->IsFullscreenGroup() ? XUSER_INDEX_ANY : m_iPad,
        m_threadCompleted ? IDS_TOOLTIPS_SELECT : -1,
        m_threadCompleted ? -1 : m_cancelText, -1, -1);
}

void UIScene_FullscreenProgress::handleDestroy() {
    int code = thread->getExitCode();
    const unsigned int exitcode = static_cast<unsigned int>(code);

    // If we're active, have a cancel func, and haven't already cancelled, call
    // cancel func
    if (exitcode == C4JThread::kStillActive && m_cancelFunc != nullptr &&
        !m_bWasCancelled) {
        m_bWasCancelled = true;
        m_cancelFunc(m_cancelFuncParam);
    }
}

void UIScene_FullscreenProgress::tick() {
    UIScene::tick();

    Minecraft* pMinecraft = Minecraft::GetInstance();

    int currentProgress = pMinecraft->progressRenderer->getCurrentPercent();
    if (currentProgress < 0) currentProgress = 0;
    if (currentProgress != m_lastProgress) {
        m_lastProgress = currentProgress;
        m_progressBar.setProgress(currentProgress);
        // app.DebugPrintf("Updated progress value\n");
    }

    int title = pMinecraft->progressRenderer->getCurrentTitle();
    if (title >= 0 && title != m_lastTitle) {
        m_lastTitle = title;
        m_titleText = app.GetString(title);
        m_labelTitle.setLabel(m_titleText);
    }

    ProgressRenderer::eProgressStringType eProgressType =
        pMinecraft->progressRenderer->getType();

    if (eProgressType == ProgressRenderer::eProgressStringType_ID) {
        int status = pMinecraft->progressRenderer->getCurrentStatus();
        if (status >= 0 && status != m_lastStatus) {
            m_lastStatus = status;
            m_statusText = app.GetString(status);
            m_progressBar.setLabel(m_statusText.c_str());
        }
    } else {
        std::string& wstrText =
            pMinecraft->progressRenderer->getProgressString();
        m_progressBar.setLabel(wstrText.c_str());
    }

    int code = thread->getExitCode();
    uint32_t exitcode = *((uint32_t*)&code);

    // app.DebugPrintf("CScene_FullscreenProgress Timer %d\n",pTimer->nId);

    if (exitcode != C4JThread::kStillActive) {
        // If we failed (currently used by network connection thread), navigate
        // back
        if (exitcode != 0) {
            if (exitcode == 1223 /* ERROR_CANCELLED */) {
                // Current thread cancelled for whatever reason
                // Currently used only for the
                // Game::RemoteSaveThreadProc thread Assume to
                // just ignore this thread as something else is now running that
                // will cause another action
            } else {
                /*m_threadCompleted = true;
                m_buttonConfirm.SetShow( true );
                m_buttonConfirm.SetFocus( m_CompletionData->iPad );
                m_CompletionData->type =
                e_ProgressCompletion_NavigateToHomeMenu;

                int exitReasonStringId;
                switch( app.GetDisconnectReason() )
                {
                default:
                exitReasonStringId = IDS_CONNECTION_FAILED;
                }
                Minecraft *pMinecraft=Minecraft::GetInstance();
                pMinecraft->progressRenderer->progressStartNoAbort(
                exitReasonStringId );*/
                // app.NavigateBack(m_CompletionData->iPad);

                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(
                    g_NetworkManager.CorrectErrorIDS(IDS_CONNECTION_FAILED),
                    g_NetworkManager.CorrectErrorIDS(
                        IDS_CONNECTION_LOST_SERVER),
                    uiIDA, 1, XUSER_INDEX_ANY);

                ui.NavigateToHomeMenu();
                ui.UpdatePlayerBasePositions();
            }
        } else {
            if ((m_CompletionData->bRequiresUserAction == true) &&
                (!m_bWasCancelled)) {
                m_threadCompleted = true;
                m_buttonConfirm.setVisible(true);
                // 4J-TomK - rebuild touch after confirm button made visible
                // again
                updateTooltips();
            } else {
                if (m_bWasCancelled) {
                    m_threadCompleted = true;
                }
                app.DebugPrintf("FullScreenProgress complete with action: ");
                switch (m_CompletionData->type) {
                    case e_ProgressCompletion_AutosaveNavigateBack:
                        app.DebugPrintf(
                            "e_ProgressCompletion_AutosaveNavigateBack\n");
                        {
                            // 4J Stu - Fix for #65437 - Customer Encountered:
                            // Code: Settings: Autosave option doesn't work when
                            // the Host goes into idle state during gameplay.
                            // Autosave obviously cannot occur if an ignore
                            // autosave menu is displayed, so even if we
                            // navigate back to a scene and not empty then we
                            // still want to reset this flag which was set true
                            // by the navigate to the fullscreen progress
                            ui.SetIgnoreAutosaveMenuDisplayed(m_iPad, false);

                            // This just allows it to be shown
                            Minecraft* pMinecraft = Minecraft::GetInstance();
                            if (pMinecraft->localgameModes
                                    [PlatformProfile.GetPrimaryPad()] !=
                                nullptr)
                                pMinecraft
                                    ->localgameModes[PlatformProfile
                                                         .GetPrimaryPad()]
                                    ->getTutorial()
                                    ->showTutorialPopup(true);
                            ui.UpdatePlayerBasePositions();
                            navigateBack();
                        }
                        break;

                    case e_ProgressCompletion_NavigateBack:
                        app.DebugPrintf("e_ProgressCompletion_NavigateBack\n");
                        {
                            ui.UpdatePlayerBasePositions();
                            navigateBack();
                        }
                        break;
                    case e_ProgressCompletion_NavigateBackToScene:
                        app.DebugPrintf(
                            "e_ProgressCompletion_NavigateBackToScene\n");
                        ui.UpdatePlayerBasePositions();
                        // 4J Stu - If used correctly this scene will not have
                        // interfered with any other scene at all, so just
                        // navigate back
                        navigateBack();
                        break;
                    case e_ProgressCompletion_CloseUIScenes:
                        app.DebugPrintf("e_ProgressCompletion_CloseUIScenes\n");
                        ui.CloseUIScenes(m_CompletionData->iPad);
                        ui.UpdatePlayerBasePositions();
                        break;
                    case e_ProgressCompletion_CloseAllPlayersUIScenes:
                        app.DebugPrintf(
                            "e_ProgressCompletion_CloseAllPlayersUIScenes\n");
                        ui.CloseAllPlayersScenes();
                        ui.UpdatePlayerBasePositions();
                        break;
                    case e_ProgressCompletion_NavigateToHomeMenu:
                        app.DebugPrintf(
                            "e_ProgressCompletion_NavigateToHomeMenu\n");
                        ui.NavigateToHomeMenu();
                        ui.UpdatePlayerBasePositions();
                        break;
                    default:
                        app.DebugPrintf("Default\n");
                        break;
                }
            }
        }
    }
}

void UIScene_FullscreenProgress::handleInput(int iPad, int key, bool repeat,
                                             bool pressed, bool released,
                                             bool& handled) {
    // if( m_showTooltips )
    {
        // ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

        switch (key) {
            case ACTION_MENU_OK:
                if (pressed) {
                    sendInputToMovie(key, repeat, pressed, released);
                }
                break;
            case ACTION_MENU_B:
            case ACTION_MENU_CANCEL:
                if (pressed && m_cancelFunc != nullptr && !m_bWasCancelled) {
                    m_bWasCancelled = true;
                    m_cancelFunc(m_cancelFuncParam);
                }
                break;
        }
    }
}

void UIScene_FullscreenProgress::handlePress(F64 controlId, F64 childId) {
    if (m_threadCompleted && (int)controlId == eControl_Confirm) {
        // This assumes all buttons can only be pressed with the A button
        ui.AnimateKeyPress(m_iPad, ACTION_MENU_A, false, true, false);

        // if there's a complete function, call it
        if (m_completeFunc) {
            m_completeFunc(m_completeFuncParam);
        }

        switch (m_CompletionData->type) {
            case e_ProgressCompletion_NavigateBack:
                app.DebugPrintf("e_ProgressCompletion_NavigateBack\n");
                {
                    ui.UpdatePlayerBasePositions();
                    navigateBack();
                }
                break;
            case e_ProgressCompletion_NavigateBackToScene:
                app.DebugPrintf("e_ProgressCompletion_NavigateBackToScene\n");
                ui.UpdatePlayerBasePositions();
                // 4J Stu - If used correctly this scene will not have
                // interfered with any other scene at all, so just navigate back
                navigateBack();
                break;
            case e_ProgressCompletion_CloseUIScenes:
                app.DebugPrintf("e_ProgressCompletion_CloseUIScenes\n");
                ui.CloseUIScenes(m_CompletionData->iPad);
                ui.UpdatePlayerBasePositions();
                break;
            case e_ProgressCompletion_CloseAllPlayersUIScenes:
                app.DebugPrintf(
                    "e_ProgressCompletion_CloseAllPlayersUIScenes\n");
                ui.CloseAllPlayersScenes();
                ui.UpdatePlayerBasePositions();
                break;
            case e_ProgressCompletion_NavigateToHomeMenu:
                app.DebugPrintf("e_ProgressCompletion_NavigateToHomeMenu\n");
                ui.NavigateToHomeMenu();
                ui.UpdatePlayerBasePositions();
                break;
            default:
                break;
        }
    }
}

void UIScene_FullscreenProgress::handleTimerComplete(int id) {
    switch (id) {
        case TIMER_FULLSCREEN_TIPS: {
            // display the next tip
            std::string wsText =
                app.FormatHTMLString(m_iPad, app.GetString(app.GetNextTip()));
            char startTags[64];
            snprintf(startTags, 64, "<font color=\"#%08x\"><p align=center>",
                     app.GetHTMLColour(eHTMLColor_White));
            wsText = startTags + wsText + "</p>";
            m_labelTip.setLabel(wsText);
        } break;
    }
}

void UIScene_FullscreenProgress::SetWasCancelled(bool wasCancelled) {
    m_bWasCancelled = wasCancelled;
}

bool UIScene_FullscreenProgress::isReadyToDelete() {
    if (m_bWaitForThreadToDelete) {
        return !thread->isRunning();
    } else {
        return true;
    }
}