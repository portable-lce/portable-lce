#pragma once

#include <cstdint>
#include <string>

#include "IUIScene_StartGame.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_BitmapIcon.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/Controls/UIControl_TexturePackList.h"
#include "app/common/UI/UIScene.h"
#include "platform/storage/storage.h"

class DLCPack;
class LevelGenerationOptions;
class UILayer;

class UIScene_LoadMenu : public IUIScene_StartGame {
private:
    enum EControls {
        eControl_GameMode,
        eControl_Difficulty,
        eControl_MoreOptions,
        eControl_LoadWorld,
        eControl_TexturePackList,
        eControl_OnlineGame,
    };

    static int m_iDifficultyTitleSettingA[4];

    UIControl m_controlMainPanel;
    UIControl_Label m_labelGameName, m_labelSeed, m_labelCreatedMode;
    UIControl_Button m_buttonGamemode, m_buttonMoreOptions, m_buttonLoadWorld;
    UIControl_Slider m_sliderDifficulty;
    UIControl_BitmapIcon m_bitmapIcon;

    UIControl_CheckBox m_checkboxOnline;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(IUIScene_StartGame)
    UI_MAP_ELEMENT(m_controlMainPanel, "MainPanel")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlMainPanel)
    UI_MAP_ELEMENT(m_labelGameName, "GameName")
    UI_MAP_ELEMENT(m_labelCreatedMode, "CreatedMode")
    UI_MAP_ELEMENT(m_labelSeed, "Seed")
    UI_MAP_ELEMENT(m_texturePackList, "TexturePackSelector")
    UI_MAP_ELEMENT(m_buttonGamemode, "GameModeToggle")
    UI_MAP_ELEMENT(m_checkboxOnline, "CheckboxOnline")
    UI_MAP_ELEMENT(m_buttonMoreOptions, "MoreOptions")
    UI_MAP_ELEMENT(m_buttonLoadWorld, "LoadSettings")
    UI_MAP_ELEMENT(m_sliderDifficulty, "Difficulty")
    UI_MAP_ELEMENT(m_bitmapIcon, "LevelIcon")
    UI_END_MAP_CHILD_ELEMENTS()
    UI_END_MAP_ELEMENTS_AND_NAMES()

    LevelGenerationOptions* m_levelGen;
    DLCPack* m_pDLCPack;

    int m_iSaveGameInfoIndex;
    int m_CurrentDifficulty;
    bool m_bGameModeCreative;
    int m_iGameModeId;
    bool m_bHasBeenInCreative;
    bool m_bIsSaveOwner;
    bool m_bRetrievingSaveThumbnail;
    bool m_bSaveThumbnailReady;
    bool m_bMultiplayerAllowed;
    bool m_bShowTimer;
    bool m_bAvailableTexturePacksChecked;
    bool m_bRequestQuadrantSignin;
    bool m_bIsCorrupt;
    bool m_bThumbnailGetFailed;
    int64_t m_seed;

    // int *m_iConfigA; // track the texture packs that we don't have installed

    std::uint8_t* m_pbThumbnailData;
    unsigned int m_uiThumbnailSize;
    std::string m_thumbnailName;

    bool m_bRebuildTouchBoxes;

public:
    UIScene_LoadMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual EUIScene getSceneType() { return eUIScene_LoadMenu; }

    virtual void tick();

    virtual UIControl* GetMainPanel();

    virtual void handleTouchBoxRebuild();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    virtual void handleTimerComplete(int id);

protected:
    void handlePress(F64 controlId, F64 childId);
    void handleSliderMove(F64 sliderId, F64 currentValue);
    virtual void handleGainFocus(bool navBack);

private:
    void StartSharedLaunchFlow();
    virtual void checkStateAndStartGame();
    void LaunchGame(void);

    static int ConfirmLoadReturned(void* pParam, int iPad,
                                   IPlatformStorage::EMessageResult result);
    static void StartGameFromSave(UIScene_LoadMenu* pClass, int localUsersMask);
    int loadSaveDataReturned(bool bIsCorrupt, bool bIsOwner);
    static int TrophyDialogReturned(void* pParam, int iPad,
                                    IPlatformStorage::EMessageResult result);
    static int LoadDataComplete(void* pParam);
    static int CheckResetNetherReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int DeleteSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    int deleteSaveDataReturned(bool bSuccess);
    static int MustSignInReturnedPSN(void* pParam, int iPad,
                                     IPlatformStorage::EMessageResult result);

public:
    int loadSaveDataThumbnailReturned(std::uint8_t* pbThumbnail,
                                      unsigned int thumbnailBytes);
    static int StartGame_SignInReturned(void* pParam, bool, int);
};