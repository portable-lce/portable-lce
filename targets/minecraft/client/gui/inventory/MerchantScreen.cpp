#include "MerchantScreen.h"
#include "platform/stubs.h"

#include <memory>
#include <string>
#include <vector>


#include "AbstractContainerScreen.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/TradeSwitchButton.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/entity/ItemRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/network/packet/CustomPayloadPacket.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/inventory/MerchantContainer.h"
#include "minecraft/world/inventory/MerchantMenu.h"
#include "minecraft/world/item/trading/Merchant.h"
#include "minecraft/world/item/trading/MerchantRecipeList.h"
#include "minecraft/world/item/trading/MerchantRecipe.h"

class Level;

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing
// container classes (and iggy too)
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_VILLAGER_LOCATION = ResourceLocation(TN_GUI_VILLAGER);
#endif

MerchantScreen::MerchantScreen(std::shared_ptr<Inventory> inventory,
                               std::shared_ptr<Merchant> merchant, Level* level)
    : AbstractContainerScreen(new MerchantMenu(inventory, merchant, level)) {
    this->inventory = inventory;
    this->merchantMenu = static_cast<MerchantMenu*>(menu);
    this->merchant = merchant;
    this->currentRecipeIndex = 0;
    this->nextRecipeButton = nullptr;
    this->prevRecipeButton = nullptr;
}

MerchantScreen::~MerchantScreen() = default;

void MerchantScreen::init() {
    AbstractContainerScreen::init();

    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;

    nextRecipeButton =
        new TradeSwitchButton(1, xo + 120 + 27, yo + 24 - 1, true);
    prevRecipeButton =
        new TradeSwitchButton(2, xo + 36 - 19, yo + 24 - 1, false);

    nextRecipeButton->active = false;
    prevRecipeButton->active = false;

    buttons.push_back(nextRecipeButton);
    buttons.push_back(prevRecipeButton);
}

void MerchantScreen::removed() { AbstractContainerScreen::removed(); }

void MerchantScreen::renderLabels() {
    font->draw(merchant->getDisplayName(),
               (imageWidth / 2) - (font->width(merchant->getDisplayName()) / 2),
               6, 0x404040);

    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);
}

void MerchantScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    minecraft->textures->bindTexture(&GUI_VILLAGER_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    MerchantRecipe* activeRecipe =
        merchantMenu->getTradeContainer()->getActiveRecipe();
    if (activeRecipe != nullptr && activeRecipe->isDeprecated()) {
        blit(xo + 83, yo + 21, 212, 0, 28, 21);
        blit(xo + 83, yo + 51, 212, 0, 28, 21);
    }
#endif
}

void MerchantScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);

#ifdef ENABLE_JAVA_GUIS
    std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(
        inventory->player->shared_from_this());
    MerchantRecipeList* offers = merchant->getOffers(player);

    if (offers != nullptr && !offers->empty()) {
        int xo = (width - imageWidth) / 2;
        int yo = (height - imageHeight) / 2;

        MerchantRecipe* recipe = offers->at(currentRecipeIndex);
        if (recipe != nullptr && !recipe->isDeprecated()) {
            std::shared_ptr<ItemInstance> buyItem1 = recipe->getBuyAItem();
            std::shared_ptr<ItemInstance> buyItem2 = recipe->getBuyBItem();
            std::shared_ptr<ItemInstance> sellItem = recipe->getSellItem();

            glPushMatrix();
            glTranslatef((float)xo, (float)yo, 0.0f);

            Lighting::turnOn();
            glEnable(GL_RESCALE_NORMAL);
            glEnable(GL_LIGHTING);

            if (buyItem1 != nullptr) {
                itemRenderer->renderGuiItem(font, minecraft->textures, buyItem1,
                                            36, 24);
                itemRenderer->renderGuiItemDecorations(
                    font, minecraft->textures, buyItem1, 36, 24);
            }

            if (buyItem2 != nullptr) {
                itemRenderer->renderGuiItem(font, minecraft->textures, buyItem2,
                                            62, 24);
                itemRenderer->renderGuiItemDecorations(
                    font, minecraft->textures, buyItem2, 62, 24);
            }

            if (sellItem != nullptr) {
                itemRenderer->renderGuiItem(font, minecraft->textures, sellItem,
                                            120, 24);
                itemRenderer->renderGuiItemDecorations(
                    font, minecraft->textures, sellItem, 120, 24);
            }

            glDisable(GL_LIGHTING);
            glDisable(GL_RESCALE_NORMAL);
            Lighting::turnOff();

            glPopMatrix();

            if (buyItem1 != nullptr && isHoveringOver(36, 24, 16, 16, xm, ym)) {
                renderTooltip(buyItem1, xm, ym);
            } else if (buyItem2 != nullptr &&
                       isHoveringOver(62, 24, 16, 16, xm, ym)) {
                renderTooltip(buyItem2, xm, ym);
            } else if (sellItem != nullptr &&
                       isHoveringOver(120, 24, 16, 16, xm, ym)) {
                renderTooltip(sellItem, xm, ym);
            }
        }
    }
#endif
}

void MerchantScreen::tick() {
    AbstractContainerScreen::tick();

    std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(
        inventory->player->shared_from_this());

    MerchantRecipeList* offers = merchant->getOffers(player);

    if (offers != nullptr) {
        int offerCount = (int)offers->size();

        nextRecipeButton->active = (currentRecipeIndex < offerCount - 1);
        prevRecipeButton->active = (currentRecipeIndex > 0);

        if (currentRecipeIndex >= offerCount && offerCount > 0) {
            currentRecipeIndex = offerCount - 1;
            merchantMenu->setSelectionHint(currentRecipeIndex);

            // 4jcraft: taken from IUIScene_TradingMenu
            ByteArrayOutputStream rawOutput;
            DataOutputStream output(&rawOutput);
            output.writeInt(currentRecipeIndex);
            minecraft->player->connection->send(
                std::make_shared<CustomPayloadPacket>(
                    CustomPayloadPacket::TRADER_SELECTION_PACKET,
                    rawOutput.toByteArray()));
        }
    } else {
        nextRecipeButton->active = false;
        prevRecipeButton->active = false;
    }
}

void MerchantScreen::buttonClicked(Button* button) {
    bool changed = false;

    if (button == nextRecipeButton) {
        ++currentRecipeIndex;
        changed = true;
    } else if (button == prevRecipeButton) {
        --currentRecipeIndex;
        changed = true;
    }

    if (changed) {
        merchantMenu->setSelectionHint(currentRecipeIndex);

        // 4jcraft: taken from IUIScene_TradingMenu
        ByteArrayOutputStream rawOutput;
        DataOutputStream output(&rawOutput);
        output.writeInt(currentRecipeIndex);
        minecraft->player->connection->send(
            std::make_shared<CustomPayloadPacket>(
                CustomPayloadPacket::TRADER_SELECTION_PACKET,
                rawOutput.toByteArray()));
    }
}