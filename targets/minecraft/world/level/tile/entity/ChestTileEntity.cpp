#include "minecraft/IGameServices.h"
#include "ChestTileEntity.h"

#include <stdint.h>

#include <vector>

#include "Direction.h"
#include "SharedConstants.h"
#include "TileEntity.h"
#include "java/Random.h"
#include "minecraft/network/packet/ContainerOpenPacket.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/world/CompoundContainer.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/inventory/ContainerMenu.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/ChestTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"

class Entity;

int ChestTileEntity::getContainerType() {
    if (isBonusChest)
        return ContainerOpenPacket::BONUS_CHEST;
    else
        return ContainerOpenPacket::CONTAINER;
}

void ChestTileEntity::_init(bool isBonusChest) {
    items = new std::vector<std::shared_ptr<ItemInstance>>(9 * 4);

    hasCheckedNeighbors = false;
    this->isBonusChest = isBonusChest;

    openness = 0.0f;
    oOpenness = 0.0f;
    openCount = 0;
    tickInterval = 0;

    type = -1;
    name = "";
}

ChestTileEntity::ChestTileEntity(bool isBonusChest /* = false*/)
    : TileEntity() {
    _init(isBonusChest);
}

ChestTileEntity::ChestTileEntity(int type, bool isBonusChest /* = false*/)
    : TileEntity() {
    _init(isBonusChest);

    this->type = type;
}

ChestTileEntity::~ChestTileEntity() { delete items; }

unsigned int ChestTileEntity::getContainerSize() { return 9 * 3; }

std::shared_ptr<ItemInstance> ChestTileEntity::getItem(unsigned int slot) {
    return (*items)[slot];
}

