#include "VillagerGolemRenderer.h"
#include "platform/stubs.h"

#include <cmath>
#include <memory>
#include <numbers>

#include "platform/renderer/renderer.h"

#include "minecraft/SharedConstants.h"
#include "minecraft/client/model/VillagerGolemModel.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/VillagerGolem.h"
#include "minecraft/world/level/tile/PlantTile.h"
#include "minecraft/world/level/tile/Tile.h"

ResourceLocation VillagerGolemRenderer::GOLEM_LOCATION =
    ResourceLocation(TN_MOB_VILLAGER_GOLEM);

VillagerGolemRenderer::VillagerGolemRenderer()
    : MobRenderer(new VillagerGolemModel(), 0.5f) {
    golemModel = (VillagerGolemModel*)model;
}

void VillagerGolemRenderer::render(std::shared_ptr<Entity> mob, double x,
                                   double y, double z, float rot, float a) {
    MobRenderer::render(mob, x, y, z, rot, a);
}

void VillagerGolemRenderer::setupRotations(std::shared_ptr<LivingEntity> _mob,
                                           float bob, float bodyRot, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Blaze rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<VillagerGolem> mob =
        std::dynamic_pointer_cast<VillagerGolem>(_mob);
    MobRenderer::setupRotations(mob, bob, bodyRot, a);
    if (mob->walkAnimSpeed < 0.01) return;

    float p = 13;
    float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1 - a) + 6;
    float triangleWave =
        (std::abs(std::fmod(wp, p) - p * 0.5f) - p * 0.25f) / (p * 0.25f);
    glRotatef(6.5f * triangleWave, 0, 0, 1);
}

ResourceLocation* VillagerGolemRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &GOLEM_LOCATION;
}

void VillagerGolemRenderer::additionalRendering(
    std::shared_ptr<LivingEntity> _mob, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Blaze rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<VillagerGolem> mob =
        std::dynamic_pointer_cast<VillagerGolem>(_mob);
    MobRenderer::additionalRendering(mob, a);
    if (mob->getOfferFlowerTick() == 0) return;

    glEnable(GL_RESCALE_NORMAL);
    glPushMatrix();

    // dont ask me how I got the flower into his hand.
    glRotatef(5 + 180 * golemModel->arm0->xRot / std::numbers::pi, 1, 0, 0);
    glTranslatef(-11 / 16.0f, 20 / 16.0f, -15 / 16.0f);
    glRotatef(90, 1, 0, 0);
    float s = 0.8f;
    glScalef(s, -s, s);

    if (SharedConstants::TEXTURE_LIGHTING) {
        int col = mob->getLightColor(a);
        int u = col % 65536;
        int v = col / 65536;
        glMultiTexCoord2f(GL_TEXTURE1, u / 1.0f, v / 1.0f);
        glColor4f(1, 1, 1, 1);
    }

    glColor4f(1, 1, 1, 1);
    bindTexture(&TextureAtlas::LOCATION_BLOCKS);  // TODO: By Icon
    tileRenderer->renderTile(Tile::rose, 0, 1);
    glPopMatrix();
    glDisable(GL_RESCALE_NORMAL);
}