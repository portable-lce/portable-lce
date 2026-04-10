#include "app/common/LocalizationManager.h"

#include <assert.h>
#include <stdlib.h>
#include <wchar.h>

#include <string>

#include "app/common/App_structs.h"
#include "app/common/Game.h"
#include "app/common/UI/All Platforms/ArchiveFile.h"
#include "java/Random.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/locale/StringTable.h"
#include "platform/XboxStubs.h"
#include "platform/input/input.h"
#include "platform/renderer/renderer.h"
#include "strings.h"
#include "util/StringHelpers.h"

int LocalizationManager::s_iHTMLFontSizesA[eHTMLSize_COUNT] = {20, 13, 20, 26};

TIPSTRUCT LocalizationManager::m_GameTipA[MAX_TIPS_GAMETIP] = {
    {0, IDS_TIPS_GAMETIP_1},  {0, IDS_TIPS_GAMETIP_2},
    {0, IDS_TIPS_GAMETIP_3},  {0, IDS_TIPS_GAMETIP_4},
    {0, IDS_TIPS_GAMETIP_5},  {0, IDS_TIPS_GAMETIP_6},
    {0, IDS_TIPS_GAMETIP_7},  {0, IDS_TIPS_GAMETIP_8},
    {0, IDS_TIPS_GAMETIP_9},  {0, IDS_TIPS_GAMETIP_10},
    {0, IDS_TIPS_GAMETIP_11}, {0, IDS_TIPS_GAMETIP_12},
    {0, IDS_TIPS_GAMETIP_13}, {0, IDS_TIPS_GAMETIP_14},
    {0, IDS_TIPS_GAMETIP_15}, {0, IDS_TIPS_GAMETIP_16},
    {0, IDS_TIPS_GAMETIP_17}, {0, IDS_TIPS_GAMETIP_18},
    {0, IDS_TIPS_GAMETIP_19}, {0, IDS_TIPS_GAMETIP_20},
    {0, IDS_TIPS_GAMETIP_21}, {0, IDS_TIPS_GAMETIP_22},
    {0, IDS_TIPS_GAMETIP_23}, {0, IDS_TIPS_GAMETIP_24},
    {0, IDS_TIPS_GAMETIP_25}, {0, IDS_TIPS_GAMETIP_26},
    {0, IDS_TIPS_GAMETIP_27}, {0, IDS_TIPS_GAMETIP_28},
    {0, IDS_TIPS_GAMETIP_29}, {0, IDS_TIPS_GAMETIP_30},
    {0, IDS_TIPS_GAMETIP_31}, {0, IDS_TIPS_GAMETIP_32},
    {0, IDS_TIPS_GAMETIP_33}, {0, IDS_TIPS_GAMETIP_34},
    {0, IDS_TIPS_GAMETIP_35}, {0, IDS_TIPS_GAMETIP_36},
    {0, IDS_TIPS_GAMETIP_37}, {0, IDS_TIPS_GAMETIP_38},
    {0, IDS_TIPS_GAMETIP_39}, {0, IDS_TIPS_GAMETIP_40},
    {0, IDS_TIPS_GAMETIP_41}, {0, IDS_TIPS_GAMETIP_42},
    {0, IDS_TIPS_GAMETIP_43}, {0, IDS_TIPS_GAMETIP_44},
    {0, IDS_TIPS_GAMETIP_45}, {0, IDS_TIPS_GAMETIP_46},
    {0, IDS_TIPS_GAMETIP_47}, {0, IDS_TIPS_GAMETIP_48},
    {0, IDS_TIPS_GAMETIP_49}, {0, IDS_TIPS_GAMETIP_50},
};

TIPSTRUCT LocalizationManager::m_TriviaTipA[MAX_TIPS_TRIVIATIP] = {
    {0, IDS_TIPS_TRIVIA_1},  {0, IDS_TIPS_TRIVIA_2},  {0, IDS_TIPS_TRIVIA_3},
    {0, IDS_TIPS_TRIVIA_4},  {0, IDS_TIPS_TRIVIA_5},  {0, IDS_TIPS_TRIVIA_6},
    {0, IDS_TIPS_TRIVIA_7},  {0, IDS_TIPS_TRIVIA_8},  {0, IDS_TIPS_TRIVIA_9},
    {0, IDS_TIPS_TRIVIA_10}, {0, IDS_TIPS_TRIVIA_11}, {0, IDS_TIPS_TRIVIA_12},
    {0, IDS_TIPS_TRIVIA_13}, {0, IDS_TIPS_TRIVIA_14}, {0, IDS_TIPS_TRIVIA_15},
    {0, IDS_TIPS_TRIVIA_16}, {0, IDS_TIPS_TRIVIA_17}, {0, IDS_TIPS_TRIVIA_18},
    {0, IDS_TIPS_TRIVIA_19}, {0, IDS_TIPS_TRIVIA_20},
};

Random* LocalizationManager::TipRandom = new Random();

LocalizationManager::LocalizationManager()
    : m_stringTable(nullptr), m_uiCurrentTip(0) {
    memset(m_TipIDA, 0, sizeof(m_TipIDA));
}

