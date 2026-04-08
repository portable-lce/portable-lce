#include "BeaconScreen.h"
#include "platform/stubs.h"



#include <memory>
#include <string>
#include <vector>

#include "platform/renderer/renderer.h"
#include "BeaconCancelButton.h"
#include "BeaconConfirmButton.h"
#include "BeaconPowerButton.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Button.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractBeaconButton.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/entity/ItemRenderer.h"
#include "minecraft/locale/Language.h"
#include "minecraft/network/packet/CustomPayloadPacket.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/BeaconMenu.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/tile/entity/BeaconTileEntity.h"

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing
// container classes (and iggy too)
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_BEACON_LOCATION = ResourceLocation(TN_GUI_BEACON);
#endif

BeaconScreen::BeaconScreen(std::shared_ptr<Inventory> inventory,
                           std::shared_ptr<BeaconTileEntity> beacon)
    : AbstractContainerScreen(new BeaconMenu(inventory, beacon)) {
    this->inventory = inventory;
    this->beacon = beacon;
    this->beaconMenu = static_cast<BeaconMenu*>(menu);
    this->imageWidth = 230;
    this->imageHeight = 219;
    this->buttonsNotDrawn = true;
    this->beaconConfirmButton = nullptr;
}

BeaconScreen::~BeaconScreen() = default;

void BeaconScreen::init() {
    AbstractContainerScreen::init();

    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;

    beaconConfirmButton = new BeaconConfirmButton(this, -1, xo + 164, yo + 107);
    buttons.push_back(beaconConfirmButton);
    buttons.push_back(new BeaconCancelButton(this, -2, xo + 190, yo + 107));

    buttonsNotDrawn = true;
    beaconConfirmButton->active = false;
}

void BeaconScreen::tick() {
    if (buttonsNotDrawn && beacon->getLevels() >= 0) {
        buttonsNotDrawn = false;

        int xo = (width - imageWidth) / 2;
        int yo = (height - imageHeight) / 2;

        for (int tier = 0; tier <= 2; ++tier) {
            int effectCount = BeaconTileEntity::BEACON_EFFECTS_EFFECTS;
            int actualCount = 0;
            for (int e = 0; e < effectCount; ++e) {
                if (BeaconTileEntity::BEACON_EFFECTS[tier][e] != nullptr) {
                    actualCount++;
                } else {
                    break;
                }
            }

            int totalWidth = actualCount * 22 + (actualCount - 1) * 2;
            int startX = xo + 53 + (actualCount * 24 - totalWidth) / 2;

            for (int e = 0; e < actualCount; ++e) {
                MobEffect* effect = BeaconTileEntity::BEACON_EFFECTS[tier][e];
                if (effect == nullptr) break;

                int buttonId = (tier << 8) | effect->id;
                BeaconPowerButton* button = new BeaconPowerButton(
                    this, buttonId, startX + e * 24, yo + 22 + tier * 25,
                    effect->id, tier);
                buttons.push_back(button);

                if (tier >= beacon->getLevels()) {
                    button->active = false;
                } else if (effect->id == beacon->getPrimaryPower()) {
                    button->setSelected(true);
                }
            }
        }

        int tier = 3;
        int effectCount = BeaconTileEntity::BEACON_EFFECTS_EFFECTS;
        int actualCount = 0;
        for (int e = 0; e < effectCount; ++e) {
            if (BeaconTileEntity::BEACON_EFFECTS[tier][e] != nullptr) {
                actualCount++;
            } else {
                break;
            }
        }

        int totalWidth = (actualCount + 1) * 22 + actualCount * 2;
        int startX = xo + 143 + ((actualCount + 1) * 24 - totalWidth) / 2;

        for (int e = 0; e < actualCount; ++e) {
            MobEffect* effect = BeaconTileEntity::BEACON_EFFECTS[tier][e];
            if (effect == nullptr) break;

            int buttonId = (tier << 8) | effect->id;
            BeaconPowerButton* button = new BeaconPowerButton(
                this, buttonId, startX + e * 24, yo + 47, effect->id, tier);
            buttons.push_back(button);

            if (tier >= beacon->getLevels()) {
                button->active = false;
            } else if (effect->id == beacon->getSecondaryPower()) {
                button->setSelected(true);
            }
        }

        if (beacon->getPrimaryPower() > 0) {
            int buttonId = (tier << 8) | beacon->getPrimaryPower();
            BeaconPowerButton* button =
                new BeaconPowerButton(this, buttonId, startX + actualCount * 24,
                                      yo + 47, beacon->getPrimaryPower(), tier);
            buttons.push_back(button);

            if (tier >= beacon->getLevels()) {
                button->active = false;
            } else if (beacon->getPrimaryPower() ==
                       beacon->getSecondaryPower()) {
                button->setSelected(true);
            }
        }
    }

    beaconConfirmButton->active =
        (beacon->getItem(0) != nullptr && beacon->getPrimaryPower() > 0);
}

