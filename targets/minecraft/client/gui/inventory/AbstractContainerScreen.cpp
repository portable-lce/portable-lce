#include "AbstractContainerScreen.h"

#include <wchar.h>

#include <vector>

#include "minecraft/IGameServices.h"
#include "minecraft/client/KeyMapping.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/entity/ItemRenderer.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/inventory/Slot.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/Rarity.h"
#include "platform/stubs.h"

ItemRenderer* AbstractContainerScreen::itemRenderer = new ItemRenderer();

AbstractContainerScreen::AbstractContainerScreen(AbstractContainerMenu* menu) {
    // 4J - added initialisers
    imageWidth = 176;
    imageHeight = 166;

    this->menu = menu;
}

void AbstractContainerScreen::init() {
    Screen::init();
    minecraft->player->containerMenu = menu;
    // 	leftPos = (width - imageWidth) / 2;
    // 	topPos = (height - imageHeight) / 2;
}

void AbstractContainerScreen::render(int xm, int ym, float a) {
    // 4J Stu - Not used
#ifdef ENABLE_JAVA_GUIS
    renderBackground();
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;

    renderBg(a);

    glPushMatrix();
    glRotatef(120, 1, 0, 0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef((float)xo, (float)yo, 0);

    glColor4f(1, 1, 1, 1);
    glEnable(GL_RESCALE_NORMAL);
    Lighting::turnOn();

    Slot* hoveredSlot = nullptr;

    auto itEnd = menu->slots.end();
    for (auto it = menu->slots.begin(); it != itEnd; it++) {
        Slot* slot = *it;  // menu->slots.at(i);

        renderSlot(slot);

        if (isHovering(slot, xm, ym)) {
            hoveredSlot = slot;

            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);

            int x = slot->x;
            int y = slot->y;
            fillGradient(x, y, x + 16, y + 16, 0x80ffffff, 0x80ffffff);
            glEnable(GL_LIGHTING);
            glEnable(GL_DEPTH_TEST);
        }
    }

    std::shared_ptr<Inventory> inventory = minecraft->player->inventory;
    if (inventory->getCarried() != nullptr) {
        glTranslatef(0, 0, 32);
        // Slot old = carriedSlot;
        // carriedSlot = null;
        itemRenderer->renderGuiItem(font, minecraft->textures,
                                    inventory->getCarried(), xm - xo - 8,
                                    ym - yo - 8);
        itemRenderer->renderGuiItemDecorations(font, minecraft->textures,
                                               inventory->getCarried(),
                                               xm - xo - 8, ym - yo - 8);
        // carriedSlot = old;
    }
    Lighting::turnOff();
    glDisable(GL_RESCALE_NORMAL);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    renderLabels();

    // 4jcraft: newer tooltips backported from java edition 1.3.x (MCP 7.x)
    if (inventory->getCarried() == nullptr && hoveredSlot != nullptr &&
        hoveredSlot->hasItem()) {
        std::shared_ptr<ItemInstance> item = hoveredSlot->getItem();

        int xo = (width - imageWidth) / 2;
        int yo = (height - imageHeight) / 2;

        // 4jcraft: abstracted tooltip rendering into a new method
        renderTooltip(item, xm - xo, ym - yo);
    }

    glPopMatrix();

    Screen::render(xm, ym, a);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
#endif
}

