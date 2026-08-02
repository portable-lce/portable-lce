#include "CreeperRenderer.h"

#include <math.h>

#include <memory>

#include "minecraft/client/model/CreeperModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/Creeper.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation CreeperRenderer::POWER_LOCATION =
    ResourceLocation(TN_POWERED_CREEPER);
ResourceLocation CreeperRenderer::CREEPER_LOCATION =
    ResourceLocation(TN_MOB_CREEPER);

CreeperRenderer::CreeperRenderer() : MobRenderer(new CreeperModel(), 0.5f) {
    armorModel = new CreeperModel(2);
}

void CreeperRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    std::shared_ptr<Creeper> creeper = std::dynamic_pointer_cast<Creeper>(mob);

    float g = creeper->getSwelling(a);

    float wobble = 1.0f + sinf(g * 100) * g * 0.01f;
    if (g < 0) g = 0;
    if (g > 1) g = 1;
    g = g * g;
    g = g * g;
    float s = (1.0f + g * 0.4f) * wobble;
    float hs = (1.0f + g * 0.1f) / wobble;
    RenderPath.MatrixScale(s, hs, s);
}

int CreeperRenderer::getOverlayColor(std::shared_ptr<LivingEntity> mob,
                                     float br, float a) {
    std::shared_ptr<Creeper> creeper = std::dynamic_pointer_cast<Creeper>(mob);

    float step = creeper->getSwelling(a);

    if ((int)(step * 10) % 2 == 0) return 0;

    int _a = (int)(step * 0.2f * 255) +
             25;  // 4J - added 25 here as our entities are rendered with alpha
                  // test still enabled, and so anything less is invisible
    if (_a < 0) _a = 0;
    if (_a > 255) _a = 255;

    int r = 255;
    int g = 255;
    int b = 255;

    return (_a << 24) | (r << 16) | (g << 8) | b;
}

int CreeperRenderer::prepareArmor(std::shared_ptr<LivingEntity> _mob, int layer,
                                  float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Creeper> mob = std::dynamic_pointer_cast<Creeper>(_mob);
    if (mob->isPowered()) {
        if (mob->isInvisible())
            RenderPath.StateSetDepthMask(false);
        else
            RenderPath.StateSetDepthMask(true);

        if (layer == 1) {
            float time = mob->tickCount + a;
            bindTexture(&POWER_LOCATION);
            RenderPath.MatrixMode(rp::MatrixStack::texture);
            RenderPath.MatrixSetIdentity();
            float uo = time * 0.01f;
            float vo = time * 0.01f;
            RenderPath.MatrixTranslate(uo, vo, 0);
            setArmor(armorModel);
            RenderPath.MatrixMode(rp::MatrixStack::modelview);
            RenderPath.StateSetBlendEnable(true);
            float br = 0.5f;
            RenderPath.StateSetColour(br, br, br, 1);
            RenderPath.StateSetLightingEnable(false);
            RenderPath.StateSetBlendFunc(rp::BlendFactor::one, rp::BlendFactor::one);
            return 1;
        }
        if (layer == 2) {
            RenderPath.MatrixMode(rp::MatrixStack::texture);
            RenderPath.MatrixSetIdentity();
            RenderPath.MatrixMode(rp::MatrixStack::modelview);
            RenderPath.StateSetLightingEnable(true);
            RenderPath.StateSetBlendEnable(false);
        }
    }
    return -1;
}

int CreeperRenderer::prepareArmorOverlay(std::shared_ptr<LivingEntity> mob,
                                         int layer, float a) {
    return -1;
}

ResourceLocation* CreeperRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &CREEPER_LOCATION;
}