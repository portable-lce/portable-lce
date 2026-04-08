#pragma once

#include <string>

#include "platform/profile/ProfileConstants.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "platform/NetTypes.h"
#include "minecraft/client/model/SkinBox.h"
#include "platform/XboxStubs.h"

class UILayer;

class UIScene_LanguageSelector : public UIScene {
public:
    enum ELangButtons {
        eLanguageSelector_LabelNone = -1,
        eLanguageSelector_system,
        eLanguageSelector_EN_US,
        eLanguageSelector_DE_DE,
        eLanguageSelector_ES_ES,
        eLanguageSelector_ES_MX,
        eLanguageSelector_FR_FR,
        eLanguageSelector_IT_IT,
        eLanguageSelector_PT_PT,
        eLanguageSelector_PT_BR,
        eLanguageSelector_JA_JP,
        eLanguageSelector_KO_KR,
        eLanguageSelector_CN_TW,
        eLanguageSelector_CN_CN,
        eLanguageSelector_DA_DK,
        eLanguageSelector_FI_FI,
        eLanguageSelector_NL_NL,
        eLanguageSelector_PL_PL,
        eLanguageSelector_RU_RU,
        eLanguageSelector_SV_SE,
        eLanguageSelector_NB_NO,
        eLanguageSelector_SK_SK,
        eLanguageSelector_CZ_CZ,
        eLanguageSelector_EL_GR,
        eLanguageSelector_TR_TR,
        eLanguageSelector_MAX
    };

private:
    enum EControls {
        eControl_Buttons,
    };

    static const unsigned int m_uiHTPButtonNameA[eLanguageSelector_MAX];

    UIControl_DynamicButtonList m_buttonListHowTo;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonListHowTo, "HowToList")
    UI_END_MAP_ELEMENTS_AND_NAMES()

public:
    UIScene_LanguageSelector(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_LanguageSelector; }

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual void handleReload();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    void handlePress(F64 controlId, F64 childId);
};

const int uiLangMap[UIScene_LanguageSelector::eLanguageSelector_MAX] = {
    MINECRAFT_LANGUAGE_DEFAULT, XC_LANGUAGE_ENGLISH,    XC_LANGUAGE_GERMAN,
    XC_LANGUAGE_SPANISH,        XC_LANGUAGE_SPANISH,    XC_LANGUAGE_FRENCH,
    XC_LANGUAGE_ITALIAN,        XC_LANGUAGE_PORTUGUESE, XC_LANGUAGE_PORTUGUESE,
    XC_LANGUAGE_JAPANESE,       XC_LANGUAGE_KOREAN,     XC_LANGUAGE_TCHINESE,
    XC_LANGUAGE_SCHINESE,       XC_LANGUAGE_DANISH,     XC_LANGUAGE_FINISH,
    XC_LANGUAGE_DUTCH,          XC_LANGUAGE_POLISH,     XC_LANGUAGE_RUSSIAN,
    XC_LANGUAGE_SWEDISH,        XC_LANGUAGE_BNORWEGIAN, XC_LANGUAGE_SLOVAK,
    XC_LANGUAGE_CZECH,          XC_LANGUAGE_GREEK,      XC_LANGUAGE_TURKISH,
};

const int uiLocaleMap[UIScene_LanguageSelector::eLanguageSelector_MAX] = {
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, XC_LOCALE_SPAIN,
    XC_LOCALE_LATIN_AMERICA,    MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, XC_LOCALE_PORTUGAL,
    XC_LOCALE_BRAZIL,           MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
    MINECRAFT_LANGUAGE_DEFAULT, MINECRAFT_LANGUAGE_DEFAULT,
};