std::shared_ptr<ItemInstance> ChestTileEntity::removeItem(unsigned int slot,
                                                          int count) {
    if ((*items)[slot] != nullptr) {
        if ((*items)[slot]->count <= count) {
            std::shared_ptr<ItemInstance> item = (*items)[slot];
            (*items)[slot] = nullptr;
            setChanged();
            // 4J Stu - Fix for duplication glitch
            if (item->count <= 0) return nullptr;
            return item;
        } else {
            std::shared_ptr<ItemInstance> i = (*items)[slot]->remove(count);
            if ((*items)[slot]->count == 0) (*items)[slot] = nullptr;
            setChanged();
            // 4J Stu - Fix for duplication glitch
            if (i->count <= 0) return nullptr;
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> ChestTileEntity::removeItemNoUpdate(int slot) {
    if ((*items)[slot] != nullptr) {
        std::shared_ptr<ItemInstance> item = (*items)[slot];
        (*items)[slot] = nullptr;
        return item;
    }
    return nullptr;
}

void ChestTileEntity::setItem(unsigned int slot,
                              std::shared_ptr<ItemInstance> item) {
    (*items)[slot] = item;
    if (item != nullptr && item->count > getMaxStackSize())
        item->count = getMaxStackSize();
    this->setChanged();
}

std::string ChestTileEntity::getName() {
    return hasCustomName() ? name : gameServices().getString(IDS_TILE_CHEST);
}

std::string ChestTileEntity::getCustomName() {
    return hasCustomName() ? name : "";
}

bool ChestTileEntity::hasCustomName() { return !name.empty(); }

void ChestTileEntity::setCustomName(const std::string& name) {
    this->name = name;
}

void ChestTileEntity::load(CompoundTag* base) {
    TileEntity::load(base);
    ListTag<CompoundTag>* inventoryList =
        (ListTag<CompoundTag>*)base->getList("Items");
    if (items) {
        delete items;
    }
    items = new std::vector<std::shared_ptr<ItemInstance>>(getContainerSize());
    if (base->contains("CustomName")) name = base->getString("CustomName");
    for (int i = 0; i < inventoryList->size(); i++) {
        CompoundTag* tag = inventoryList->get(i);
        unsigned int slot = tag->getByte("Slot") & 0xff;
        if (slot >= 0 && slot < items->size())
            (*items)[slot] = ItemInstance::fromTag(tag);
    }
    isBonusChest = base->getBoolean("bonus");
}

void ChestTileEntity::save(CompoundTag* base) {
    TileEntity::save(base);
    ListTag<CompoundTag>* listTag = new ListTag<CompoundTag>;

    for (unsigned int i = 0; i < items->size(); i++) {
        if ((*items)[i] != nullptr) {
            CompoundTag* tag = new CompoundTag();
            tag->putByte("Slot", (uint8_t)i);
            (*items)[i]->save(tag);
            listTag->add(tag);
        }
    }
    base->put("Items", listTag);
    if (hasCustomName()) base->putString("CustomName", name);
    base->putBoolean("bonus", isBonusChest);
}

int ChestTileEntity::getMaxStackSize() {
    return Container::LARGE_MAX_STACK_SIZE;
}

bool ChestTileEntity::stillValid(std::shared_ptr<Player> player) {
    if (level->getTileEntity(x, y, z) != shared_from_this()) return false;
    if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
    return true;
}

void ChestTileEntity::setChanged() { TileEntity::setChanged(); }

void ChestTileEntity::clearCache() {
    TileEntity::clearCache();
    hasCheckedNeighbors = false;
}

void ChestTileEntity::heyImYourNeighbor(
    std::shared_ptr<ChestTileEntity> neighbor, int from) {
    if (neighbor->isRemoved()) {
        hasCheckedNeighbors = false;
    } else if (hasCheckedNeighbors) {
        switch (from) {
            case Direction::NORTH:
                if (n.lock() != neighbor) hasCheckedNeighbors = false;
                break;
            case Direction::SOUTH:
                if (s.lock() != neighbor) hasCheckedNeighbors = false;
                break;
            case Direction::EAST:
                if (e.lock() != neighbor) hasCheckedNeighbors = false;
                break;
            case Direction::WEST:
                if (w.lock() != neighbor) hasCheckedNeighbors = false;
                break;
        }
    }
}

void ChestTileEntity::checkNeighbors() {
    if (hasCheckedNeighbors) return;

    hasCheckedNeighbors = true;
    n = std::weak_ptr<ChestTileEntity>();
    e = std::weak_ptr<ChestTileEntity>();
    w = std::weak_ptr<ChestTileEntity>();
    s = std::weak_ptr<ChestTileEntity>();

    if (isSameChest(x - 1, y, z)) {
        w = std::dynamic_pointer_cast<ChestTileEntity>(
            level->getTileEntity(x - 1, y, z));
    }
    if (isSameChest(x + 1, y, z)) {
        e = std::dynamic_pointer_cast<ChestTileEntity>(
            level->getTileEntity(x + 1, y, z));
    }
    if (isSameChest(x, y, z - 1)) {
        n = std::dynamic_pointer_cast<ChestTileEntity>(
            level->getTileEntity(x, y, z - 1));
    }
    if (isSameChest(x, y, z + 1)) {
        s = std::dynamic_pointer_cast<ChestTileEntity>(
            level->getTileEntity(x, y, z + 1));
    }

    std::shared_ptr<ChestTileEntity> cteThis =
        std::dynamic_pointer_cast<ChestTileEntity>(shared_from_this());
    if (n.lock() != nullptr)
        n.lock()->heyImYourNeighbor(cteThis, Direction::SOUTH);
    if (s.lock() != nullptr)
        s.lock()->heyImYourNeighbor(cteThis, Direction::NORTH);
    if (e.lock() != nullptr)
        e.lock()->heyImYourNeighbor(cteThis, Direction::WEST);
    if (w.lock() != nullptr)
        w.lock()->heyImYourNeighbor(cteThis, Direction::EAST);
}

bool ChestTileEntity::isSameChest(int x, int y, int z) {
    Tile* tile = Tile::tiles[level->getTile(x, y, z)];
    if (tile == nullptr || !(dynamic_cast<ChestTile*>(tile) != nullptr))
        return false;
    return ((ChestTile*)tile)->type == getType();
}

void ChestTileEntity::tick() {
    TileEntity::tick();
    checkNeighbors();

    ++tickInterval;
    if (!level->isClientSide && openCount != 0 &&
        (tickInterval + x + y + z) % (SharedConstants::TICKS_PER_SECOND * 10) ==
            0) {
        //            level.tileEvent(x, y, z, Tile.chest.id,
        //            ChestTile.EVENT_SET_OPEN_COUNT, openCount);

        openCount = 0;

        float range = 5;
        AABB player_aabb(x - range, y - range, z - range, x + 1 + range,
                         y + 1 + range, z + 1 + range);
        std::vector<std::shared_ptr<Entity>>* players =
            level->getEntitiesOfClass(typeid(Player), &player_aabb);
        for (auto it = players->begin(); it != players->end(); ++it) {
            std::shared_ptr<Player> player =
                std::dynamic_pointer_cast<Player>(*it);

            ContainerMenu* containerMenu =
                dynamic_cast<ContainerMenu*>(player->containerMenu);
            if (containerMenu != nullptr) {
                std::shared_ptr<Container> container =
                    containerMenu->getContainer();
                std::shared_ptr<Container> thisContainer =
                    std::dynamic_pointer_cast<Container>(shared_from_this());
                std::shared_ptr<CompoundContainer> compoundContainer =
                    std::dynamic_pointer_cast<CompoundContainer>(container);
                if ((container == thisContainer) ||
                    (compoundContainer != nullptr &&
                     compoundContainer->contains(thisContainer))) {
                    openCount++;
                }
            }
        }
        delete players;
    }

    oOpenness = openness;

    float speed = 0.10f;
    if (openCount > 0 && openness == 0) {
        if (n.lock() == nullptr && w.lock() == nullptr) {
            double xc = x + 0.5;
            double zc = z + 0.5;
            if (s.lock() != nullptr) zc += 0.5;
            if (e.lock() != nullptr) xc += 0.5;

            // 4J-PB - Seems the chest open volume is much louder than other
            // sounds from user reports. We'll tone it down a bit
            level->playSound(xc, y + 0.5, zc, eSoundType_RANDOM_CHEST_OPEN,
                             0.2f, level->random->nextFloat() * 0.1f + 0.9f);
        }
    }
    if ((openCount == 0 && openness > 0) || (openCount > 0 && openness < 1)) {
        float oldOpen = openness;
        if (openCount > 0)
            openness += speed;
        else
            openness -= speed;
        if (openness > 1) {
            openness = 1;
        }
        float lim = 0.5f;
        if (openness < lim && oldOpen >= lim) {
            // Fix for #64546 - Customer Encountered: TU7: Chests placed by the
            // Player are closing too fast.
            // openness = 0;
            if (n.lock() == nullptr && w.lock() == nullptr) {
                double xc = x + 0.5;
                double zc = z + 0.5;
                if (s.lock() != nullptr) zc += 0.5;
                if (e.lock() != nullptr) xc += 0.5;

                // 4J-PB - Seems the chest open volume is much louder than other
                // sounds from user reports. We'll tone it down a bit
                level->playSound(xc, y + 0.5, zc, eSoundType_RANDOM_CHEST_CLOSE,
                                 0.2f,
                                 level->random->nextFloat() * 0.1f + 0.9f);
            }
        }
        if (openness < 0) {
            openness = 0;
        }
    }
}

bool ChestTileEntity::triggerEvent(int b0, int b1) {
    if (b0 == ChestTile::EVENT_SET_OPEN_COUNT) {
        openCount = b1;
        return true;
    }
    return TileEntity::triggerEvent(b0, b1);
}

void ChestTileEntity::startOpen() {
    if (openCount < 0) {
        openCount = 0;
    }
    openCount++;
    level->tileEvent(x, y, z, getTile()->id, ChestTile::EVENT_SET_OPEN_COUNT,
                     openCount);
    level->updateNeighborsAt(x, y, z, getTile()->id);
    level->updateNeighborsAt(x, y - 1, z, getTile()->id);
}

void ChestTileEntity::stopOpen() {
    if (getTile() == nullptr ||
        !(dynamic_cast<ChestTile*>(getTile()) != nullptr))
        return;
    openCount--;
    level->tileEvent(x, y, z, getTile()->id, ChestTile::EVENT_SET_OPEN_COUNT,
                     openCount);
    level->updateNeighborsAt(x, y, z, getTile()->id);
    level->updateNeighborsAt(x, y - 1, z, getTile()->id);
}

bool ChestTileEntity::canPlaceItem(int slot,
                                   std::shared_ptr<ItemInstance> item) {
    return true;
}

void ChestTileEntity::setRemoved() {
    TileEntity::setRemoved();
    clearCache();
    checkNeighbors();
}

int ChestTileEntity::getType() {
    if (type == -1) {
        if (level != nullptr &&
            dynamic_cast<ChestTile*>(getTile()) != nullptr) {
            type = ((ChestTile*)getTile())->type;
        } else {
            return ChestTile::TYPE_BASIC;
        }
    }

    return type;
}

// 4J Added
std::shared_ptr<TileEntity> ChestTileEntity::clone() {
    std::shared_ptr<ChestTileEntity> result =
        std::make_shared<ChestTileEntity>();
    TileEntity::clone(result);

    for (unsigned int i = 0; i < items->size(); i++) {
        if ((*items)[i] != nullptr) {
            (*result->items)[i] = ItemInstance::clone((*items)[i]);
        }
    }
    return result;
}
