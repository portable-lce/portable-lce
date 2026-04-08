#pragma once
#include "minecraft/client/gui/Screen.h"
class ClientConnection;

class ReceivingLevelScreen : public Screen {
private:
    ClientConnection* connection;
    int tickCount;

public:
    ReceivingLevelScreen(ClientConnection* connection);

protected:
    using Screen::keyPressed;

    virtual void keyPressed(char eventCharacter, int eventKey) override;

public:
    virtual void init() override;
    virtual void tick() override;

protected:
    virtual void buttonClicked(Button* button) override;

public:
    virtual void render(int xm, int ym, float a) override;
};
