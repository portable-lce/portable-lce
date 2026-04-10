
#include "UIScene_EndPoem.h"

#include <string.h>
#include <wchar.h>

#include <memory>

#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "java/Random.h"
#include "minecraft/GameEnums.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "strings.h"
#include "util/StringHelpers.h"

class UILayer;

UIScene_EndPoem::UIScene_EndPoem(int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // ui.setFontCachingCalculationBuffer(20000);

    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_bIgnoreInput = false;

    // 4J Stu - Don't need these, the AS handles the scrolling and makes it look
    // nice

    // char startTags[64];
    // snprintf(startTags,64,"<font
    // size=\"%d\">",app.GetHTMLFontSize(eHTMLSize_EndPoem));
    // noNoiseString.append(halfScreenLineBreaks);
    // noNoiseString.append(halfScreenLineBreaks);
    noNoiseString.append(app.GetString(IDS_WIN_TEXT));
    noNoiseString.append(app.GetString(IDS_WIN_TEXT_PART_2));
    noNoiseString.append(app.GetString(IDS_WIN_TEXT_PART_3));

    // noNoiseString.append(halfScreenLineBreaks);

    // 4J Stu - Iggy seems to strip our trailing linebreaks, so added a space to
    // made sure it scrolls this far
    noNoiseString.append(" ");

    noNoiseString = app.FormatHTMLString(m_iPad, noNoiseString, 0xff000000);

    Minecraft* pMinecraft = Minecraft::GetInstance();

    std::string playerName = "";
    if (pMinecraft->localplayers[ui.GetWinUserIndex()] != nullptr) {
        playerName = escapeXML(
            pMinecraft->localplayers[ui.GetWinUserIndex()]->getDisplayName());
    } else {
        playerName =
            escapeXML(pMinecraft->localplayers[PlatformProfile.GetPrimaryPad()]
                          ->getDisplayName());
    }
    noNoiseString = replaceAll(noNoiseString, "{*PLAYER*}", playerName);

    Random random(8124371);
    int found = (int)noNoiseString.find("{*NOISE*}");
    int length;
    while (found != std::string::npos) {
        length = random.nextInt(4) + 3;
        m_noiseLengths.push_back(length);
        found = (int)noNoiseString.find("{*NOISE*}", found + 1);
    }

    updateNoise();

    // 4J-JEV: Find paragraph start and end points.
    m_paragraphs = std::vector<std::string>();
    int lastIndex = 0;
    for (int index = 0; index != std::string::npos;
         index = noiseString.find("<br /><br />", index + 12, 12)) {
        m_paragraphs.push_back(
            noiseString.substr(lastIndex, index - lastIndex));
        lastIndex = index;
    }
    // lastIndex += 12;
    m_paragraphs.push_back(
        noiseString.substr(lastIndex, noiseString.length() - lastIndex));

    // m_htmlPoem.init(noiseString.c_str());
    // m_htmlPoem.startAutoScroll();

    // std::string result = m_htmlControl.GetText();

    // cout << result.c_str();

#if TO_BE_IMPLEMENTED
    m_scrollDir = 1;
    int32_t hr = XuiHtmlControlSetSmoothScroll(
        m_htmlControl.m_hObj, XUI_SMOOTHSCROLL_VERTICAL, true,
        AUTO_SCROLL_SPEED, 1.0f, AUTO_SCROLL_SPEED);
    XuiHtmlControlVScrollBy(m_htmlControl.m_hObj, m_scrollDir * 1000);

    SetTimer(0, 200);
#endif

    m_requestedLabel = 0;
}

std::string UIScene_EndPoem::getMoviePath() { return "EndPoem"; }

void UIScene_EndPoem::updateTooltips() {
    ui.SetTooltips(XUSER_INDEX_ANY, -1,
                   m_bIgnoreInput ? -1 : IDS_TOOLTIPS_CONTINUE);
}

void UIScene_EndPoem::tick() {
    UIScene::tick();

    if (m_requestedLabel >= 0 && m_requestedLabel < m_paragraphs.size()) {
        std::string label = m_paragraphs[m_requestedLabel];

        IggyDataValue result;
        IggyDataValue value[3];

        IggyStringUTF8 stringVal;
        stringVal.string = const_cast<char*>(label.c_str());
        stringVal.length = label.length();
        value[0].type = IGGY_DATATYPE_string_UTF8;
        value[0].string8 = stringVal;

        value[1].type = IGGY_DATATYPE_number;
        value[1].number = m_requestedLabel;

        value[2].type = IGGY_DATATYPE_boolean;
        value[2].boolval = (m_requestedLabel == (m_paragraphs.size() - 1));

        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcSetNextLabel, 3, value);

        m_requestedLabel = -1;
    }
}

