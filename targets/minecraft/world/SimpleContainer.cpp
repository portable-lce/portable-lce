#include "minecraft/IGameServices.h"
#include "SimpleContainer.h"

#include <vector>

#include "minecraft/world/Container.h"
#include "minecraft/world/item/ItemInstance.h"
#include "net.minecraft.world.ContainerListener.h"

SimpleContainer::SimpleContainer(int name, std::string stringName,
                                 bool customName, int size) {
    this->name = name;
    this->stringName = stringName;
    this->customName = customName;
    this->size = size;
    items = new std::vector<std::shared_ptr<ItemInstance>>(size);

    listeners = nullptr;
}

void SimpleContainer::addListener(
    net_minecraft_world::ContainerListener* listener) {
    if (listeners == nullptr)
        listeners = new std::vector<net_minecraft_world::ContainerListener*>();
    listeners->push_back(listener);
}

void SimpleContainer::removeListener(
    net_minecraft_world::ContainerListener* listener) {
    // 4J Java has a remove function on lists that will find the first occurence
    // of an object and remove it. We need to replicate that ourselves

    std::vector<net_minecraft_world::ContainerListener*>::iterator it =
        listeners->begin();
    std::vector<net_minecraft_world::ContainerListener*>::iterator itEnd =
        listeners->end();
    while (it != itEnd && *it != listener) it++;

    if (it != itEnd) listeners->erase(it);
}

std::shared_ptr<ItemInstance> SimpleContainer::getItem(unsigned int slot) {
    return (*items)[slot];
}

std::shared_ptr<ItemInstance> SimpleContainer::removeItem(unsigned int slot,
                                                          int count) {
    if ((*items)[slot] != nullptr) {
        if ((*items)[slot]->count <= count) {
            std::shared_ptr<ItemInstance> item = (*items)[slot];
            (*items)[slot] = nullptr;
            setChanged();
            return item;
        } else {
            std::shared_ptr<ItemInstance> i = (*items)[slot]->remove(count);
            if ((*items)[slot]->count == 0) (*items)[slot] = nullptr;
            setChanged();
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<ItemInstance> SimpleContainer::removeItemNoUpdate(int slot) {
    if ((*items)[slot] != nullptr) {
        std::shared_ptr<ItemInstance> item = (*items)[slot];
        (*items)[slot] = nullptr;
        return item;
    }
    return nullptr;
}

void SimpleContainer::setItem(unsigned int slot,
                              std::shared_ptr<ItemInstance> item) {
    (*items)[slot] = item;
    if (item != nullptr && item->count > getMaxStackSize())
        item->count = getMaxStackSize();
    setChanged();
}

unsigned int SimpleContainer::getContainerSize() { return size; }

std::string SimpleContainer::getName() {
    return stringName.empty() ? gameServices().getString(name) : stringName;
}

std::string SimpleContainer::getCustomName() {
    return hasCustomName() ? stringName : "";
}

bool SimpleContainer::hasCustomName() { return customName; }

void SimpleContainer::setCustomName(const std::string& name) {
    customName = true;
    this->stringName = name;
}

int SimpleContainer::getMaxStackSize() {
    return Container::LARGE_MAX_STACK_SIZE;
}

void SimpleContainer::setChanged() {
    if (listeners != nullptr)
        for (unsigned int i = 0; i < listeners->size(); i++) {
            listeners->at(i)->containerChanged();  // shared_from_this());
        }
}

bool SimpleContainer::stillValid(std::shared_ptr<Player> player) {
    return true;
}

bool SimpleContainer::canPlaceItem(int slot,
                                   std::shared_ptr<ItemInstance> item) {
    return true;
}