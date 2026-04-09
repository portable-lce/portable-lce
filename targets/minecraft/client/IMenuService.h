#pragma once

#include <memory>
#include <string>

class LocalPlayer;
class Inventory;
class Container;
class Level;
class FurnaceTileEntity;
class BrewingStandTileEntity;
class DispenserTileEntity;
class SignTileEntity;
class CommandBlockEntity;
class HopperTileEntity;
class MinecartHopper;
class EntityHorse;
class BeaconTileEntity;
class Merchant;

class IMenuService {
public:
    virtual ~IMenuService() = default;

    virtual bool openInventory(int iPad, std::shared_ptr<LocalPlayer> player,
                               bool navigateBack = false) = 0;
    virtual bool openCreative(int iPad, std::shared_ptr<LocalPlayer> player,
                              bool navigateBack = false) = 0;
    virtual bool openCrafting2x2(int iPad,
                                 std::shared_ptr<LocalPlayer> player) = 0;
    virtual bool openCrafting3x3(int iPad, std::shared_ptr<LocalPlayer> player,
                                 int x, int y, int z) = 0;
    virtual bool openEnchanting(int iPad, std::shared_ptr<Inventory> inventory,
                                int x, int y, int z, Level* level,
                                const std::string& name) = 0;
    virtual bool openFurnace(int iPad, std::shared_ptr<Inventory> inventory,
                             std::shared_ptr<FurnaceTileEntity> furnace) = 0;
    virtual bool openBrewingStand(
        int iPad, std::shared_ptr<Inventory> inventory,
        std::shared_ptr<BrewingStandTileEntity> brewingStand) = 0;
    virtual bool openContainer(int iPad, std::shared_ptr<Container> inventory,
                               std::shared_ptr<Container> container) = 0;
    virtual bool openTrap(int iPad, std::shared_ptr<Container> inventory,
                          std::shared_ptr<DispenserTileEntity> trap) = 0;
    virtual bool openFireworks(int iPad, std::shared_ptr<LocalPlayer> player,
                               int x, int y, int z) = 0;
    virtual bool openSign(int iPad, std::shared_ptr<SignTileEntity> sign) = 0;
    virtual bool openRepairing(int iPad, std::shared_ptr<Inventory> inventory,
                               Level* level, int x, int y, int z) = 0;
    virtual bool openTrading(int iPad, std::shared_ptr<Inventory> inventory,
                             std::shared_ptr<Merchant> trader, Level* level,
                             const std::string& name) = 0;
    virtual bool openCommandBlock(
        int iPad, std::shared_ptr<CommandBlockEntity> commandBlock) = 0;
    virtual bool openHopper(int iPad, std::shared_ptr<Inventory> inventory,
                            std::shared_ptr<HopperTileEntity> hopper) = 0;
    virtual bool openHopperMinecart(int iPad,
                                    std::shared_ptr<Inventory> inventory,
                                    std::shared_ptr<MinecartHopper> hopper) = 0;
    virtual bool openHorse(int iPad, std::shared_ptr<Inventory> inventory,
                           std::shared_ptr<Container> container,
                           std::shared_ptr<EntityHorse> horse) = 0;
    virtual bool openBeacon(int iPad, std::shared_ptr<Inventory> inventory,
                            std::shared_ptr<BeaconTileEntity> beacon) = 0;
};
