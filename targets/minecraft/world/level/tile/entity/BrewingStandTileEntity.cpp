#include "BrewingStandTileEntity.h"

#include <stdint.h>

#include <algorithm>
#include <format>

#include "Facing.h"
#include "minecraft/IGameServices.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/item/alchemy/PotionBrewing.h"
#include "minecraft/world/item/alchemy/PotionMacros.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"

class MobEffectInstance;

int slotsForUp[] = {BrewingStandTileEntity::INGREDIENT_SLOT};
int slotsForOtherFaces[] = {0, 1, 2};

std::vector<int> BrewingStandTileEntity::SLOTS_FOR_UP =
    std::vector<int>(slotsForUp, slotsForUp + 1);
std::vector<int> BrewingStandTileEntity::SLOTS_FOR_OTHER_FACES =
    std::vector<int>(slotsForOtherFaces, slotsForOtherFaces + 3);

BrewingStandTileEntity::BrewingStandTileEntity() {
    brewTime = 0;
    items = std::vector<std::shared_ptr<ItemInstance>>(4);
    name = "";
}

BrewingStandTileEntity::~BrewingStandTileEntity() {}

std::string BrewingStandTileEntity::getName() {
    return hasCustomName() ? name
                           : gameServices().getString(IDS_TILE_BREWINGSTAND);
}

std::string BrewingStandTileEntity::getCustomName() {
    return hasCustomName() ? name : "";
}

bool BrewingStandTileEntity::hasCustomName() { return !name.empty(); }

void BrewingStandTileEntity::setCustomName(const std::string& name) {
    this->name = name;
}

unsigned int BrewingStandTileEntity::getContainerSize() { return items.size(); }

void BrewingStandTileEntity::tick() {
    if (brewTime > 0) {
        brewTime--;

        if (brewTime == 0) {
            // apply ingredients to all potions
            doBrew();
            setChanged();
        } else if (!isBrewable()) {
            brewTime = 0;
            setChanged();
        } else if (ingredientId != items[INGREDIENT_SLOT]->id) {
            brewTime = 0;
            setChanged();
        }
    } else if (isBrewable()) {
        brewTime = SharedConstants::TICKS_PER_SECOND *
                   PotionBrewing::BREWING_TIME_SECONDS;
        ingredientId = items[INGREDIENT_SLOT]->id;
    }

    int newCount = getPotionBits();
    if (newCount != lastPotionCount) {
        lastPotionCount = newCount;
        level->setData(x, y, z, newCount, Tile::UPDATE_CLIENTS);
    }

    TileEntity::tick();
}

int BrewingStandTileEntity::getBrewTime() { return brewTime; }

bool BrewingStandTileEntity::isBrewable() {
    if (items[INGREDIENT_SLOT] == nullptr ||
        items[INGREDIENT_SLOT]->count <= 0) {
        return false;
    }
    std::shared_ptr<ItemInstance> ingredient = items[INGREDIENT_SLOT];
    if (PotionBrewing::SIMPLIFIED_BREWING) {
        if (!Item::items[ingredient->id]->hasPotionBrewingFormula()) {
            return false;
        }

        bool oneResult = false;
        for (int dest = 0; dest < 3; dest++) {
            if (items[dest] != nullptr && items[dest]->id == Item::potion_Id) {
                int currentBrew = items[dest]->getAuxValue();
                int newBrew = NORMALISE_POTION_AUXVAL(
                    applyIngredient(currentBrew, ingredient));

                if (!PotionItem::isThrowable(currentBrew) &&
                    PotionItem::isThrowable(newBrew)) {
                    oneResult = true;
                    break;
                }

                std::vector<MobEffectInstance*>* currentEffects =
                    Item::potion->getMobEffects(currentBrew);
                std::vector<MobEffectInstance*>* newEffects =
                    Item::potion->getMobEffects(newBrew);

                // 4J - this code replaces an expression
                // "currentEffects.equals(newEffects)" in the java.
                // TODO - find out whether actually checking pointers to
                // MobEffectInstance classes for equality is of any use
                bool equals = false;
                if ((currentEffects != nullptr) && (newEffects != nullptr)) {
                    if (currentEffects->size() == newEffects->size()) {
                        if (std::equal(currentEffects->begin(),
                                       currentEffects->end(),
                                       newEffects->begin())) {
                            equals = true;
                        }
                    }
                }

                if ((currentBrew > 0 && currentEffects == newEffects) ||
                    (currentEffects != nullptr &&
                     (equals || newEffects == nullptr))) {
                } else if (currentBrew != newBrew) {
                    oneResult = true;
                    break;
                }
            }
        }
        return oneResult;

    } else {
        if (!Item::items[ingredient->id]->hasPotionBrewingFormula() &&
            ingredient->id != Item::bucket_water_Id &&
            ingredient->id != Item::netherwart_seeds_Id) {
            return false;
        }
        bool isWater = ingredient->id == Item::bucket_water_Id;

        // at least one destination potion must have a result
        bool oneResult = false;
        for (int dest = 0; dest < 3; dest++) {
            if (items[dest] != nullptr && items[dest]->id == Item::potion_Id) {
                int currentBrew = items[dest]->getAuxValue();
                int newBrew = NORMALISE_POTION_AUXVAL(
                    applyIngredient(currentBrew, ingredient));
                if (currentBrew != newBrew) {
                    oneResult = true;
                    break;
                }
            } else if (isWater && items[dest] != nullptr &&
                       items[dest]->id == Item::glassBottle_Id) {
                oneResult = true;
                break;
            }
        }
        return oneResult;
    }
}