// 4jcraft: extracted from render() into a standalone method so this can be used
// in other derived classes
// update: also added 1.6.x era overloads (for the creative inventory and other
// places)
void AbstractContainerScreen::renderTooltipInternal(
    const std::vector<std::string>& cleanedLines,
    const std::vector<int>& lineColors, int xm, int ym) {
    if (cleanedLines.empty()) return;

    int tooltipWidth = 0;
    for (const auto& line : cleanedLines) {
        int lineWidth = font->width(line);
        if (lineWidth > tooltipWidth) tooltipWidth = lineWidth;
    }

    int tooltipX = xm + 12;
    int tooltipY = ym - 12;
    int tooltipHeight = 8;

    if (cleanedLines.size() > 1) {
        tooltipHeight += 2 + (cleanedLines.size() - 1) * 10;
    }

    int bgColor = 0xf0100010;
    fillGradient(tooltipX - 3, tooltipY - 4, tooltipX + tooltipWidth + 3,
                 tooltipY - 3, bgColor, bgColor);
    fillGradient(tooltipX - 3, tooltipY + tooltipHeight + 3,
                 tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 4,
                 bgColor, bgColor);
    fillGradient(tooltipX - 3, tooltipY - 3, tooltipX + tooltipWidth + 3,
                 tooltipY + tooltipHeight + 3, bgColor, bgColor);
    fillGradient(tooltipX - 4, tooltipY - 3, tooltipX - 3,
                 tooltipY + tooltipHeight + 3, bgColor, bgColor);
    fillGradient(tooltipX + tooltipWidth + 3, tooltipY - 3,
                 tooltipX + tooltipWidth + 4, tooltipY + tooltipHeight + 3,
                 bgColor, bgColor);

    int borderStart = 0x505000ff;
    int borderFinish = (borderStart & 0xfefefe) >> 1 | borderStart & 0xff000000;
    fillGradient(tooltipX - 3, (tooltipY - 3) + 1, (tooltipX - 3) + 1,
                 (tooltipY + tooltipHeight + 3) - 1, borderStart, borderFinish);
    fillGradient(tooltipX + tooltipWidth + 2, (tooltipY - 3) + 1,
                 tooltipX + tooltipWidth + 3,
                 (tooltipY + tooltipHeight + 3) - 1, borderStart, borderFinish);
    fillGradient(tooltipX - 3, tooltipY - 3, tooltipX + tooltipWidth + 3,
                 (tooltipY - 3) + 1, borderStart, borderStart);
    fillGradient(tooltipX - 3, tooltipY + tooltipHeight + 2,
                 tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 3,
                 borderFinish, borderFinish);

    int currentY = tooltipY;
    for (size_t lineIndex = 0; lineIndex < cleanedLines.size(); ++lineIndex) {
        const std::string& currentLine = cleanedLines[lineIndex];
        int textColor = lineColors[lineIndex];

        font->drawShadow(currentLine, tooltipX, currentY, textColor);

        if (lineIndex == 0) {
            currentY += 2;
        }
        currentY += 10;
    }
}

void AbstractContainerScreen::renderTooltip(std::shared_ptr<ItemInstance> item,
                                            int xm, int ym) {
    if (item == nullptr) return;

    std::vector<std::string> elementName;
    std::vector<std::string>* tooltipLines =
        item->getHoverText(minecraft->player, false, elementName);

    if (tooltipLines != nullptr && tooltipLines->size() > 0) {
        std::vector<std::string> cleanedLines;
        std::vector<int> lineColors;

        for (int lineIndex = 0; lineIndex < (int)tooltipLines->size();
             ++lineIndex) {
            std::string rawLine = (*tooltipLines)[lineIndex];
            std::string clean = "";
            int lineColor = 0xffffffff;

            // 4jcraft: LCE is using HTML font elements for its tooltip
            // colors, so make sure to parse them for parity w iggy UI
            //
            // examples would be enchantment books, potions and music
            // discs
            size_t fontPos = rawLine.find("<font");
            if (fontPos != std::string::npos) {
                size_t colorPos = rawLine.find("color=\"", fontPos);
                if (colorPos != std::string::npos) {
                    colorPos += 7;
                    size_t colorEnd = rawLine.find('"', colorPos);
                    if (colorEnd != std::string::npos) {
                        std::string colorStr =
                            rawLine.substr(colorPos, colorEnd - colorPos);
                        if (!colorStr.empty() && colorStr[0] == '#') {
                            colorStr = colorStr.substr(1);
                        }
                        if (!colorStr.empty()) {
                            char* endPtr;
                            long hexColor =
                                strtol(colorStr.c_str(), &endPtr, 16);
                            if (*endPtr == '\0') {
                                lineColor = 0xff000000 | (int)hexColor;
                            }
                        }
                    }
                }
            }

            bool inTag = false;
            for (char currentChar : rawLine) {
                if (currentChar == '<') {
                    inTag = true;
                } else if (currentChar == '>') {
                    inTag = false;
                } else if (!inTag) {
                    clean += currentChar;
                }
            }

            cleanedLines.push_back(clean);
            lineColors.push_back(lineColor);
        }

        if (!cleanedLines.empty()) {
            lineColors[0] =
                gameServices().getHTMLColour(item->getRarity()->color);
        }

        renderTooltipInternal(cleanedLines, lineColors, xm, ym);
    }
}

