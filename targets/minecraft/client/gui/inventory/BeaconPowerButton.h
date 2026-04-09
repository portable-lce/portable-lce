#pragma once
#include "AbstractBeaconButton.h"

class BeaconScreen;

class BeaconPowerButton : public AbstractBeaconButton {
public:
    BeaconPowerButton(BeaconScreen* screen, int id, int x, int y, int effectId,
                      int tier);
    void renderTooltip(int xm, int ym) override;
    bool isSelected() const { return selected; }

private:
    BeaconScreen* screen;
    int effectId;
    int tier;
};