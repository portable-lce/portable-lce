#include "LeashKnotRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"

#include "minecraft/client/model/LeashKnotModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"

ResourceLocation LeashKnotRenderer::KNOT_LOCATION =
    ResourceLocation(TN_ITEM_LEASHKNOT);

LeashKnotRenderer::LeashKnotRenderer() : EntityRenderer() {
    model = new LeashKnotModel();
}

LeashKnotRenderer::~LeashKnotRenderer() { delete model; }

void LeashKnotRenderer::render(std::shared_ptr<Entity> entity, double x,
                               double y, double z, float rot, float a) {
    glPushMatrix();
    glDisable(GL_CULL_FACE);

    glTranslatef((float)x, (float)y, (float)z);

    float scale = 1 / 16.0f;
    glEnable(GL_RESCALE_NORMAL);
    glScalef(-1, -1, 1);

    glEnable(GL_ALPHA_TEST);

    bindTexture(entity);
    model->render(entity, 0, 0, 0, 0, 0, scale, true);

    glPopMatrix();
}

ResourceLocation* LeashKnotRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    return &KNOT_LOCATION;
}