#include "DispenserTileEntity.h"

#include <stdint.h>

#include "TileEntity.h"
#include "java/Random.h"
#include "minecraft/IGameServices.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"

DispenserTileEntity::DispenserTileEntity() : TileEntity() {
    items = std::vector<std::shared_ptr<ItemInstance>>(9);
    random = new Random();
    name = "";
}

DispenserTileEntity::~DispenserTileEntity() { delete random; }

unsigned int DispenserTileEntity::getContainerSize() { return 9; }

std::shared_ptr<ItemInstance> DispenserTileEntity::getItem(unsigned int slot) {
    return items[slot];
}

std::shared_ptr<ItemInstance> DispenserTileEntity::removeItem(unsigned int slot,
                                                              int count) {
    if (items[slot] != nullptr) {
        if (items[slot]->count <= count) {
            std::shared_ptr<ItemInstance> item = items[slot];
            items[slot] = nullptr;
            setChanged();
            // 4J Stu - Fix for duplication glitch
            if (item->count <= 0) return nullptr;
            return item;
        } else {
            std::shared_ptr<ItemInstance> i = items[slot]->remove(count);
            if (items[slot]->count == 0) items[slot] = nullptr;
            setChanged();
            // 4J Stu - Fix for duplication glitch
            if (i->count <= 0) return nullptr;
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> DispenserTileEntity::removeItemNoUpdate(
    int slot) {
    if (items[slot] != nullptr) {
        std::shared_ptr<ItemInstance> item = items[slot];
        items[slot] = nullptr;
        return item;
    }
    return nullptr;
}

// 4J-PB added for spawn eggs not being useable due to limits, so add them in
// again
void DispenserTileEntity::AddItemBack(std::shared_ptr<ItemInstance> item,
                                      unsigned int slot) {
    if (items[slot] != nullptr) {
        // just increment the count of the items
        if (item->id == items[slot]->id) {
            items[slot]->count++;
            setChanged();
        }
    } else {
        items[slot] = item;
        if (item != nullptr && item->count > getMaxStackSize())
            item->count = getMaxStackSize();
        setChanged();
    }
}
/**
 * Removes an item with the given id and returns true if one was found.
 *
 * @param itemId
 * @return
 */
bool DispenserTileEntity::removeProjectile(int itemId) {
    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr && items[i]->id == itemId) {
            std::shared_ptr<ItemInstance> removedItem = removeItem(i, 1);
            return removedItem != nullptr;
        }
    }
    return false;
}

int DispenserTileEntity::getRandomSlot() {
    int replaceSlot = -1;
    int replaceOdds = 1;
    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr && random->nextInt(replaceOdds++) == 0) {
            replaceSlot = i;
        }
    }

    return replaceSlot;
}

void DispenserTileEntity::setItem(unsigned int slot,
                                  std::shared_ptr<ItemInstance> item) {
    items[slot] = item;
    if (item != nullptr && item->count > getMaxStackSize())
        item->count = getMaxStackSize();
    setChanged();
}

int DispenserTileEntity::addItem(std::shared_ptr<ItemInstance> item) {
    for (int i = 0; i < items.size(); i++) {
        if (items[i] == nullptr || items[i]->id == 0) {
            setItem(i, item);
            return i;
        }
    }

    return -1;
}

std::string DispenserTileEntity::getName() {
    return hasCustomName() ? name
                           : gameServices().getString(IDS_TILE_DISPENSER);
}

std::string DispenserTileEntity::getCustomName() {
    return hasCustomName() ? name : "";
}

void DispenserTileEntity::setCustomName(const std::string& name) {
    this->name = name;
}

bool DispenserTileEntity::hasCustomName() { return !name.empty(); }

void DispenserTileEntity::load(CompoundTag* base) {
    TileEntity::load(base);
    ListTag<CompoundTag>* inventoryList =
        (ListTag<CompoundTag>*)base->getList("Items");
    items = std::vector<std::shared_ptr<ItemInstance>>(getContainerSize());
    for (int i = 0; i < inventoryList->size(); i++) {
        CompoundTag* tag = inventoryList->get(i);
        unsigned int slot = tag->getByte("Slot") & 0xff;
        if (slot >= 0 && slot < items.size())
            items[slot] = ItemInstance::fromTag(tag);
    }
    if (base->contains("CustomName")) name = base->getString("CustomName");
}

void DispenserTileEntity::save(CompoundTag* base) {
    TileEntity::save(base);
    ListTag<CompoundTag>* listTag = new ListTag<CompoundTag>;

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
}

int DispenserTileEntity::getMaxStackSize() {
    return Container::LARGE_MAX_STACK_SIZE;
}

bool DispenserTileEntity::stillValid(std::shared_ptr<Player> player) {
    if (level->getTileEntity(x, y, z) != shared_from_this()) return false;
    if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
    return true;
}

void DispenserTileEntity::setChanged() { return TileEntity::setChanged(); }

void DispenserTileEntity::startOpen() {}

void DispenserTileEntity::stopOpen() {}

bool DispenserTileEntity::canPlaceItem(int slot,
                                       std::shared_ptr<ItemInstance> item) {
    return true;
}

// 4J Added
std::shared_ptr<TileEntity> DispenserTileEntity::clone() {
    std::shared_ptr<DispenserTileEntity> result =
        std::make_shared<DispenserTileEntity>();
    TileEntity::clone(result);

    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr) {
            result->items[i] = ItemInstance::clone(items[i]);
        }
    }
    return result;
}