void BrewingStandTileEntity::doBrew() {
    if (!isBrewable()) {
        return;
    }

    std::shared_ptr<ItemInstance> ingredient = items[INGREDIENT_SLOT];

    if (PotionBrewing::SIMPLIFIED_BREWING) {
        for (int dest = 0; dest < 3; dest++) {
            if (items[dest] != nullptr && items[dest]->id == Item::potion_Id) {
                int currentBrew = items[dest]->getAuxValue();
                int newBrew = NORMALISE_POTION_AUXVAL(
                    applyIngredient(currentBrew, ingredient));

                std::vector<MobEffectInstance*>* currentEffects =
                    Item::potion->getMobEffects(currentBrew);
                std::vector<MobEffectInstance*>* newEffects =
                    Item::potion->getMobEffects(newBrew);

                // 4J - this code replaces an expression
                // "currentEffects.equals(newEffects)" in the java.
                // TODO - find out whether actually checking pointers to
                // MobEffectInstance classes for equality is of any use
                bool equals = false;
                if ((currentEffects != nullptr) && (newEffects != nullptr)) {
                    if (currentEffects->size() == newEffects->size()) {
                        if (std::equal(currentEffects->begin(),
                                       currentEffects->end(),
                                       newEffects->begin())) {
                            equals = true;
                        }
                    }
                }

                if ((currentBrew > 0 && currentEffects == newEffects) ||
                    (currentEffects != nullptr &&
                     (equals || newEffects == nullptr))) {
                    if (!PotionItem::isThrowable(currentBrew) &&
                        PotionItem::isThrowable(newBrew)) {
                        items[dest]->setAuxValue(newBrew);
                    }

                } else if (currentBrew != newBrew) {
                    items[dest]->setAuxValue(newBrew);
                }
            }
        }

    } else {
        bool isWater = ingredient->id == Item::bucket_water_Id;

        for (int dest = 0; dest < 3; dest++) {
            if (items[dest] != nullptr && items[dest]->id == Item::potion_Id) {
                int currentBrew = items[dest]->getAuxValue();
                int newBrew = NORMALISE_POTION_AUXVAL(
                    applyIngredient(currentBrew, ingredient));
                items[dest]->setAuxValue(newBrew);
            } else if (isWater && items[dest] != nullptr &&
                       items[dest]->id == Item::glassBottle_Id) {
                items[dest] = std::shared_ptr<ItemInstance>(
                    new ItemInstance(Item::potion));
            }
        }
    }

    if (Item::items[ingredient->id]->hasCraftingRemainingItem()) {
        items[INGREDIENT_SLOT] = std::make_shared<ItemInstance>(
            Item::items[ingredient->id]->getCraftingRemainingItem());
    } else {
        items[INGREDIENT_SLOT]->count--;
        if (items[INGREDIENT_SLOT]->count <= 0) {
            items[INGREDIENT_SLOT] = nullptr;
        }
    }
}

int BrewingStandTileEntity::applyIngredient(
    int currentBrew, std::shared_ptr<ItemInstance> ingredient) {
    if (ingredient == nullptr) {
        return currentBrew;
    }
    if (!PotionBrewing::SIMPLIFIED_BREWING) {
#if !(_SIMPLIFIED_BREWING)
        // 4J Stu - SIMPLIFIED_BREWING is on, so we never use this
        if (ingredient->id == Item::bucket_water_Id) {
            return PotionBrewing::applyBrew(currentBrew,
                                            PotionBrewing::MOD_WATER);
        }
        if (ingredient->id == Item::netherwart_seeds_Id) {
            return PotionBrewing::stirr(currentBrew);
        }
#endif
    }
    if (Item::items[ingredient->id]->hasPotionBrewingFormula()) {
        return PotionBrewing::applyBrew(
            currentBrew,
            Item::items[ingredient->id]->getPotionBrewingFormula());
    }
    return currentBrew;
}

void BrewingStandTileEntity::load(CompoundTag* base) {
    TileEntity::load(base);

    ListTag<CompoundTag>* inventoryList =
        (ListTag<CompoundTag>*)base->getList("Items");
    items = std::vector<std::shared_ptr<ItemInstance>>(getContainerSize());
    for (int i = 0; i < inventoryList->size(); i++) {
        CompoundTag* tag = inventoryList->get(i);
        int slot = tag->getByte("Slot");
        if (slot >= 0 && slot < items.size())
            items[slot] = ItemInstance::fromTag(tag);
    }

    brewTime = base->getShort("BrewTime");
    if (base->contains("CustomName")) name = base->getString("CustomName");
}

