#include "LavaSlimeRenderer.h"

#include <memory>

#include "minecraft/client/model/LavaSlimeModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/LavaSlime.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation LavaSlimeRenderer::MAGMACUBE_LOCATION =
    ResourceLocation(TN_MOB_LAVA);

LavaSlimeRenderer::LavaSlimeRenderer()
    : MobRenderer(new LavaSlimeModel(), .25f) {
    this->modelVersion = ((LavaSlimeModel*)model)->getModelVersion();
}

ResourceLocation* LavaSlimeRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &MAGMACUBE_LOCATION;
}

void LavaSlimeRenderer::scale(std::shared_ptr<LivingEntity> _slime, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type LavaSlime rather than shared_ptr<Mob>  we have here - do some
    // casting around instead
    std::shared_ptr<LavaSlime> slime =
        std::dynamic_pointer_cast<LavaSlime>(_slime);
    int size = slime->getSize();
    float ss = (slime->oSquish + (slime->squish - slime->oSquish) * a) /
               (size * 0.5f + 1);
    float w = 1 / (ss + 1);
    float s = size;
    glScalef(w * s, 1 / w * s, w * s);
}