#include "EnchantmentScreen.h"
#include "platform/stubs.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>


#include "AbstractContainerScreen.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/ScreenSizeCalculator.h"
#include "minecraft/client/model/BookModel.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/locale/Language.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/inventory/EnchantmentMenu.h"
#include "minecraft/world/inventory/Slot.h"
#include "minecraft/world/item/ItemInstance.h"

class Level;

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4) and the existing
// container classes (and iggy too)
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_ENCHANT_LOCATION = ResourceLocation(TN_GUI_ENCHANT);
ResourceLocation ITEM_BOOK_LOCATION = ResourceLocation(TN_ITEM_BOOK);
#endif

EnchantmentScreen::EnchantmentScreen(std::shared_ptr<Inventory> inventory,
                                     Level* level, int x, int y, int z)
    : AbstractContainerScreen(new EnchantmentMenu(inventory, level, x, y, z)) {
    xMouse = yMouse = 0.0f;

    this->inventory = inventory;
    this->enchantMenu = static_cast<EnchantmentMenu*>(menu);
    bookTick = 0;
    flip = oFlip = flipT = flipA = 0.0f;
    open = oOpen = 0.0f;
    last = nullptr;
}

EnchantmentScreen::~EnchantmentScreen() = default;

void EnchantmentScreen::init() { AbstractContainerScreen::init(); }

void EnchantmentScreen::removed() { AbstractContainerScreen::removed(); }

void EnchantmentScreen::renderLabels() {
    font->draw(Language::getInstance()->getElement("container.enchant"), 12, 5,
               0x404040);
    font->draw(inventory->getName(), 8, imageHeight - 96 + 2, 0x404040);

    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;

    // 4jcraft: our own refactor, text rendering has been moved to the
    // foreground here from renderBg() (which is where it was in the JE 1.6.4
    // code)
    bool needsUpdate = false;
    for (int i = 0; i < 3; ++i) {
        if (enchantMenu->costs[i] != lastCosts[i]) {
            needsUpdate = true;
            lastCosts[i] = enchantMenu->costs[i];
        }
    }
    if (needsUpdate) {
        for (int i = 0; i < 3; ++i) {
            if (enchantMenu->costs[i] > 0) {
                enchantNames[i] = EnchantmentNames::instance.getRandomName();
            } else {
                enchantNames[i] = "";
            }
        }
    }

    for (int i = 0; i < 3; ++i) {
        int cost = enchantMenu->costs[i];

        int buttonX = 60;
        int buttonY = 14 + 19 * i;

        bool isHovering =
            (xMouse >= xo + buttonX && xMouse < xo + buttonX + 108 &&
             yMouse >= yo + buttonY && yMouse < yo + buttonY + 19);

        bool hasEnoughLevels = (minecraft->player->experienceLevel >= cost) ||
                               minecraft->player->abilities.instabuild;

        if (cost > 0) {
            std::string enchantName = enchantNames[i];

            Font* weirdEnchantTableFont = minecraft->altFont;
            if (weirdEnchantTableFont) {
                int nameColor;

                if (!hasEnoughLevels) {
                    nameColor = 0x342F25;
                } else if (isHovering) {
                    nameColor = 0xFFFF80;
                } else {
                    nameColor = 0x685E4A;
                }

                int textX = xo + buttonX + 2;
                int textY = yo + buttonY + 2;

                weirdEnchantTableFont->drawWordWrap(
                    enchantName, buttonX + 2, buttonY + 2, 104, nameColor, 64);
            }

            std::string costStr = std::to_string(cost);
            int costX = buttonX + 108 - font->width(costStr) - 2;
            int costY = buttonY + 8;

            int costColor;
            if (!hasEnoughLevels) {
                costColor = 0x407F10;
            } else {
                costColor = 0x80FF20;
            }

            font->drawShadow(costStr, costX, costY, costColor);
        }
    }
}

void EnchantmentScreen::render(int xm, int ym, float a) {
    AbstractContainerScreen::render(xm, ym, a);
    this->xMouse = (float)xm;
    this->yMouse = (float)ym;
}