void BrewingStandTileEntity::save(CompoundTag* base) {
    TileEntity::save(base);

    base->putShort("BrewTime", (short)(brewTime));
    ListTag<CompoundTag>* listTag = new ListTag<CompoundTag>();

    for (int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr) {
            CompoundTag* tag = new CompoundTag();
            tag->putByte("Slot", (uint8_t)i);
            items[i]->save(tag);
            listTag->add(tag);
        }
    }
    base->put("Items", listTag);
    if (hasCustomName()) base->putString("CustomName", name);
}

std::shared_ptr<ItemInstance> BrewingStandTileEntity::getItem(
    unsigned int slot) {
    if (slot >= 0 && slot < items.size()) {
        return items[slot];
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> BrewingStandTileEntity::removeItem(
    unsigned int slot, int count) {
    // 4J Stu - Changed the implementation of this function to be the same as
    // ChestTileEntity to enable the "Pickup Half" option on the ingredients
    // slot Fix for #65373 - TU8: Content: UI: Command "Take Half" in the
    // Brewing Stand interface doesn't work as intended.

    if (slot >= 0 && slot < items.size() && items[slot] != nullptr) {
        if (items[slot]->count <= count) {
            std::shared_ptr<ItemInstance> item = items[slot];
            items[slot] = nullptr;
            this->setChanged();
            // 4J Stu - Fix for duplication glitch
            if (item->count <= 0) return nullptr;
            return item;
        } else {
            std::shared_ptr<ItemInstance> i = items[slot]->remove(count);
            if (items[slot]->count == 0) items[slot] = nullptr;
            this->setChanged();
            // 4J Stu - Fix for duplication glitch
            if (i->count <= 0) return nullptr;
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> BrewingStandTileEntity::removeItemNoUpdate(
    int slot) {
    if (slot >= 0 && slot < items.size()) {
        std::shared_ptr<ItemInstance> item = items[slot];
        items[slot] = nullptr;
        return item;
    }
    return nullptr;
}

void BrewingStandTileEntity::setItem(unsigned int slot,
                                     std::shared_ptr<ItemInstance> item) {
    if (slot >= 0 && slot < items.size()) {
        items[slot] = item;
    }
}

int BrewingStandTileEntity::getMaxStackSize() {
    // this value is not used for the potion slots
    return 64;
}

bool BrewingStandTileEntity::stillValid(std::shared_ptr<Player> player) {
    if (level->getTileEntity(x, y, z) != shared_from_this()) return false;
    if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
    return true;
}

void BrewingStandTileEntity::startOpen() {}

void BrewingStandTileEntity::stopOpen() {}

bool BrewingStandTileEntity::canPlaceItem(int slot,
                                          std::shared_ptr<ItemInstance> item) {
    if (slot == INGREDIENT_SLOT) {
        if (PotionBrewing::SIMPLIFIED_BREWING) {
            return Item::items[item->id]->hasPotionBrewingFormula();
        } else {
            return Item::items[item->id]->hasPotionBrewingFormula() ||
                   item->id == Item::netherwart_seeds_Id ||
                   item->id == Item::bucket_water_Id;
        }
    }

    return item->id == Item::potion_Id || item->id == Item::glassBottle_Id;
}

void BrewingStandTileEntity::setBrewTime(int value) { brewTime = value; }

int BrewingStandTileEntity::getPotionBits() {
    int newCount = 0;
    for (int potion = 0; potion < 3; potion++) {
        if (items[potion] != nullptr) {
            newCount |= (1 << potion);
        }
    }
    return newCount;
}

std::vector<int> BrewingStandTileEntity::getSlotsForFace(int face) {
    if (face == Facing::UP) {
        return SLOTS_FOR_UP;
    }

    return SLOTS_FOR_OTHER_FACES;
}

bool BrewingStandTileEntity::canPlaceItemThroughFace(
    int slot, std::shared_ptr<ItemInstance> item, int face) {
    return canPlaceItem(slot, item);
}

bool BrewingStandTileEntity::canTakeItemThroughFace(
    int slot, std::shared_ptr<ItemInstance> item, int face) {
    return true;
}

// 4J Added
std::shared_ptr<TileEntity> BrewingStandTileEntity::clone() {
    std::shared_ptr<BrewingStandTileEntity> result =
        std::make_shared<BrewingStandTileEntity>();
    TileEntity::clone(result);

    result->brewTime = brewTime;
    result->lastPotionCount = lastPotionCount;
    result->ingredientId = ingredientId;

    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr) {
            result->items[i] = ItemInstance::clone(items[i]);
        }
    }
    return result;
}