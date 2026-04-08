#include "minecraft/IGameServices.h"
#include "PauseScreen.h"

#include <math.h>

#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include "platform/input/input.h"
#include "Button.h"
#include "MessageScreen.h"
#include "minecraft/GameEnums.h"
#include "app/common/Network/GameNetworkManager.h"
#include "OptionsScreen.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/locale/I18n.h"
#include "minecraft/server/MinecraftServer.h"

PauseScreen::PauseScreen() {
    saveStep = 0;
    visibleTime = 0;
}

void PauseScreen::init() {
    saveStep = 0;
    buttons.clear();
    int yo = -16;
    // 4jcraft: solves the issue of client-side only pausing in the java gui
    if (g_NetworkManager.IsLocalGame() &&
        g_NetworkManager.GetPlayerCount() == 1)
        gameServices().setXuiServerAction(PlatformInput.GetPrimaryPad(),
                               eXuiServerAction_PauseServer, true);
    buttons.push_back(new Button(1, width / 2 - 100, height / 4 + 24 * 5 + yo,
                                 I18n::get("menu.returnToMenu")));
    if (!g_NetworkManager.IsHost()) {
        buttons[0]->msg = I18n::get("menu.disconnect");
    }

    buttons.push_back(new Button(4, width / 2 - 100, height / 4 + 24 * 1 + yo,
                                 "LBack to game"));
    buttons.push_back(new Button(0, width / 2 - 100, height / 4 + 24 * 4 + yo,
                                 "LOptions..."));

    buttons.push_back(new Button(4, width / 2 - 100, height / 4 + 24 * 1 + yo,
                                 I18n::get("menu.returnToGame")));
    buttons.push_back(new Button(0, width / 2 - 100, height / 4 + 24 * 4 + yo,
                                 I18n::get("menu.options")));

    buttons.push_back(new Button(5, width / 2 - 100, height / 4 + 24 * 2 + yo,
                                 98, 20, I18n::get("gui.achievements")));
    buttons.push_back(new Button(6, width / 2 + 2, height / 4 + 24 * 2 + yo, 98,
                                 20, I18n::get("gui.stats")));
    /*
     * if (minecraft->serverConnection!=null) { buttons.get(1).active =
     * false; buttons.get(2).active = false; buttons.get(3).active = false;
     * }
     */
}

void PauseScreen::exitWorld(Minecraft* minecraft, bool save) {
    // 4jcraft: made our own static method for use in the java gui (other
    // places such as the deathscreen need this)
    MinecraftServer* server = MinecraftServer::getInstance();

    minecraft->setScreen(new MessageScreen("Leaving world"));
    if (g_NetworkManager.IsHost()) {
        server->setSaveOnExit(save);
    }
    gameServices().setAction(minecraft->player->GetXboxPad(), eAppAction_ExitWorld);
}

void PauseScreen::buttonClicked(Button* button) {
    if (button->id == 0) {
        minecraft->setScreen(new OptionsScreen(this, minecraft->options));
    }
    if (button->id == 1) {
        // if (minecraft->isClientSide())
        // {
        //     minecraft->level->disconnect();
        // }

        // minecraft->setLevel(nullptr);
        // minecraft->setScreen(new TitleScreen());

        // 4jcraft: exit with our new exitWorld method
        exitWorld(minecraft, true);
    }
    if (button->id == 4) {
        gameServices().setXuiServerAction(PlatformInput.GetPrimaryPad(),
                               eXuiServerAction_PauseServer, false);
        minecraft->setScreen(nullptr);
        //       minecraft->grabMouse();		// 4J - removed
    }

    if (button->id == 5) {
        //        minecraft->setScreen(new AchievementScreen(minecraft->stats));
        //        // 4J TODO - put back
    }
    if (button->id == 6) {
        //        minecraft->setScreen(new StatsScreen(this, minecraft->stats));
        //        // 4J TODO - put back
    }
}

void PauseScreen::tick() {
    Screen::tick();
    visibleTime++;
}

void PauseScreen::render(int xm, int ym, float a) {
    renderBackground();

    bool isSaving = false;  //! minecraft->level->pauseSave(saveStep++);
    if (isSaving || visibleTime < 20) {
        float col = ((visibleTime % 10) + a) / 10.0f;
        col = sinf(col * std::numbers::pi * 2) * 0.2f + 0.8f;
        int br = (int)(255 * col);

        drawString(font, "Saving level..", 8, height - 16,
                   br << 16 | br << 8 | br);
    }

    drawCenteredString(font, "Game menu", width / 2, 40, 0xffffff);

    Screen::render(xm, ym, a);
}