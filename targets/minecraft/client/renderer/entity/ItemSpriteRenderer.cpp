#include "ItemSpriteRenderer.h"

#include <memory>

#include "EntityRenderDispatcher.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/projectile/ThrownPotion.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/item/alchemy/PotionBrewing.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ItemSpriteRenderer::ItemSpriteRenderer(Item* sourceItem,
                                       int sourceItemAuxValue /*= 0*/)
    : EntityRenderer() {
    this->sourceItem = sourceItem;
    this->sourceItemAuxValue = sourceItemAuxValue;
}

// ItemSpriteRenderer::ItemSpriteRenderer(int icon) : EntityRenderer()
//{
//	this(sourceItem, 0);
// }

void ItemSpriteRenderer::render(std::shared_ptr<Entity> e, double x, double y,
                                double z, float rot, float a) {
    // the icon is already cached in the item object, so there should not be any
    // performance impact by not caching it here
    Icon* icon = sourceItem->getIcon(sourceItemAuxValue);
    if (icon == nullptr) {
        return;
    }

    glPushMatrix();

    glTranslatef((float)x, (float)y, (float)z);
    glEnable(GL_RESCALE_NORMAL);
    glScalef(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
    bindTexture(e);
    Tesselator* t = Tesselator::getInstance();

    if (icon == PotionItem::getTexture(PotionItem::THROWABLE_ICON)) {
        int col = PotionBrewing::getColorValue(
            (std::dynamic_pointer_cast<ThrownPotion>(e))->getPotionValue(),
            false);
        float red = ((col >> 16) & 0xff) / 255.0f;
        float g = ((col >> 8) & 0xff) / 255.0f;
        float b = ((col) & 0xff) / 255.0f;

        glColor3f(red, g, b);
        glPushMatrix();
        renderIcon(t, PotionItem::getTexture(PotionItem::CONTENTS_ICON));
        glPopMatrix();
        glColor3f(1, 1, 1);
    }

    renderIcon(t, icon);

    glDisable(GL_RESCALE_NORMAL);
    glPopMatrix();
}

void ItemSpriteRenderer::renderIcon(Tesselator* t, Icon* icon) {
    float u0 = icon->getU0();
    float u1 = icon->getU1();
    float v0 = icon->getV0();
    float v1 = icon->getV1();

    float r = 1.0f;
    float xo = 0.5f;
    float yo = 0.25f;

    glRotatef(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
    glRotatef(-entityRenderDispatcher->playerRotX, 1, 0, 0);
    t->begin();
    t->normal(0, 1, 0);
    t->vertexUV((float)(0 - xo), (float)(0 - yo), (float)(0), (float)(u0),
                (float)(v1));
    t->vertexUV((float)(r - xo), (float)(0 - yo), (float)(0), (float)(u1),
                (float)(v1));
    t->vertexUV((float)(r - xo), (float)(r - yo), (float)(0), (float)(u1),
                (float)(v0));
    t->vertexUV((float)(0 - xo), (float)(r - yo), (float)(0), (float)(u0),
                (float)(v0));
    t->end();
}

ResourceLocation* ItemSpriteRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &TextureAtlas::LOCATION_ITEMS;
}