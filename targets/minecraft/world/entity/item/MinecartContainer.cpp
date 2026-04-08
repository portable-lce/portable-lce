#include "minecraft/IGameServices.h"
#include "MinecartContainer.h"

#include <stdint.h>

#include "java/Random.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/entity/item/Minecart.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/redstone/Redstone.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"

void MinecartContainer::_init() {
    items = std::vector<std::shared_ptr<ItemInstance>>(9 * 4);
    dropEquipment = true;

    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
}

MinecartContainer::MinecartContainer(Level* level) : Minecart(level) {
    _init();
}

MinecartContainer::MinecartContainer(Level* level, double x, double y, double z)
    : Minecart(level, x, y, z) {
    _init();
}

void MinecartContainer::destroy(DamageSource* source) {
    Minecart::destroy(source);

    for (int i = 0; i < getContainerSize(); i++) {
        std::shared_ptr<ItemInstance> item = getItem(i);
        if (item != nullptr) {
            float xo = random->nextFloat() * 0.8f + 0.1f;
            float yo = random->nextFloat() * 0.8f + 0.1f;
            float zo = random->nextFloat() * 0.8f + 0.1f;

            while (item->count > 0) {
                int count = random->nextInt(21) + 10;
                if (count > item->count) count = item->count;
                item->count -= count;

                std::shared_ptr<ItemEntity> itemEntity =
                    std::make_shared<ItemEntity>(
                        level, x + xo, y + yo, z + zo,
                        std::make_shared<ItemInstance>(item->id, count,
                                                       item->getAuxValue()));
                float pow = 0.05f;
                itemEntity->xd = (float)random->nextGaussian() * pow;
                itemEntity->yd = (float)random->nextGaussian() * pow + 0.2f;
                itemEntity->zd = (float)random->nextGaussian() * pow;
                level->addEntity(itemEntity);
            }
        }
    }
}

std::shared_ptr<ItemInstance> MinecartContainer::getItem(unsigned int slot) {
    return items[slot];
}

std::shared_ptr<ItemInstance> MinecartContainer::removeItem(unsigned int slot,
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

std::shared_ptr<ItemInstance> MinecartContainer::removeItemNoUpdate(int slot) {
    if (items[slot] != nullptr) {
        std::shared_ptr<ItemInstance> item = items[slot];
        items[slot] = nullptr;
        return item;
    }
    return nullptr;
}

void MinecartContainer::setItem(unsigned int slot,
                                std::shared_ptr<ItemInstance> item) {
    items[slot] = item;
    if (item != nullptr && item->count > getMaxStackSize())
        item->count = getMaxStackSize();
}

void MinecartContainer::setChanged() {}

bool MinecartContainer::stillValid(std::shared_ptr<Player> player) {
    if (removed) return false;
    if (player->distanceToSqr(shared_from_this()) > 8 * 8) return false;
    return true;
}

void MinecartContainer::startOpen() {}

void MinecartContainer::stopOpen() {}

bool MinecartContainer::canPlaceItem(int slot,
                                     std::shared_ptr<ItemInstance> item) {
    return true;
}

std::string MinecartContainer::getName() {
    return hasCustomName() ? getCustomName()
                           : gameServices().getString(IDS_CONTAINER_MINECART);
}

int MinecartContainer::getMaxStackSize() {
    return Container::LARGE_MAX_STACK_SIZE;
}

void MinecartContainer::changeDimension(int i) {
    dropEquipment = false;
    Minecart::changeDimension(i);
}

void MinecartContainer::remove() {
    if (dropEquipment) {
        for (int i = 0; i < getContainerSize(); i++) {
            std::shared_ptr<ItemInstance> item = getItem(i);
            if (item != nullptr) {
                float xo = random->nextFloat() * 0.8f + 0.1f;
                float yo = random->nextFloat() * 0.8f + 0.1f;
                float zo = random->nextFloat() * 0.8f + 0.1f;

                while (item->count > 0) {
                    int count = random->nextInt(21) + 10;
                    if (count > item->count) count = item->count;
                    item->count -= count;

                    std::shared_ptr<ItemEntity> itemEntity =
                        std::make_shared<ItemEntity>(
                            level, x + xo, y + yo, z + zo,
                            std::make_shared<ItemInstance>(
                                item->id, count, item->getAuxValue()));

                    if (item->hasTag()) {
                        itemEntity->getItem()->setTag(
                            (CompoundTag*)item->getTag()->copy());
                    }

                    float pow = 0.05f;
                    itemEntity->xd = (float)random->nextGaussian() * pow;
                    itemEntity->yd = (float)random->nextGaussian() * pow + 0.2f;
                    itemEntity->zd = (float)random->nextGaussian() * pow;
                    level->addEntity(itemEntity);
                }
            }
        }
    }

    Minecart::remove();
}

void MinecartContainer::addAdditonalSaveData(CompoundTag* base) {
    Minecart::addAdditonalSaveData(base);

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
}

void MinecartContainer::readAdditionalSaveData(CompoundTag* base) {
    Minecart::readAdditionalSaveData(base);

    ListTag<CompoundTag>* inventoryList =
        (ListTag<CompoundTag>*)base->getList("Items");
    items = std::vector<std::shared_ptr<ItemInstance>>(getContainerSize());
    for (int i = 0; i < inventoryList->size(); i++) {
        CompoundTag* tag = inventoryList->get(i);
        int slot = tag->getByte("Slot") & 0xff;
        if (slot >= 0 && slot < (int)items.size())
            items[slot] = ItemInstance::fromTag(tag);
    }
}

bool MinecartContainer::interact(std::shared_ptr<Player> player) {
    if (!level->isClientSide) {
        player->openContainer(
            std::dynamic_pointer_cast<Container>(shared_from_this()));
    }

    return true;
}

void MinecartContainer::applyNaturalSlowdown() {
    std::shared_ptr<Container> container =
        std::dynamic_pointer_cast<Container>(shared_from_this());
    int emptiness =
        Redstone::SIGNAL_MAX -
        AbstractContainerMenu::getRedstoneSignalFromContainer(container);
    float keep = 0.98f + (emptiness * 0.001f);

    xd *= keep;
    yd *= 0;
    zd *= keep;
}
