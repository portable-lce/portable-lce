#include "app/common/GameMenuService.h"

#include "app/common/Game.h"

bool GameMenuService::openInventory(int iPad,
                                    std::shared_ptr<LocalPlayer> player,
                                    bool navigateBack) {
    return game_.LoadInventoryMenu(iPad, player, navigateBack);
}
bool GameMenuService::openCreative(int iPad,
                                   std::shared_ptr<LocalPlayer> player,
                                   bool navigateBack) {
    return game_.LoadCreativeMenu(iPad, player, navigateBack);
}
bool GameMenuService::openCrafting2x2(int iPad,
                                      std::shared_ptr<LocalPlayer> player) {
    return game_.LoadCrafting2x2Menu(iPad, player);
}
bool GameMenuService::openCrafting3x3(int iPad,
                                      std::shared_ptr<LocalPlayer> player,
                                      int x, int y, int z) {
    return game_.LoadCrafting3x3Menu(iPad, player, x, y, z);
}
bool GameMenuService::openEnchanting(int iPad,
                                     std::shared_ptr<Inventory> inventory,
                                     int x, int y, int z, Level* level,
                                     const std::string& name) {
    return game_.LoadEnchantingMenu(iPad, inventory, x, y, z, level, name);
}
bool GameMenuService::openFurnace(int iPad,
                                  std::shared_ptr<Inventory> inventory,
                                  std::shared_ptr<FurnaceTileEntity> furnace) {
    return game_.LoadFurnaceMenu(iPad, inventory, furnace);
}
bool GameMenuService::openBrewingStand(
    int iPad, std::shared_ptr<Inventory> inventory,
    std::shared_ptr<BrewingStandTileEntity> brewingStand) {
    return game_.LoadBrewingStandMenu(iPad, inventory, brewingStand);
}
bool GameMenuService::openContainer(int iPad,
                                    std::shared_ptr<Container> inventory,
                                    std::shared_ptr<Container> container) {
    return game_.LoadContainerMenu(iPad, inventory, container);
}
bool GameMenuService::openTrap(int iPad, std::shared_ptr<Container> inventory,
                               std::shared_ptr<DispenserTileEntity> trap) {
    return game_.LoadTrapMenu(iPad, inventory, trap);
}
bool GameMenuService::openFireworks(int iPad,
                                    std::shared_ptr<LocalPlayer> player, int x,
                                    int y, int z) {
    return game_.LoadFireworksMenu(iPad, player, x, y, z);
}
bool GameMenuService::openSign(int iPad, std::shared_ptr<SignTileEntity> sign) {
    return game_.LoadSignEntryMenu(iPad, sign);
}
bool GameMenuService::openRepairing(int iPad,
                                    std::shared_ptr<Inventory> inventory,
                                    Level* level, int x, int y, int z) {
    return game_.LoadRepairingMenu(iPad, inventory, level, x, y, z);
}
bool GameMenuService::openTrading(int iPad,
                                  std::shared_ptr<Inventory> inventory,
                                  std::shared_ptr<Merchant> trader,
                                  Level* level, const std::string& name) {
    return game_.LoadTradingMenu(iPad, inventory, trader, level, name);
}
bool GameMenuService::openCommandBlock(
    int iPad, std::shared_ptr<CommandBlockEntity> commandBlock) {
    return game_.LoadCommandBlockMenu(iPad, commandBlock);
}
bool GameMenuService::openHopper(int iPad, std::shared_ptr<Inventory> inventory,
                                 std::shared_ptr<HopperTileEntity> hopper) {
    return game_.LoadHopperMenu(iPad, inventory, hopper);
}
bool GameMenuService::openHopperMinecart(
    int iPad, std::shared_ptr<Inventory> inventory,
    std::shared_ptr<MinecartHopper> hopper) {
    return game_.LoadHopperMenu(iPad, inventory, hopper);
}
bool GameMenuService::openHorse(int iPad, std::shared_ptr<Inventory> inventory,
                                std::shared_ptr<Container> container,
                                std::shared_ptr<EntityHorse> horse) {
    return game_.LoadHorseMenu(iPad, inventory, container, horse);
}
bool GameMenuService::openBeacon(int iPad, std::shared_ptr<Inventory> inventory,
                                 std::shared_ptr<BeaconTileEntity> beacon) {
    return game_.LoadBeaconMenu(iPad, inventory, beacon);
}
