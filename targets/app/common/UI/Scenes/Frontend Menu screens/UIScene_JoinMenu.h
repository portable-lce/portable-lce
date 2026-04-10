#pragma once

#include <string>

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "platform/storage/storage.h"

class FriendSessionInfo;
class UILayer;

class UIScene_JoinMenu : public UIScene {
private:
    enum EControls { eControl_JoinGame, eControl_GamePlayers };

    enum ELabels {
        eLabel_Difficulty,
        eLabel_GameType,
        eLabel_GamertagsOn,
        eLabel_Structures,
        eLabel_LevelType,
        eLabel_PVP,
        eLabel_Trust,
        eLabel_TNTOn,
        eLabel_FireOn,

        eLabel_COUNT
    };

    UIControl_Button m_buttonJoinGame;
    UIControl_ButtonList m_buttonListPlayers;

    UIControl_Label m_labelLabels[eLabel_COUNT];
    UIControl_Label m_labelValues[eLabel_COUNT];
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonJoinGame, "JoinGame")
    UI_MAP_ELEMENT(m_buttonListPlayers, "GamePlayers")

    UI_MAP_ELEMENT(m_labelLabels[0], "Label0")
    UI_MAP_ELEMENT(m_labelLabels[1], "Label1")
    UI_MAP_ELEMENT(m_labelLabels[2], "Label2")
    UI_MAP_ELEMENT(m_labelLabels[3], "Label3")
    UI_MAP_ELEMENT(m_labelLabels[4], "Label4")
    UI_MAP_ELEMENT(m_labelLabels[5], "Label5")
    UI_MAP_ELEMENT(m_labelLabels[6], "Label6")
    UI_MAP_ELEMENT(m_labelLabels[7], "Label7")
    UI_MAP_ELEMENT(m_labelLabels[8], "Label8")

    UI_MAP_ELEMENT(m_labelValues[0], "Value0")
    UI_MAP_ELEMENT(m_labelValues[1], "Value1")
    UI_MAP_ELEMENT(m_labelValues[2], "Value2")
    UI_MAP_ELEMENT(m_labelValues[3], "Value3")
    UI_MAP_ELEMENT(m_labelValues[4], "Value4")
    UI_MAP_ELEMENT(m_labelValues[5], "Value5")
    UI_MAP_ELEMENT(m_labelValues[6], "Value6")
    UI_MAP_ELEMENT(m_labelValues[7], "Value7")
    UI_MAP_ELEMENT(m_labelValues[8], "Value8")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    FriendSessionInfo* m_selectedSession;
    bool m_bIgnoreInput;
    bool m_friendInfoRequestIssued;
    bool m_friendInfoUpdatedOK;
    bool m_friendInfoUpdatedERROR;

public:
    UIScene_JoinMenu(int iPad, void* initData, UILayer* parentLayer);
    void tick();
    static void friendSessionUpdated(bool success, void* pParam);
    static int ErrorDialogReturned(void* pParam, int iPad,
                                   const IPlatformStorage::EMessageResult);

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual EUIScene getSceneType() { return eUIScene_LoadMenu; }

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    virtual void handleFocusChange(F64 controlId, F64 childId);
    virtual void handleTimerComplete(int id);

protected:
    void handlePress(F64 controlId, F64 childId);

    void StartSharedLaunchFlow();

    static int StartGame_SignInReturned(void* pParam, bool, int);
    static void JoinGame(UIScene_JoinMenu* pClass);
};
