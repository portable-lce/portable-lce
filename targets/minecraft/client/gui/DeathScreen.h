#pragma once
#include "Screen.h"

class DeathScreen : public Screen {
public:
    virtual void init() override;

protected:
    virtual void keyPressed(char eventCharacter, int eventKey) override;
    virtual void buttonClicked(Button* button) override;

public:
    virtual void render(int xm, int ym, float a) override;
    virtual bool isPauseScreen() override;
};