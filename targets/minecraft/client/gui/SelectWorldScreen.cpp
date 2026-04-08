#include "minecraft/util/Log.h"
#include "SelectWorldScreen.h"

#include <stdint.h>
#include <time.h>
#include <wchar.h>

#include <chrono>
#include <ctime>
#include <vector>

#include "Button.h"
#include "ConfirmScreen.h"
#include "CreateWorldScreen.h"
#include "RenameWorldScreen.h"
#include "util/StringHelpers.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/gui/ScrolledSelectionList.h"
#include "minecraft/locale/Language.h"
#include "minecraft/world/level/storage/LevelStorageSource.h"
#include "minecraft/world/level/storage/LevelSummary.h"

SelectWorldScreen::SelectWorldScreen(Screen* lastScreen) {
    // 4J - added initialisers
    title = "Select world";
    done = false;
    selectedWorld = 0;
    worldSelectionList = nullptr;
    isDeleting = false;
    deleteButton = nullptr;
    selectButton = nullptr;
    renameButton = nullptr;

    this->lastScreen = lastScreen;
}

void SelectWorldScreen::init() {
    Log::info("SelectWorldScreen::init() START\n");
    Language* language = Language::getInstance();
    title = language->getElement("selectWorld.title");

    worldLang = language->getElement("selectWorld.world");
    conversionLang = language->getElement("selectWorld.conversion");
    loadLevelList();

    worldSelectionList = new WorldSelectionList(this);
    worldSelectionList->init(&buttons, BUTTON_UP_ID, BUTTON_DOWN_ID);

    postInit();
}

void SelectWorldScreen::loadLevelList() {
    LevelStorageSource* levelSource = minecraft->getLevelSource();
    levelList = levelSource->getLevelList();
    //	Collections.sort(levelList);	// 4J - TODO - get sort functor etc.
    selectedWorld = -1;
}

std::string SelectWorldScreen::getWorldId(int id) {
    return levelList->at(id)->getLevelId();
}

std::string SelectWorldScreen::getWorldName(int id) {
    std::string levelName = levelList->at(id)->getLevelName();

    if (levelName.length() == 0) {
        Language* language = Language::getInstance();
        levelName = language->getElement("selectWorld.world") + " " +
                    toWString<int>(id + 1);
    }

    return levelName;
}

void SelectWorldScreen::postInit() {
    Language* language = Language::getInstance();

    buttons.push_back(selectButton = new Button(
                          BUTTON_SELECT_ID, width / 2 - 154, height - 52, 150,
                          20, language->getElement("selectWorld.select")));
    buttons.push_back(deleteButton = new Button(
                          BUTTON_RENAME_ID, width / 2 - 154, height - 28, 70,
                          20, language->getElement("selectWorld.rename")));
    buttons.push_back(renameButton = new Button(
                          BUTTON_DELETE_ID, width / 2 - 74, height - 28, 70, 20,
                          language->getElement("selectWorld.delete")));
    buttons.push_back(new Button(BUTTON_CREATE_ID, width / 2 + 4, height - 52,
                                 150, 20,
                                 language->getElement("selectWorld.create")));
    buttons.push_back(new Button(BUTTON_CANCEL_ID, width / 2 + 4, height - 28,
                                 150, 20, language->getElement("gui.cancel")));

    selectButton->active = false;
    deleteButton->active = false;
    renameButton->active = false;
}

void SelectWorldScreen::buttonClicked(Button* button) {
    Log::info("SelectWorldScreen::buttonClicked START\n");
    if (!button->active) return;
    if (button->id == BUTTON_DELETE_ID) {
        std::string worldName = getWorldName(selectedWorld);
        if (worldName != "") {
            isDeleting = true;

            Language* language = Language::getInstance();
            std::string title =
                language->getElement("selectWorld.deleteQuestion");
            std::string warning =
                "'" + worldName + "' " +
                language->getElement("selectWorld.deleteWarning");
            std::string yes =
                language->getElement("selectWorld.deleteButton");
            std::string no = language->getElement("gui.cancel");

            ConfirmScreen* confirmScreen =
                new ConfirmScreen(this, title, warning, yes, no, selectedWorld);
            minecraft->setScreen(confirmScreen);
        }
    } else if (button->id == BUTTON_SELECT_ID) {
        worldSelected(selectedWorld);
    } else if (button->id == BUTTON_CREATE_ID) {
        minecraft->setScreen(new CreateWorldScreen(this));
    } else if (button->id == BUTTON_RENAME_ID) {
        minecraft->setScreen(
            new RenameWorldScreen(this, getWorldId(selectedWorld)));
    } else if (button->id == BUTTON_CANCEL_ID) {
        Log::info(
            "SelectWorldScreen::buttonClicked 'Cancel' "
            "minecraft->setScreen(lastScreen)\n");
        minecraft->setScreen(lastScreen);
    } else {
        worldSelectionList->buttonClicked(button);
    }
}

void SelectWorldScreen::worldSelected(int id) {
    minecraft->setScreen(nullptr);
    if (done) return;
    done = true;
    minecraft->gameMode = nullptr;  // new SurvivalMode(minecraft);

    std::string worldFolderName = getWorldId(id);
    if (worldFolderName == "")  // 4J - was nullptr comparison
    {
        worldFolderName = "World" + toWString<int>(id);
    }
    // 4J Stu - Not used, so commenting to stop the build failing
}

