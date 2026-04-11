#pragma once

#include <memory>
#include <vector>
#include <string>

class ItemInstance;
class Player;

class Container {
public:
    virtual ~Container() {}
    static const int LARGE_MAX_STACK_SIZE = 64;

    // 4J-JEV: Added to distinguish between ender, bonus, small and large chests
    virtual int getContainerType() { return -1; }

    virtual unsigned int getContainerSize() = 0;
    virtual std::shared_ptr<ItemInstance> getItem(unsigned int slot) = 0;
    virtual std::shared_ptr<ItemInstance> removeItem(unsigned int slot,
                                                     int count) = 0;
    virtual std::shared_ptr<ItemInstance> removeItemNoUpdate(int slot) = 0;
    virtual void setItem(unsigned int slot,
                         std::shared_ptr<ItemInstance> item) = 0;
    virtual std::string getName() = 0;
    virtual std::string
    getCustomName() = 0;  // 4J Stu added for sending over the network
    virtual bool hasCustomName() = 0;
    virtual int getMaxStackSize() = 0;
    virtual void setChanged() = 0;
    virtual bool stillValid(std::shared_ptr<Player> player) = 0;
    virtual void startOpen() = 0;
    virtual void stopOpen() = 0;
    virtual bool canPlaceItem(int slot, std::shared_ptr<ItemInstance> item) = 0;
};