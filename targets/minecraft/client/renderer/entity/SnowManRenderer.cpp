#include "SnowManRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"
#include "EntityRenderDispatcher.h"
#include "minecraft/client/model/SnowManModel.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/SnowMan.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"

ResourceLocation SnowManRenderer::SNOWMAN_LOCATION =
    ResourceLocation(TN_MOB_SNOWMAN);

SnowManRenderer::SnowManRenderer() : MobRenderer(new SnowManModel(), 0.5f) {
    model = (SnowManModel*)MobRenderer::model;
    this->setArmor(model);
}

void SnowManRenderer::additionalRendering(std::shared_ptr<LivingEntity> _mob,
                                          float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type SnowMan rather than shared_ptr<Mob>  we have here - do some casting
    // around instead
    std::shared_ptr<SnowMan> mob = std::dynamic_pointer_cast<SnowMan>(_mob);

    MobRenderer::additionalRendering(mob, a);
    std::shared_ptr<ItemInstance> headGear =
        std::make_shared<ItemInstance>(Tile::pumpkin, 1);
    if (headGear != nullptr && headGear->getItem()->id < 256) {
        glPushMatrix();
        model->head->translateTo(1 / 16.0f);

        if (TileRenderer::canRender(
                Tile::tiles[headGear->id]->getRenderShape())) {
            float s = 10 / 16.0f;
            glTranslatef(-0 / 16.0f, -5.5f / 16.0f, 0 / 16.0f);
            glRotatef(90, 0, 1, 0);
            glScalef(s, -s, s);
        }

        entityRenderDispatcher->itemInHandRenderer->renderItem(mob, headGear,
                                                               0);

        glPopMatrix();
    }
}

ResourceLocation* SnowManRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &SNOWMAN_LOCATION;
}