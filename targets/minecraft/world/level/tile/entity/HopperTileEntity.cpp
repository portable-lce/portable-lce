#include "minecraft/IGameServices.h"
#include "HopperTileEntity.h"

#include <stdint.h>

#include <algorithm>
#include <format>

#include "Facing.h"
#include "java/Random.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/WorldlyContainer.h"
#include "minecraft/world/entity/EntitySelector.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/ChestTile.h"
#include "minecraft/world/level/tile/HopperTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/ChestTileEntity.h"
#include "minecraft/world/level/tile/entity/Hopper.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"

class Entity;

HopperTileEntity::HopperTileEntity() {
    items = std::vector<std::shared_ptr<ItemInstance>>(5);
    name = "";
    cooldownTime = -1;
}

HopperTileEntity::~HopperTileEntity() {}

void HopperTileEntity::load(CompoundTag* base) {
    TileEntity::load(base);

    ListTag<CompoundTag>* inventoryList =
        (ListTag<CompoundTag>*)base->getList("Items");
    items = std::vector<std::shared_ptr<ItemInstance>>(getContainerSize());
    if (base->contains("CustomName")) name = base->getString("CustomName");
    cooldownTime = base->getInt("TransferCooldown");
    for (int i = 0; i < inventoryList->size(); i++) {
        CompoundTag* tag = inventoryList->get(i);
        int slot = tag->getByte("Slot");
        if (slot >= 0 && slot < items.size())
            items[slot] = ItemInstance::fromTag(tag);
    }
}

void HopperTileEntity::save(CompoundTag* base) {
    TileEntity::save(base);
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
    base->putInt("TransferCooldown", cooldownTime);
    if (hasCustomName()) base->putString("CustomName", name);
}

void HopperTileEntity::setChanged() { TileEntity::setChanged(); }

unsigned int HopperTileEntity::getContainerSize() { return items.size(); }

std::shared_ptr<ItemInstance> HopperTileEntity::getItem(unsigned int slot) {
    return items[slot];
}

