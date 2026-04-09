#include "RepairScreen.h"

#include <memory>
#include <string>

#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/EditBox.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/inventory/AbstractContainerScreen.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/locale/Language.h"
#include "minecraft/network/packet/CustomPayloadPacket.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/inventory/AnvilMenu.h"
#include "minecraft/world/inventory/Slot.h"
#include "minecraft/world/item/ItemInstance.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

class Inventory;
class Level;

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing
// IUIScene_AnvilMenu (from iggy UI)
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_ANVIL_LOCATION = ResourceLocation(TN_GUI_ANVIL);
#endif

RepairScreen::RepairScreen(std::shared_ptr<Inventory> inventory, Level* level,
                           int x, int y, int z)
    : AbstractContainerScreen(
          new AnvilMenu(inventory, level, x, y, z,
                        Minecraft::GetInstance()->localplayers[0])) {
    this->inventory = inventory;
    this->level = level;
    this->repairMenu = static_cast<AnvilMenu*>(menu);
    this->passEvents = false;
}

RepairScreen::~RepairScreen() = default;

void RepairScreen::init() {
    AbstractContainerScreen::init();

    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    editName = new EditBox(this, font, xo + 62, yo + 24, 103, 12, "");
    editName->setMaxLength(40);
    editName->setEnableBackgroundDrawing(false);
    editName->inFocus = true;

    repairMenu->removeSlotListener(this);
    repairMenu->addSlotListener(this);
}

void RepairScreen::removed() {
    AbstractContainerScreen::removed();
    repairMenu->removeSlotListener(this);
}

void RepairScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);
    glDisable(GL_LIGHTING);
    if (editName) {
        editName->render();
    }
}

void RepairScreen::renderLabels() {
    std::string title = Language::getInstance()->getElement("container.repair");
    font->draw(title, 60, 6, 0x404040);

    if (repairMenu->cost > 0) {
        int textColor = 0x80ff20;
        bool showCost = true;
        std::string costString;

        if (repairMenu->cost >= 40 &&
            !Minecraft::GetInstance()->localplayers[0]->abilities.instabuild) {
            costString = Language::getInstance()->getElement(
                "container.repair.expensive");
            textColor = 0xff6060;
        } else if (!repairMenu->getSlot(AnvilMenu::RESULT_SLOT)->hasItem()) {
            showCost = false;
        } else if (!repairMenu->getSlot(AnvilMenu::RESULT_SLOT)
                        ->mayPickup(
                            Minecraft::GetInstance()->localplayers[0])) {
            textColor = 0xff6060;
        }

        if (showCost) {
            if (costString.empty()) {
                costString = Language::getInstance()->getElement(
                    "container.repair.cost", repairMenu->cost);
            }

            int shadowColor = -0x00ffffff | ((textColor & 0xfcfcfc) >> 2) |
                              (textColor & -0x00ffffff);
            int costX = imageWidth - 8 - font->width(costString);
            int costY = 67;

            // if (this.fontRenderer.getUnicodeFlag())
            // {
            //     drawRect(i1 - 3, b0 - 2, this.xSize - 7, b0 + 10, -16777216);
            //     drawRect(i1 - 2, b0 - 1, this.xSize - 8, b0 + 9, -12895429);
            // }
            // else
            // {
            font->draw(costString, costX, costY + 1, shadowColor);
            font->draw(costString, costX + 1, costY, shadowColor);
            font->draw(costString, costX + 1, costY + 1, shadowColor);
            font->draw(costString, costX, costY, textColor);
            // }
        }
    }
}

void RepairScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    Minecraft::GetInstance()->textures->bindTexture(&GUI_ANVIL_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    int texV = imageHeight + (repairMenu->getSlot(0)->hasItem() ? 0 : 16);
    blit(xo + 59, yo + 20, 0, texV, 110, 16);

    if ((repairMenu->getSlot(AnvilMenu::INPUT_SLOT)->hasItem() ||
         repairMenu->getSlot(AnvilMenu::ADDITIONAL_SLOT)->hasItem()) &&
        !repairMenu->getSlot(AnvilMenu::RESULT_SLOT)->hasItem()) {
        blit(xo + 99, yo + 45, imageWidth, 0, 28, 21);
    }
#endif
}

void RepairScreen::keyPressed(char ch, int eventKey) {
    if (editName) {
        editName->keyPressed(ch, eventKey);
        updateItemName();
    } else {
        AbstractContainerScreen::keyPressed(ch, eventKey);
    }
}

void RepairScreen::mouseClicked(int mouseX, int mouseY, int buttonNum) {
    AbstractContainerScreen::mouseClicked(mouseX, mouseY, buttonNum);
    if (editName) {
        editName->mouseClicked(mouseX, mouseY, buttonNum);
    }
}

void RepairScreen::updateItemName() {
    std::string itemName;
    Slot* slot = repairMenu->getSlot(0);
    if (slot != nullptr && slot->hasItem()) {
        if (!slot->getItem()->hasCustomHoverName() &&
            itemName == slot->getItem()->getHoverName()) {
            itemName = "";
        }
    }

    repairMenu->setItemName(itemName);

    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);
    dos.writeUTF(itemName);
    Minecraft::GetInstance()->player->connection->send(
        std::make_shared<CustomPayloadPacket>(
            CustomPayloadPacket::SET_ITEM_NAME_PACKET, baos.toByteArray()));
}

// 4jcraft: these 3 are to implement Containerlistener (see IUIScene_AnvilMenu
// and net.minecraft.world.inventory.ContainerListener)
void RepairScreen::refreshContainer(
    AbstractContainerMenu* container,
    std::vector<std::shared_ptr<ItemInstance> >* items) {
    slotChanged(container, AnvilMenu::INPUT_SLOT,
                container->getSlot(0)->getItem());
}

void RepairScreen::slotChanged(AbstractContainerMenu* container, int slotIndex,
                               std::shared_ptr<ItemInstance> item) {
    if (slotIndex == AnvilMenu::INPUT_SLOT) {
        std::string itemName = item == nullptr ? "" : item->getHoverName();
        editName->setValue(itemName);
        if (item != nullptr) {
            editName->focus(true);
            updateItemName();
        } else {
            editName->focus(false);
        }
    }
}

void RepairScreen::setContainerData(AbstractContainerMenu* container, int id,
                                    int value) {}