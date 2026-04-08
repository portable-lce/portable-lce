#include "SkeletonRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"
#include "minecraft/client/model/SkeletonModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/HumanoidMobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/Skeleton.h"

ResourceLocation SkeletonRenderer::SKELETON_LOCATION =
    ResourceLocation(TN_MOB_SKELETON);
ResourceLocation SkeletonRenderer::WITHER_SKELETON_LOCATION =
    ResourceLocation(TN_MOB_WITHER_SKELETON);

SkeletonRenderer::SkeletonRenderer()
    : HumanoidMobRenderer(new SkeletonModel(), .5f) {}

void SkeletonRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    if (std::dynamic_pointer_cast<Skeleton>(mob)->getSkeletonType() ==
        Skeleton::TYPE_WITHER) {
        glScalef(1.2f, 1.2f, 1.2f);
    }
}

void SkeletonRenderer::translateWeaponItem() {
    glTranslatef(1.5f / 16.0f, 3 / 16.0f, 0);
}

ResourceLocation* SkeletonRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    std::shared_ptr<Skeleton> skeleton =
        std::dynamic_pointer_cast<Skeleton>(entity);

    if (skeleton->getSkeletonType() == Skeleton::TYPE_WITHER) {
        return &WITHER_SKELETON_LOCATION;
    }
    return &SKELETON_LOCATION;
}