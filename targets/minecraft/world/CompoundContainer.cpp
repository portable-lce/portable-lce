#include "CompoundContainer.h"

#include "minecraft/IGameServices.h"
#include "minecraft/network/packet/ContainerOpenPacket.h"
#include "minecraft/world/Container.h"

CompoundContainer::CompoundContainer(int name, std::shared_ptr<Container> c1,
                                     std::shared_ptr<Container> c2) {
    this->name = name;
    if (c1 == nullptr) c1 = c2;
    if (c2 == nullptr) c2 = c1;
    this->c1 = c1;
    this->c2 = c2;
}

int CompoundContainer::getContainerType() {
    return ContainerOpenPacket::LARGE_CHEST;
}

unsigned int CompoundContainer::getContainerSize() {
    return c1->getContainerSize() + c2->getContainerSize();
}

bool CompoundContainer::contains(std::shared_ptr<Container> c) {
    return c1 == c || c2 == c;
}

std::string CompoundContainer::getName() {
    if (c1->hasCustomName()) return c1->getName();
    if (c2->hasCustomName()) return c2->getName();
    return gameServices().getString(name);
}

std::string CompoundContainer::getCustomName() {
    if (c1->hasCustomName()) return c1->getName();
    if (c2->hasCustomName()) return c2->getName();
    return "";
}

bool CompoundContainer::hasCustomName() {
    return c1->hasCustomName() || c2->hasCustomName();
}

std::shared_ptr<ItemInstance> CompoundContainer::getItem(unsigned int slot) {
    if (slot >= c1->getContainerSize())
        return c2->getItem(slot - c1->getContainerSize());
    else
        return c1->getItem(slot);
}

std::shared_ptr<ItemInstance> CompoundContainer::removeItem(unsigned int slot,
                                                            int i) {
    if (slot >= c1->getContainerSize())
        return c2->removeItem(slot - c1->getContainerSize(), i);
    else
        return c1->removeItem(slot, i);
}

std::shared_ptr<ItemInstance> CompoundContainer::removeItemNoUpdate(int slot) {
    if (slot >= c1->getContainerSize())
        return c2->removeItemNoUpdate(slot - c1->getContainerSize());
    else
        return c1->removeItemNoUpdate(slot);
}

void CompoundContainer::setItem(unsigned int slot,
                                std::shared_ptr<ItemInstance> item) {
    if (slot >= c1->getContainerSize())
        c2->setItem(slot - c1->getContainerSize(), item);
    else
        c1->setItem(slot, item);
}

int CompoundContainer::getMaxStackSize() { return c1->getMaxStackSize(); }

void CompoundContainer::setChanged() {
    c1->setChanged();
    c2->setChanged();
}

bool CompoundContainer::stillValid(std::shared_ptr<Player> player) {
    return c1->stillValid(player) && c2->stillValid(player);
}

void CompoundContainer::startOpen() {
    c1->startOpen();
    c2->startOpen();
}

void CompoundContainer::stopOpen() {
    c1->stopOpen();
    c2->stopOpen();
}

bool CompoundContainer::canPlaceItem(int slot,
                                     std::shared_ptr<ItemInstance> item) {
    return true;
}