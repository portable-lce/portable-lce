#include "HorseInventoryScreen.h"

#include <cmath>
#include <numbers>
#include <string>

#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/animal/EntityHorse.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/HorseInventoryMenu.h"
#include "platform/stubs.h"

class EntityHorse;

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing InventoryScreen
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_HORSE_LOCATION = ResourceLocation(TN_GUI_HORSE);
#endif

HorseInventoryScreen::HorseInventoryScreen(
    std::shared_ptr<Inventory> inventory,
    std::shared_ptr<Container> horseContainer,
    std::shared_ptr<EntityHorse> horse)
    : AbstractContainerScreen(
          new HorseInventoryMenu(inventory, horseContainer, horse)) {
    xMouse = yMouse = 0.0f;  // 4J added

    this->inventory = inventory;
    this->horseContainer = horseContainer;
    this->horse = horse;
    this->passEvents = false;
}

void HorseInventoryScreen::init() { AbstractContainerScreen::init(); }

void HorseInventoryScreen::renderLabels() {
    font->draw(horseContainer->getName(), 8, 6, 0x404040);
    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);
}

void HorseInventoryScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);
    this->xMouse = (float)xm;
    this->yMouse = (float)ym;
}

void HorseInventoryScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    RenderPath.StateSetColour(1, 1, 1, 1);
    minecraft->textures->bindTexture(&GUI_HORSE_LOCATION);

    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    if (horse->isChestedHorse()) {
        blit(xo + 79, yo + 17, 0, imageHeight, 90, 54);
    }

    if (horse->canWearArmor()) {
        blit(xo + 7, yo + 35, 0, imageHeight + 54, 18, 18);
    }

    (void)0;
    (void)0;

    RenderPath.MatrixPush();
    RenderPath.MatrixTranslate((float)xo + 51, (float)yo + 60, 50);
    float ss = 30;
    RenderPath.MatrixScale(-ss, ss, ss);
    RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

    float oybr = horse->yBodyRot;
    float oyr = horse->yRot;
    float oxr = horse->xRot;
    float oyh = horse->yHeadRot;
    float oyhp = horse->yHeadRotO;

    float xd = (xo + 51) - xMouse;
    float yd = (yo + 75 - 50) - yMouse;

    RenderPath.MatrixRotate((45 + 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    Lighting::turnOn();
    RenderPath.MatrixRotate((-45 - 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

    RenderPath.MatrixRotate((-(float)atan(yd / 40.0f) * 20)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

    horse->yBodyRot = (float)atan(xd / 40.0f) * 20;
    horse->yRot = (float)atan(xd / 40.0f) * 40;
    horse->xRot = -(float)atan(yd / 40.0f) * 20;
    horse->yHeadRot = (float)atan(xd / 40.0f) * 40;
    horse->yHeadRotO = (float)atan(xd / 40.0f) * 40;
    RenderPath.MatrixTranslate(0, horse->heightOffset, 0);
    EntityRenderDispatcher::instance->playerRotY = 180;
    EntityRenderDispatcher::instance->render(horse, 0, 0, 0, 0, 1);
    horse->yBodyRot = oybr;
    horse->yRot = oyr;
    horse->xRot = oxr;
    horse->yHeadRot = oyh;
    horse->yHeadRotO = oyhp;
    RenderPath.MatrixPop();
    Lighting::turnOff();
    (void)0;
#endif
}