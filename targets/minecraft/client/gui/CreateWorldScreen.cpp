#include "CreateWorldScreen.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "Button.h"
#include "EditBox.h"
#include "MessageScreen.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/network/INetworkService.h"
#include "minecraft/util/Log.h"
#include "platform/storage/storage.h"
// Needed for the &CGameNetworkManager::RunNetworkGameThreadProc address-of
// below. Static thread procs can't be virtual; this one consumer keeps the
// concrete type include.
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/locale/Language.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/world/level/LevelSettings.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "platform/network/NetTypes.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

CreateWorldScreen::CreateWorldScreen(Screen* lastScreen) {
    done = false;  // 4J added
    moreOptions = false;
    gameMode = "survival";
    generateStructures = true;
    bonusChest = false;
    cheatsEnabled = false;
    flatWorld = false;
    this->lastScreen = lastScreen;
}

void CreateWorldScreen::tick() {
    nameEdit->tick();
    if (moreOptions) seedEdit->tick();

    // 4J - debug code - to be removed
    // static int count = 0;
    // if (count++ == 100) buttonClicked(buttons[0]);
}

void CreateWorldScreen::init() {
    Language* language = Language::getInstance();

    Keyboard::enableRepeatEvents(true);
    buttons.clear();
    buttons.push_back(new Button(0, width / 2 - 155, height - 28, 150, 20,
                                 language->getElement("selectWorld.create")));
    buttons.push_back(new Button(1, width / 2 + 5, height - 28, 150, 20,
                                 language->getElement("gui.cancel")));

    nameEdit = new EditBox(this, font, width / 2 - 100, 60, 200, 20,
                           language->getElement("selectWorld.newWorld"));
    nameEdit->inFocus = true;
    nameEdit->setMaxLength(32);

    seedEdit = new EditBox(this, font, width / 2 - 100, 60, 200, 20, "");

    buttons.push_back(gameModeButton = new Button(
                          2, width / 2 - 75, 100, 150, 20,
                          language->getElement("selectWorld.gameMode")));
    buttons.push_back(
        moreWorldOptionsButton =
            new Button(3, width / 2 - 75, 172, 150, 20,
                       language->getElement("selectWorld.moreWorldOptions")));
    buttons.push_back(generateStructuresButton = new Button(
                          4, width / 2 - 155, 100, 150, 20,
                          language->getElement("selectWorld.mapFeatures")));
    generateStructuresButton->visible = false;
    generateStructuresButton->active = false;
    buttons.push_back(bonusChestButton = new Button(
                          7, width / 2 + 5, 136, 150, 20,
                          language->getElement("selectWorld.bonusItems")));
    bonusChestButton->visible = false;
    bonusChestButton->active = false;
    buttons.push_back(worldTypeButton = new Button(
                          5, width / 2 + 5, 100, 150, 20,
                          language->getElement("selectWorld.mapType")));
    worldTypeButton->visible = false;
    worldTypeButton->active = false;
    buttons.push_back(cheatsEnabledButton = new Button(
                          6, width / 2 - 155, 136, 150, 20,
                          language->getElement("selectWorld.allowCommands")));
    cheatsEnabledButton->visible = false;
    cheatsEnabledButton->active = false;

    updateStrings();
    updateResultFolder();
}

// 4jcraft: referenced from func_73914_h in MCP 7.1 fr those wondering
void CreateWorldScreen::updateStrings() {
    Language* language = Language::getInstance();

    gameModeButton->msg =
        language->getElement("selectWorld.gameMode") + " " +
        language->getElement("selectWorld.gameMode." + gameMode);

    std::string line1Key = "selectWorld.gameMode." + gameMode + ".line1";
    std::string line2Key = "selectWorld.gameMode." + gameMode + ".line2";
    gameModeDescriptionLine1 = language->getElement(line1Key);
    gameModeDescriptionLine2 = language->getElement(line2Key);

    generateStructuresButton->msg =
        language->getElement("selectWorld.mapFeatures") + " " +
        (generateStructures ? language->getElement("options.on")
                            : language->getElement("options.off"));

    bonusChestButton->msg = language->getElement("selectWorld.bonusItems") +
                            " " +
                            (bonusChest ? language->getElement("options.on")
                                        : language->getElement("options.off"));

    worldTypeButton->msg =
        language->getElement("selectWorld.mapType") + " " +
        (flatWorld ? language->getElement("selectWorld.mapType.flat")
                   : language->getElement("selectWorld.mapType.normal"));

    cheatsEnabledButton->msg =
        language->getElement("selectWorld.allowCommands") + " " +
        (cheatsEnabled ? language->getElement("options.on")
                       : language->getElement("options.off"));
}

