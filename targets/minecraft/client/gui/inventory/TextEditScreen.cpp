#include "TextEditScreen.h"

#include <vector>

#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Button.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "minecraft/network/packet/SignUpdatePacket.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/SignTileEntity.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

const std::string TextEditScreen::allowedChars =
    SharedConstants::acceptableLetters;

TextEditScreen::TextEditScreen(std::shared_ptr<SignTileEntity> sign) {
    // 4J - added initialisers
    line = 0;
    frame = 0;
    title = "Edit sign message:";

    this->sign = sign;
}

void TextEditScreen::init() {
    buttons.clear();
    Keyboard::enableRepeatEvents(true);
    buttons.push_back(
        new Button(0, width / 2 - 100, height / 4 + 24 * 5, "Done"));
}

void TextEditScreen::removed() {
    Keyboard::enableRepeatEvents(false);
    if (minecraft->level->isClientSide) {
        minecraft->getConnection(0)->send(std::shared_ptr<SignUpdatePacket>(
            new SignUpdatePacket(sign->x, sign->y, sign->z, sign->IsVerified(),
                                 sign->IsCensored(), sign->GetMessages())));
    }
}

void TextEditScreen::tick() { frame++; }

void TextEditScreen::buttonClicked(Button* button) {
    if (!button->active) return;

    if (button->id == 0) {
        sign->setChanged();
        minecraft->setScreen(nullptr);
    }
}

void TextEditScreen::keyPressed(char ch, int eventKey) {
    if (eventKey == Keyboard::KEY_UP) line = (line - 1) & 3;
    if (eventKey == Keyboard::KEY_DOWN || eventKey == Keyboard::KEY_RETURN)
        line = (line + 1) & 3;

    std::string temp = sign->GetMessage(line);
    if (eventKey == Keyboard::KEY_BACK && temp.length() > 0) {
        temp = temp.substr(0, temp.length() - 1);
    }
    if (allowedChars.find(ch) != std::string::npos && temp.length() < 15) {
        temp += ch;
    }

    sign->SetMessage(line, temp);
}

void TextEditScreen::render(int xm, int ym, float a) {
    renderBackground();

    drawCenteredString(font, title, width / 2, 40, 0xffffff);

    glPushMatrix();
    glTranslatef((float)width / 2, (float)height / 2, 50);
    float ss = 60 / (16 / 25.0f);
    glScalef(-ss, -ss, -ss);
    glRotatef(180, 0, 1, 0);

    Tile* tile = sign->getTile();

    if (tile == Tile::sign) {
        float rot = sign->getData() * 360 / 16.0f;
        glRotatef(rot, 0, 1, 0);
        glTranslatef(0, 5 / 16.0f, 0);
    } else {
        int face = sign->getData();
        float rot = 0;

        if (face == 2) rot = 180;
        if (face == 4) rot = 90;
        if (face == 5) rot = -90;
        glRotatef(rot, 0, 1, 0);
        glTranslatef(0, 5 / 16.0f, 0);
    }

    if (frame / 6 % 2 == 0) sign->SetSelectedLine(line);

    TileEntityRenderDispatcher::instance->render(sign, 0 - 0.5f, -0.75f,
                                                 0 - 0.5f, 0);
    sign->SetSelectedLine(-1);

    glPopMatrix();

    Screen::render(xm, ym, a);
}