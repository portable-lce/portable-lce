#include "minecraft/IGameServices.h"
#include "FireworksChargeItem.h"

#include <stdint.h>

#include "minecraft/util/HtmlString.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/item/DyePowderItem.h"
#include "minecraft/world/item/FireworksItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "nbt/CompoundTag.h"
#include "nbt/IntArrayTag.h"
#include "strings.h"

class Tag;

FireworksChargeItem::FireworksChargeItem(int id) : Item(id) {}

Icon* FireworksChargeItem::getLayerIcon(int auxValue, int spriteLayer) {
    if (spriteLayer > 0) {
        return overlay;
    }
    return Item::getLayerIcon(auxValue, spriteLayer);
}

int FireworksChargeItem::getColor(std::shared_ptr<ItemInstance> item,
                                  int spriteLayer) {
    if (spriteLayer == 1) {
        Tag* colorTag = getExplosionTagField(item, FireworksItem::TAG_E_COLORS);
        if (colorTag != nullptr) {
            IntArrayTag* colors = (IntArrayTag*)colorTag;
            if (colors->data.size() == 1) {
                return colors->data[0];
            }
            int totalRed = 0;
            int totalGreen = 0;
            int totalBlue = 0;
            for (unsigned int i = 0; i < colors->data.size(); ++i) {
                int c = colors->data[i];
                totalRed += (c & 0xff0000) >> 16;
                totalGreen += (c & 0x00ff00) >> 8;
                totalBlue += (c & 0x0000ff) >> 0;
            }
            totalRed /= colors->data.size();
            totalGreen /= colors->data.size();
            totalBlue /= colors->data.size();
            return (totalRed << 16) | (totalGreen << 8) | totalBlue;
        }
        return 0x8a8a8a;
    }
    return Item::getColor(item, spriteLayer);
}

bool FireworksChargeItem::hasMultipleSpriteLayers() { return true; }

Tag* FireworksChargeItem::getExplosionTagField(
    std::shared_ptr<ItemInstance> instance, const std::string& field) {
    if (instance->hasTag()) {
        CompoundTag* explosion =
            instance->getTag()->getCompound(FireworksItem::TAG_EXPLOSION);
        if (explosion != nullptr) {
            return explosion->get(field);
        }
    }
    return nullptr;
}

void FireworksChargeItem::appendHoverText(
    std::shared_ptr<ItemInstance> itemInstance, std::shared_ptr<Player> player,
    std::vector<HtmlString>* lines, bool advanced) {
    if (itemInstance->hasTag()) {
        CompoundTag* explosion =
            itemInstance->getTag()->getCompound(FireworksItem::TAG_EXPLOSION);
        if (explosion != nullptr) {
            appendHoverText(explosion, lines);
        }
    }
}

const unsigned int FIREWORKS_CHARGE_TYPE_NAME[] = {
    IDS_FIREWORKS_CHARGE_TYPE_0, IDS_FIREWORKS_CHARGE_TYPE_1,
    IDS_FIREWORKS_CHARGE_TYPE_2, IDS_FIREWORKS_CHARGE_TYPE_3,
    IDS_FIREWORKS_CHARGE_TYPE_4};

const unsigned int FIREWORKS_CHARGE_COLOUR_NAME[] = {
    IDS_FIREWORKS_CHARGE_BLACK,      IDS_FIREWORKS_CHARGE_RED,
    IDS_FIREWORKS_CHARGE_GREEN,      IDS_FIREWORKS_CHARGE_BROWN,
    IDS_FIREWORKS_CHARGE_BLUE,       IDS_FIREWORKS_CHARGE_PURPLE,
    IDS_FIREWORKS_CHARGE_CYAN,       IDS_FIREWORKS_CHARGE_SILVER,
    IDS_FIREWORKS_CHARGE_GRAY,       IDS_FIREWORKS_CHARGE_PINK,
    IDS_FIREWORKS_CHARGE_LIME,       IDS_FIREWORKS_CHARGE_YELLOW,
    IDS_FIREWORKS_CHARGE_LIGHT_BLUE, IDS_FIREWORKS_CHARGE_MAGENTA,
    IDS_FIREWORKS_CHARGE_ORANGE,     IDS_FIREWORKS_CHARGE_WHITE};

void FireworksChargeItem::appendHoverText(CompoundTag* expTag,
                                          std::vector<HtmlString>* lines) {
    // shape
    uint8_t type = expTag->getByte(FireworksItem::TAG_E_TYPE);
    if (type >= FireworksItem::TYPE_MIN && type <= FireworksItem::TYPE_MAX) {
        lines->push_back(
            HtmlString(gameServices().getString(FIREWORKS_CHARGE_TYPE_NAME[type])));
    } else {
        lines->push_back(HtmlString(gameServices().getString(IDS_FIREWORKS_CHARGE_TYPE)));
    }

    // colors
    std::vector<int> colorList =
        expTag->getIntArray(FireworksItem::TAG_E_COLORS);
    if (colorList.size() > 0) {
        bool first = true;
        std::string output = "";
        for (unsigned int i = 0; i < colorList.size(); ++i) {
            int c = colorList[i];
            if (!first) {
                output +=
                    ",\n";  // 4J-PB  - without the newline, they tend to go
                             // offscreen in split-screen or localised languages
            }
            first = false;

            // find color name by lookup
            bool found = false;
            for (int dc = 0; dc < 16; dc++) {
                if (c == DyePowderItem::COLOR_RGB[dc]) {
                    found = true;
                    output += gameServices().getString(FIREWORKS_CHARGE_COLOUR_NAME[dc]);
                    break;
                }
            }
            if (!found) {
                output += gameServices().getString(IDS_FIREWORKS_CHARGE_CUSTOM);
            }
        }
        lines->push_back(output);
    }

    // has fade?
    std::vector<int> fadeList =
        expTag->getIntArray(FireworksItem::TAG_E_FADECOLORS);
    if (fadeList.size() > 0) {
        bool first = true;
        std::string output =
            std::string(gameServices().getString(IDS_FIREWORKS_CHARGE_FADE_TO)) + " ";
        for (unsigned int i = 0; i < fadeList.size(); ++i) {
            int c = fadeList[i];
            if (!first) {
                output +=
                    ",\n";  // 4J-PB  - without the newline, they tend to go
                             // offscreen in split-screen or localised languages
            }
            first = false;

            // find color name by lookup
            bool found = false;
            for (int dc = 0; dc < 16; dc++) {
                if (c == DyePowderItem::COLOR_RGB[dc]) {
                    found = true;
                    output += gameServices().getString(FIREWORKS_CHARGE_COLOUR_NAME[dc]);
                    break;
                }
            }
            if (!found) {
                output += gameServices().getString(IDS_FIREWORKS_CHARGE_CUSTOM);
            }
        }
        lines->push_back(output);
    }

    // has trail
    bool trail = expTag->getBoolean(FireworksItem::TAG_E_TRAIL);
    if (trail) {
        lines->push_back(HtmlString(gameServices().getString(IDS_FIREWORKS_CHARGE_TRAIL)));
    }

    // has flicker
    bool flicker = expTag->getBoolean(FireworksItem::TAG_E_FLICKER);
    if (flicker) {
        lines->push_back(
            HtmlString(gameServices().getString(IDS_FIREWORKS_CHARGE_FLICKER)));
    }
}

void FireworksChargeItem::registerIcons(IconRegister* iconRegister) {
    Item::registerIcons(iconRegister);
    overlay = iconRegister->registerIcon(getIconName() + "_overlay");
}