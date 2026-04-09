#include "WitchRenderer.h"

#include <memory>
#include <vector>

#include "EntityRenderDispatcher.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/model/WitchModel.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/item/BowItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation WitchRenderer::WITCH_LOCATION = ResourceLocation(TN_MOB_WITCH);

WitchRenderer::WitchRenderer() : MobRenderer(new WitchModel(0), 0.5f) {
    witchModel = dynamic_cast<WitchModel*>(model);
}

void WitchRenderer::render(std::shared_ptr<Entity> entity, double x, double y,
                           double z, float rot, float a) {
    std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(entity);

    std::shared_ptr<ItemInstance> item = mob->getCarriedItem();

    witchModel->holdingItem = item != nullptr;
    MobRenderer::render(mob, x, y, z, rot, a);
}

ResourceLocation* WitchRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    return &WITCH_LOCATION;
}

void WitchRenderer::additionalRendering(std::shared_ptr<LivingEntity> entity,
                                        float a) {
    std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(entity);

    float brightness =
        SharedConstants::TEXTURE_LIGHTING ? 1 : mob->getBrightness(a);
    glColor3f(brightness, brightness, brightness);

    MobRenderer::additionalRendering(mob, a);

    std::shared_ptr<ItemInstance> item = mob->getCarriedItem();

    if (item != nullptr) {
        glPushMatrix();

        if (model->young) {
            float s = 0.5f;
            glTranslatef(0 / 16.0f, 10 / 16.0f, 0 / 16.0f);
            glRotatef(-20, -1, 0, 0);
            glScalef(s, s, s);
        }

        witchModel->nose->translateTo(1 / 16.0f);
        glTranslatef(-1 / 16.0f, 8.5f / 16.0f, 3.5f / 16.0f);

        if (item->id < 256 &&
            TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
            float s = 8 / 16.0f;
            glTranslatef(-0 / 16.0f, 3 / 16.0f, -5 / 16.0f);
            s *= 0.75f;
            glRotatef(20, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
            glScalef(s, -s, s);
        } else if (item->id == Item::bow->id) {
            float s = 10 / 16.0f;
            glTranslatef(0 / 16.0f, 2 / 16.0f, 5 / 16.0f);
            glRotatef(-20, 0, 1, 0);
            glScalef(s, -s, s);
            glRotatef(-100, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
        } else if (Item::items[item->id]->isHandEquipped()) {
            float s = 10 / 16.0f;
            if (Item::items[item->id]->isMirroredArt()) {
                glRotatef(180, 0, 0, 1);
                glTranslatef(0, -2 / 16.0f, 0);
            }
            translateWeaponItem();
            glScalef(s, -s, s);
            glRotatef(-100, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
        } else {
            float s = 6 / 16.0f;
            glTranslatef(+4 / 16.0f, +3 / 16.0f, -3 / 16.0f);
            glScalef(s, s, s);
            glRotatef(60, 0, 0, 1);
            glRotatef(-90, 1, 0, 0);
            glRotatef(20, 0, 0, 1);
        }

        glRotatef(-15, 1, 0, 0);
        glRotatef(40, 0, 0, 1);

        entityRenderDispatcher->itemInHandRenderer->renderItem(mob, item, 0);
        if (item->getItem()->hasMultipleSpriteLayers()) {
            entityRenderDispatcher->itemInHandRenderer->renderItem(mob, item,
                                                                   1);
        }
        glPopMatrix();
    }
}

void WitchRenderer::translateWeaponItem() { glTranslatef(0, 3 / 16.0f, 0); }

void WitchRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    float s = 15 / 16.0f;
    glScalef(s, s, s);
}