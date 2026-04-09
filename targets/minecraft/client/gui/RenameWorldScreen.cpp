#include "RenameWorldScreen.h"

#include <vector>

#include "Button.h"
#include "EditBox.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/locale/Language.h"
#include "minecraft/world/level/storage/LevelStorageSource.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

RenameWorldScreen::RenameWorldScreen(Screen* lastScreen,
                                     const std::string& levelId) {
    nameEdit = nullptr;
    this->lastScreen = lastScreen;
    this->levelId = levelId;
}

void RenameWorldScreen::tick() { nameEdit->tick(); }

void RenameWorldScreen::init() {
    // 4J Stu - Removed this as we don't need the screen. Changed to how we pass
    // save data around stopped this compiling
}

void RenameWorldScreen::removed() { Keyboard::enableRepeatEvents(false); }

void RenameWorldScreen::buttonClicked(Button* button) {
    if (!button->active) return;
    if (button->id == 1) {
        minecraft->setScreen(lastScreen);
    } else if (button->id == 0) {
        LevelStorageSource* levelSource = minecraft->getLevelSource();
        levelSource->renameLevel(levelId, trimString(nameEdit->getValue()));

        minecraft->setScreen(lastScreen);
    }
}

void RenameWorldScreen::keyPressed(char ch, int eventKey) {
    nameEdit->keyPressed(ch, eventKey);
    buttons[0]->active = trimString(nameEdit->getValue()).length() > 0;

    if (ch == 13) {
        buttonClicked(buttons[0]);
    }
}

void RenameWorldScreen::mouseClicked(int x, int y, int buttonNum) {
    Screen::mouseClicked(x, y, buttonNum);

    nameEdit->mouseClicked(x, y, buttonNum);
}

void RenameWorldScreen::render(int xm, int ym, float a) {
    Language* language = Language::getInstance();

    // fill(0, 0, width, height, 0x40000000);
    renderBackground();

    drawCenteredString(font, language->getElement("selectWorld.renameTitle"),
                       width / 2, height / 4 - 60 + 20, 0xffffff);
    drawString(font, language->getElement("selectWorld.enterName"),
               width / 2 - 100, 47, 0xa0a0a0);

    nameEdit->render();

    Screen::render(xm, ym, a);
}