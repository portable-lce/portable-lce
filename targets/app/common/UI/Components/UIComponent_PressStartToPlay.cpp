#include "UIComponent_PressStartToPlay.h"

#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/linux/Linux_UIController.h"
#include "strings.h"

class UILayer;

UIComponent_PressStartToPlay::UIComponent_PressStartToPlay(int iPad,
                                                           void* initData,
                                                           UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_showingSaveIcon = false;
    m_showingAutosaveTimer = false;
    m_showingTrialTimer = false;
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        m_showingPressStart[i] = false;
    }
    m_trialTimer = "";
    m_autosaveTimer = "";

    m_labelTrialTimer.init("");
    m_labelTrialTimer.setVisible(false);

    // 4J-JEV: This object is persistent, so this string needs to be able to
    // handle language changes.
    m_labelPressStart.init(IDS_PRESS_START_TO_JOIN);

    m_controlSaveIcon.setVisible(false);
    m_controlPressStartPanel.setVisible(false);
    m_playerDisplayName.setVisible(false);
}

std::string UIComponent_PressStartToPlay::getMoviePath() {
    return "PressStartToPlay";
}

void UIComponent_PressStartToPlay::handleReload() {
    // 4J Stu - It's possible these could change during the reload, so can't use
    // the normal controls refresh of it's state
    m_controlSaveIcon.setVisible(m_showingSaveIcon);
    m_labelTrialTimer.setVisible(m_showingAutosaveTimer);
    m_labelTrialTimer.setLabel(m_autosaveTimer);
    m_labelTrialTimer.setVisible(m_showingTrialTimer);
    m_labelTrialTimer.setLabel(m_trialTimer);

    bool showPressStart = false;
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        bool show = m_showingPressStart[i];
        showPressStart |= show;

        if (show) {
            addTimer(0, 3000);

            IggyDataValue result;
            IggyDataValue value[1];
            value[0].type = IGGY_DATATYPE_number;
            value[0].number = i;

            IggyResult out = IggyPlayerCallMethodRS(
                getMovie(), &result, IggyPlayerRootPath(getMovie()),
                m_funcShowController, 1, value);
        }
    }
    m_controlPressStartPanel.setVisible(showPressStart);
}

void UIComponent_PressStartToPlay::handleTimerComplete(int id) {
    m_controlPressStartPanel.setVisible(false);
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        m_showingPressStart[i] = false;
    }
    ui.ClearPressStart();
}

void UIComponent_PressStartToPlay::showPressStart(int iPad, bool show) {
    m_showingPressStart[iPad] = show;
    if (!ui.IsExpectingOrReloadingSkin() && hasMovie()) {
        m_controlPressStartPanel.setVisible(show);

        if (show) {
            addTimer(0, 3000);

            IggyDataValue result;
            IggyDataValue value[1];
            value[0].type = IGGY_DATATYPE_number;
            value[0].number = iPad;

            IggyResult out = IggyPlayerCallMethodRS(
                getMovie(), &result, IggyPlayerRootPath(getMovie()),
                m_funcShowController, 1, value);
        }
    }
}

void UIComponent_PressStartToPlay::setTrialTimer(const std::string& label) {
    m_trialTimer = label;
    if (!ui.IsExpectingOrReloadingSkin() && hasMovie()) {
        m_labelTrialTimer.setLabel(label);
    }
}

void UIComponent_PressStartToPlay::showTrialTimer(bool show) {
    m_showingTrialTimer = show;
    if (!ui.IsExpectingOrReloadingSkin() && hasMovie()) {
        m_labelTrialTimer.setVisible(show);
    }
}

void UIComponent_PressStartToPlay::setAutosaveTimer(const std::string& label) {
    m_autosaveTimer = label;
    if (!ui.IsExpectingOrReloadingSkin() && hasMovie()) {
        m_labelTrialTimer.setLabel(label);
    }
}

void UIComponent_PressStartToPlay::showAutosaveTimer(bool show) {
    m_showingAutosaveTimer = show;
    if (!ui.IsExpectingOrReloadingSkin() && hasMovie()) {
        m_labelTrialTimer.setVisible(show);
    }
}

void UIComponent_PressStartToPlay::showSaveIcon(bool show) {
    m_showingSaveIcon = show;
    if (!ui.IsExpectingOrReloadingSkin() && hasMovie()) {
        m_controlSaveIcon.setVisible(show);
    } else {
        if (show)
            app.DebugPrintf(
                "Tried to show save icon while texture pack reload was in "
                "progress\n");
    }
}

void UIComponent_PressStartToPlay::showPlayerDisplayName(bool show) {
    m_playerDisplayName.setVisible(false);
}