void CreateWorldScreen::updateResultFolder() {
    resultFolder = trimString(nameEdit->getValue());

    for (int i = 0; i < SharedConstants::ILLEGAL_FILE_CHARACTERS_LENGTH; i++) {
        size_t pos;
        while ((pos = resultFolder.find(
                    SharedConstants::ILLEGAL_FILE_CHARACTERS[i])) !=
               std::string::npos) {
            resultFolder[pos] = '_';
        }
    }

    if (resultFolder.length() == 0) {
        resultFolder = "World";
    }
    resultFolder = CreateWorldScreen::findAvailableFolderName(
        minecraft->getLevelSource(), resultFolder);
}

std::string CreateWorldScreen::findAvailableFolderName(
    LevelStorageSource* levelSource, const std::string& folder) {
    std::string folder2 = folder;  // 4J - copy input as it is const

    return folder2;
}

void CreateWorldScreen::removed() { Keyboard::enableRepeatEvents(false); }

void CreateWorldScreen::buttonClicked(Button* button) {
    Log::info("CreateWorldScreen::buttonClicked START\n");
    if (!button->active) return;
    if (button->id == 1) {
        Log::info(
            "CreateWorldScreen::buttonClicked 'Cancel' "
            "minecraft->setScreen(lastScreen)\n");
        minecraft->setScreen(lastScreen);
    } else if (button->id == 0) {
        minecraft->setScreen(
            new Screen());  // blank screen while the world loads
        if (done) return;
        done = true;

        MoreOptionsParams* moreOptionsParams = new MoreOptionsParams();

        // these r just the defaults from the createworldmenu UIscene
        // i had higher ambitions for what id do with these but its not worth it
        // for a temp ui
        moreOptionsParams->bGenerateOptions = true;
        moreOptionsParams->bStructures = generateStructures;
        moreOptionsParams->bFlatWorld = flatWorld;
        moreOptionsParams->bBonusChest = bonusChest;
        moreOptionsParams->bPVP = true;
        moreOptionsParams->bTrust = true;
        moreOptionsParams->bFireSpreads = true;
        moreOptionsParams->bTNT = true;
        moreOptionsParams->bHostPrivileges = false;
        moreOptionsParams->bOnlineGame = false;
        moreOptionsParams->bInviteOnly = false;
        moreOptionsParams->bAllowFriendsOfFriends = false;
        moreOptionsParams->bOnlineSettingChangedBySystem = false;
        moreOptionsParams->bCheatsEnabled = cheatsEnabled;
        moreOptionsParams->iPad = 0;

        moreOptionsParams->worldName = nameEdit->getValue();
        moreOptionsParams->seed = seedEdit->getValue();

        moreOptionsParams->dwTexturePack = 0;

        std::string worldName = nameEdit->getValue();
        if (worldName.empty()) {
            worldName = "2slimey";
        }

        PlatformStorage.ResetSaveData();
        PlatformStorage.SetSaveTitle((char*)worldName.c_str());

        std::string seedString = seedEdit->getValue();

        int64_t seedValue = 0;
        NetworkGameInitData* param = new NetworkGameInitData();

        if (seedString.length() != 0) {
            // try to convert it to a long first
            //            try {	// 4J - removed try/catch
            int64_t value = fromWString<int64_t>(seedString);

            bool isNumber = true;
            for (unsigned int i = 0; i < seedString.length(); ++i) {
                if (seedString.at(i) < '0' || seedString.at(i) > '9') {
                    if (!(i == 0 && seedString.at(i) == '-')) {
                        isNumber = false;
                        break;
                    }
                }
            }

            if (isNumber) value = fromWString<int64_t>(seedString);

            if (value != 0) {
                seedValue = value;
            } else {
                int hashValue = 0;
                for (unsigned int i = 0; i < seedString.length(); ++i)
                    hashValue = 31 * hashValue + seedString.at(i);
                seedValue = hashValue;
            }
            //           } catch (NumberFormatException e) {
            //               // not a number, fetch hash value
            //               seedValue = seedString.hashCode();
            //           }
        } else {
            param->findSeed = true;
        }

        param->seed = seedValue;
        param->saveData = nullptr;
        param->texturePackId = 0;
        param->settings = 0;

        gameServices().setGameHostOption(eGameHostOption_Difficulty,
                                         minecraft->options->difficulty);
        gameServices().setGameHostOption(
            eGameHostOption_FriendsOfFriends,
            moreOptionsParams->bAllowFriendsOfFriends);
        gameServices().setGameHostOption(eGameHostOption_Gamertags, 1);
        gameServices().setGameHostOption(eGameHostOption_BedrockFog, 0);
        gameServices().setGameHostOption(eGameHostOption_GameType,
                                         (gameMode == "survival")
                                             ? GameType::SURVIVAL->getId()
                                             : GameType::CREATIVE->getId());
        gameServices().setGameHostOption(eGameHostOption_LevelType,
                                         moreOptionsParams->bFlatWorld);
        gameServices().setGameHostOption(eGameHostOption_Structures,
                                         moreOptionsParams->bStructures);
        gameServices().setGameHostOption(eGameHostOption_BonusChest,
                                         moreOptionsParams->bBonusChest);
        gameServices().setGameHostOption(eGameHostOption_PvP,
                                         moreOptionsParams->bPVP);
        gameServices().setGameHostOption(eGameHostOption_TrustPlayers,
                                         moreOptionsParams->bTrust);
        gameServices().setGameHostOption(eGameHostOption_FireSpreads,
                                         moreOptionsParams->bFireSpreads);
        gameServices().setGameHostOption(eGameHostOption_TNT,
                                         moreOptionsParams->bTNT);
        gameServices().setGameHostOption(eGameHostOption_HostCanFly,
                                         moreOptionsParams->bHostPrivileges);
        gameServices().setGameHostOption(eGameHostOption_HostCanChangeHunger,
                                         moreOptionsParams->bHostPrivileges);
        gameServices().setGameHostOption(eGameHostOption_HostCanBeInvisible,
                                         moreOptionsParams->bHostPrivileges);
        gameServices().setGameHostOption(eGameHostOption_CheatsEnabled,
                                         moreOptionsParams->bHostPrivileges);

        param->settings = gameServices().getGameHostOption(eGameHostOption_All);
        param->xzSize = LEVEL_MAX_WIDTH;
        param->hellScale = HELL_LEVEL_MAX_SCALE;

        NetworkService.HostGame(0, false, false, MINECRAFT_NET_MAX_PLAYERS, 0);

        NetworkService.FakeLocalPlayerJoined();

        LoadingInputParams* loadingParams = new LoadingInputParams();
        loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
        loadingParams->lpParam = param;

        gameServices().setAutosaveTimerTime();

        UIFullscreenProgressCompletionData* completionData =
            new UIFullscreenProgressCompletionData();
        completionData->bShowBackground = true;
        completionData->bShowLogo = true;
        completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
        completionData->iPad = 0;
        loadingParams->completionData = completionData;

        ui.NavigateToScene(0, eUIScene_FullscreenProgress, loadingParams);
        Language* language = Language::getInstance();
        minecraft->setScreen(
            new MessageScreen(language->getElement("menu.generatingLevel")));
        // 4J Stu - This screen is not used, so removing this to stop the build
        // failing
    } else if (button->id == 2) {
        if (gameMode == "survival")
            gameMode = "creative";
        else
            gameMode = "survival";
        updateStrings();
    } else if (button->id == 3) {
        moreOptions = !moreOptions;
        gameModeButton->visible = !moreOptions;
        gameModeButton->active = !moreOptions;
        generateStructuresButton->visible = moreOptions;
        generateStructuresButton->active = moreOptions;
        bonusChestButton->visible = moreOptions;
        bonusChestButton->active = moreOptions;
        worldTypeButton->visible = moreOptions;
        worldTypeButton->active = moreOptions;
        cheatsEnabledButton->visible = moreOptions;
        cheatsEnabledButton->active = moreOptions;

        Language* language = Language::getInstance();
        if (moreOptions) {
            moreWorldOptionsButton->msg = language->getElement("gui.done");
        } else {
            moreWorldOptionsButton->msg =
                language->getElement("selectWorld.moreWorldOptions");
        }
    } else if (button->id == 4) {
        generateStructures = !generateStructures;
        updateStrings();
    } else if (button->id == 7) {
        bonusChest = !bonusChest;
        updateStrings();
    } else if (button->id == 5) {
        flatWorld = !flatWorld;
        updateStrings();
    } else if (button->id == 6) {
        cheatsEnabled = !cheatsEnabled;
        updateStrings();
    }
}

