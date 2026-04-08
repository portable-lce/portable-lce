#include "FurnaceScreen.h"
#include "platform/stubs.h"

#include <string>

#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/FurnaceMenu.h"
#include "minecraft/world/level/tile/entity/FurnaceTileEntity.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/client/Minecraft.h"

#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_FURNACE_LOCATION = ResourceLocation(TN_GUI_FURNACE);
#endif

FurnaceScreen::FurnaceScreen(std::shared_ptr<Inventory> inventory,
                             std::shared_ptr<FurnaceTileEntity> furnace)
    : AbstractContainerScreen(new FurnaceMenu(inventory, furnace)) {
    this->inventory = inventory;
    this->furnace = furnace;
}

void FurnaceScreen::renderLabels() {
    font->draw(furnace->getName(), 16 + 4 + 40, 2 + 2 + 2, 0x404040);
    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);
}

void FurnaceScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1, 1, 1, 1);
    minecraft->textures->bindTexture(&GUI_FURNACE_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    this->blit(xo, yo, 0, 0, imageWidth, imageHeight);
    if (furnace->isLit()) {
        int p = furnace->getLitProgress(12);
        this->blit(xo + 56, yo + 36 + 12 - p, 176, 12 - p, 14, p + 2);
    }

    int p = furnace->getBurnProgress(24);
    this->blit(xo + 79, yo + 34, 176, 14, p + 1, 16);
#endif
}