#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Progress.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"

class C4JThread;
class UILayer;

class UIScene_FullscreenProgress : public UIScene {
private:
    enum EControl {
        eControl_Confirm,
    };

    static const int TIMER_FULLSCREEN_TIPS = 1;
    static const int TIMER_FULLSCREEN_TIPS_TIME = 7000;

    C4JThread* thread;
    bool threadStarted;
    UIFullscreenProgressCompletionData* m_CompletionData;
    bool m_threadCompleted;
    int m_iPad;
    void (*m_cancelFunc)(void* param);
    void (*m_completeFunc)(void* param);
    void* m_cancelFuncParam;
    void* m_completeFuncParam;
    bool m_bWaitForThreadToDelete;

    std::string m_titleText, m_statusText;
    int m_lastTitle, m_lastStatus, m_lastProgress;
    int m_cancelText;
    bool m_bWasCancelled;

    UIControl_Progress m_progressBar;
    UIControl_Label m_labelTitle, m_labelTip;
    UIControl_Button m_buttonConfirm;
    UIControl m_controlTimer;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_progressBar, "ProgressBar")
    UI_MAP_ELEMENT(m_labelTitle, "Title")
    UI_MAP_ELEMENT(m_labelTip, "Tip")
    UI_MAP_ELEMENT(m_buttonConfirm, "Confirm")
    UI_MAP_ELEMENT(m_controlTimer, "Timer")
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_FullscreenProgress(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_FullscreenProgress();

    virtual EUIScene getSceneType() { return eUIScene_FullscreenProgress; }
    virtual void updateTooltips();
    virtual void handleDestroy();

    void tick();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

    virtual long long getDefaultGtcButtons() { return 0; }

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    void handlePress(F64 controlId, F64 childId);

    virtual void handleTimerComplete(int id);

    void SetWasCancelled(bool wasCancelled);

    virtual bool isReadyToDelete();
};
