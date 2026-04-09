#include "OcelotRenderer.h"

#include <memory>

#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/Ocelot.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

class Model;

ResourceLocation OcelotRenderer::CAT_BLACK_LOCATION =
    ResourceLocation(TN_MOB_CAT_BLACK);
ResourceLocation OcelotRenderer::CAT_OCELOT_LOCATION =
    ResourceLocation(TN_MOB_OCELOT);
ResourceLocation OcelotRenderer::CAT_RED_LOCATION =
    ResourceLocation(TN_MOB_CAT_RED);
ResourceLocation OcelotRenderer::CAT_SIAMESE_LOCATION =
    ResourceLocation(TN_MOB_CAT_SIAMESE);

OcelotRenderer::OcelotRenderer(Model* model, float shadow)
    : MobRenderer(model, shadow) {}

void OcelotRenderer::render(std::shared_ptr<Entity> _mob, double x, double y,
                            double z, float rot, float a) {
    MobRenderer::render(_mob, x, y, z, rot, a);
}

ResourceLocation* OcelotRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    std::shared_ptr<Ocelot> cat = std::dynamic_pointer_cast<Ocelot>(entity);

    switch (cat->getCatType()) {
        default:
        case Ocelot::TYPE_OCELOT:
            return &CAT_OCELOT_LOCATION;
        case Ocelot::TYPE_BLACK:
            return &CAT_BLACK_LOCATION;
        case Ocelot::TYPE_RED:
            return &CAT_RED_LOCATION;
        case Ocelot::TYPE_SIAMESE:
            return &CAT_SIAMESE_LOCATION;
    }
}

void OcelotRenderer::scale(std::shared_ptr<LivingEntity> _mob, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Blaze rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<Ocelot> mob = std::dynamic_pointer_cast<Ocelot>(_mob);
    MobRenderer::scale(mob, a);
    if (mob->isTame()) {
        glScalef(.8f, .8f, .8f);
    }
}