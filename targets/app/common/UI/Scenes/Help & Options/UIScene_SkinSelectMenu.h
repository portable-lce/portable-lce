#pragma once
#include <cstdint>
#include <format>
#include <string>

#include "app/common/DLC/DLCPack.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_PlayerSkinPreview.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/iggy.h"
#include "platform/storage/storage.h"
#ifndef _ENABLEIGGY
#include "app/linux/Stubs/iggy_stubs.h"
#endif
#include "app/linux/Iggy/include/rrCore.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/world/entity/player/SkinTypes.h"

class DLCPack;
class UILayer;

class UIScene_SkinSelectMenu : public UIScene {
private:
    static const char*
        wchDefaultNamesA[std::to_underlying(EDefaultSkins::Count)];

    // 4J Stu - How many to show on each side of the main control
    static const int sidePreviewControls = 4;

    enum ESkinSelectNavigation {
        eSkinNavigation_Pack,
        eSkinNavigation_Skin,

        eSkinNavigation_Count,
    };

    enum ECharacters {
        eCharacter_Current,
        eCharacter_Next1,
        eCharacter_Next2,
        eCharacter_Next3,
        eCharacter_Next4,
        eCharacter_Previous1,
        eCharacter_Previous2,
        eCharacter_Previous3,
        eCharacter_Previous4,

        eCharacter_COUNT,
    };

    UIControl_PlayerSkinPreview m_characters[eCharacter_COUNT];
    UIControl_Label m_labelSkinName, m_labelSkinOrigin;
    UIControl_Label m_labelSelected;
    UIControl m_controlSkinNamePlate, m_controlSelectedPanel,
        m_controlIggyCharacters, m_controlTimer;
    IggyName m_funcSetPlayerCharacterSelected, m_funcSetCharacterLocked;
    IggyName m_funcSetLeftLabel, m_funcSetRightLabel, m_funcSetCentreLabel;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_controlSkinNamePlate, "SkinNamePlate")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlSkinNamePlate)
    UI_MAP_ELEMENT(m_labelSkinName, "SkinTitle1")
    UI_MAP_ELEMENT(m_labelSkinOrigin, "SkinTitle2")
    UI_END_MAP_CHILD_ELEMENTS()

    UI_MAP_ELEMENT(m_controlSelectedPanel, "SelectedPanel")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlSelectedPanel)
    UI_MAP_ELEMENT(m_labelSelected, "SelectedPanelLabel")
    UI_END_MAP_CHILD_ELEMENTS()

    UI_MAP_ELEMENT(m_controlTimer, "Timer")

    // 4J Stu - These aren't really used a AS3 controls, but adding here means
    // that they get ticked by the scene
    UI_MAP_ELEMENT(m_controlIggyCharacters, "IggyCharacters")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlIggyCharacters)
    UI_MAP_ELEMENT(m_characters[eCharacter_Current], "iggy_Character0")

    UI_MAP_ELEMENT(m_characters[eCharacter_Next1], "iggy_Character1")
    UI_MAP_ELEMENT(m_characters[eCharacter_Next2], "iggy_Character2")
    UI_MAP_ELEMENT(m_characters[eCharacter_Next3], "iggy_Character3")
    UI_MAP_ELEMENT(m_characters[eCharacter_Next4], "iggy_Character4")

    UI_MAP_ELEMENT(m_characters[eCharacter_Previous1], "iggy_Character5")
    UI_MAP_ELEMENT(m_characters[eCharacter_Previous2], "iggy_Character6")
    UI_MAP_ELEMENT(m_characters[eCharacter_Previous3], "iggy_Character7")
    UI_MAP_ELEMENT(m_characters[eCharacter_Previous4], "iggy_Character8")
    UI_END_MAP_CHILD_ELEMENTS()

    UI_MAP_NAME(m_funcSetPlayerCharacterSelected, "SetPlayerCharacterSelected")
    UI_MAP_NAME(m_funcSetCharacterLocked, "SetCharacterLocked")

    UI_MAP_NAME(m_funcSetLeftLabel, "SetLeftLabel")
    UI_MAP_NAME(m_funcSetCentreLabel, "SetCenterLabel")
    UI_MAP_NAME(m_funcSetRightLabel, "SetRightLabel")
    UI_END_MAP_ELEMENTS_AND_NAMES()

    DLCPack* m_currentPack;
    int m_packIndex, m_skinIndex;
    std::uint32_t m_originalSkinId;
    std::string m_currentSkinPath, m_selectedSkinPath, m_selectedCapePath;
    std::vector<SKIN_BOX*>* m_vAdditionalSkinBoxes;

    bool m_bSlidingSkins, m_bAnimatingMove;
    ESkinSelectNavigation m_currentNavigation;

    bool m_bNoSkinsToShow;
    int m_currentPackCount;
    bool m_bIgnoreInput;
    bool m_bSkinIndexChanged;
    std::string m_leftLabel, m_centreLabel, m_rightLabel;

    S32 m_iTouchXStart;
    bool m_bTouchScrolled;

public:
    UIScene_SkinSelectMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual void tick();

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual EUIScene getSceneType() { return eUIScene_SkinSelectMenu; }

    virtual void handleAnimationEnd();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

    virtual void customDraw(IggyCustomDrawCallbackRegion* region);

private:
    void handleSkinIndexChanged();
    int getNextSkinIndex(int sourceIndex);
    int getPreviousSkinIndex(int sourceIndex);

    TEXTURE_NAME getTextureId(int skinIndex);

    void handlePackIndexChanged();
    void updatePackDisplay();
    int getNextPackIndex(int sourceIndex);
    int getPreviousPackIndex(int sourceIndex);

    void setCharacterSelected(bool selected);
    void setCharacterLocked(bool locked);

    void setLeftLabel(const std::string& label);
    void setCentreLabel(const std::string& label);
    void setRightLabel(const std::string& label);

    virtual void HandleDLCMountingComplete();
    virtual void HandleDLCInstalled();

    void showNotOnlineDialog(int iPad);

    static int UnlockSkinReturned(void* pParam, int iPad,
                                  IPlatformStorage::EMessageResult result);
    static int RenableInput(void* lpVoid, int, int);
    void AddFavoriteSkin(int iPad, int iSkinID);

    void InputActionOK(unsigned int iPad);
    virtual void handleReload();
};