void EnchantmentScreen::renderBg(float a) {
#ifdef ENABLE_JAVA_GUIS
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    Minecraft::GetInstance()->textures->bindTexture(&GUI_ENCHANT_LOCATION);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    ScreenSizeCalculator screenSize(minecraft->options, minecraft->width,
                                    minecraft->height);
    int scaledWidth = screenSize.getWidth();
    int scaledHeight = screenSize.getHeight();
    int scaleFactor = screenSize.scale;

    glViewport(((scaledWidth - 320) / 2) * scaleFactor,
               ((scaledHeight - 240) / 2) * scaleFactor, 320 * scaleFactor,
               240 * scaleFactor);

    glTranslatef(-0.34f, 0.23f, 0.0f);
    PlatformRenderer.MatrixPerspective(90.0f, 1.3333334f, 9.0f, 80.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Lighting::turnOn();

    glTranslatef(0.0f, 3.3f, -16.0f);
    glScalef(5.0f, 5.0f, 5.0f);
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);

    Minecraft::GetInstance()->textures->bindTexture(&ITEM_BOOK_LOCATION);
    glRotatef(20.0f, 1.0f, 0.0f, 0.0f);

    // 4jcraft: brought over from UIControl_EnchantmentBook
    float o = oOpen + (open - oOpen) * a;
    glTranslatef((1 - o) * 0.2f, (1 - o) * 0.1f, (1 - o) * 0.25f);
    glRotatef(-(1 - o) * 90 - 90, 0, 1, 0);
    glRotatef(180, 1, 0, 0);

    float ff1 = oFlip + (flip - oFlip) * a + 0.25f;
    float ff2 = oFlip + (flip - oFlip) * a + 0.75f;
    ff1 = (ff1 - floor(ff1)) * 1.6f - 0.3f;
    ff2 = (ff2 - floor(ff2)) * 1.6f - 0.3f;

    if (ff1 < 0.0f) ff1 = 0.0f;
    if (ff2 < 0.0f) ff2 = 0.0f;
    if (ff1 > 1.0f) ff1 = 1.0f;
    if (ff2 > 1.0f) ff2 = 1.0f;

    glEnable(GL_RESCALE_NORMAL);

    static BookModel bookModel;
    bookModel.render(nullptr, 0.0f, ff1, ff2, o, 0.0f, 0.0625f, true);

    glDisable(GL_RESCALE_NORMAL);
    Lighting::turnOff();

    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, minecraft->width, minecraft->height);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    Minecraft::GetInstance()->textures->bindTexture(&GUI_ENCHANT_LOCATION);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < 3; ++i) {
        int cost = enchantMenu->costs[i];
        int buttonX = 60;
        int buttonY = 14 + 19 * i;

        bool isHovering =
            (xMouse >= xo + buttonX && xMouse < xo + buttonX + 108 &&
             yMouse >= yo + buttonY && yMouse < yo + buttonY + 19);

        if (cost == 0) {
            blit(xo + buttonX, yo + buttonY, 0, 185, 108, 19);
        } else {
            bool hasEnoughLevels =
                (minecraft->player->experienceLevel >= cost) ||
                minecraft->player->abilities.instabuild;

            int texV;
            if (!hasEnoughLevels) {
                texV = 185;
            } else if (isHovering) {
                texV = 204;
            } else {
                texV = 166;
            }

            blit(xo + buttonX, yo + buttonY, 0, texV, 108, 19);
        }
    }
#endif
}

void EnchantmentScreen::mouseClicked(int x, int y, int buttonNum) {
    AbstractContainerScreen::mouseClicked(x, y, buttonNum);
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;

    for (int i = 0; i < 3; ++i) {
        int buttonX = xo + 60;
        int buttonY = yo + 14 + 19 * i;

        if (x >= buttonX && x < buttonX + 108 && y >= buttonY &&
            y < buttonY + 19) {
            if (enchantMenu->clickMenuButton(minecraft->player, i)) {
                minecraft->gameMode->handleInventoryButtonClick(
                    enchantMenu->containerId, i);
            }
            break;
        }
    }
}

void EnchantmentScreen::tick() {
    AbstractContainerScreen::tick();

    // 4jcraft: brought over from UIControl_EnchantmentBook
    oFlip = flip;
    oOpen = open;

    std::shared_ptr<ItemInstance> current =
        enchantMenu->getSlot(EnchantmentMenu::INGREDIENT_SLOT)->getItem();
    if (!ItemInstance::matches(current, last)) {
        last = current;

        if (current) {
            do {
                flipT += random.nextInt(4) - random.nextInt(4);
            } while (flip <= flipT + 1 && flip >= flipT - 1);
        } else {
            flipT = 0.0f;
        }
    }

    bool shouldBeOpen = false;
    for (int i = 0; i < 3; ++i) {
        if (enchantMenu->costs[i] != 0) {
            shouldBeOpen = true;
            break;
        }
    }

    if (shouldBeOpen)
        open += 0.2f;
    else
        open -= 0.2f;

    if (open < 0.0f) open = 0.0f;
    if (open > 1.0f) open = 1.0f;

    float diff = (flipT - flip) * 0.4f;
    float max = 0.2f;
    if (diff < -max) diff = -max;
    if (diff > max) diff = max;
    flipA += (diff - flipA) * 0.9f;
    flip = flip + flipA;
}

// 4jcraft: brought over from UIControl_EnchantmentButton
EnchantmentScreen::EnchantmentNames
    EnchantmentScreen::EnchantmentNames::instance;

EnchantmentScreen::EnchantmentNames::EnchantmentNames() {
    std::string allWords =
        "the elder scrolls klaatu berata niktu xyzzy bless curse light "
        "darkness fire air earth water hot dry cold wet ignite snuff embiggen "
        "twist shorten stretch fiddle destroy imbue galvanize enchant free "
        "limited range of towards inside sphere cube self other ball mental "
        "physical grow shrink demon elemental spirit animal creature beast "
        "humanoid undead fresh stale ";
    std::istringstream iss(allWords);
    std::copy(std::istream_iterator<std::string, char,
                                    std::char_traits<char> >(iss),
              std::istream_iterator<std::string, char,
                                    std::char_traits<char> >(),
              std::back_inserter(words));
}

std::string EnchantmentScreen::EnchantmentNames::getRandomName() {
    int wordCount = random.nextInt(2) + 3;
    std::string word = "";
    for (int i = 0; i < wordCount; i++) {
        if (i > 0) word += " ";
        word += words[random.nextInt(words.size())];
    }
    return word;
}