void AbstractContainerScreen::renderTooltip(
    const std::vector<std::string>& lines, int xm, int ym) {
    if (lines.empty()) return;

    std::vector<std::string> cleanedLines = lines;
    std::vector<int> lineColors;
    lineColors.reserve(lines.size());

    for (size_t i = 0; i < lines.size(); ++i) {
        if (i == 0) {
            lineColors.push_back(0xffffffff);
        } else {
            lineColors.push_back(0xffaaaaaa);
        }
    }

    renderTooltipInternal(cleanedLines, lineColors, xm, ym);
}

void AbstractContainerScreen::renderTooltip(const std::string& line, int xm,
                                            int ym) {
    renderTooltip(std::vector<std::string>{line}, xm, ym);
}

void AbstractContainerScreen::renderLabels() {}

void AbstractContainerScreen::renderSlot(Slot* slot) {
#if ENABLE_JAVA_GUIS
    int x = slot->x;
    int y = slot->y;
    std::shared_ptr<ItemInstance> item = slot->getItem();

    // if (item == nullptr)
    // {
    //     int icon = slot->getNoItemIcon();
    //     if (icon >= 0)
    // 	{
    //         glDisable(GL_LIGHTING);
    //         minecraft->textures->bind(minecraft->textures->loadTexture(TN_GUI_ITEMS));//"/gui/items.png"));
    //         blit(x, y, icon % 16 * 16, icon / 16 * 16, 16, 16);
    //         glEnable(GL_LIGHTING);
    //         return;
    //     }
    // }

    if (item == nullptr) {
        return;
    }

    itemRenderer->renderGuiItem(font, minecraft->textures, item, x, y);
    itemRenderer->renderGuiItemDecorations(font, minecraft->textures, item, x,
                                           y);
#endif
}

Slot* AbstractContainerScreen::findSlot(int x, int y) {
    auto itEnd = menu->slots.end();
    for (auto it = menu->slots.begin(); it != itEnd; it++) {
        Slot* slot = *it;  // menu->slots.at(i);
        if (isHovering(slot, x, y)) return slot;
    }
    return nullptr;
}

// 4jcraft: equivalent to MCP 8.11 (1.6.x)'s GuiContainer.isPointInRegion() for
// use in other derived classes
bool AbstractContainerScreen::isHoveringOver(int x, int y, int w, int h, int xm,
                                             int ym) {
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    xm -= xo;
    ym -= yo;

    return xm >= x - 1 && xm < x + w + 1 && ym >= y - 1 && ym < y + h + 1;
}

bool AbstractContainerScreen::isHovering(Slot* slot, int xm, int ym) {
    return isHoveringOver(slot->x, slot->y, 16, 16, xm, ym);
}

void AbstractContainerScreen::mouseClicked(int x, int y, int buttonNum) {
    Screen::mouseClicked(x, y, buttonNum);
    if (buttonNum == 0 || buttonNum == 1) {
        Slot* slot = findSlot(x, y);

        int xo = (width - imageWidth) / 2;
        int yo = (height - imageHeight) / 2;
        bool clickedOutside =
            (x < xo || y < yo || x >= xo + imageWidth || y >= yo + imageHeight);

        int slotId = -1;
        if (slot != nullptr) slotId = slot->index;

        if (clickedOutside) {
            slotId = AbstractContainerMenu::SLOT_CLICKED_OUTSIDE;
        }

        if (slotId != -1) {
            bool quickKey =
                slotId != AbstractContainerMenu::SLOT_CLICKED_OUTSIDE &&
                (Keyboard::isKeyDown(Keyboard::KEY_LSHIFT) ||
                 Keyboard::isKeyDown(Keyboard::KEY_RSHIFT));
            minecraft->gameMode->handleInventoryMouseClick(
                menu->containerId, slotId, buttonNum, quickKey,
                minecraft->player);
        }
    }
}

void AbstractContainerScreen::mouseReleased(int x, int y, int buttonNum) {
    if (buttonNum == 0) {
    }
}

void AbstractContainerScreen::keyPressed(char eventCharacter, int eventKey) {
    if (eventKey == Keyboard::KEY_ESCAPE ||
        eventKey == minecraft->options->keyBuild->key) {
        minecraft->player->closeContainer();
    }
}

void AbstractContainerScreen::removed() {
    if (minecraft->player == nullptr) return;
}

void AbstractContainerScreen::slotsChanged(
    std::shared_ptr<Container> container) {}

bool AbstractContainerScreen::isPauseScreen() { return false; }

void AbstractContainerScreen::tick() {
    Screen::tick();
    if (!minecraft->player->isAlive() || minecraft->player->removed)
        minecraft->player->closeContainer();
}
