#include "JoinMultiplayerScreen.h"

#include <vector>

#include "Button.h"
#include "EditBox.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/locale/Language.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

JoinMultiplayerScreen::JoinMultiplayerScreen(Screen* lastScreen) {
    ipEdit = nullptr;
    this->lastScreen = lastScreen;
}

void JoinMultiplayerScreen::tick() { ipEdit->tick(); }

void JoinMultiplayerScreen::init() {
    Language* language = Language::getInstance();

    Keyboard::enableRepeatEvents(true);
    buttons.clear();
    buttons.push_back(new Button(0, width / 2 - 100, height / 4 + 24 * 4 + 12,
                                 language->getElement("multiplayer.connect")));
    buttons.push_back(new Button(1, width / 2 - 100, height / 4 + 24 * 5 + 12,
                                 language->getElement("gui.cancel")));
    std::string ip = replaceAll(minecraft->options->lastMpIp, "_", ":");
    buttons[0]->active = ip.length() > 0;

    ipEdit = new EditBox(this, font, width / 2 - 100, height / 4 - 10 + 50 + 18,
                         200, 20, ip);
    ipEdit->inFocus = true;
    ipEdit->setMaxLength(128);
}

void JoinMultiplayerScreen::removed() { Keyboard::enableRepeatEvents(false); }

void JoinMultiplayerScreen::buttonClicked(Button* button) {
    if (!button->active) return;
    if (button->id == 1) {
        minecraft->setScreen(lastScreen);
    } else if (button->id == 0) {
        std::string ip = trimString(ipEdit->getValue());

        minecraft->options->lastMpIp = replaceAll(ip, ":", "_");
        minecraft->options->save();

        std::vector<std::string> parts = stringSplit(ip, 'L');
        if (ip[0] == '[') {
            int pos = (int)ip.find("]");
            if (pos != std::string::npos) {
                std::string path = ip.substr(1, pos);
                std::string port = trimString(ip.substr(pos + 1));
                if (port[0] == ':' && port.length() > 0) {
                    port = port.substr(1);
                    parts.clear();
                    parts.push_back(path);
                    parts.push_back(port);
                } else {
                    parts.clear();
                    parts.push_back(path);
                }
            }
        }
        if (parts.size() > 2) {
            parts.clear();
            parts.push_back(ip);
        }

        // 4J - TODO
        //        minecraft->setScreen(new ConnectScreen(minecraft, parts[0],
        //        parts.size() > 1 ? parseInt(parts[1], 25565) : 25565));
    }
}

int JoinMultiplayerScreen::parseInt(const std::string& str, int def) {
    return fromWString<int>(str);
}

void JoinMultiplayerScreen::keyPressed(char ch, int eventKey) {
    ipEdit->keyPressed(ch, eventKey);

    if (ch == 13) {
        buttonClicked(buttons[0]);
    }
    buttons[0]->active = ipEdit->getValue().length() > 0;
}

void JoinMultiplayerScreen::mouseClicked(int x, int y, int buttonNum) {
    Screen::mouseClicked(x, y, buttonNum);

    ipEdit->mouseClicked(x, y, buttonNum);
}

void JoinMultiplayerScreen::render(int xm, int ym, float a) {
    Language* language = Language::getInstance();

    // fill(0, 0, width, height, 0x40000000);
    renderBackground();

    drawCenteredString(font, language->getElement("multiplayer.title"),
                       width / 2, height / 4 - 60 + 20, 0xffffff);
    drawString(font, language->getElement("multiplayer.info1"), width / 2 - 140,
               height / 4 - 60 + 60 + 9 * 0, 0xa0a0a0);
    drawString(font, language->getElement("multiplayer.info2"), width / 2 - 140,
               height / 4 - 60 + 60 + 9 * 1, 0xa0a0a0);
    drawString(font, language->getElement("multiplayer.ipinfo"),
               width / 2 - 140, height / 4 - 60 + 60 + 9 * 4, 0xa0a0a0);

    ipEdit->render();

    Screen::render(xm, ym, a);
}