void LocalizationManager::loadStringTable(ArchiveFile* mediaArchive) {
    if (m_stringTable != nullptr) {
        // we need to unload the current std::string table, this is a reload
        delete m_stringTable;
    }
    std::string localisationFile = "languages.loc";
    if (mediaArchive->hasFile(localisationFile)) {
        std::vector<uint8_t> locFile = mediaArchive->getFile(localisationFile);
        std::vector<std::string> locales;
        getLocale(locales);
        m_stringTable = new StringTable(locFile, locales);
    } else {
        m_stringTable = nullptr;
        assert(false);
        // AHHHHHHHHH.
    }
}

const char* LocalizationManager::getString(int iID) const {
    return m_stringTable->getString(iID);
}

int LocalizationManager::TipsSortFunction(const void* a, const void* b) {
    int s1 = ((TIPSTRUCT*)a)->iSortValue;
    int s2 = ((TIPSTRUCT*)b)->iSortValue;

    if (s1 > s2) {
        return 1;
    } else if (s1 == s2) {
        return 0;
    }

    return -1;
}

void LocalizationManager::initialiseTips() {
    memset(m_TipIDA, 0, sizeof(m_TipIDA));

    if (!PlatformRenderer.IsHiDef()) {
        m_GameTipA[0].uiStringID = IDS_TIPS_GAMETIP_0;
    }

#if defined(_CONTENT_PACKAGE)
    for (int i = 1; i < MAX_TIPS_GAMETIP; i++) {
        m_GameTipA[i].iSortValue = TipRandom->nextInt();
    }
    qsort(&m_GameTipA[1], MAX_TIPS_GAMETIP - 1, sizeof(TIPSTRUCT),
          TipsSortFunction);
#endif

    for (int i = 0; i < MAX_TIPS_TRIVIATIP; i++) {
        m_TriviaTipA[i].iSortValue = TipRandom->nextInt();
    }
    qsort(m_TriviaTipA, MAX_TIPS_TRIVIATIP, sizeof(TIPSTRUCT),
          TipsSortFunction);

    int iCurrentGameTip = 0;
    int iCurrentTriviaTip = 0;

    for (int i = 0; i < MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP; i++) {
        if ((i % 3 == 2) && (iCurrentTriviaTip < MAX_TIPS_TRIVIATIP)) {
            m_TipIDA[i] = m_TriviaTipA[iCurrentTriviaTip++].uiStringID;
        } else {
            if (iCurrentGameTip < MAX_TIPS_GAMETIP) {
                m_TipIDA[i] = m_GameTipA[iCurrentGameTip++].uiStringID;
            } else {
                m_TipIDA[i] = m_TriviaTipA[iCurrentTriviaTip++].uiStringID;
            }
        }

        if (m_TipIDA[i] == 0) {
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
        }
    }

    m_uiCurrentTip = 0;
}

int LocalizationManager::getNextTip() {
    static bool bShowSkinDLCTip = true;
    if (app.GetNewDLCAvailable() && app.DisplayNewDLCTip()) {
        return IDS_TIPS_GAMETIP_NEWDLC;
    } else {
        if (bShowSkinDLCTip) {
            bShowSkinDLCTip = false;
            if (app.DLCInstallProcessCompleted()) {
                if (app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin) ==
                    0) {
                    return IDS_TIPS_GAMETIP_SKINPACKS;
                }
            } else {
                return IDS_TIPS_GAMETIP_SKINPACKS;
            }
        }
    }

    if (m_uiCurrentTip == MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP)
        m_uiCurrentTip = 0;

    return m_TipIDA[m_uiCurrentTip++];
}

int LocalizationManager::getHTMLColour(eMinecraftColour colour) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    return pMinecraft->skins->getSelected()->getColourTable()->getColour(
        colour);
}

int LocalizationManager::getHTMLFontSize(EHTMLFontSize size) {
    return s_iHTMLFontSizesA[size];
}

