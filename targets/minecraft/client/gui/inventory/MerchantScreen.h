#pragma once

#include <memory>

#include "AbstractContainerScreen.h"
#include "minecraft/world/inventory/MerchantMenu.h"

class TradeSwitchButton;
class Inventory;
class Level;
class Merchant;
class MerchantMenu;

class MerchantScreen : public AbstractContainerScreen {
public:
    MerchantScreen(std::shared_ptr<Inventory> inventory,
                   std::shared_ptr<Merchant> merchant, Level* level);
    virtual ~MerchantScreen();

    void init() override;
    void removed() override;
    void renderLabels() override;
    void renderBg(float a) override;
    void render(int xm, int ym, float a) override;
    void tick() override;
    void buttonClicked(Button* button) override;

    std::shared_ptr<Merchant> getMerchant() { return merchant; }

private:
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<Merchant> merchant;
    MerchantMenu* merchantMenu;
    TradeSwitchButton* nextRecipeButton;
    TradeSwitchButton* prevRecipeButton;
    int currentRecipeIndex;
};