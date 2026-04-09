#pragma once

#include "minecraft/client/IMenuService.h"

class Game;

class GameMenuService : public IMenuService {
public:
    explicit GameMenuService(Game& game) : game_(game) {}

    bool openInventory(int iPad, std::shared_ptr<LocalPlayer> player,
                       bool navigateBack) override;
    bool openCreative(int iPad, std::shared_ptr<LocalPlayer> player,
                      bool navigateBack) override;
    bool openCrafting2x2(int iPad,
                         std::shared_ptr<LocalPlayer> player) override;
    bool openCrafting3x3(int iPad, std::shared_ptr<LocalPlayer> player, int x,
                         int y, int z) override;
    bool openEnchanting(int iPad, std::shared_ptr<Inventory> inventory, int x,
                        int y, int z, Level* level,
                        const std::string& name) override;
    bool openFurnace(int iPad, std::shared_ptr<Inventory> inventory,
                     std::shared_ptr<FurnaceTileEntity> furnace) override;
    bool openBrewingStand(
        int iPad, std::shared_ptr<Inventory> inventory,
        std::shared_ptr<BrewingStandTileEntity> brewingStand) override;
    bool openContainer(int iPad, std::shared_ptr<Container> inventory,
                       std::shared_ptr<Container> container) override;
    bool openTrap(int iPad, std::shared_ptr<Container> inventory,
                  std::shared_ptr<DispenserTileEntity> trap) override;
    bool openFireworks(int iPad, std::shared_ptr<LocalPlayer> player, int x,
                       int y, int z) override;
    bool openSign(int iPad, std::shared_ptr<SignTileEntity> sign) override;
    bool openRepairing(int iPad, std::shared_ptr<Inventory> inventory,
                       Level* level, int x, int y, int z) override;
    bool openTrading(int iPad, std::shared_ptr<Inventory> inventory,
                     std::shared_ptr<Merchant> trader, Level* level,
                     const std::string& name) override;
    bool openCommandBlock(
        int iPad, std::shared_ptr<CommandBlockEntity> commandBlock) override;
    bool openHopper(int iPad, std::shared_ptr<Inventory> inventory,
                    std::shared_ptr<HopperTileEntity> hopper) override;
    bool openHopperMinecart(int iPad, std::shared_ptr<Inventory> inventory,
                            std::shared_ptr<MinecartHopper> hopper) override;
    bool openHorse(int iPad, std::shared_ptr<Inventory> inventory,
                   std::shared_ptr<Container> container,
                   std::shared_ptr<EntityHorse> horse) override;
    bool openBeacon(int iPad, std::shared_ptr<Inventory> inventory,
                    std::shared_ptr<BeaconTileEntity> beacon) override;

private:
    Game& game_;
};