void UIScene_EndPoem::handleInput(int iPad, int key, bool repeat, bool pressed,
                                  bool released, bool& handled) {
    if (m_bIgnoreInput) return;

    if (pressed) ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                m_bIgnoreInput = true;
                Minecraft* pMinecraft = Minecraft::GetInstance();
                for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                    if (pMinecraft->localplayers[i] != nullptr) {
                        app.SetAction(i, eAppAction_Respawn);
                    }
                }

                // This just allows it to be shown
                if (pMinecraft
                        ->localgameModes[PlatformProfile.GetPrimaryPad()] !=
                    nullptr)
                    pMinecraft->localgameModes[PlatformProfile.GetPrimaryPad()]
                        ->getTutorial()
                        ->showTutorialPopup(true);

                updateTooltips();
                navigateBack();

                handled = true;
            }
            break;
        case ACTION_MENU_DOWN:
        case ACTION_MENU_UP:
        case ACTION_MENU_OTHER_STICK_DOWN:
        case ACTION_MENU_OTHER_STICK_UP:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_EndPoem::handleDestroy() {
    // ui.setFontCachingCalculationBuffer(-1);
}

void UIScene_EndPoem::handleRequestMoreData(F64 startIndex, bool up) {
    m_requestedLabel = (int)startIndex;
}

void UIScene_EndPoem::updateNoise() {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    noiseString = noNoiseString;

    int length = 0;
    char replacements[64];
    std::string replaceString = "";
    char randomChar = 'a';
    Random* random = pMinecraft->font->random;

    bool darken = false;

    std::string tag = "{*NOISE*}";

    auto it = m_noiseLengths.begin();
    int found = (int)noiseString.find(tag);
    while (found != std::string::npos && it != m_noiseLengths.end()) {
        length = *it;
        ++it;

        replaceString = "";
        for (int i = 0; i < length; ++i) {
            if (ui.UsingBitmapFont()) {
                randomChar = SharedConstants::acceptableLetters[random->nextInt(
                    (int)SharedConstants::acceptableLetters.length())];
            } else {
                // 4J-JEV: It'd be nice to avoid null characters when using
                // asian languages.
                static std::string acceptableLetters =
                    "!\"#$%&'()*+,-./0123456789:;<=>?@[\\]^_'|}~";
                randomChar = acceptableLetters[random->nextInt(
                    (int)acceptableLetters.length())];
            }

            std::string randomCharStr = "";
            randomCharStr.push_back(randomChar);
            if (randomChar == '<') {
                randomCharStr = "&lt;";
            } else if (randomChar == '>') {
                randomCharStr = "&gt;";
            } else if (randomChar == '"') {
                randomCharStr = "&quot;";
            } else if (randomChar == '&') {
                randomCharStr = "&amp;";
            } else if (randomChar == '\\') {
                randomCharStr = "\\\\";
            } else if (randomChar == '{') {
                randomCharStr = "}";
            }

            int randomVal = random->nextInt(2);
            eMinecraftColour colour = eHTMLColor_8;
            if (randomVal == 1)
                colour = eHTMLColor_9;
            else if (randomVal == 2)
                colour = eHTMLColor_a;
            memset(replacements, 0, 64 * sizeof(char));
            snprintf(
                replacements, 64,
                "<font color=\"#%08x\" shadowcolor=\"#80000000\">%s</font>",
                app.GetHTMLColour(colour), randomCharStr.c_str());
            replaceString.append(replacements);
        }

        noiseString.replace(found, tag.length(), replaceString);

        // int pos = 0;
        // do {
        //	pos =
        // random->nextInt(SharedConstants::acceptableLetters.length()); } while
        // (pMinecraft->font->charWidths[ch + 32] !=
        // pMinecraft->font->charWidths[pos + 32]); ib.put(listPos + 256 +
        // random->nextInt(2) + 8 + (darken ? 16 : 0)); ib.put(listPos + pos +
        // 32);

        found = (int)noiseString.find(tag, found + 1);
    }
}
