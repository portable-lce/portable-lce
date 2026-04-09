#include "FurnaceTileEntity.h"

#include <stdint.h>

#include "Facing.h"
#include "minecraft/IGameServices.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/CoalItem.h"
#include "minecraft/world/item/DiggerItem.h"
#include "minecraft/world/item/HoeItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/WeaponItem.h"
#include "minecraft/world/item/crafting/FurnaceRecipes.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/FurnaceTile.h"
#include "minecraft/world/level/tile/HalfSlabTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"

int furnaceSlotsForUp[] = {FurnaceTileEntity::SLOT_INPUT};
int furnaceSlotsForDown[] = {FurnaceTileEntity::SLOT_RESULT,
                             FurnaceTileEntity::SLOT_FUEL};
int furnaceSlotsForSides[] = {FurnaceTileEntity::SLOT_FUEL};

const std::vector<int> FurnaceTileEntity::SLOTS_FOR_UP =
    std::vector<int>(furnaceSlotsForUp, furnaceSlotsForUp + 1);
const std::vector<int> FurnaceTileEntity::SLOTS_FOR_DOWN =
    std::vector<int>(furnaceSlotsForDown, furnaceSlotsForDown + 2);
const std::vector<int> FurnaceTileEntity::SLOTS_FOR_SIDES =
    std::vector<int>(furnaceSlotsForSides, furnaceSlotsForSides + 1);

const int FurnaceTileEntity::BURN_INTERVAL = 10 * 20;

// 4J Stu - Need a ctor to initialise member variables
FurnaceTileEntity::~FurnaceTileEntity() {}

FurnaceTileEntity::FurnaceTileEntity() : TileEntity() {
    items = std::vector<std::shared_ptr<ItemInstance>>(3);

    litTime = 0;
    litDuration = 0;
    tickCount = 0;
    m_charcoalUsed = false;
    name = "";
}

unsigned int FurnaceTileEntity::getContainerSize() { return items.size(); }

std::shared_ptr<ItemInstance> FurnaceTileEntity::getItem(unsigned int slot) {
    return items[slot];
}

