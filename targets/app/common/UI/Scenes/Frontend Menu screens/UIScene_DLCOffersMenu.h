#pragma once

#include <string>

#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_BitmapIcon.h"
#include "app/common/UI/Controls/UIControl_DLCList.h"
#include "app/common/UI/Controls/UIControl_HTMLLabel.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "platform/storage/storage.h"

class UILayer;

class UIScene_DLCOffersMenu : public UIScene {
private:
    enum EControls {
        eControl_OffersList,
    };

    bool m_bIsSD;
    bool m_bHasPurchased;
    bool m_bIsSelected;

    UIControl_DLCList m_buttonListOffers;
    UIControl_Label m_labelOffers, m_labelPriceTag, m_labelXboxStore;
    UIControl_HTMLLabel m_labelHTMLSellText;
    UIControl_BitmapIcon m_bitmapIconOfferImage;
    UIControl m_Timer;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonListOffers, "OffersList")
    UI_MAP_ELEMENT(m_labelOffers, "OffersList_Title")
    UI_MAP_ELEMENT(m_labelPriceTag, "PriceTag")
    UI_MAP_ELEMENT(m_labelHTMLSellText, "HTMLSellText")
    UI_MAP_ELEMENT(m_bitmapIconOfferImage, "DLCIcon")
    UI_MAP_ELEMENT(m_Timer, "Timer")

    if (m_loadedResolution == eSceneResolution_1080) {
        UI_MAP_ELEMENT(m_labelXboxStore, "XboxLabel")
    }
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_DLCOffersMenu(int iPad, void* initData, UILayer* parentLayer);
    ~UIScene_DLCOffersMenu();
    static int ExitDLCOffersMenu(void* pParam, int iPad,
                                 IPlatformStorage::EMessageResult result);

    virtual EUIScene getSceneType() { return eUIScene_DLCOffersMenu; }
    virtual void tick();
    virtual void updateTooltips();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

    virtual void handlePress(F64 controlId, F64 childId);
    virtual void handleSelectionChanged(F64 selectedId);
    virtual void handleFocusChange(F64 controlId, F64 childId);
    virtual void handleTimerComplete(int id);

private:
    bool m_bProductInfoShown;
    int m_iProductInfoIndex;
    int m_iCurrentDLC;
    int m_iTotalDLC;
    bool m_bAddAllDLCButtons;
};