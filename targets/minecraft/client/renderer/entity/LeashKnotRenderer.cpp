#include "LeashKnotRenderer.h"

#include <memory>

#include "minecraft/client/model/LeashKnotModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation LeashKnotRenderer::KNOT_LOCATION =
    ResourceLocation(TN_ITEM_LEASHKNOT);

LeashKnotRenderer::LeashKnotRenderer() : EntityRenderer() {
    model = new LeashKnotModel();
}

LeashKnotRenderer::~LeashKnotRenderer() { delete model; }

void LeashKnotRenderer::render(std::shared_ptr<Entity> entity, double x,
                               double y, double z, float rot, float a) {
    RenderPath.MatrixPush();
    RenderPath.StateSetFaceCull(false);

    RenderPath.MatrixTranslate((float)x, (float)y, (float)z);

    float scale = 1 / 16.0f;
    (void)0;
    RenderPath.MatrixScale(-1, -1, 1);

    RenderPath.StateSetAlphaTestEnable(true);

    bindTexture(entity);
    model->render(entity, 0, 0, 0, 0, 0, scale, true);

    RenderPath.MatrixPop();
}

ResourceLocation* LeashKnotRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    return &KNOT_LOCATION;
}