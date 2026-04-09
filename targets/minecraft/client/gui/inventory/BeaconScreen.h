#pragma once

#include <memory>

#include "AbstractContainerScreen.h"
#include "minecraft/world/inventory/BeaconMenu.h"

class BeaconConfirmButton;
class BeaconCancelButton;
class BeaconMenu;
class BeaconTileEntity;
class Inventory;

class BeaconScreen : public AbstractContainerScreen {
public:
    BeaconScreen(std::shared_ptr<Inventory> inventory,
                 std::shared_ptr<BeaconTileEntity> beacon);
    virtual ~BeaconScreen();

    void init() override;
    void removed() override;
    void tick() override;
    void renderLabels() override;
    void renderBg(float a) override;
    void render(int xm, int ym, float a) override;
    void buttonClicked(Button* button) override;

    std::shared_ptr<BeaconTileEntity> getBeacon() { return beacon; }

private:
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<BeaconTileEntity> beacon;
    BeaconMenu* beaconMenu;
    BeaconConfirmButton* beaconConfirmButton;
    bool buttonsNotDrawn;
};