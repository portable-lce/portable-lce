#include "BeaconPowerButton.h"

#include <string>

#include "BeaconScreen.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/gui/inventory/AbstractBeaconButton.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/effect/MobEffect.h"

// 4jcraft: referenced from MCP 8.11 (JE 1.6.4)
#ifdef ENABLE_JAVA_GUIS
ResourceLocation GUI_INVENTORY_LOCATION = ResourceLocation(TN_GUI_INVENTORY);
#endif

BeaconPowerButton::BeaconPowerButton(BeaconScreen* screen, int id, int x, int y,
                                     int effectId, int tier)
    : AbstractBeaconButton(id, x, y) {
    this->screen = screen;
    this->effectId = effectId;
    this->tier = tier;

#ifdef ENABLE_JAVA_GUIS
    this->iconRes = &GUI_INVENTORY_LOCATION;
#endif

    int statusIconIndex = MobEffect::javaId(effectId);
    this->iconU = (statusIconIndex % 8) * 18;
    this->iconV = 198 + (statusIconIndex / 8) * 18;
}

void BeaconPowerButton::renderTooltip(int xm, int ym) {
    MobEffect* effect = MobEffect::effects[effectId];
    if (!effect) return;

    std::string name = gameServices().getString(effect->getDescriptionId());
    if (tier >= 3 && effect->id != MobEffect::regeneration->id) {
        name += " II";
    }
    screen->renderTooltip(name, xm, ym);
}