#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "platform/PlatformTypes.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif

class UILayer;

class UIComponent_PressStartToPlay : public UIScene {
private:
    bool m_showingSaveIcon;
    bool m_showingAutosaveTimer;
    bool m_showingTrialTimer;
    bool m_showingPressStart[XUSER_MAX_COUNT];
    std::string m_trialTimer;
    std::string m_autosaveTimer;

protected:
    UIControl_Label m_labelTrialTimer, m_labelPressStart, m_playerDisplayName;
    UIControl m_controlSaveIcon, m_controlPressStartPanel;
    IggyName m_funcShowController;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_labelTrialTimer, "TrialTimer")
    UI_MAP_ELEMENT(m_controlSaveIcon, "SaveIcon")
    UI_MAP_ELEMENT(m_playerDisplayName, "PlayerName")
    UI_MAP_ELEMENT(m_controlPressStartPanel, "MainPanel")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlPressStartPanel)
    UI_MAP_ELEMENT(m_labelPressStart, "PressStartLabel")
    UI_END_MAP_CHILD_ELEMENTS()

    UI_MAP_NAME(m_funcShowController, "ShowController");
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIComponent_PressStartToPlay(int iPad, void* initData,
                                 UILayer* parentLayer);

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    virtual EUIScene getSceneType() { return eUIComponent_PressStartToPlay; }

    // Returns true if this scene handles input
    virtual bool stealsFocus() { return false; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return false; }

    // Returns true if lower scenes in this scenes layer, or in any layer below
    // this scenes layers should be hidden
    virtual bool hidesLowerScenes() { return false; }

    virtual void handleReload();
    virtual void handleTimerComplete(int id);

    void showPressStart(int iPad, bool show);
    void setTrialTimer(const std::string& label);
    void showTrialTimer(bool show);
    void setAutosaveTimer(const std::string& label);
    void showAutosaveTimer(bool show);
    void showSaveIcon(bool show);
    void showPlayerDisplayName(bool show);
};