std::shared_ptr<ItemInstance> FurnaceTileEntity::removeItem(unsigned int slot,
                                                            int count) {
    m_charcoalUsed = false;

    if (items[slot] != nullptr) {
        if (items[slot]->count <= count) {
            std::shared_ptr<ItemInstance> item = items[slot];
            items[slot] = nullptr;
            // 4J Stu - Fix for duplication glitch
            if (item->count <= 0) return nullptr;
            return item;
        } else {
            std::shared_ptr<ItemInstance> i = items[slot]->remove(count);
            if (items[slot]->count == 0) items[slot] = nullptr;
            // 4J Stu - Fix for duplication glitch
            if (i->count <= 0) return nullptr;
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> FurnaceTileEntity::removeItemNoUpdate(int slot) {
    m_charcoalUsed = false;

    if (items[slot] != nullptr) {
        std::shared_ptr<ItemInstance> item = items[slot];
        items[slot] = nullptr;
        return item;
    }
    return nullptr;
}

void FurnaceTileEntity::setItem(unsigned int slot,
                                std::shared_ptr<ItemInstance> item) {
    items[slot] = item;
    if (item != nullptr && item->count > getMaxStackSize())
        item->count = getMaxStackSize();
}

std::string FurnaceTileEntity::getName() {
    return hasCustomName() ? name : gameServices().getString(IDS_TILE_FURNACE);
}

std::string FurnaceTileEntity::getCustomName() {
    return hasCustomName() ? name : "";
}

bool FurnaceTileEntity::hasCustomName() { return !name.empty(); }

void FurnaceTileEntity::setCustomName(const std::string& name) {
    this->name = name;
}

void FurnaceTileEntity::load(CompoundTag* base) {
    TileEntity::load(base);
    ListTag<CompoundTag>* inventoryList =
        (ListTag<CompoundTag>*)base->getList("Items");
    items = std::vector<std::shared_ptr<ItemInstance>>(getContainerSize());
    for (int i = 0; i < inventoryList->size(); i++) {
        CompoundTag* tag = inventoryList->get(i);
        unsigned int slot = tag->getByte("Slot");
        if (slot >= 0 && slot < items.size())
            items[slot] = ItemInstance::fromTag(tag);
    }

    litTime = base->getShort("BurnTime");
    tickCount = base->getShort("CookTime");
    litDuration = getBurnDuration(items[SLOT_FUEL]);
    if (base->contains("CustomName")) name = base->getString("CustomName");
    m_charcoalUsed = base->getBoolean("CharcoalUsed");
}

void FurnaceTileEntity::save(CompoundTag* base) {
    TileEntity::save(base);
    base->putShort("BurnTime", (short)(litTime));
    base->putShort("CookTime", (short)(tickCount));
    ListTag<CompoundTag>* listTag = new ListTag<CompoundTag>();

    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr) {
            CompoundTag* tag = new CompoundTag();
            tag->putByte("Slot", (uint8_t)i);
            items[i]->save(tag);
            listTag->add(tag);
        }
    }
    base->put("Items", listTag);
    if (hasCustomName()) base->putString("CustomName", name);
    base->putBoolean("CharcoalUsed", m_charcoalUsed);
}

int FurnaceTileEntity::getMaxStackSize() {
    return Container::LARGE_MAX_STACK_SIZE;
}

int FurnaceTileEntity::getBurnProgress(int max) {
    return tickCount * max / BURN_INTERVAL;
}

int FurnaceTileEntity::getLitProgress(int max) {
    if (litDuration == 0) litDuration = BURN_INTERVAL;
    return litTime * max / litDuration;
}

bool FurnaceTileEntity::isLit() { return litTime > 0; }

void FurnaceTileEntity::tick() {
    bool wasLit = litTime > 0;
    bool changed = false;
    if (litTime > 0) {
        litTime--;
    }

    if (level != nullptr && !level->isClientSide) {
        if (litTime == 0 && canBurn()) {
            litDuration = litTime = getBurnDuration(items[SLOT_FUEL]);
            if (litTime > 0) {
                changed = true;
                if (items[SLOT_FUEL] != nullptr) {
                    // 4J Added: Keep track of whether charcoal was used in
                    // production of current stack.
                    if (items[SLOT_FUEL]->getItem()->id == Item::coal_Id &&
                        items[SLOT_FUEL]->getAuxValue() ==
                            CoalItem::CHAR_COAL) {
                        m_charcoalUsed = true;
                    }

                    items[SLOT_FUEL]->count--;
                    if (items[SLOT_FUEL]->count == 0) {
                        Item* remaining = items[SLOT_FUEL]
                                              ->getItem()
                                              ->getCraftingRemainingItem();
                        items[SLOT_FUEL] =
                            remaining != nullptr
                                ? std::shared_ptr<ItemInstance>(
                                      new ItemInstance(remaining))
                                : nullptr;
                    }
                }
            }
        }

        if (isLit() && canBurn()) {
            tickCount++;
            if (tickCount == BURN_INTERVAL) {
                tickCount = 0;
                burn();
                changed = true;
            }
        } else {
            tickCount = 0;
        }

        if (wasLit != litTime > 0) {
            changed = true;
            FurnaceTile::setLit(litTime > 0, level, x, y, z);
        }
    }

    if (changed) setChanged();
}

bool FurnaceTileEntity::canBurn() {
    if (items[SLOT_INPUT] == nullptr) return false;
    ItemInstance* burnResult = FurnaceRecipes::getInstance()->getResult(
        items[SLOT_INPUT]->getItem()->id);
    if (burnResult == nullptr) return false;
    if (items[SLOT_RESULT] == nullptr) return true;
    if (!items[SLOT_RESULT]->sameItem_not_shared(burnResult)) return false;
    if (items[SLOT_RESULT]->count < getMaxStackSize() &&
        items[SLOT_RESULT]->count < items[SLOT_RESULT]->getMaxStackSize())
        return true;
    if (items[SLOT_RESULT]->count < burnResult->getMaxStackSize()) return true;
    return false;
}

void FurnaceTileEntity::burn() {
    if (!canBurn()) return;

    ItemInstance* result = FurnaceRecipes::getInstance()->getResult(
        items[SLOT_INPUT]->getItem()->id);
    if (items[SLOT_RESULT] == nullptr)
        items[SLOT_RESULT] = result->copy();
    else if (items[SLOT_RESULT]->id == result->id)
        items[SLOT_RESULT]->count++;

    items[SLOT_INPUT]->count--;
    if (items[SLOT_INPUT]->count <= 0) items[SLOT_INPUT] = nullptr;
}

int FurnaceTileEntity::getBurnDuration(
    std::shared_ptr<ItemInstance> itemInstance) {
    if (itemInstance == nullptr) return 0;
    int id = itemInstance->getItem()->id;

    Item* item = itemInstance->getItem();

    if (id < 256 && Tile::tiles[id] != nullptr) {
        Tile* tile = Tile::tiles[id];

        if (tile == Tile::woodSlabHalf) {
            return BURN_INTERVAL * 3 / 4;
        }

        if (tile->material == Material::wood) {
            return BURN_INTERVAL * 3 / 2;
        }

        if (tile == Tile::coalBlock) {
            return BURN_INTERVAL * 8 * 10;
        }
    }

    if (dynamic_cast<DiggerItem*>(item) &&
        ((DiggerItem*)item)->getTier() == Item::Tier::WOOD) {
        return BURN_INTERVAL;
    } else if (dynamic_cast<WeaponItem*>(item) &&
               ((WeaponItem*)item)->getTier() == Item::Tier::WOOD) {
        return BURN_INTERVAL;
    } else if (dynamic_cast<HoeItem*>(item) &&
               ((HoeItem*)item)->getTier() == Item::Tier::WOOD) {
        return BURN_INTERVAL;
    }

    if (id == Item::stick->id) {
        return BURN_INTERVAL / 2;
    }

    if (id == Item::coal->id) return BURN_INTERVAL * 8;

    if (id == Item::bucket_lava->id) return BURN_INTERVAL * 100;

    if (id == Tile::sapling_Id) return BURN_INTERVAL / 2;

    if (id == Item::blazeRod_Id) return BURN_INTERVAL * 12;

    return 0;
}

bool FurnaceTileEntity::isFuel(std::shared_ptr<ItemInstance> item) {
    return getBurnDuration(item) > 0;
}

bool FurnaceTileEntity::stillValid(std::shared_ptr<Player> player) {
    if (level->getTileEntity(x, y, z) != shared_from_this()) return false;
    if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
    return true;
}

void FurnaceTileEntity::setChanged() { return TileEntity::setChanged(); }

void FurnaceTileEntity::startOpen() {}

void FurnaceTileEntity::stopOpen() {}

bool FurnaceTileEntity::canPlaceItem(int slot,
                                     std::shared_ptr<ItemInstance> item) {
    if (slot == SLOT_RESULT) return false;
    if (slot == SLOT_FUEL) return isFuel(item);
    return true;
}

std::vector<int> FurnaceTileEntity::getSlotsForFace(int face) {
    if (face == Facing::DOWN) {
        return SLOTS_FOR_DOWN;
    } else if (face == Facing::UP) {
        return SLOTS_FOR_UP;
    } else {
        return SLOTS_FOR_SIDES;
    }
}

bool FurnaceTileEntity::canPlaceItemThroughFace(
    int slot, std::shared_ptr<ItemInstance> item, int face) {
    return canPlaceItem(slot, item);
}

bool FurnaceTileEntity::canTakeItemThroughFace(
    int slot, std::shared_ptr<ItemInstance> item, int face) {
    if (face == Facing::DOWN && slot == SLOT_FUEL) {
        if (item->id != Item::bucket_empty_Id) return false;
    }

    return true;
}

// 4J Added
std::shared_ptr<TileEntity> FurnaceTileEntity::clone() {
    std::shared_ptr<FurnaceTileEntity> result =
        std::make_shared<FurnaceTileEntity>();
    TileEntity::clone(result);

    result->litTime = litTime;
    result->tickCount = tickCount;
    result->litDuration = litDuration;

    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr) {
            result->items[i] = ItemInstance::clone(items[i]);
        }
    }
    return result;
}