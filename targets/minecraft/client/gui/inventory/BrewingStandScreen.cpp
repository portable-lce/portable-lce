#include "BrewingStandScreen.h"
#include "platform/stubs.h"

#include <memory>


#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/BrewingStandMenu.h"
#include "minecraft/world/level/tile/entity/BrewingStandTileEntity.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/client/Minecraft.h"

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing
// container classes
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_BREWING_STAND_LOCATION =
    ResourceLocation(TN_GUI_BREWING_STAND);
#endif

BrewingStandScreen::BrewingStandScreen(
    std::shared_ptr<Inventory> inventory,
    std::shared_ptr<BrewingStandTileEntity> brewingStand)
    : AbstractContainerScreen(new BrewingStandMenu(inventory, brewingStand)) {
    this->inventory = inventory;
    this->brewingStand = brewingStand;
    this->brewMenu = static_cast<BrewingStandMenu*>(menu);
}

BrewingStandScreen::~BrewingStandScreen() = default;

void BrewingStandScreen::init() { AbstractContainerScreen::init(); }

void BrewingStandScreen::removed() { AbstractContainerScreen::removed(); }

void BrewingStandScreen::renderLabels() {
    font->draw(brewingStand->getName(),
               (imageWidth / 2) - (font->width(brewingStand->getName()) / 2), 6,
               0x404040);
    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);
}

void BrewingStandScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    Minecraft::GetInstance()->textures->bindTexture(
        &GUI_BREWING_STAND_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    int brewTime = brewingStand->getBrewTime();

    if (brewTime > 0) {
        int arrowHeight = (int)(28.0f * (1.0f - (float)brewTime / 400.0f));

        if (arrowHeight > 0) {
            blit(xo + 97, yo + 16 + (28 - arrowHeight), 176, 28 - arrowHeight,
                 9, arrowHeight);
        }

        int bubbleStep = (brewTime / 2) % 7;
        int bubbleHeight = 0;

        switch (bubbleStep) {
            case 0:
                bubbleHeight = 29;
                break;
            case 1:
                bubbleHeight = 24;
                break;
            case 2:
                bubbleHeight = 20;
                break;
            case 3:
                bubbleHeight = 16;
                break;
            case 4:
                bubbleHeight = 11;
                break;
            case 5:
                bubbleHeight = 6;
                break;
            case 6:
                bubbleHeight = 0;
                break;
        }

        if (bubbleHeight > 0) {
            blit(xo + 65, yo + 14 + (29 - bubbleHeight), 185, 29 - bubbleHeight,
                 12, bubbleHeight);
        }
    }
#endif
}

void BrewingStandScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);
}