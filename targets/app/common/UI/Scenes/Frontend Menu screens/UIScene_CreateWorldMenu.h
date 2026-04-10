#pragma once

#include <stdint.h>

#include <string>

#include "IUIScene_StartGame.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_BitmapIcon.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/Controls/UIControl_TexturePackList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"
#include "platform/storage/storage.h"

class DLCPack;
class UILayer;

class UIScene_CreateWorldMenu : public IUIScene_StartGame {
private:
    enum EControls {
        eControl_EditWorldName,
        eControl_TexturePackList,
        eControl_GameModeToggle,
        eControl_Difficulty,
        eControl_MoreOptions,
        eControl_NewWorld,
        eControl_OnlineGame,
    };

    static int m_iDifficultyTitleSettingA[4];

    std::string m_worldName;
    std::string m_seed;

    UIControl m_controlMainPanel;
    UIControl_Label m_labelWorldName;
    UIControl_Button m_buttonGamemode, m_buttonMoreOptions, m_buttonCreateWorld;
    UIControl_TextInput m_editWorldName;
    UIControl_Slider m_sliderDifficulty;
    UIControl_CheckBox m_checkboxOnline;

    UIControl_BitmapIcon m_bitmapIcon, m_bitmapComparison;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(IUIScene_StartGame)
    UI_MAP_ELEMENT(m_controlMainPanel, "MainPanel")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_labelWorldName, "WorldName")
    UI_MAP_ELEMENT(m_editWorldName, "EditWorldName")
    UI_MAP_ELEMENT(m_texturePackList, "TexturePackSelector")
    UI_MAP_ELEMENT(m_buttonGamemode, "GameModeToggle")
    UI_MAP_ELEMENT(m_checkboxOnline, "CheckboxOnline")
    UI_MAP_ELEMENT(m_buttonMoreOptions, "MoreOptions")
    UI_MAP_ELEMENT(m_buttonCreateWorld, "NewWorld")
    UI_MAP_ELEMENT(m_sliderDifficulty, "Difficulty")
    UI_END_MAP_CHILD_ELEMENTS()
    UI_END_MAP_ELEMENTS_AND_NAMES()

    bool m_bGameModeCreative;
    int m_iGameModeId;
    bool m_bMultiplayerAllowed;
    DLCPack* m_pDLCPack;
    bool m_bRebuildTouchBoxes;

public:
    UIScene_CreateWorldMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_CreateWorldMenu();

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual EUIScene getSceneType() { return eUIScene_CreateWorldMenu; }

    virtual void handleDestroy();
    virtual void tick();

    virtual UIControl* GetMainPanel();

    virtual void handleTouchBoxRebuild();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

    virtual void handleTimerComplete(int id);
    virtual void handleGainFocus(bool navBack);

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

private:
    void StartSharedLaunchFlow();
    bool IsLocalMultiplayerAvailable();

protected:
    void handlePress(F64 controlId, F64 childId);
    void handleSliderMove(F64 sliderId, F64 currentValue);

    static void CreateGame(UIScene_CreateWorldMenu* pClass,
                           int32_t LocalUsersMask);
    static int ConfirmCreateReturned(void* pParam, int iPad,
                                     IPlatformStorage::EMessageResult result);
    static int StartGame_SignInReturned(void* pParam, bool bContinue, int iPad);
    static int MustSignInReturnedPSN(void* pParam, int iPad,
                                     IPlatformStorage::EMessageResult result);

    virtual void checkStateAndStartGame();
};