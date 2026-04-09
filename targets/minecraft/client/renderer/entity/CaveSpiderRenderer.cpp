#include "CaveSpiderRenderer.h"

#include <memory>

#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/SpiderRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation CaveSpiderRenderer::CAVE_SPIDER_LOCATION =
    ResourceLocation(TN_MOB_CAVE_SPIDER);
float CaveSpiderRenderer::s_scale = 0.7f;

CaveSpiderRenderer::CaveSpiderRenderer() : SpiderRenderer() {
    shadowRadius *= s_scale;
}

void CaveSpiderRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    glScalef(s_scale, s_scale, s_scale);
}

ResourceLocation* CaveSpiderRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &CAVE_SPIDER_LOCATION;
}