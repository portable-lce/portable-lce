#include "InBedChatScreen.h"

#include <memory>
#include <string>
#include <vector>

#include "Button.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/ChatScreen.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/locale/Language.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

void InBedChatScreen::init() {
    Keyboard::enableRepeatEvents(true);

    Language* language = Language::getInstance();

    buttons.push_back(
        new Button(WAKE_UP_BUTTON, width / 2 - 100, height - 40,
                   language->getElement("multiplayer.stopSleeping")));
}

void InBedChatScreen::removed() { Keyboard::enableRepeatEvents(false); }

void InBedChatScreen::keyPressed(char ch, int eventKey) {
    if (eventKey == Keyboard::KEY_ESCAPE) {
        sendWakeUp();
    } else if (eventKey == Keyboard::KEY_RETURN) {
        std::string msg = trimString(message);
        if (msg.length() > 0) {
            minecraft->player->chat(trimString(message));
        }
        message = "";
    } else {
        ChatScreen::keyPressed(ch, eventKey);
    }
}

void InBedChatScreen::render(int xm, int ym, float a) {
    ChatScreen::render(xm, ym, a);
}

void InBedChatScreen::buttonClicked(Button* button) {
    if (button->id == WAKE_UP_BUTTON) {
        sendWakeUp();
    } else {
        ChatScreen::buttonClicked(button);
    }
}

void InBedChatScreen::sendWakeUp() {
    /* 4J - TODO
if (minecraft.player instanceof MultiplayerLocalPlayer)
    {
    ClientConnection connection = ((MultiplayerLocalPlayer)
minecraft.player).connection; connection.send(new
PlayerCommandPacket(minecraft.player, PlayerCommandPacket.STOP_SLEEPING));
}
    */
}