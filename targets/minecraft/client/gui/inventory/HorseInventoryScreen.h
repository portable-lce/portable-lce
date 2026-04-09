#pragma once
#include <memory>

#include "AbstractContainerScreen.h"

class Container;
class EntityHorse;
class Inventory;

class HorseInventoryScreen : public AbstractContainerScreen {
public:
    HorseInventoryScreen(std::shared_ptr<Inventory> inventory,
                         std::shared_ptr<Container> horseContainer,
                         std::shared_ptr<EntityHorse> horse);

    virtual void init() override;
    virtual void renderLabels() override;
    virtual void renderBg(float a) override;
    virtual void render(int xm, int ym, float a) override;

private:
    std::shared_ptr<Inventory> inventory;
    std::shared_ptr<Container> horseContainer;
    std::shared_ptr<EntityHorse> horse;
    float xMouse, yMouse;
};