void CreateWorldScreen::keyPressed(char ch, int eventKey) {
    if (nameEdit->inFocus && !moreOptions)
        nameEdit->keyPressed(ch, eventKey);
    else
        seedEdit->keyPressed(ch, eventKey);

    if (ch == 13) {
        buttonClicked(buttons[0]);
    }
    buttons[0]->active = nameEdit->getValue().length() > 0;

    updateResultFolder();
}

void CreateWorldScreen::mouseClicked(int x, int y, int buttonNum) {
    Screen::mouseClicked(x, y, buttonNum);

    if (!moreOptions)
        nameEdit->mouseClicked(x, y, buttonNum);
    else
        seedEdit->mouseClicked(x, y, buttonNum);
}

void CreateWorldScreen::render(int xm, int ym, float a) {
    Language* language = Language::getInstance();

    // fill(0, 0, width, height, 0x40000000);
    renderBackground();

    drawCenteredString(font, language->getElement("selectWorld.create"),
                       width / 2, 20, 0xffffff);
    if (!moreOptions) {
        drawString(font, language->getElement("selectWorld.enterName"),
                   width / 2 - 100, 47, 0xa0a0a0);
        drawString(font,
                   language->getElement("selectWorld.resultFolder") + " " +
                       resultFolder,
                   width / 2 - 100, 85, 0xa0a0a0);

        nameEdit->render();

        drawString(font, gameModeDescriptionLine1, width / 2 - 100, 122,
                   0xa0a0a0);
        drawString(font, gameModeDescriptionLine2, width / 2 - 100, 134,
                   0xa0a0a0);
    } else {
        drawString(font, language->getElement("selectWorld.enterSeed"),
                   width / 2 - 100, 47, 0xa0a0a0);
        drawString(font, language->getElement("selectWorld.seedInfo"),
                   width / 2 - 100, 85, 0xa0a0a0);
        drawString(font, language->getElement("selectWorld.mapFeatures.info"),
                   width / 2 - 150, 122, 0xa0a0a0);
        drawString(font, language->getElement("selectWorld.allowCommands.info"),
                   width / 2 - 150, 157, 0xa0a0a0);

        seedEdit->render();
    }

    Screen::render(xm, ym, a);

    Screen::render(xm, ym, a);
}

void CreateWorldScreen::tabPressed() {
    if (!moreOptions) return;

    if (nameEdit->inFocus) {
        nameEdit->focus(false);
        seedEdit->focus(true);
    } else {
        nameEdit->focus(true);
        seedEdit->focus(false);
    }
}