std::shared_ptr<ItemInstance> HopperTileEntity::removeItem(unsigned int slot,
                                                           int count) {
    if (items[slot] != nullptr) {
        if (items[slot]->count <= count) {
            std::shared_ptr<ItemInstance> item = items[slot];
            items[slot] = nullptr;
            return item;
        } else {
            std::shared_ptr<ItemInstance> i = items[slot]->remove(count);
            if (items[slot]->count == 0) items[slot] = nullptr;
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> HopperTileEntity::removeItemNoUpdate(int slot) {
    if (items[slot] != nullptr) {
        std::shared_ptr<ItemInstance> item = items[slot];
        items[slot] = nullptr;
        return item;
    }
    return nullptr;
}

void HopperTileEntity::setItem(unsigned int slot,
                               std::shared_ptr<ItemInstance> item) {
    items[slot] = item;
    if (item != nullptr && item->count > getMaxStackSize())
        item->count = getMaxStackSize();
}

std::string HopperTileEntity::getName() {
    return hasCustomName() ? name : gameServices().getString(IDS_CONTAINER_HOPPER);
}

std::string HopperTileEntity::getCustomName() {
    return hasCustomName() ? name : "";
}

bool HopperTileEntity::hasCustomName() { return !name.empty(); }

void HopperTileEntity::setCustomName(const std::string& name) {
    this->name = name;
}

int HopperTileEntity::getMaxStackSize() {
    return Container::LARGE_MAX_STACK_SIZE;
}

bool HopperTileEntity::stillValid(std::shared_ptr<Player> player) {
    if (level->getTileEntity(x, y, z) != shared_from_this()) return false;
    if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
    return true;
}

void HopperTileEntity::startOpen() {}

void HopperTileEntity::stopOpen() {}

bool HopperTileEntity::canPlaceItem(int slot,
                                    std::shared_ptr<ItemInstance> item) {
    return true;
}

void HopperTileEntity::tick() {
    if (level == nullptr || level->isClientSide) return;

    cooldownTime--;

    if (!isOnCooldown()) {
        setCooldown(0);
        tryMoveItems();
    }
}

bool HopperTileEntity::tryMoveItems() {
    if (level == nullptr || level->isClientSide) return false;

    if (!isOnCooldown() && HopperTile::isTurnedOn(getData())) {
        bool changed = ejectItems();
        changed = suckInItems(this) || changed;

        if (changed) {
            setCooldown(MOVE_ITEM_SPEED);
            setChanged();
            return true;
        }
    }

    return false;
}

bool HopperTileEntity::ejectItems() {
    std::shared_ptr<Container> container = getAttachedContainer();
    if (container == nullptr) {
        return false;
    }

    for (int slot = 0; slot < getContainerSize(); slot++) {
        if (getItem(slot) == nullptr) continue;

        std::shared_ptr<ItemInstance> original = getItem(slot)->copy();
        std::shared_ptr<ItemInstance> result = addItem(
            container.get(), removeItem(slot, 1),
            Facing::OPPOSITE_FACING[HopperTile::getAttachedFace(getData())]);

        if (result == nullptr || result->count == 0) {
            container->setChanged();
            return true;
        } else {
            setItem(slot, original);
        }
    }

    return false;
}

bool HopperTileEntity::suckInItems(Hopper* hopper) {
    std::shared_ptr<Container> container = getSourceContainer(hopper);

    if (container != nullptr) {
        int face = Facing::DOWN;

        std::shared_ptr<WorldlyContainer> worldly =
            std::dynamic_pointer_cast<WorldlyContainer>(container);
        if ((worldly != nullptr) && (face > -1)) {
            std::vector<int> slots = worldly->getSlotsForFace(face);

            for (int i = 0; i < slots.size(); i++) {
                if (tryTakeInItemFromSlot(hopper, container.get(), slots[i],
                                          face))
                    return true;
            }
        } else {
            int size = container->getContainerSize();
            for (int i = 0; i < size; i++) {
                if (tryTakeInItemFromSlot(hopper, container.get(), i, face))
                    return true;
            }
        }
    } else {
        std::shared_ptr<ItemEntity> above =
            getItemAt(hopper->getLevel(), hopper->getLevelX(),
                      hopper->getLevelY() + 1, hopper->getLevelZ());

        if (above != nullptr) {
            return addItem(hopper, above);
        }
    }

    return false;
}

bool HopperTileEntity::tryTakeInItemFromSlot(Hopper* hopper,
                                             Container* container, int slot,
                                             int face) {
    std::shared_ptr<ItemInstance> item = container->getItem(slot);

    if (item != nullptr &&
        canTakeItemFromContainer(container, item, slot, face)) {
        std::shared_ptr<ItemInstance> original = item->copy();
        std::shared_ptr<ItemInstance> result =
            addItem(hopper, container->removeItem(slot, 1), -1);

        if (result == nullptr || result->count == 0) {
            container->setChanged();
            return true;
        } else {
            container->setItem(slot, original);
        }
    }

    return false;
}

bool HopperTileEntity::addItem(Container* container,
                               std::shared_ptr<ItemEntity> item) {
    bool changed = false;
    if (item == nullptr) return false;

    std::shared_ptr<ItemInstance> copy = item->getItem()->copy();
    std::shared_ptr<ItemInstance> result = addItem(container, copy, -1);

    if (result == nullptr || result->count == 0) {
        changed = true;

        item->remove();
    } else {
        item->setItem(result);
    }

    return changed;
}

std::shared_ptr<ItemInstance> HopperTileEntity::addItem(
    Container* container, std::shared_ptr<ItemInstance> item, int face) {
    if (dynamic_cast<WorldlyContainer*>(container) != nullptr && face > -1) {
        WorldlyContainer* worldly = (WorldlyContainer*)container;
        std::vector<int> slots = worldly->getSlotsForFace(face);

        for (int i = 0; i < slots.size() && item != nullptr && item->count > 0;
             i++) {
            item = tryMoveInItem(container, item, slots[i], face);
        }
    } else {
        int size = container->getContainerSize();
        for (int i = 0; i < size && item != nullptr && item->count > 0; i++) {
            item = tryMoveInItem(container, item, i, face);
        }
    }

    if (item != nullptr && item->count == 0) {
        item = nullptr;
    }

    return item;
}

bool HopperTileEntity::canPlaceItemInContainer(
    Container* container, std::shared_ptr<ItemInstance> item, int slot,
    int face) {
    if (!container->canPlaceItem(slot, item)) return false;
    if (dynamic_cast<WorldlyContainer*>(container) != nullptr &&
        !dynamic_cast<WorldlyContainer*>(container)->canPlaceItemThroughFace(
            slot, item, face))
        return false;
    return true;
}

bool HopperTileEntity::canTakeItemFromContainer(
    Container* container, std::shared_ptr<ItemInstance> item, int slot,
    int face) {
    if (dynamic_cast<WorldlyContainer*>(container) != nullptr &&
        !dynamic_cast<WorldlyContainer*>(container)->canTakeItemThroughFace(
            slot, item, face))
        return false;
    return true;
}

std::shared_ptr<ItemInstance> HopperTileEntity::tryMoveInItem(
    Container* container, std::shared_ptr<ItemInstance> item, int slot,
    int face) {
    std::shared_ptr<ItemInstance> current = container->getItem(slot);

    if (canPlaceItemInContainer(container, item, slot, face)) {
        bool success = false;
        if (current == nullptr) {
            container->setItem(slot, item);
            item = nullptr;
            success = true;
        } else if (canMergeItems(current, item)) {
            int space = item->getMaxStackSize() - current->count;
            int count = std::min(item->count, space);

            item->count -= count;
            current->count += count;
            success = count > 0;
        }
        if (success) {
            HopperTileEntity* hopper =
                dynamic_cast<HopperTileEntity*>(container);
            if (hopper != nullptr) {
                hopper->setCooldown(MOVE_ITEM_SPEED);
                container->setChanged();
            }
            container->setChanged();
        }
    }
    return item;
}

std::shared_ptr<Container> HopperTileEntity::getAttachedContainer() {
    int face = HopperTile::getAttachedFace(getData());
    return getContainerAt(getLevel(), x + Facing::STEP_X[face],
                          y + Facing::STEP_Y[face], z + Facing::STEP_Z[face]);
}

std::shared_ptr<Container> HopperTileEntity::getSourceContainer(
    Hopper* hopper) {
    return getContainerAt(hopper->getLevel(), hopper->getLevelX(),
                          hopper->getLevelY() + 1, hopper->getLevelZ());
}

std::shared_ptr<ItemEntity> HopperTileEntity::getItemAt(Level* level, double xt,
                                                        double yt, double zt) {
    AABB item_entity_aabb{xt, yt, zt, xt + 1, yt + 1, zt + 1};
    std::vector<std::shared_ptr<Entity>>* entities =
        level->getEntitiesOfClass(typeid(ItemEntity), &item_entity_aabb,
                                  EntitySelector::ENTITY_STILL_ALIVE);

    if (entities->size() > 0) {
        std::shared_ptr<ItemEntity> out =
            std::dynamic_pointer_cast<ItemEntity>(entities->at(0));
        delete entities;
        return out;
    } else {
        delete entities;
        return nullptr;
    }
}

std::shared_ptr<Container> HopperTileEntity::getContainerAt(Level* level,
                                                            double x, double y,
                                                            double z) {
    std::shared_ptr<Container> result = nullptr;

    int xt = Mth::floor(x);
    int yt = Mth::floor(y);
    int zt = Mth::floor(z);

    std::shared_ptr<TileEntity> entity = level->getTileEntity(xt, yt, zt);

    result = std::dynamic_pointer_cast<Container>(entity);
    if (result != nullptr) {
        if (std::dynamic_pointer_cast<ChestTileEntity>(result) != nullptr) {
            int id = level->getTile(xt, yt, zt);
            Tile* tile = Tile::tiles[id];

            if (dynamic_cast<ChestTile*>(tile) != nullptr) {
                result = ((ChestTile*)tile)->getContainer(level, xt, yt, zt);
            }
        }
    }

    if (result == nullptr) {
        AABB block_above{x, y, z, x + 1, y + 1, z + 1};
        std::vector<std::shared_ptr<Entity>>* entities = level->getEntities(
            nullptr, &block_above, EntitySelector::CONTAINER_ENTITY_SELECTOR);

        if ((entities != nullptr) && (entities->size() > 0)) {
            result = std::dynamic_pointer_cast<Container>(
                entities->at(level->random->nextInt(entities->size())));
        }
    }

    return result;
}

bool HopperTileEntity::canMergeItems(std::shared_ptr<ItemInstance> a,
                                     std::shared_ptr<ItemInstance> b) {
    if (a->id != b->id) return false;
    if (a->getAuxValue() != b->getAuxValue()) return false;
    if (a->count > a->getMaxStackSize()) return false;
    if (!ItemInstance::tagMatches(a, b)) return false;
    return true;
}

Level* HopperTileEntity::getLevel() { return TileEntity::getLevel(); }

double HopperTileEntity::getLevelX() { return x; }

double HopperTileEntity::getLevelY() { return y; }

double HopperTileEntity::getLevelZ() { return z; }

void HopperTileEntity::setCooldown(int time) { cooldownTime = time; }

bool HopperTileEntity::isOnCooldown() { return cooldownTime > 0; }

// 4J Added
std::shared_ptr<TileEntity> HopperTileEntity::clone() {
    std::shared_ptr<HopperTileEntity> result =
        std::make_shared<HopperTileEntity>();
    TileEntity::clone(result);

    result->name = name;
    result->cooldownTime = cooldownTime;
    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i] != nullptr) {
            result->items[i] = ItemInstance::clone(items[i]);
        }
    }
    return result;
}
