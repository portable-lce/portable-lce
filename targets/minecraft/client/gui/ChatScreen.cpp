#include "ChatScreen.h"

#include <memory>

#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

const std::string ChatScreen::allowedChars = SharedConstants::acceptableLetters;

ChatScreen::ChatScreen() { frame = 0; }

void ChatScreen::init() { Keyboard::enableRepeatEvents(true); }

void ChatScreen::removed() { Keyboard::enableRepeatEvents(false); }

void ChatScreen::tick() { frame++; }

void ChatScreen::keyPressed(char ch, int eventKey) {
    if (eventKey == Keyboard::KEY_ESCAPE) {
        minecraft->setScreen(nullptr);
        return;
    }
    if (eventKey == Keyboard::KEY_RETURN) {
        std::string msg = trimString(message);
        if (msg.length() > 0) {
            std::string trim = trimString(message);
            if (!minecraft->handleClientSideCommand(trim)) {
                minecraft->player->chat(trim);
            }
        }
        minecraft->setScreen(nullptr);
        return;
    }
    if (eventKey == Keyboard::KEY_BACK && message.length() > 0)
        message = message.substr(0, message.length() - 1);
    if (allowedChars.find(ch) >= 0 &&
        message.length() < SharedConstants::maxChatLength) {
        message += ch;
    }
}

void ChatScreen::render(int xm, int ym, float a) {
    fill(2, height - 14, width - 2, height - 2, 0x80000000);
    drawString(font, "> " + message + (frame / 6 % 2 == 0 ? "_" : ""), 4,
               height - 12, 0xe0e0e0);

    Screen::render(xm, ym, a);
}

void ChatScreen::mouseClicked(int x, int y, int buttonNum) {
    if (buttonNum == 0) {
        if (minecraft->gui->selectedName != "")  // 4J - was nullptr comparison
        {
            if (message.length() > 0 && message[message.length() - 1] != ' ') {
                message += " ";
            }
            message += minecraft->gui->selectedName;
            unsigned int maxLength = SharedConstants::maxChatLength;
            if (message.length() > maxLength) {
                message = message.substr(0, maxLength);
            }
        } else {
            Screen::mouseClicked(x, y, buttonNum);
        }
    }
}