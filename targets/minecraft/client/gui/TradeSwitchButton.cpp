#include "TradeSwitchButton.h"

#include <string>

#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Button.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "platform/stubs.h"

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4)
#ifdef ENABLE_JAVA_GUIS
// ResourceLocation GUI_VILLAGER_LOCATION = ResourceLocation(TN_GUI_VILLAGER);
extern ResourceLocation GUI_VILLAGER_LOCATION;
#endif

TradeSwitchButton::TradeSwitchButton(int id, int x, int y, bool mirrored)
    : Button(id, x, y, 12, 19, "") {
    this->mirrored = mirrored;
}

int TradeSwitchButton::getYImage(bool hovered) { return 0; }

void TradeSwitchButton::renderBg(Minecraft* minecraft, int xm, int ym) {
#ifdef ENABLE_JAVA_GUIS
    if (!visible) return;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    minecraft->textures->bindTexture(&GUI_VILLAGER_LOCATION);

    bool hovered = (xm >= x && ym >= y && xm < x + w && ym < y + h);

    int textureX = 176;
    int textureY = 0;

    if (!active) {
        textureX += w * 2;
    } else if (hovered) {
        textureX += w;
    }

    if (!mirrored) {
        textureY += h;
    }

    blit(x, y, textureX, textureY, w, h);
#endif
}