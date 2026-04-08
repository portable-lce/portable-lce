#pragma once
#include <string>

#include "minecraft/client/gui/Screen.h"

class ClientConnection;
class Minecraft;

class ConnectScreen : public Screen {
private:
    ClientConnection* connection;
    bool aborted;

public:
    ConnectScreen(Minecraft* minecraft, const std::string& ip, int port);
    virtual void tick() override;

protected:
    virtual void keyPressed(char eventCharacter, int eventKey) override;

public:
    virtual void init() override;

protected:
    virtual void buttonClicked(Button* button) override;

public:
    virtual void render(int xm, int ym, float a) override;
};