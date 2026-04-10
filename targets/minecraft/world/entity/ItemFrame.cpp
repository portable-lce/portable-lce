#include "ItemFrame.h"

#include <stdint.h>

#include <string>

#include "java/Random.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/HangingEntity.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/MapItem.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/saveddata/MapItemSavedData.h"
#include "nbt/CompoundTag.h"

// 4J - added for common ctor code
void ItemFrame::_init() {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();

    dropChance = 1;
}

ItemFrame::ItemFrame(Level* level) : HangingEntity(level) { _init(); }

ItemFrame::ItemFrame(Level* level, int xTile, int yTile, int zTile, int dir)
    : HangingEntity(level, xTile, yTile, zTile, dir) {
    _init();
    setDir(dir);
}

void ItemFrame::defineSynchedData() {
    getEntityData()->defineNULL(DATA_ITEM, nullptr);
    getEntityData()->define(DATA_ROTATION, (uint8_t)0);
}

bool ItemFrame::shouldRenderAtSqrDistance(double distance) {
    double size = 16;
    size *= 64.0f * viewScale;
    return distance < size * size;
}

void ItemFrame::dropItem(std::shared_ptr<Entity> causedBy) {
    std::shared_ptr<ItemInstance> item = getItem();

    if (causedBy != nullptr && causedBy->instanceof(eTYPE_PLAYER)) {
        if (std::dynamic_pointer_cast<Player>(causedBy)->abilities.instabuild) {
            removeFramedMap(item);
            return;
        }
    }

    spawnAtLocation(std::make_shared<ItemInstance>(Item::frame), 0);
    if ((item != nullptr) && (random->nextFloat() < dropChance)) {
        item = item->copy();
        removeFramedMap(item);
        spawnAtLocation(item, 0);
    }
}

void ItemFrame::removeFramedMap(std::shared_ptr<ItemInstance> item) {
    if (item == nullptr) return;
    if (item->id == Item::map_Id) {
        std::shared_ptr<MapItemSavedData> mapItemSavedData =
            Item::map->getSavedData(item, level);
        mapItemSavedData->removeItemFrameDecoration(item);
        // mapItemSavedData.decorations.remove("frame-" + entityId);
    }
    item->setFramed(nullptr);
}

std::shared_ptr<ItemInstance> ItemFrame::getItem() {
    return getEntityData()->getItemInstance(DATA_ITEM);
}

void ItemFrame::setItem(std::shared_ptr<ItemInstance> item) {
    if (item != nullptr) {
        item = item->copy();
        item->count = 1;

        item->setFramed(
            std::dynamic_pointer_cast<ItemFrame>(shared_from_this()));
    }
    getEntityData()->set(DATA_ITEM, item);
    getEntityData()->markDirty(DATA_ITEM);
}

int ItemFrame::getRotation() { return getEntityData()->getByte(DATA_ROTATION); }

void ItemFrame::setRotation(int rotation) {
    getEntityData()->set(DATA_ROTATION, (uint8_t)(rotation % 4));
}

void ItemFrame::addAdditonalSaveData(CompoundTag* tag) {
    if (getItem() != nullptr) {
        tag->putCompound("Item", getItem()->save(new CompoundTag()));
        tag->putByte("ItemRotation", (uint8_t)getRotation());
        tag->putFloat("ItemDropChance", dropChance);
    }
    HangingEntity::addAdditonalSaveData(tag);
}

void ItemFrame::readAdditionalSaveData(CompoundTag* tag) {
    CompoundTag* itemTag = tag->getCompound("Item");
    if (itemTag != nullptr && !itemTag->isEmpty()) {
        setItem(ItemInstance::fromTag(itemTag));
        setRotation(tag->getByte("ItemRotation"));

        if (tag->contains("ItemDropChance"))
            dropChance = tag->getFloat("ItemDropChance");
    }
    HangingEntity::readAdditionalSaveData(tag);
}

bool ItemFrame::interact(std::shared_ptr<Player> player) {
    if (!player->isAllowedToInteract(shared_from_this())) {
        return false;
    }

    if (getItem() == nullptr) {
        std::shared_ptr<ItemInstance> item = player->getCarriedItem();

        if (item != nullptr) {
            if (!level->isClientSide)  // isClientSide)
            {
                setItem(item);

                if (!player->abilities.instabuild) {
                    if (--item->count <= 0) {
                        player->inventory->setItem(player->inventory->selected,
                                                   nullptr);
                    }
                }
            }
        }
    } else {
        if (!level->isClientSide)  // isClientSide)
        {
            setRotation(getRotation() + 1);
        }
    }

    return true;
}
