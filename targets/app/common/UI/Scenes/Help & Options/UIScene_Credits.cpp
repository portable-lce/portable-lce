
#include "UIScene_Credits.h"

#include <string.h>
#include <wchar.h>

#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "strings.h"
#include "util/StringHelpers.h"

#define CREDIT_ICON -2

SCreditTextItemDef UIScene_Credits::gs_aCreditDefs[MAX_CREDIT_STRINGS] = {
    {"MOJANG", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eExtraLargeText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"%s", IDS_CREDITS_ORIGINALDESIGN, NO_TRANSLATED_STRING, eLargeText},
    {"Markus Persson", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"%s", IDS_CREDITS_PMPROD, NO_TRANSLATED_STRING, eLargeText},
    {"Daniel Kaplan", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"%s", IDS_CREDITS_RESTOFMOJANG, NO_TRANSLATED_STRING, eMediumText},
    {"%s", IDS_CREDITS_LEADPC, NO_TRANSLATED_STRING, eLargeText},
    {"Jens Bergensten", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_JON_KAGSTROM, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_CEO, NO_TRANSLATED_STRING, eLargeText},
    {"Carl Manneh", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_DOF, NO_TRANSLATED_STRING, eLargeText},
    {"Lydia Winters", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_WCW, NO_TRANSLATED_STRING, eLargeText},
    {"Karin Severinsson", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"%s", IDS_CREDITS_CUSTOMERSUPPORT, NO_TRANSLATED_STRING, eLargeText},
    {"Marc Watson", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"%s", IDS_CREDITS_DESPROG, NO_TRANSLATED_STRING, eLargeText},
    {"Aron Nieminen", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"%s", IDS_CREDITS_CHIEFARCHITECT, NO_TRANSLATED_STRING, eLargeText},
    {"Daniel Frisk", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_CODENINJA, NO_TRANSLATED_STRING, eLargeText},
    {"%s", IDS_CREDITS_TOBIAS_MOLLSTAM, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_OFFICEDJ, NO_TRANSLATED_STRING, eLargeText},
    {"Kristoffer Jelbring", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"%s", IDS_CREDITS_DEVELOPER, NO_TRANSLATED_STRING, eLargeText},
    {"Leonard Axelsson", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"%s", IDS_CREDITS_BULLYCOORD, NO_TRANSLATED_STRING, eLargeText},
    {"Jakob Porser", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_ARTDEVELOPER, NO_TRANSLATED_STRING, eLargeText},
    {"Junkboy", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_EXPLODANIM, NO_TRANSLATED_STRING, eLargeText},
    {"Mattis Grahm", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_CONCEPTART, NO_TRANSLATED_STRING, eLargeText},
    {"Henrik Petterson", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"%s", IDS_CREDITS_CRUNCHER, NO_TRANSLATED_STRING, eLargeText},
    {"Patrick Geuder", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"%s", IDS_CREDITS_MUSICANDSOUNDS, NO_TRANSLATED_STRING, eLargeText},
    {"Daniel Rosenfeld (C418)", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line

    // Added credit for horses
    {"Developers of Mo' Creatures:", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eExtraLargeText},
    {"John Olarte (DrZhark)", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"Kent Christian Jensen", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"Dan Roque", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line

    {"4J Studios", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eExtraLargeText},
    {"%s", IDS_CREDITS_PROGRAMMING, NO_TRANSLATED_STRING, eLargeText},
    {"Paddy Burns", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Richard Reavy", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Stuart Ross", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"James Vaughan", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Mark Hughes", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Harry Gordon", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Thomas Kronberg", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},

    {"%s", IDS_CREDITS_ART, NO_TRANSLATED_STRING, eLargeText},
    {"David Keningale", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Alan Redmond", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Chris Reeves", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Kate Wright", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Michael Hansen", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Donald Robertson", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"Jamie Keddie", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Thomas Naylor", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Brian Lindsay", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Hannah Watts", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Rebecca O'Neil", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},

    {"%s", IDS_CREDITS_QA, NO_TRANSLATED_STRING, eLargeText},
    {"Steven Gary Woodward", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"George Vaughan", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"%s", IDS_CREDITS_SPECIALTHANKS, NO_TRANSLATED_STRING, eLargeText},
    {"Chris van der Kuyl", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},
    {"Roni Percy", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Anne Clarke", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},
    {"Anthony Kent", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING, eSmallText},

    // Miles & Iggy credits
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},                                     // extra blank line
    {"", CREDIT_ICON, eCreditIcon_Iggy, eSmallText},  // extra blank line
    {"Uses Iggy.", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"Copyright (C) 2009-2014 by RAD Game Tools, Inc.", NO_TRANSLATED_STRING,
     NO_TRANSLATED_STRING, eSmallText},  // extra blank line
    {"", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},                                      // extra blank line
    {"", CREDIT_ICON, eCreditIcon_Miles, eSmallText},  // extra blank line
    {"Uses Miles Sound System.", NO_TRANSLATED_STRING, NO_TRANSLATED_STRING,
     eSmallText},  // extra blank line
    {"Copyright (C) 1991-2014 by RAD Game Tools, Inc.", NO_TRANSLATED_STRING,
     NO_TRANSLATED_STRING, eSmallText},  // extra blank line
};

UIScene_Credits::UIScene_Credits(int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_bAddNextLabel = false;

    // How many lines of text are in the credits?
    m_iNumTextDefs = MAX_CREDIT_STRINGS;

    // Are there any additional lines needed for the DLC credits?
    m_iNumTextDefs += app.GetDLCCreditsCount();

    m_iCurrDefIndex = -1;

    // Add the first 20 Flash can cope with
    for (unsigned int i = 0; i < 20; ++i) {
        ++m_iCurrDefIndex;

        // Set up the new text element.
        if (gs_aCreditDefs[i].m_iStringID[0] == NO_TRANSLATED_STRING) {
            setNextLabel(gs_aCreditDefs[i].m_Text, gs_aCreditDefs[i].m_eType);
        } else  // using additional translated string.
        {
            char* creditsString = new char[128];
            if (gs_aCreditDefs[i].m_iStringID[1] != NO_TRANSLATED_STRING) {
                snprintf(creditsString, 128, gs_aCreditDefs[i].m_Text,
                         app.GetString(gs_aCreditDefs[i].m_iStringID[0]),
                         app.GetString(gs_aCreditDefs[i].m_iStringID[1]));
            } else {
                snprintf(creditsString, 128, gs_aCreditDefs[i].m_Text,
                         app.GetString(gs_aCreditDefs[i].m_iStringID[0]));
            }
            setNextLabel(creditsString, gs_aCreditDefs[i].m_eType);
            delete[] creditsString;
        }
    }
}

std::string UIScene_Credits::getMoviePath() { return "Credits"; }

void UIScene_Credits::updateTooltips() {
    ui.SetTooltips(m_iPad, -1, IDS_TOOLTIPS_BACK);
}

void UIScene_Credits::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
}

void UIScene_Credits::handleReload() {
    // We don't allow this in splitscreen, so just go back
    navigateBack();
}

void UIScene_Credits::tick() {
    UIScene::tick();

    if (m_bAddNextLabel) {
        m_bAddNextLabel = false;

        const SCreditTextItemDef* pDef;

        // Time to create next text item.
        ++m_iCurrDefIndex;

        // Wrap back to start.
        if (m_iCurrDefIndex >= m_iNumTextDefs) {
            m_iCurrDefIndex = 0;
        }

        if (m_iCurrDefIndex >= MAX_CREDIT_STRINGS) {
            app.DebugPrintf("DLC credit %d\n",
                            m_iCurrDefIndex - MAX_CREDIT_STRINGS);
            // DLC credit
            pDef = app.GetDLCCredits(m_iCurrDefIndex - MAX_CREDIT_STRINGS);
        } else {
            // Get text def for this item.
            pDef = &(gs_aCreditDefs[m_iCurrDefIndex]);
        }

        // Set up the new text element.
        if (pDef->m_Text != nullptr)  // 4J-PB - think the RAD logo ones aren't
                                      // set up yet and are coming is as null
        {
            if (pDef->m_iStringID[0] == CREDIT_ICON) {
                addImage((ECreditIcons)pDef->m_iStringID[1]);
            } else  // using additional translated string.
            {
                std::string sanitisedString = std::string(pDef->m_Text);

                // 4J-JEV: Some DLC credits contain copyright or registered
                // symbols that are not rendered in some fonts.
                if (!ui.UsingBitmapFont()) {
                    sanitisedString =
                        replaceAll(sanitisedString, "\u00A9", "(C)");
                    sanitisedString =
                        replaceAll(sanitisedString, "\u00AE", "(R)");
                    sanitisedString =
                        replaceAll(sanitisedString, "\u2013", "-");
                }

                char* creditsString = new char[128];
                if (pDef->m_iStringID[0] == NO_TRANSLATED_STRING) {
                    memset(creditsString, 0, 128);
                    memcpy(creditsString, sanitisedString.c_str(),
                           sizeof(char) * sanitisedString.length());
                } else if (pDef->m_iStringID[1] != NO_TRANSLATED_STRING) {
                    snprintf(creditsString, 128, sanitisedString.c_str(),
                             app.GetString(pDef->m_iStringID[0]),
                             app.GetString(pDef->m_iStringID[1]));
                } else {
                    snprintf(creditsString, 128, sanitisedString.c_str(),
                             app.GetString(pDef->m_iStringID[0]));
                }

                setNextLabel(creditsString, pDef->m_eType);
                delete[] creditsString;
            }
        }
    }
}

void UIScene_Credits::handleInput(int iPad, int key, bool repeat, bool pressed,
                                  bool released, bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key,
    // down?"true":"false", pressed?"true":"false", released?"true":"false");

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed && !repeat) {
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_Credits::setNextLabel(const std::string& label,
                                   ECreditTextTypes size) {
    IggyDataValue result;
    IggyDataValue value[3];

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(label.c_str());
    stringVal.length = label.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = (int)size;

    value[2].type = IGGY_DATATYPE_boolean;
    value[2].boolval = (m_iCurrDefIndex == (m_iNumTextDefs - 1));

    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetNextLabel, 3, value);
}

void UIScene_Credits::addImage(ECreditIcons icon) {
    IggyDataValue result;
    IggyDataValue value[2];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = (int)icon;

    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = (m_iCurrDefIndex == (m_iNumTextDefs - 1));

    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcAddImage, 2, value);
}

void UIScene_Credits::handleRequestMoreData(F64 startIndex, bool up) {
    m_bAddNextLabel = true;
}
