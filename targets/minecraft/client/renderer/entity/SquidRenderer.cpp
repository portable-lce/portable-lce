#include "SquidRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/Squid.h"

class Model;

ResourceLocation SquidRenderer::SQUID_LOCATION = ResourceLocation(TN_MOB_SQUID);

SquidRenderer::SquidRenderer(Model* model, float shadow)
    : MobRenderer(model, shadow) {}

void SquidRenderer::render(std::shared_ptr<Entity> mob, double x, double y,
                           double z, float rot, float a) {
    MobRenderer::render(mob, x, y, z, rot, a);
}

void SquidRenderer::setupRotations(std::shared_ptr<LivingEntity> _mob,
                                   float bob, float bodyRot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Squid> mob = std::dynamic_pointer_cast<Squid>(_mob);

    float bodyXRot = (mob->xBodyRotO + (mob->xBodyRot - mob->xBodyRotO) * a);
    float bodyZRot = (mob->zBodyRotO + (mob->zBodyRot - mob->zBodyRotO) * a);

    glTranslatef(0, 0.5f, 0);
    glRotatef(180 - bodyRot, 0, 1, 0);
    glRotatef(bodyXRot, 1, 0, 0);
    glRotatef(bodyZRot, 0, 1, 0);
    glTranslatef(0, -1.2f, 0);
}

float SquidRenderer::getBob(std::shared_ptr<LivingEntity> _mob, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Squid> mob = std::dynamic_pointer_cast<Squid>(_mob);

    return mob->oldTentacleAngle +
           (mob->tentacleAngle - mob->oldTentacleAngle) * a;
}

ResourceLocation* SquidRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &SQUID_LOCATION;
}