#include "InventoryScreen.h"
#include "platform/stubs.h"

#include <cmath>
#include <string>
#include <vector>

#include "platform/renderer/renderer.h"
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
    glColor4f(1, 1, 1, 1);
    minecraft->textures->bind(tex);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    this->blit(xo, yo, 0, 0, imageWidth, imageHeight);

    glEnable(GL_RESCALE_NORMAL);
    glEnable(GL_COLOR_MATERIAL);

    glPushMatrix();
    glTranslatef((float)xo + 51, (float)yo + 75, 50);
    float ss = 30;
    glScalef(-ss, ss, ss);
    glRotatef(180, 0, 0, 1);

    float oybr = minecraft->player->yBodyRot;
    float oyr = minecraft->player->yRot;
    float oxr = minecraft->player->xRot;
    float oyh = minecraft->player->yHeadRot;
    float oyhp = minecraft->player->yHeadRotO;

    float xd = (xo + 51) - xMouse;
    float yd = (yo + 75 - 50) - yMouse;

    glRotatef(45 + 90, 0, 1, 0);
    Lighting::turnOn();
    glRotatef(-45 - 90, 0, 1, 0);

    glRotatef(-(float)atan(yd / 40.0f) * 20, 1, 0, 0);

    minecraft->player->yBodyRot = (float)atan(xd / 40.0f) * 20;
    minecraft->player->yRot = (float)atan(xd / 40.0f) * 40;
    minecraft->player->xRot = -(float)atan(yd / 40.0f) * 20;
    minecraft->player->yHeadRot = (float)atan(xd / 40.0f) * 40;
    minecraft->player->yHeadRotO = (float)atan(xd / 40.0f) * 40;
    glTranslatef(0, minecraft->player->heightOffset, 0);
    EntityRenderDispatcher::instance->playerRotY = 180;
    EntityRenderDispatcher::instance->render(minecraft->player, 0, 0, 0, 0, 1);
    minecraft->player->yBodyRot = oybr;
    minecraft->player->yRot = oyr;
    minecraft->player->xRot = oxr;
    minecraft->player->yHeadRot = oyh;
    minecraft->player->yHeadRotO = oyhp;
    glPopMatrix();
    Lighting::turnOff();
    glDisable(GL_RESCALE_NORMAL);
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