void BeaconScreen::removed() { AbstractContainerScreen::removed(); }

void BeaconScreen::renderLabels() {
    std::string primaryLabel =
        Language::getInstance()->getElement("tile.beacon.primary");
    font->drawShadow(primaryLabel, 25, 10, 0xE1E1E1);

    std::string secondaryLabel =
        Language::getInstance()->getElement("tile.beacon.secondary");
    font->drawShadow(secondaryLabel, 125, 10, 0xE1E1E1);
}

void BeaconScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    minecraft->textures->bindTexture(&GUI_BEACON_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    // Render payment item icons
    itemRenderer->renderGuiItem(
        font, minecraft->textures,
        std::make_shared<ItemInstance>(Item::emerald_Id, 1, 0), xo + 42,
        yo + 109);
    itemRenderer->renderGuiItem(
        font, minecraft->textures,
        std::make_shared<ItemInstance>(Item::diamond_Id, 1, 0), xo + 42 + 22,
        yo + 109);
    itemRenderer->renderGuiItem(font, minecraft->textures,
                                std::shared_ptr<ItemInstance>(
                                    new ItemInstance(Item::goldIngot_Id, 1, 0)),
                                xo + 42 + 44, yo + 109);
    itemRenderer->renderGuiItem(font, minecraft->textures,
                                std::shared_ptr<ItemInstance>(
                                    new ItemInstance(Item::ironIngot_Id, 1, 0)),
                                xo + 42 + 66, yo + 109);
#endif
}

void BeaconScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);
    for (Button* button : buttons) {
        AbstractBeaconButton* beaconButton =
            dynamic_cast<AbstractBeaconButton*>(button);
        if (beaconButton && beaconButton->isHovered()) {
            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            beaconButton->renderTooltip(xm, ym);
            glEnable(GL_LIGHTING);
            glEnable(GL_DEPTH_TEST);
            break;
        }
    }
}

void BeaconScreen::buttonClicked(Button* button) {
    if (button->id == -2) {
        minecraft->player->closeContainer();
    } else if (button->id == -1) {
        // 4jcraft: copied from IUIScene_BeaconMenu
        ByteArrayOutputStream baos;
        DataOutputStream dos(&baos);
        dos.writeInt(beacon->getPrimaryPower());
        dos.writeInt(beacon->getSecondaryPower());

        minecraft->player->connection->send(
            std::make_shared<CustomPayloadPacket>(
                CustomPayloadPacket::SET_BEACON_PACKET, baos.toByteArray()));
        minecraft->player->closeContainer();
    } else if (dynamic_cast<BeaconPowerButton*>(button)) {
        int effectId = button->id & 255;
        int tier = button->id >> 8;

        if (tier < 3) {
            beacon->setPrimaryPower(effectId);
        } else {
            beacon->setSecondaryPower(effectId);
        }

        for (Button* btn : buttons) {
            delete btn;
        }
        buttons.clear();
        init();
        tick();
    }
}