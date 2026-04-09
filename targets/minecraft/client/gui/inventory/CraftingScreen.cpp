#include "CraftingScreen.h"

#include <string>

#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/locale/Language.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/inventory/CraftingMenu.h"
#include "platform/stubs.h"

class Player;

#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_CRAFTING_LOCATION = ResourceLocation(TN_GUI_CRAFTING);
#endif

CraftingScreen::CraftingScreen(std::shared_ptr<Inventory> inventory,
                               Level* level, int x, int y, int z)
    : AbstractContainerScreen(new CraftingMenu(inventory, level, x, y, z)) {
    this->inventory = inventory;
}

void CraftingScreen::removed() {
    AbstractContainerScreen::removed();
    menu->removed(std::dynamic_pointer_cast<Player>(minecraft->player));
}

void CraftingScreen::renderLabels() {
    font->draw(Language::getInstance()->getElement("container.crafting"),
               8 + 16 + 4, 2 + 2 + 2, 0x404040);
    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);
}

void CraftingScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1, 1, 1, 1);
    minecraft->textures->bindTexture(&GUI_CRAFTING_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    this->blit(xo, yo, 0, 0, imageWidth, imageHeight);
#endif
}