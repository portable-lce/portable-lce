#include "GhastRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"
#include "minecraft/client/model/GhastModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/Ghast.h"

ResourceLocation GhastRenderer::GHAST_LOCATION = ResourceLocation(TN_MOB_GHAST);
ResourceLocation GhastRenderer::GHAST_SHOOTING_LOCATION =
    ResourceLocation(TN_MOB_GHAST_FIRE);

GhastRenderer::GhastRenderer() : MobRenderer(new GhastModel(), 0.5f) {}

void GhastRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    std::shared_ptr<Ghast> ghast = std::dynamic_pointer_cast<Ghast>(mob);

    float ss = (ghast->oCharge + (ghast->charge - ghast->oCharge) * a) / 20.0f;
    if (ss < 0) ss = 0;
    ss = 1 / (ss * ss * ss * ss * ss * 2 + 1);
    float s = (8 + ss) / 2;
    float hs = (8 + 1 / ss) / 2;
    glScalef(hs, s, hs);
    glColor4f(1, 1, 1, 1);
}

ResourceLocation* GhastRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    std::shared_ptr<Ghast> ghast = std::dynamic_pointer_cast<Ghast>(mob);

    if (ghast->isCharging()) {
        return &GHAST_SHOOTING_LOCATION;
    }

    return &GHAST_LOCATION;
}