#include "SlimeRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"

#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/Slime.h"

class Model;

ResourceLocation SlimeRenderer::SLIME_LOCATION = ResourceLocation(TN_MOB_SLIME);

SlimeRenderer::SlimeRenderer(Model* model, Model* armor, float shadow)
    : MobRenderer(model, shadow) {
    this->armor = armor;
}

int SlimeRenderer::prepareArmor(std::shared_ptr<LivingEntity> _slime, int layer,
                                float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Slime> slime = std::dynamic_pointer_cast<Slime>(_slime);

    if (slime->isInvisible()) {
        return 0;
    }
    if (layer == 0) {
        setArmor(armor);

        glEnable(GL_NORMALIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        return 1;
    }
    if (layer == 1) {
        glDisable(GL_BLEND);
        glColor4f(1, 1, 1, 1);
    }
    return -1;
}

void SlimeRenderer::scale(std::shared_ptr<LivingEntity> _slime, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Slime> slime = std::dynamic_pointer_cast<Slime>(_slime);

    float size = (float)slime->getSize();
    float ss = (slime->oSquish + (slime->squish - slime->oSquish) * a) /
               (size * 0.5f + 1);
    float w = 1 / (ss + 1);
    glScalef(w * size, 1 / w * size, w * size);
}

ResourceLocation* SlimeRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &SLIME_LOCATION;
}