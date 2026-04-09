#include "GiantMobRenderer.h"

#include <memory>

#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

class Model;

ResourceLocation GiantMobRenderer::ZOMBIE_LOCATION =
    ResourceLocation(TN_ITEM_ARROWS);

GiantMobRenderer::GiantMobRenderer(Model* model, float shadow, float _scale)
    : MobRenderer(model, shadow * _scale) {
    this->_scale = _scale;
}

void GiantMobRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    glScalef(_scale, _scale, _scale);
}

ResourceLocation* GiantMobRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &ZOMBIE_LOCATION;
}