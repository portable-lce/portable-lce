#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_BitmapIcon.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TexturePackList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"
#include "platform/storage/storage.h"

class UILayer;

// Shared functions between CreteWorld, Load and Join
class IUIScene_StartGame : public UIScene {
protected:
    UIControl_TexturePackList m_texturePackList;

    UIControl m_controlTexturePackPanel;
    UIControl_Label m_labelTexturePackName, m_labelTexturePackDescription;
    UIControl_BitmapIcon m_bitmapTexturePackIcon, m_bitmapComparison;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_controlTexturePackPanel, "TexturePackPanel")
    UI_BEGIN_MAP_CHILD_ELEMENTS(m_controlTexturePackPanel)
    UI_MAP_ELEMENT(m_labelTexturePackName, "TexturePackName")
    UI_MAP_ELEMENT(m_labelTexturePackDescription, "TexturePackDescription")
    UI_MAP_ELEMENT(m_bitmapTexturePackIcon, "Icon")
    UI_MAP_ELEMENT(m_bitmapComparison, "ComparisonPic")
    UI_END_MAP_CHILD_ELEMENTS()
    UI_END_MAP_ELEMENTS_AND_NAMES()

    LaunchMoreOptionsMenuInitData m_MoreOptionsParams;
    bool m_bIgnoreInput;

    int m_iTexturePacksNotInstalled;
    unsigned int m_currentTexturePackIndex;
    bool m_bShowTexturePackDescription;
    bool m_texturePackDescDisplayed;
    int m_iSetTexturePackDescription;

    IUIScene_StartGame(int iPad, UILayer* parentLayer);

    virtual void checkStateAndStartGame() = 0;

    virtual void handleSelectionChanged(F64 selectedId);

    virtual void HandleDLCMountingComplete();

    void UpdateTexturePackDescription(int index);
    void UpdateCurrentTexturePack(int iSlot);

    static int TrialTexturePackWarningReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int UnlockTexturePackReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int TexturePackDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
};