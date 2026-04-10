#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#include "platform/storage/storage.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "java/Random.h"

class Random;
class UILayer;

class UIScene_MainMenu : public UIScene {
private:
    enum EControls {
        eControl_PlayGame,
        eControl_Leaderboards,
        eControl_Achievements,
        eControl_HelpAndOptions,
        eControl_UnlockOrDLC,
        eControl_Exit,
        eControl_Count,
    };

    // #ifdef 0
    // 	enum EPatchCheck
    // 	{
    // 		ePatchCheck_Idle,
    // 		ePatchCheck_Init,
    // 		ePatchCheck_Running,
    // 	};
    // #endif

    UIControl_Button m_buttons[eControl_Count];
    UIControl m_controlTimer;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttons[(int)eControl_PlayGame], "Button1")
    UI_MAP_ELEMENT(m_buttons[(int)eControl_Leaderboards], "Button2")
    UI_MAP_ELEMENT(m_buttons[(int)eControl_Achievements], "Button3")
    UI_MAP_ELEMENT(m_buttons[(int)eControl_HelpAndOptions], "Button4")
    UI_MAP_ELEMENT(m_buttons[(int)eControl_UnlockOrDLC], "Button5")
    UI_MAP_ELEMENT(m_buttons[(int)eControl_Exit], "Button6")
    UI_MAP_ELEMENT(m_controlTimer, "Timer")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    static Random* random;
    bool m_bIgnorePress;
    bool m_bTrialVersion;
    bool m_bLoadTrialOnNetworkManagerReady;

    float m_fScreenWidth, m_fScreenHeight;
    float m_fRawWidth, m_fRawHeight;
    std::vector<std::string> m_splashes;
    std::string m_splash;
    enum eSplashIndexes {
        eSplashHappyBirthdayEx = 0,
        eSplashHappyBirthdayNotch,
        eSplashMerryXmas,
        eSplashHappyNewYear,

        // The start index in the splashes vector from which we can select a
        // random splash
        eSplashRandomStart,
    };

    enum eActions {
        eAction_None = 0,
        eAction_RunGame,
        eAction_RunLeaderboards,
        eAction_RunAchievements,
        eAction_RunHelpAndOptions,
        eAction_RunUnlockOrDLC,

    };
    eActions m_eAction;

private:
    // 4J-JEV: Delay navigation until font changes.
    static int eNavigateWhenReady;

    static void proceedToScene(int iPad, EUIScene eScene) {
        eNavigateWhenReady = (int)eScene;
    }

public:
    UIScene_MainMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_MainMenu();

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return bHasFocus; }

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual EUIScene getSceneType() { return eUIScene_MainMenu; }

    virtual void customDraw(IggyCustomDrawCallbackRegion* region);

protected:
    void customDrawSplash(IggyCustomDrawCallbackRegion* region);

    virtual std::string getMoviePath();

public:
    virtual void tick();
    virtual void handleReload();
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

    virtual void handleUnlockFullVersion();

protected:
    void handlePress(F64 controlId, F64 childId);

    void handleGainFocus(bool navBack);

    virtual long long getDefaultGtcButtons() { return 0; }

private:
    void RunPlayGame(int iPad);
    void RunLeaderboards(int iPad);
    void RunUnlockOrDLC(int iPad);
    void RunAchievements(int iPad);
    void RunHelpAndOptions(int iPad);

    void RunAction(int iPad);

    static void LoadTrial();

    static int CreateLoad_SignInReturned(void* pParam, bool bContinue,
                                         int iPad);
    static int HelpAndOptions_SignInReturned(void* pParam, bool bContinue,
                                             int iPad);
    static int Achievements_SignInReturned(void* pParam, bool bContinue,
                                           int iPad);
    static int MustSignInReturned(void* pParam, int iPad,
                                  IPlatformStorage::EMessageResult result);

    static int Leaderboards_SignInReturned(void* pParam, bool bContinue,
                                           int iPad);
    static int UnlockFullGame_SignInReturned(void* pParam, bool bContinue,
                                             int iPad);
    static int ExitGameReturned(void* pParam, int iPad,
                                IPlatformStorage::EMessageResult result);
    bool m_bRunGameChosen;
    int32_t m_errorCode;
    bool m_bErrorDialogRunning;
};
