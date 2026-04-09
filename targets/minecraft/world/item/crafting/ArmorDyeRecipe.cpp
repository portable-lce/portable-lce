#include "ArmorDyeRecipe.h"

#include <string.h>

#include <algorithm>
#include <vector>

#include "minecraft/world/entity/animal/Sheep.h"
#include "minecraft/world/inventory/CraftingContainer.h"
#include "minecraft/world/item/ArmorItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/crafting/Recipes.h"
#include "minecraft/world/item/crafting/Recipy.h"
#include "minecraft/world/item/crafting/ShapedRecipy.h"
#include "minecraft/world/level/tile/ColoredTile.h"
#include "platform/PlatformTypes.h"

bool ArmorDyeRecipe::matches(std::shared_ptr<CraftingContainer> craftSlots,
                             Level* level) {
    std::shared_ptr<ItemInstance> target = nullptr;
    std::vector<std::shared_ptr<ItemInstance> > dyes;

    for (int slot = 0; slot < craftSlots->getContainerSize(); slot++) {
        std::shared_ptr<ItemInstance> item = craftSlots->getItem(slot);
        if (item == nullptr) continue;

        ArmorItem* armor = dynamic_cast<ArmorItem*>(item->getItem());
        if (armor) {
            if (armor->getMaterial() == ArmorItem::ArmorMaterial::CLOTH &&
                target == nullptr) {
                target = item;
            } else {
                return false;
            }
        } else if (item->id == Item::dye_powder_Id) {
            dyes.push_back(item);
        } else {
            return false;
        }
    }

    return target != nullptr && !dyes.empty();
}

std::shared_ptr<ItemInstance> ArmorDyeRecipe::assembleDyedArmor(
    std::shared_ptr<CraftingContainer> craftSlots) {
    std::shared_ptr<ItemInstance> target = nullptr;
    int colorTotals[3] = {0, 0, 0};
    int intensityTotal = 0;
    int colourCounts = 0;
    ArmorItem* armor = nullptr;

    if (craftSlots != nullptr) {
        for (int slot = 0; slot < craftSlots->getContainerSize(); slot++) {
            std::shared_ptr<ItemInstance> item = craftSlots->getItem(slot);
            if (item == nullptr) continue;

            armor = dynamic_cast<ArmorItem*>(item->getItem());
            if (armor) {
                if (armor->getMaterial() == ArmorItem::ArmorMaterial::CLOTH &&
                    target == nullptr) {
                    target = item->copy();
                    target->count = 1;

                    if (armor->hasCustomColor(item)) {
                        int color = armor->getColor(target);
                        float red = (float)((color >> 16) & 0xFF) / 0xFF;
                        float green = (float)((color >> 8) & 0xFF) / 0xFF;
                        float blue = (float)(color & 0xFF) / 0xFF;

                        intensityTotal +=
                            std::max(red, std::max(green, blue)) * 0xFF;

                        colorTotals[0] += red * 0xFF;
                        colorTotals[1] += green * 0xFF;
                        colorTotals[2] += blue * 0xFF;
                        colourCounts++;
                    }
                } else {
                    return nullptr;
                }
            } else if (item->id == Item::dye_powder_Id) {
                int tileData = ColoredTile::getTileDataForItemAuxValue(
                    item->getAuxValue());
                int red = (int)(Sheep::COLOR[tileData][0] * 0xFF);
                int green = (int)(Sheep::COLOR[tileData][1] * 0xFF);
                int blue = (int)(Sheep::COLOR[tileData][2] * 0xFF);

                intensityTotal += std::max(red, std::max(green, blue));

                colorTotals[0] += red;
                colorTotals[1] += green;
                colorTotals[2] += blue;
                colourCounts++;
            } else {
                return nullptr;
            }
        }
    }

    if (armor == nullptr) return nullptr;

    int red = (colorTotals[0] / colourCounts);
    int green = (colorTotals[1] / colourCounts);
    int blue = (colorTotals[2] / colourCounts);

    float averageIntensity = (float)intensityTotal / colourCounts;
    float resultIntensity = (float)std::max(red, std::max(green, blue));
    //        System.out.println(averageIntensity + ", " + resultIntensity);

    red = (int)((float)red * averageIntensity / resultIntensity);
    green = (int)((float)green * averageIntensity / resultIntensity);
    blue = (int)((float)blue * averageIntensity / resultIntensity);

    int rgb = red;
    rgb = (rgb << 8) + green;
    rgb = (rgb << 8) + blue;

    armor->setColor(target, rgb);
    return target;
}

std::shared_ptr<ItemInstance> ArmorDyeRecipe::assemble(
    std::shared_ptr<CraftingContainer> craftSlots) {
    return ArmorDyeRecipe::assembleDyedArmor(craftSlots);
}

int ArmorDyeRecipe::size() { return 10; }

const ItemInstance* ArmorDyeRecipe::getResultItem() { return nullptr; }

const int ArmorDyeRecipe::getGroup() { return ShapedRecipy::eGroupType_Armour; }

// 4J-PB
bool ArmorDyeRecipe::requiresRecipe(int iRecipe) { return false; }

void ArmorDyeRecipe::collectRequirements(INGREDIENTS_REQUIRED* pIngReq) {
    // int iCount=0;
    // bool bFound;
    // int j;
    INGREDIENTS_REQUIRED TempIngReq;

    // shapeless doesn't have the 3x3 shape, but we'll just use this to store
    // the ingredients anyway
    TempIngReq.iIngC = 0;
    TempIngReq.iType = RECIPE_TYPE_2x2;  // all the dyes can be made in a 2x2
    TempIngReq.uiGridA = new unsigned int[9];
    TempIngReq.iIngIDA = new int[3 * 3];
    TempIngReq.iIngValA = new int[3 * 3];
    TempIngReq.iIngAuxValA = new int[3 * 3];

    memset(TempIngReq.iIngIDA, 0, sizeof(int) * 9);
    memset(TempIngReq.iIngValA, 0, sizeof(int) * 9);
    memset(TempIngReq.iIngAuxValA, Recipes::ANY_AUX_VALUE, sizeof(int) * 9);
    memset(TempIngReq.uiGridA, 0, sizeof(unsigned int) * 9);

    pIngReq->iIngIDA = new int[TempIngReq.iIngC];
    pIngReq->iIngValA = new int[TempIngReq.iIngC];
    pIngReq->iIngAuxValA = new int[TempIngReq.iIngC];
    pIngReq->uiGridA = new unsigned int[9];

    pIngReq->pRecipy = this;

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        pIngReq->bCanMake[i] = false;
    }

    pIngReq->iIngC = TempIngReq.iIngC;
    pIngReq->iType = TempIngReq.iType;

    if (pIngReq->iIngC != 0) {
        memcpy(pIngReq->iIngIDA, TempIngReq.iIngIDA,
               sizeof(int) * TempIngReq.iIngC);
        memcpy(pIngReq->iIngValA, TempIngReq.iIngValA,
               sizeof(int) * TempIngReq.iIngC);
        memcpy(pIngReq->iIngAuxValA, TempIngReq.iIngAuxValA,
               sizeof(int) * TempIngReq.iIngC);
    }
    memcpy(pIngReq->uiGridA, TempIngReq.uiGridA, sizeof(unsigned int) * 9);

    delete[] TempIngReq.iIngIDA;
    delete[] TempIngReq.iIngValA;
    delete[] TempIngReq.iIngAuxValA;
    delete[] TempIngReq.uiGridA;
}
