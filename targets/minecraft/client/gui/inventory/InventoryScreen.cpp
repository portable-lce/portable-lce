#include "InventoryScreen.h"

#include <cmath>
#include <numbers>
#include <string>
#include <vector>

#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Button.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/achievement/AchievementScreen.h"
#include "minecraft/client/gui/achievement/StatsScreen.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/world/entity/player/Player.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

InventoryScreen::InventoryScreen(std::shared_ptr<Player> player)
    : AbstractContainerScreen(player->inventoryMenu) {
    xMouse = yMouse = 0.0f;  // 4J added

    this->passEvents = true;
    player->awardStat(GenericStats::openInventory(),
                      GenericStats::param_noArgs());
}

void InventoryScreen::init() { buttons.clear(); }

void InventoryScreen::renderLabels() {
    font->draw("Crafting", 84 + 2, 8 * 2, 0x404040);
}

void InventoryScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);
    this->xMouse = (float)xm;
    this->yMouse = (float)ym;
}

void InventoryScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    int tex = minecraft->textures->loadTexture(TN_GUI_INVENTORY);
    RenderPath.StateSetColour(1, 1, 1, 1);
    minecraft->textures->bind(tex);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    this->blit(xo, yo, 0, 0, imageWidth, imageHeight);

    (void)0;
    (void)0;

    RenderPath.MatrixPush();
    RenderPath.MatrixTranslate((float)xo + 51, (float)yo + 75, 50);
    float ss = 30;
    RenderPath.MatrixScale(-ss, ss, ss);
    RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

    float oybr = minecraft->player->yBodyRot;
    float oyr = minecraft->player->yRot;
    float oxr = minecraft->player->xRot;
    float oyh = minecraft->player->yHeadRot;
    float oyhp = minecraft->player->yHeadRotO;

    float xd = (xo + 51) - xMouse;
    float yd = (yo + 75 - 50) - yMouse;

    RenderPath.MatrixRotate((45 + 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    Lighting::turnOn();
    RenderPath.MatrixRotate((-45 - 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

    RenderPath.MatrixRotate((-(float)atan(yd / 40.0f) * 20)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

    minecraft->player->yBodyRot = (float)atan(xd / 40.0f) * 20;
    minecraft->player->yRot = (float)atan(xd / 40.0f) * 40;
    minecraft->player->xRot = -(float)atan(yd / 40.0f) * 20;
    minecraft->player->yHeadRot = (float)atan(xd / 40.0f) * 40;
    minecraft->player->yHeadRotO = (float)atan(xd / 40.0f) * 40;
    RenderPath.MatrixTranslate(0, minecraft->player->heightOffset, 0);
    EntityRenderDispatcher::instance->playerRotY = 180;
    EntityRenderDispatcher::instance->render(minecraft->player, 0, 0, 0, 0, 1);
    minecraft->player->yBodyRot = oybr;
    minecraft->player->yRot = oyr;
    minecraft->player->xRot = oxr;
    minecraft->player->yHeadRot = oyh;
    minecraft->player->yHeadRotO = oyhp;
    RenderPath.MatrixPop();
    Lighting::turnOff();
    (void)0;
#endif
}

void InventoryScreen::buttonClicked(Button* button) {
    if (button->id == 0) {
        minecraft->setScreen(new AchievementScreen(
            minecraft->stats[minecraft->player->GetXboxPad()]));
    }
    if (button->id == 1) {
        minecraft->setScreen(new StatsScreen(
            this, minecraft->stats[minecraft->player->GetXboxPad()]));
    }
}
