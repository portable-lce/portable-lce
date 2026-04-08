#include "HopperScreen.h"
#include "platform/stubs.h"

#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/HopperMenu.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/renderer/Textures.h"

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing
// container classes
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_HOPPER_LOCATION = ResourceLocation(TN_GUI_HOPPER);
#endif

HopperScreen::HopperScreen(std::shared_ptr<Inventory> inventory,
                           std::shared_ptr<Container> hopper)
    : AbstractContainerScreen(new HopperMenu(inventory, hopper)) {
    this->hopper = hopper;
    this->inventory = inventory;
    imageHeight = 133;
}

void HopperScreen::renderLabels() {
    font->draw(hopper->getName(), 8, 6, 0x404040);
    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);
}

void HopperScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1, 1, 1, 1);
    minecraft->textures->bindTexture(&GUI_HOPPER_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    this->blit(xo, yo, 0, 0, imageWidth, imageHeight);
#endif
}