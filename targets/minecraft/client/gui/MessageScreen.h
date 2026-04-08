#pragma once
#include <string>

#include "Screen.h"

class MessageScreen : public Screen {
private:
    std::string message;

public:
    MessageScreen(const std::string& message);

protected:
    using Screen::keyPressed;

    virtual void keyPressed(char eventCharacter, int eventKey) override;

public:
    virtual void init() override;

protected:
    virtual void buttonClicked(Button* button) override;

public:
    virtual void render(int xm, int ym, float a) override;
};