void SelectWorldScreen::confirmResult(bool result, int id) {
    if (isDeleting) {
        isDeleting = false;
        if (result) {
            LevelStorageSource* levelSource = minecraft->getLevelSource();
            levelSource->clearAll();
            levelSource->deleteLevel(getWorldId(id));

            loadLevelList();
        }
        minecraft->setScreen(this);
    }
}

void SelectWorldScreen::render(int xm, int ym, float a) {
    // fill(0, 0, width, height, 0x40000000);
    renderDirtBackground(0);
    worldSelectionList->render(xm, ym, a);

    drawCenteredString(font, title, width / 2, 20, 0xffffff);

    Screen::render(xm, ym, a);

    // 4J - debug code - remove
    if (0) {
        static int count = 0;
        static bool forceCreateLevel = false;
        if (count++ >= 100) {
            if (!forceCreateLevel && levelList->size() > 0) {
                // 4J Stu - For some obscures reason the "delete" button is
                // called "renameButton" and vice versa. if( levelList->size() >
                // 2 && deleteButton->active )
                //{
                //	this->selectedWorld = 2;
                //	count = 0;
                //	buttonClicked(deleteButton);
                //}
                // else
                if (levelList->size() > 1 && renameButton->active) {
                    this->selectedWorld = 1;
                    count = 0;
                    buttonClicked(renameButton);
                } else if (selectButton->active == true) {
                    this->selectedWorld = 0;
                    buttonClicked(selectButton);
                    // this->worldSelected( 0 );
                } else {
                    selectButton->active = true;
                    deleteButton->active = true;
                    renameButton->active = true;
                    count = 0;
                }
            } else {
                Log::info(
                    "SelectWorldScreen::render minecraft->setScreen(new "
                    "CreateWorldScreen(this))\n");
                minecraft->setScreen(new CreateWorldScreen(this));
            }
        }
    }
}

SelectWorldScreen::WorldSelectionList::WorldSelectionList(
    SelectWorldScreen* sws)
    : ScrolledSelectionList(sws->minecraft, sws->width, sws->height, 32,
                            sws->height - 64, 36) {
    parent = sws;
}

int SelectWorldScreen::WorldSelectionList::getNumberOfItems() {
    return (int)this->parent->levelList->size();
}

void SelectWorldScreen::WorldSelectionList::selectItem(int item,
                                                       bool doubleClick) {
    parent->selectedWorld = item;
    bool active = (this->parent->selectedWorld >= 0 &&
                   this->parent->selectedWorld < getNumberOfItems());
    parent->selectButton->active = active;
    parent->deleteButton->active = active;
    parent->renameButton->active = active;

    if (doubleClick && active) {
        parent->worldSelected(item);
    }
}

bool SelectWorldScreen::WorldSelectionList::isSelectedItem(int item) {
    return item == parent->selectedWorld;
}

int SelectWorldScreen::WorldSelectionList::getMaxPosition() {
    return (int)parent->levelList->size() * 36;
}

void SelectWorldScreen::WorldSelectionList::renderBackground() {
    parent->renderBackground();  // 4J - was
                                 // SelectWorldScreen.this.renderBackground();
}

void SelectWorldScreen::WorldSelectionList::renderItem(int i, int x, int y,
                                                       int h, Tesselator* t) {
    LevelSummary* levelSummary = parent->levelList->at(i);

    std::string name = levelSummary->getLevelName();
    if (name.length() == 0) {
        name = parent->worldLang + " " + toWString<int>(i + 1);
    }

    std::string id = levelSummary->getLevelId();

    // levelSummary->getLastPlayed() is milliseconds since the FILETIME
    // epoch (1601-01-01 UTC). Convert to chrono::system_clock (1970
    // epoch) by subtracting the constant offset, then break down with
    // gmtime_r for display.
    constexpr int64_t kFileTimeEpochToUnixEpochMs = 11644473600000LL;
    const int64_t lastPlayedUnixMs =
        levelSummary->getLastPlayed() - kFileTimeEpochToUnixEpochMs;
    const auto tp = std::chrono::system_clock::time_point{
        std::chrono::milliseconds{lastPlayedUnixMs}};
    time_t lastPlayedTime = std::chrono::system_clock::to_time_t(tp);
    std::tm utc;
    gmtime_r(&lastPlayedTime, &utc);

    char buffer[20];
    // 4J Stu - Currently shows years as 4 digits, where java only showed 2
    snprintf(buffer, 20, "%d/%d/%d %d:%02d", utc.tm_mday, utc.tm_mon + 1,
             utc.tm_year + 1900, utc.tm_hour,
             utc.tm_min);  // 4J - TODO Localise this
    id = id + " (" + buffer;

    int64_t size = levelSummary->getSizeOnDisk();
    id = id + ", " + toWString<float>(size / 1024 * 100 / 1024 / 100.0f) +
         " MB)";
    std::string info;

    if (levelSummary->isRequiresConversion()) {
        info = parent->conversionLang + " " + info;
    }

    parent->drawString(parent->font, name, x + 2, y + 1, 0xffffff);
    parent->drawString(parent->font, id, x + 2, y + 12, 0x808080);
    parent->drawString(parent->font, info, x + 2, y + 12 + 10, 0x808080);
}
