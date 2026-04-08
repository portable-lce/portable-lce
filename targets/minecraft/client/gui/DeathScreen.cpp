#include "DeathScreen.h"
#include "platform/stubs.h"

#include <memory>
#include <string>
#include <vector>

#include "platform/renderer/renderer.h"
#include "Button.h"
#include "PauseScreen.h"
#include "util/StringHelpers.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"

void DeathScreen::init() {
    buttons.clear();
    buttons.push_back(
        new Button(1, width / 2 - 100, height / 4 + 24 * 3, "Respawn"));
    buttons.push_back(
        new Button(2, width / 2 - 100, height / 4 + 24 * 4, "Title menu"));

    if (minecraft->user == nullptr) {
        buttons[1]->active = false;
    }
}

void DeathScreen::keyPressed(char eventCharacter, int eventKey) {}

void DeathScreen::buttonClicked(Button* button) {
    if (button->id == 0) {
        //            minecraft.setScreen(new OptionsScreen(this,
        //            minecraft.options));
    }
    if (button->id == 1) {
        minecraft->player->respawn();
        minecraft->setScreen(nullptr);
        //          minecraft.setScreen(new NewLevelScreen(this));
    }
    if (button->id == 2) {
        // minecraft->setLevel(nullptr);
        // minecraft->setScreen(new TitleScreen());

        // 4jcraft: use the static method from PauseScreen to exit
        PauseScreen::exitWorld(minecraft, true);
    }
}

void DeathScreen::render(int xm, int ym, float a) {
    fillGradient(0, 0, width, height, 0x60500000, 0xa0803030);

    glPushMatrix();
    glScalef(2, 2, 2);
    drawCenteredString(font, "Game over!", width / 2 / 2, 60 / 2, 0xffffff);
    glPopMatrix();
    drawCenteredString(font,
                       "Score: &e" + toWString(minecraft->player->getScore()),
                       width / 2, 100, 0xffffff);

    Screen::render(xm, ym, a);

    // 4J - debug code - remove
    // static int count = 0;
    // if (count++ == 100) {
    //     count = 0;
    //     buttonClicked(buttons[0]);
    // }
}

bool DeathScreen::isPauseScreen() { return false; }