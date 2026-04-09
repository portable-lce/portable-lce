#include "Button.h"

#include "minecraft/client/Minecraft.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
class Minecraft;

#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_GUI_LOCATION = ResourceLocation(TN_GUI_GUI);
#endif

Button::Button(int id, int x, int y, const std::string& msg) {
    init(id, x, y, 200, 20, msg);
}

Button::Button(int id, int x, int y, int w, int h, const std::string& msg) {
    init(id, x, y, w, h, msg);
}

// 4J - added
void Button::init(int id, int x, int y, int w, int h, const std::string& msg) {
    active = true;
    visible = true;

    // this bit of code from original ctor
    this->id = id;
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->msg = msg;
}

int Button::getYImage(bool hovered) {
    int res = 1;
    if (!active)
        res = 0;
    else if (hovered)
        res = 2;
    return res;
}

void Button::render(Minecraft* minecraft, int xm, int ym) {
#ifdef ENABLE_JAVA_GUIS
    if (!visible) return;

    Font* font = minecraft->font;

    // glBindTexture(GL_TEXTURE_2D, minecraft->textures->loadTexture(
    //  TN_GUI_GUI));  // 4J was "/gui/gui.png"
    minecraft->textures->bindTexture(&GUI_GUI_LOCATION);
    glColor4f(1, 1, 1, 1);

    bool hovered = xm >= x && ym >= y && xm < x + w && ym < y + h;
    int yImage = getYImage(hovered);

    blit(x, y, 0, 46 + yImage * 20, w / 2, h);
    blit(x + w / 2, y, 200 - w / 2, 46 + yImage * 20, w / 2, h);

    renderBg(minecraft, xm, ym);

    if (!active) {
        drawCenteredString(font, msg, x + w / 2, y + (h - 8) / 2, 0xffa0a0a0);
    } else {
        if (hovered) {
            drawCenteredString(font, msg, x + w / 2, y + (h - 8) / 2, 0xffffa0);
        } else {
            drawCenteredString(font, msg, x + w / 2, y + (h - 8) / 2, 0xe0e0e0);
        }
    }
#endif
}

void Button::renderBg(Minecraft* minecraft, int xm, int ym) {}

void Button::released(int mx, int my) {}

bool Button::clicked(Minecraft* minecraft, int mx, int my) {
    return active && mx >= x && my >= y && mx < x + w && my < y + h;
}