std::string LocalizationManager::formatHTMLString(
    int iPad, const std::string& desc, int shadowColour /*= 0xFFFFFFFF*/) {
    std::string text(desc);

    char replacements[64];
    text = replaceAll(text, "{*B*}", "<br />");
    snprintf(replacements, 64, "<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_T1));
    text = replaceAll(text, "{*T1*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_T2));
    text = replaceAll(text, "{*T2*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_T3));
    text = replaceAll(text, "{*T3*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_Black));
    text = replaceAll(text, "{*ETB*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_White));
    text = replaceAll(text, "{*ETW*}", replacements);
    text = replaceAll(text, "{*EF*}", "</font>");

    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_0), shadowColour);
    text = replaceAll(text, "{*C0*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_1), shadowColour);
    text = replaceAll(text, "{*C1*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_2), shadowColour);
    text = replaceAll(text, "{*C2*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_3), shadowColour);
    text = replaceAll(text, "{*C3*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_4), shadowColour);
    text = replaceAll(text, "{*C4*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_5), shadowColour);
    text = replaceAll(text, "{*C5*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_6), shadowColour);
    text = replaceAll(text, "{*C6*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_7), shadowColour);
    text = replaceAll(text, "{*C7*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_8), shadowColour);
    text = replaceAll(text, "{*C8*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_9), shadowColour);
    text = replaceAll(text, "{*C9*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_a), shadowColour);
    text = replaceAll(text, "{*CA*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_b), shadowColour);
    text = replaceAll(text, "{*CB*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_c), shadowColour);
    text = replaceAll(text, "{*CC*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_d), shadowColour);
    text = replaceAll(text, "{*CD*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_e), shadowColour);
    text = replaceAll(text, "{*CE*}", replacements);
    snprintf(replacements, 64, "<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_f), shadowColour);
    text = replaceAll(text, "{*CF*}", replacements);

    // Swap for southpaw.
    if (app.GetGameSettings(iPad, eGameSetting_ControlSouthPaw)) {
        text =
            replaceAll(text, "{*CONTROLLER_ACTION_MOVE*}",
                       getActionReplacement(iPad, MINECRAFT_ACTION_LOOK_RIGHT));
        text = replaceAll(text, "{*CONTROLLER_ACTION_LOOK*}",
                          getActionReplacement(iPad, MINECRAFT_ACTION_RIGHT));

        text = replaceAll(text, "{*CONTROLLER_MENU_NAVIGATE*}",
                          getVKReplacement(VK_PAD_RTHUMB_LEFT));
    } else {
        text = replaceAll(text, "{*CONTROLLER_ACTION_MOVE*}",
                          getActionReplacement(iPad, MINECRAFT_ACTION_RIGHT));
        text =
            replaceAll(text, "{*CONTROLLER_ACTION_LOOK*}",
                       getActionReplacement(iPad, MINECRAFT_ACTION_LOOK_RIGHT));

        text = replaceAll(text, "{*CONTROLLER_MENU_NAVIGATE*}",
                          getVKReplacement(VK_PAD_LTHUMB_LEFT));
    }

    text = replaceAll(text, "{*CONTROLLER_ACTION_JUMP*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_JUMP));
    text =
        replaceAll(text, "{*CONTROLLER_ACTION_SNEAK*}",
                   getActionReplacement(iPad, MINECRAFT_ACTION_SNEAK_TOGGLE));
    text = replaceAll(text, "{*CONTROLLER_ACTION_USE*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_USE));
    text = replaceAll(text, "{*CONTROLLER_ACTION_ACTION*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_ACTION));
    text = replaceAll(text, "{*CONTROLLER_ACTION_LEFT_SCROLL*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_LEFT_SCROLL));
    text =
        replaceAll(text, "{*CONTROLLER_ACTION_RIGHT_SCROLL*}",
                   getActionReplacement(iPad, MINECRAFT_ACTION_RIGHT_SCROLL));
    text = replaceAll(text, "{*CONTROLLER_ACTION_INVENTORY*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_INVENTORY));
    text = replaceAll(text, "{*CONTROLLER_ACTION_CRAFTING*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_CRAFTING));
    text = replaceAll(text, "{*CONTROLLER_ACTION_DROP*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DROP));
    text = replaceAll(
        text, "{*CONTROLLER_ACTION_CAMERA*}",
        getActionReplacement(iPad, MINECRAFT_ACTION_RENDER_THIRD_PERSON));
    text = replaceAll(text, "{*CONTROLLER_ACTION_MENU_PAGEDOWN*}",
                      getActionReplacement(iPad, ACTION_MENU_PAGEDOWN));
    text =
        replaceAll(text, "{*CONTROLLER_ACTION_DISMOUNT*}",
                   getActionReplacement(iPad, MINECRAFT_ACTION_SNEAK_TOGGLE));
    text = replaceAll(text, "{*CONTROLLER_VK_A*}", getVKReplacement(VK_PAD_A));
    text = replaceAll(text, "{*CONTROLLER_VK_B*}", getVKReplacement(VK_PAD_B));
    text = replaceAll(text, "{*CONTROLLER_VK_X*}", getVKReplacement(VK_PAD_X));
    text = replaceAll(text, "{*CONTROLLER_VK_Y*}", getVKReplacement(VK_PAD_Y));
    text = replaceAll(text, "{*CONTROLLER_VK_LB*}",
                      getVKReplacement(VK_PAD_LSHOULDER));
    text = replaceAll(text, "{*CONTROLLER_VK_RB*}",
                      getVKReplacement(VK_PAD_RSHOULDER));
    text = replaceAll(text, "{*CONTROLLER_VK_LS*}",
                      getVKReplacement(VK_PAD_LTHUMB_UP));
    text = replaceAll(text, "{*CONTROLLER_VK_RS*}",
                      getVKReplacement(VK_PAD_RTHUMB_UP));
    text = replaceAll(text, "{*CONTROLLER_VK_LT*}",
                      getVKReplacement(VK_PAD_LTRIGGER));
    text = replaceAll(text, "{*CONTROLLER_VK_RT*}",
                      getVKReplacement(VK_PAD_RTRIGGER));
    text = replaceAll(text, "{*ICON_SHANK_01*}",
                      getIconReplacement(XZP_ICON_SHANK_01));
    text = replaceAll(text, "{*ICON_SHANK_03*}",
                      getIconReplacement(XZP_ICON_SHANK_03));
    text = replaceAll(text, "{*CONTROLLER_ACTION_DPAD_UP*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_UP));
    text = replaceAll(text, "{*CONTROLLER_ACTION_DPAD_DOWN*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_DOWN));
    text = replaceAll(text, "{*CONTROLLER_ACTION_DPAD_RIGHT*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_RIGHT));
    text = replaceAll(text, "{*CONTROLLER_ACTION_DPAD_LEFT*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_LEFT));

    std::uint32_t dwLanguage = XGetLanguage();
    switch (dwLanguage) {
        case XC_LANGUAGE_KOREAN:
        case XC_LANGUAGE_JAPANESE:
        case XC_LANGUAGE_TCHINESE:
            text = replaceAll(text, "&nbsp;", "");
            break;
    }

    return text;
}

std::string LocalizationManager::getActionReplacement(int iPad,
                                                      unsigned char ucAction) {
    unsigned int input = PlatformInput.GetGameJoypadMaps(
        PlatformInput.GetJoypadMapVal(iPad), ucAction);

    std::string replacement = "";

    if (input & _360_JOY_BUTTON_A)
        replacement = "ButtonA";
    else if (input & _360_JOY_BUTTON_B)
        replacement = "ButtonB";
    else if (input & _360_JOY_BUTTON_X)
        replacement = "ButtonX";
    else if (input & _360_JOY_BUTTON_Y)
        replacement = "ButtonY";
    else if ((input & _360_JOY_BUTTON_LSTICK_UP) ||
             (input & _360_JOY_BUTTON_LSTICK_DOWN) ||
             (input & _360_JOY_BUTTON_LSTICK_LEFT) ||
             (input & _360_JOY_BUTTON_LSTICK_RIGHT)) {
        replacement = "ButtonLeftStick";
    } else if ((input & _360_JOY_BUTTON_RSTICK_LEFT) ||
               (input & _360_JOY_BUTTON_RSTICK_RIGHT) ||
               (input & _360_JOY_BUTTON_RSTICK_UP) ||
               (input & _360_JOY_BUTTON_RSTICK_DOWN)) {
        replacement = "ButtonRightStick";
    } else if (input & _360_JOY_BUTTON_DPAD_LEFT)
        replacement = "ButtonDpadL";
    else if (input & _360_JOY_BUTTON_DPAD_RIGHT)
        replacement = "ButtonDpadR";
    else if (input & _360_JOY_BUTTON_DPAD_UP)
        replacement = "ButtonDpadU";
    else if (input & _360_JOY_BUTTON_DPAD_DOWN)
        replacement = "ButtonDpadD";
    else if (input & _360_JOY_BUTTON_LT)
        replacement = "ButtonLeftTrigger";
    else if (input & _360_JOY_BUTTON_RT)
        replacement = "ButtonRightTrigger";
    else if (input & _360_JOY_BUTTON_RB)
        replacement = "ButtonRightBumper";
    else if (input & _360_JOY_BUTTON_LB)
        replacement = "ButtonLeftBumper";
    else if (input & _360_JOY_BUTTON_BACK)
        replacement = "ButtonBack";
    else if (input & _360_JOY_BUTTON_START)
        replacement = "ButtonStart";
    else if (input & _360_JOY_BUTTON_RTHUMB)
        replacement = "ButtonRS";
    else if (input & _360_JOY_BUTTON_LTHUMB)
        replacement = "ButtonLS";

    char string[128];

#if defined(_WIN64)
    int size = 45;
    if (ui.getScreenWidth() < 1920) size = 30;
#else
    int size = 45;
#endif

    snprintf(string, 128,
             "<img src=\"%s\" align=\"middle\" height=\"%d\" width=\"%d\"/>",
             replacement.c_str(), size, size);

    return string;
}

std::string LocalizationManager::getVKReplacement(unsigned int uiVKey) {
    std::string replacement = "";
    switch (uiVKey) {
        case VK_PAD_A:
            replacement = "ButtonA";
            break;
        case VK_PAD_B:
            replacement = "ButtonB";
            break;
        case VK_PAD_X:
            replacement = "ButtonX";
            break;
        case VK_PAD_Y:
            replacement = "ButtonY";
            break;
        case VK_PAD_LSHOULDER:
            replacement = "ButtonLeftBumper";
            break;
        case VK_PAD_RSHOULDER:
            replacement = "ButtonRightBumper";
            break;
        case VK_PAD_LTRIGGER:
            replacement = "ButtonLeftTrigger";
            break;
        case VK_PAD_RTRIGGER:
            replacement = "ButtonRightTrigger";
            break;
        case VK_PAD_LTHUMB_UP:
        case VK_PAD_LTHUMB_DOWN:
        case VK_PAD_LTHUMB_RIGHT:
        case VK_PAD_LTHUMB_LEFT:
        case VK_PAD_LTHUMB_UPLEFT:
        case VK_PAD_LTHUMB_UPRIGHT:
        case VK_PAD_LTHUMB_DOWNRIGHT:
        case VK_PAD_LTHUMB_DOWNLEFT:
            replacement = "ButtonLeftStick";
            break;
        case VK_PAD_RTHUMB_UP:
        case VK_PAD_RTHUMB_DOWN:
        case VK_PAD_RTHUMB_RIGHT:
        case VK_PAD_RTHUMB_LEFT:
        case VK_PAD_RTHUMB_UPLEFT:
        case VK_PAD_RTHUMB_UPRIGHT:
        case VK_PAD_RTHUMB_DOWNRIGHT:
        case VK_PAD_RTHUMB_DOWNLEFT:
            replacement = "ButtonRightStick";
            break;
        default:
            break;
    }
    char string[128];

#if defined(_WIN64)
    int size = 45;
    if (ui.getScreenWidth() < 1920) size = 30;
#else
    int size = 45;
#endif

    snprintf(string, 128,
             "<img src=\"%s\" align=\"middle\" height=\"%d\" width=\"%d\"/>",
             replacement.c_str(), size, size);

    return string;
}

std::string LocalizationManager::getIconReplacement(unsigned int uiIcon) {
    char string[128];

#if defined(_WIN64)
    int size = 33;
    if (ui.getScreenWidth() < 1920) size = 22;
#else
    int size = 33;
#endif

    snprintf(string, 128,
             "<img src=\"Icon_Shank\" align=\"middle\" height=\"%d\" "
             "width=\"%d\"/>",
             size, size);
    std::string result = "";
    switch (uiIcon) {
        case XZP_ICON_SHANK_01:
            result = string;
            break;
        case XZP_ICON_SHANK_03:
            result.append(string).append(string).append(string);
            break;
        default:
            break;
    }
    return result;
}

void LocalizationManager::getLocale(std::vector<std::string>& vecWstrLocales) {
    std::vector<eMCLang> locales;

    const unsigned int systemLanguage = XGetLanguage();

    switch (systemLanguage) {
        case XC_LANGUAGE_ENGLISH:
            switch (XGetLocale()) {
                case XC_LOCALE_AUSTRALIA:
                case XC_LOCALE_CANADA:
                case XC_LOCALE_CZECH_REPUBLIC:
                case XC_LOCALE_GREECE:
                case XC_LOCALE_HONG_KONG:
                case XC_LOCALE_HUNGARY:
                case XC_LOCALE_INDIA:
                case XC_LOCALE_IRELAND:
                case XC_LOCALE_ISRAEL:
                case XC_LOCALE_NEW_ZEALAND:
                case XC_LOCALE_SAUDI_ARABIA:
                case XC_LOCALE_SINGAPORE:
                case XC_LOCALE_SLOVAK_REPUBLIC:
                case XC_LOCALE_SOUTH_AFRICA:
                case XC_LOCALE_UNITED_ARAB_EMIRATES:
                case XC_LOCALE_GREAT_BRITAIN:
                    locales.push_back(eMCLang_enGB);
                    break;
                default:
                    break;
            }
            break;
        case XC_LANGUAGE_JAPANESE:
            locales.push_back(eMCLang_jaJP);
            break;
        case XC_LANGUAGE_GERMAN:
            switch (XGetLocale()) {
                case XC_LOCALE_AUSTRIA:
                    locales.push_back(eMCLang_deAT);
                    break;
                case XC_LOCALE_SWITZERLAND:
                    locales.push_back(eMCLang_deCH);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_deDE);
            break;
        case XC_LANGUAGE_FRENCH:
            switch (XGetLocale()) {
                case XC_LOCALE_BELGIUM:
                    locales.push_back(eMCLang_frBE);
                    break;
                case XC_LOCALE_CANADA:
                    locales.push_back(eMCLang_frCA);
                    break;
                case XC_LOCALE_SWITZERLAND:
                    locales.push_back(eMCLang_frCH);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_frFR);
            break;
        case XC_LANGUAGE_SPANISH:
            switch (XGetLocale()) {
                case XC_LOCALE_MEXICO:
                case XC_LOCALE_ARGENTINA:
                case XC_LOCALE_CHILE:
                case XC_LOCALE_COLOMBIA:
                case XC_LOCALE_UNITED_STATES:
                case XC_LOCALE_LATIN_AMERICA:
                    locales.push_back(eMCLang_laLAS);
                    locales.push_back(eMCLang_esMX);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_esES);
            break;
        case XC_LANGUAGE_ITALIAN:
            locales.push_back(eMCLang_itIT);
            break;
        case XC_LANGUAGE_KOREAN:
            locales.push_back(eMCLang_koKR);
            break;
        case XC_LANGUAGE_TCHINESE:
            switch (XGetLocale()) {
                case XC_LOCALE_HONG_KONG:
                    locales.push_back(eMCLang_zhHK);
                    locales.push_back(eMCLang_zhTW);
                    break;
                case XC_LOCALE_TAIWAN:
                    locales.push_back(eMCLang_zhTW);
                    locales.push_back(eMCLang_zhHK);
                default:
                    break;
            }
            locales.push_back(eMCLang_hant);
            locales.push_back(eMCLang_zhCHT);
            break;
        case XC_LANGUAGE_PORTUGUESE:
            if (XGetLocale() == XC_LOCALE_BRAZIL) {
                locales.push_back(eMCLang_ptBR);
            }
            locales.push_back(eMCLang_ptPT);
            break;
        case XC_LANGUAGE_POLISH:
            locales.push_back(eMCLang_plPL);
            break;
        case XC_LANGUAGE_RUSSIAN:
            locales.push_back(eMCLang_ruRU);
            break;
        case XC_LANGUAGE_SWEDISH:
            locales.push_back(eMCLang_svSV);
            locales.push_back(eMCLang_svSE);
            break;
        case XC_LANGUAGE_TURKISH:
            locales.push_back(eMCLang_trTR);
            break;
        case XC_LANGUAGE_BNORWEGIAN:
            locales.push_back(eMCLang_nbNO);
            locales.push_back(eMCLang_noNO);
            locales.push_back(eMCLang_nnNO);
            break;
        case XC_LANGUAGE_DUTCH:
            switch (XGetLocale()) {
                case XC_LOCALE_BELGIUM:
                    locales.push_back(eMCLang_nlBE);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_nlNL);
            break;
        case XC_LANGUAGE_SCHINESE:
            switch (XGetLocale()) {
                case XC_LOCALE_SINGAPORE:
                    locales.push_back(eMCLang_zhSG);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_hans);
            locales.push_back(eMCLang_csCS);
            locales.push_back(eMCLang_zhCN);
            break;
    }

    locales.push_back(eMCLang_enUS);
    locales.push_back(eMCLang_null);

    for (int i = 0; i < locales.size(); i++) {
        eMCLang lang = locales.at(i);
        vecWstrLocales.push_back(m_localeA[lang]);
    }
}

int LocalizationManager::get_eMCLang(char* pwchLocale) {
    return m_eMCLangA[pwchLocale];
}

int LocalizationManager::get_xcLang(char* pwchLocale) {
    return m_xcLangA[pwchLocale];
}

void LocalizationManager::localeAndLanguageInit() {
    m_localeA[eMCLang_zhCHT] = "zh-CHT";
    m_localeA[eMCLang_csCS] = "cs-CS";
    m_localeA[eMCLang_laLAS] = "la-LAS";
    m_localeA[eMCLang_null] = "en-EN";
    m_localeA[eMCLang_enUS] = "en-US";
    m_localeA[eMCLang_enGB] = "en-GB";
    m_localeA[eMCLang_enIE] = "en-IE";
    m_localeA[eMCLang_enAU] = "en-AU";
    m_localeA[eMCLang_enNZ] = "en-NZ";
    m_localeA[eMCLang_enCA] = "en-CA";
    m_localeA[eMCLang_jaJP] = "ja-JP";
    m_localeA[eMCLang_deDE] = "de-DE";
    m_localeA[eMCLang_deAT] = "de-AT";
    m_localeA[eMCLang_frFR] = "fr-FR";
    m_localeA[eMCLang_frCA] = "fr-CA";
    m_localeA[eMCLang_esES] = "es-ES";
    m_localeA[eMCLang_esMX] = "es-MX";
    m_localeA[eMCLang_itIT] = "it-IT";
    m_localeA[eMCLang_koKR] = "ko-KR";
    m_localeA[eMCLang_ptPT] = "pt-PT";
    m_localeA[eMCLang_ptBR] = "pt-BR";
    m_localeA[eMCLang_ruRU] = "ru-RU";
    m_localeA[eMCLang_nlNL] = "nl-NL";
    m_localeA[eMCLang_fiFI] = "fi-FI";
    m_localeA[eMCLang_svSV] = "sv-SV";
    m_localeA[eMCLang_daDA] = "da-DA";
    m_localeA[eMCLang_noNO] = "no-NO";
    m_localeA[eMCLang_plPL] = "pl-PL";
    m_localeA[eMCLang_trTR] = "tr-TR";
    m_localeA[eMCLang_elEL] = "el-EL";
    m_localeA[eMCLang_zhSG] = "zh-SG";
    m_localeA[eMCLang_zhCN] = "zh-CN";
    m_localeA[eMCLang_zhHK] = "zh-HK";
    m_localeA[eMCLang_zhTW] = "zh-TW";
    m_localeA[eMCLang_nlBE] = "nl-BE";
    m_localeA[eMCLang_daDK] = "da-DK";
    m_localeA[eMCLang_frBE] = "fr-BE";
    m_localeA[eMCLang_frCH] = "fr-CH";
    m_localeA[eMCLang_deCH] = "de-CH";
    m_localeA[eMCLang_nbNO] = "nb-NO";
    m_localeA[eMCLang_enGR] = "en-GR";
    m_localeA[eMCLang_enHK] = "en-HK";
    m_localeA[eMCLang_enSA] = "en-SA";
    m_localeA[eMCLang_enHU] = "en-HU";
    m_localeA[eMCLang_enIN] = "en-IN";
    m_localeA[eMCLang_enIL] = "en-IL";
    m_localeA[eMCLang_enSG] = "en-SG";
    m_localeA[eMCLang_enSK] = "en-SK";
    m_localeA[eMCLang_enZA] = "en-ZA";
    m_localeA[eMCLang_enCZ] = "en-CZ";
    m_localeA[eMCLang_enAE] = "en-AE";
    m_localeA[eMCLang_esAR] = "es-AR";
    m_localeA[eMCLang_esCL] = "es-CL";
    m_localeA[eMCLang_esCO] = "es-CO";
    m_localeA[eMCLang_esUS] = "es-US";
    m_localeA[eMCLang_svSE] = "sv-SE";
    m_localeA[eMCLang_csCZ] = "cs-CZ";
    m_localeA[eMCLang_elGR] = "el-GR";
    m_localeA[eMCLang_nnNO] = "nn-NO";
    m_localeA[eMCLang_skSK] = "sk-SK";
    m_localeA[eMCLang_hans] = "zh-HANS";
    m_localeA[eMCLang_hant] = "zh-HANT";

    m_eMCLangA["zh-CHT"] = eMCLang_zhCHT;
    m_eMCLangA["cs-CS"] = eMCLang_csCS;
    m_eMCLangA["la-LAS"] = eMCLang_laLAS;
    m_eMCLangA["en-EN"] = eMCLang_null;
    m_eMCLangA["en-US"] = eMCLang_enUS;
    m_eMCLangA["en-GB"] = eMCLang_enGB;
    m_eMCLangA["en-IE"] = eMCLang_enIE;
    m_eMCLangA["en-AU"] = eMCLang_enAU;
    m_eMCLangA["en-NZ"] = eMCLang_enNZ;
    m_eMCLangA["en-CA"] = eMCLang_enCA;
    m_eMCLangA["ja-JP"] = eMCLang_jaJP;
    m_eMCLangA["de-DE"] = eMCLang_deDE;
    m_eMCLangA["de-AT"] = eMCLang_deAT;
    m_eMCLangA["fr-FR"] = eMCLang_frFR;
    m_eMCLangA["fr-CA"] = eMCLang_frCA;
    m_eMCLangA["es-ES"] = eMCLang_esES;
    m_eMCLangA["es-MX"] = eMCLang_esMX;
    m_eMCLangA["it-IT"] = eMCLang_itIT;
    m_eMCLangA["ko-KR"] = eMCLang_koKR;
    m_eMCLangA["pt-PT"] = eMCLang_ptPT;
    m_eMCLangA["pt-BR"] = eMCLang_ptBR;
    m_eMCLangA["ru-RU"] = eMCLang_ruRU;
    m_eMCLangA["nl-NL"] = eMCLang_nlNL;
    m_eMCLangA["fi-FI"] = eMCLang_fiFI;
    m_eMCLangA["sv-SV"] = eMCLang_svSV;
    m_eMCLangA["da-DA"] = eMCLang_daDA;
    m_eMCLangA["no-NO"] = eMCLang_noNO;
    m_eMCLangA["pl-PL"] = eMCLang_plPL;
    m_eMCLangA["tr-TR"] = eMCLang_trTR;
    m_eMCLangA["el-EL"] = eMCLang_elEL;
    m_eMCLangA["zh-SG"] = eMCLang_zhSG;
    m_eMCLangA["zh-CN"] = eMCLang_zhCN;
    m_eMCLangA["zh-HK"] = eMCLang_zhHK;
    m_eMCLangA["zh-TW"] = eMCLang_zhTW;
    m_eMCLangA["nl-BE"] = eMCLang_nlBE;
    m_eMCLangA["da-DK"] = eMCLang_daDK;
    m_eMCLangA["fr-BE"] = eMCLang_frBE;
    m_eMCLangA["fr-CH"] = eMCLang_frCH;
    m_eMCLangA["de-CH"] = eMCLang_deCH;
    m_eMCLangA["nb-NO"] = eMCLang_nbNO;
    m_eMCLangA["en-GR"] = eMCLang_enGR;
    m_eMCLangA["en-HK"] = eMCLang_enHK;
    m_eMCLangA["en-SA"] = eMCLang_enSA;
    m_eMCLangA["en-HU"] = eMCLang_enHU;
    m_eMCLangA["en-IN"] = eMCLang_enIN;
    m_eMCLangA["en-IL"] = eMCLang_enIL;
    m_eMCLangA["en-SG"] = eMCLang_enSG;
    m_eMCLangA["en-SK"] = eMCLang_enSK;
    m_eMCLangA["en-ZA"] = eMCLang_enZA;
    m_eMCLangA["en-CZ"] = eMCLang_enCZ;
    m_eMCLangA["en-AE"] = eMCLang_enAE;
    m_eMCLangA["es-AR"] = eMCLang_esAR;
    m_eMCLangA["es-CL"] = eMCLang_esCL;
    m_eMCLangA["es-CO"] = eMCLang_esCO;
    m_eMCLangA["es-US"] = eMCLang_esUS;
    m_eMCLangA["sv-SE"] = eMCLang_svSE;
    m_eMCLangA["cs-CZ"] = eMCLang_csCZ;
    m_eMCLangA["el-GR"] = eMCLang_elGR;
    m_eMCLangA["nn-NO"] = eMCLang_nnNO;
    m_eMCLangA["sk-SK"] = eMCLang_skSK;
    m_eMCLangA["zh-HANS"] = eMCLang_hans;
    m_eMCLangA["zh-HANT"] = eMCLang_hant;

    m_xcLangA["zh-CHT"] = XC_LOCALE_CHINA;
    m_xcLangA["cs-CS"] = XC_LOCALE_CHINA;
    m_xcLangA["en-EN"] = XC_LOCALE_UNITED_STATES;
    m_xcLangA["en-US"] = XC_LOCALE_UNITED_STATES;
    m_xcLangA["en-GB"] = XC_LOCALE_GREAT_BRITAIN;
    m_xcLangA["en-IE"] = XC_LOCALE_IRELAND;
    m_xcLangA["en-AU"] = XC_LOCALE_AUSTRALIA;
    m_xcLangA["en-NZ"] = XC_LOCALE_NEW_ZEALAND;
    m_xcLangA["en-CA"] = XC_LOCALE_CANADA;
    m_xcLangA["ja-JP"] = XC_LOCALE_JAPAN;
    m_xcLangA["de-DE"] = XC_LOCALE_GERMANY;
    m_xcLangA["de-AT"] = XC_LOCALE_AUSTRIA;
    m_xcLangA["fr-FR"] = XC_LOCALE_FRANCE;
    m_xcLangA["fr-CA"] = XC_LOCALE_CANADA;
    m_xcLangA["es-ES"] = XC_LOCALE_SPAIN;
    m_xcLangA["es-MX"] = XC_LOCALE_MEXICO;
    m_xcLangA["it-IT"] = XC_LOCALE_ITALY;
    m_xcLangA["ko-KR"] = XC_LOCALE_KOREA;
    m_xcLangA["pt-PT"] = XC_LOCALE_PORTUGAL;
    m_xcLangA["pt-BR"] = XC_LOCALE_BRAZIL;
    m_xcLangA["ru-RU"] = XC_LOCALE_RUSSIAN_FEDERATION;
    m_xcLangA["nl-NL"] = XC_LOCALE_NETHERLANDS;
    m_xcLangA["fi-FI"] = XC_LOCALE_FINLAND;
    m_xcLangA["sv-SV"] = XC_LOCALE_SWEDEN;
    m_xcLangA["da-DA"] = XC_LOCALE_DENMARK;
    m_xcLangA["no-NO"] = XC_LOCALE_NORWAY;
    m_xcLangA["pl-PL"] = XC_LOCALE_POLAND;
    m_xcLangA["tr-TR"] = XC_LOCALE_TURKEY;
    m_xcLangA["el-EL"] = XC_LOCALE_GREECE;
    m_xcLangA["la-LAS"] = XC_LOCALE_LATIN_AMERICA;
    m_xcLangA["zh-SG"] = XC_LOCALE_SINGAPORE;
    m_xcLangA["Zh-CN"] = XC_LOCALE_CHINA;
    m_xcLangA["zh-HK"] = XC_LOCALE_HONG_KONG;
    m_xcLangA["zh-TW"] = XC_LOCALE_TAIWAN;
    m_xcLangA["nl-BE"] = XC_LOCALE_BELGIUM;
    m_xcLangA["da-DK"] = XC_LOCALE_DENMARK;
    m_xcLangA["fr-BE"] = XC_LOCALE_BELGIUM;
    m_xcLangA["fr-CH"] = XC_LOCALE_SWITZERLAND;
    m_xcLangA["de-CH"] = XC_LOCALE_SWITZERLAND;
    m_xcLangA["nb-NO"] = XC_LOCALE_NORWAY;
    m_xcLangA["en-GR"] = XC_LOCALE_GREECE;
    m_xcLangA["en-HK"] = XC_LOCALE_HONG_KONG;
    m_xcLangA["en-SA"] = XC_LOCALE_SAUDI_ARABIA;
    m_xcLangA["en-HU"] = XC_LOCALE_HUNGARY;
    m_xcLangA["en-IN"] = XC_LOCALE_INDIA;
    m_xcLangA["en-IL"] = XC_LOCALE_ISRAEL;
    m_xcLangA["en-SG"] = XC_LOCALE_SINGAPORE;
    m_xcLangA["en-SK"] = XC_LOCALE_SLOVAK_REPUBLIC;
    m_xcLangA["en-ZA"] = XC_LOCALE_SOUTH_AFRICA;
    m_xcLangA["en-CZ"] = XC_LOCALE_CZECH_REPUBLIC;
    m_xcLangA["en-AE"] = XC_LOCALE_UNITED_ARAB_EMIRATES;
    m_xcLangA["ja-IP"] = XC_LOCALE_JAPAN;
    m_xcLangA["es-AR"] = XC_LOCALE_ARGENTINA;
    m_xcLangA["es-CL"] = XC_LOCALE_CHILE;
    m_xcLangA["es-CO"] = XC_LOCALE_COLOMBIA;
    m_xcLangA["es-US"] = XC_LOCALE_UNITED_STATES;
    m_xcLangA["sv-SE"] = XC_LOCALE_SWEDEN;
    m_xcLangA["cs-CZ"] = XC_LOCALE_CZECH_REPUBLIC;
    m_xcLangA["el-GR"] = XC_LOCALE_GREECE;
    m_xcLangA["sk-SK"] = XC_LOCALE_SLOVAK_REPUBLIC;
    m_xcLangA["zh-HANS"] = XC_LOCALE_CHINA;
    m_xcLangA["zh-HANT"] = XC_LOCALE_CHINA;
}
