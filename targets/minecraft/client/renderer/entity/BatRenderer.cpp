#include "BatRenderer.h"

#include <cmath>
#include <memory>

#include "minecraft/client/model/BatModel.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/ambient/Bat.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation BatRenderer::BAT_LOCATION = ResourceLocation(TN_MOB_BAT);

BatRenderer::BatRenderer() : MobRenderer(new BatModel(), 0.25f) {
    modelVersion = ((BatModel*)model)->modelVersion();
}

void BatRenderer::render(std::shared_ptr<Entity> _mob, double x, double y,
                         double z, float rot, float a) {
    int modelVersion = (dynamic_cast<BatModel*>(model))->modelVersion();
    if (modelVersion != this->modelVersion) {
        this->modelVersion = modelVersion;
        model = new BatModel();
    }
    MobRenderer::render(_mob, x, y, z, rot, a);
}

ResourceLocation* BatRenderer::getTextureLocation(std::shared_ptr<Entity> mob) {
    return &BAT_LOCATION;
}

void BatRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    glScalef(.35f, .35f, .35f);
}

void BatRenderer::setupPosition(std::shared_ptr<LivingEntity> mob, double x,
                                double y, double z) {
    MobRenderer::setupPosition(mob, x, y, z);
}

void BatRenderer::setupRotations(std::shared_ptr<LivingEntity> _mob, float bob,
                                 float bodyRot, float a) {
    std::shared_ptr<Bat> mob = std::dynamic_pointer_cast<Bat>(_mob);
    if (!mob->isResting()) {
        glTranslatef(0, cos(bob * .3f) * .1f, 0);
    } else {
        glTranslatef(0, -.1f, 0);
    }
    MobRenderer::setupRotations(mob, bob, bodyRot, a);
}