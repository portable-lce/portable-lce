#include "AbstractBeaconButton.h"
#include "platform/stubs.h"

#include <string>

#include "platform/renderer/renderer.h"
#include "minecraft/client/gui/Button.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/client/Minecraft.h"

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4)
#ifdef ENABLE_JAVA_GUIS
extern ResourceLocation GUI_BEACON_LOCATION;
#endif

AbstractBeaconButton::AbstractBeaconButton(int id, int x, int y)
    : Button(id, x, y, 22, 22, "") {
    hovered = false;
    selected = false;
    iconRes = nullptr;
    iconU = iconV = 0;
}

void AbstractBeaconButton::renderBg(Minecraft* minecraft, int xm, int ym) {
#ifdef ENABLE_JAVA_GUIS
    if (!visible) return;

    hovered = (xm >= x && ym >= y && xm < x + w && ym < y + h);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    minecraft->textures->bindTexture(&GUI_BEACON_LOCATION);

    int texU = 0;
    if (!active) {
        texU += w * 2;
    } else if (selected) {
        texU += w * 1;
    } else if (hovered) {
        texU += w * 3;
    }
    int texV = 219;

    blit(x, y, texU, texV, w, h);

    if (iconRes != nullptr && iconRes != &GUI_BEACON_LOCATION) {
        minecraft->textures->bindTexture(iconRes);
    }
    blit(x + 2, y + 2, iconU, iconV, 18, 18);
#endif
}