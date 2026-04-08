#include "FireballRenderer.h"
#include "platform/stubs.h"

#include <memory>

#include "platform/renderer/renderer.h"
#include "EntityRenderDispatcher.h"

#include "java/Class.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/projectile/Fireball.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/tile/FireTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"

FireballRenderer::FireballRenderer(float scale) { this->scale = scale; }

void FireballRenderer::render(std::shared_ptr<Entity> _fireball, double x,
                              double y, double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Fireball> fireball =
        std::dynamic_pointer_cast<Fireball>(_fireball);

    glPushMatrix();

    glTranslatef((float)x, (float)y, (float)z);
    glEnable(GL_RESCALE_NORMAL);
    float s = scale;
    glScalef(s / 1.0f, s / 1.0f, s / 1.0f);
    Icon* icon = Item::fireball->getIcon(
        fireball->GetType() == eTYPE_DRAGON_FIREBALL ? 1 : 0);  // 14 + 2 * 16;
    bindTexture(fireball);
    Tesselator* t = Tesselator::getInstance();

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
    t->vertexUV((float)(r - xo), (float)(1 - yo), (float)(0), (float)(u1),
                (float)(v0));
    t->vertexUV((float)(0 - xo), (float)(1 - yo), (float)(0), (float)(u0),
                (float)(v0));
    t->end();

    glDisable(GL_RESCALE_NORMAL);
    glPopMatrix();
}

// 4J Added override. Based on EntityRenderer::renderFlame
void FireballRenderer::renderFlame(std::shared_ptr<Entity> e, double x,
                                   double y, double z, float a) {
    glDisable(GL_LIGHTING);
    Icon* tex = Tile::fire->getTextureLayer(0);

    glPushMatrix();
    glTranslatef((float)x, (float)y, (float)z);

    float s = e->bbWidth * 1.4f;
    glScalef(s, s, s);
    bindTexture(&TextureAtlas::LOCATION_BLOCKS);
    Tesselator* t = Tesselator::getInstance();

    float r = 1.0f;
    float xo = 0.5f;
    //        float yo = 0.0f;

    float h = e->bbHeight / s;
    float yo = (float)(e->y - e->bb.y0);

    // glRotatef(-entityRenderDispatcher->playerRotY, 0, 1, 0);

    glRotatef(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
    glRotatef(-entityRenderDispatcher->playerRotX, 1, 0, 0);
    glTranslatef(0, 0, 0.1f);
    // glTranslatef(0, 0, -0.3f + ((int) h) * 0.02f);
    glColor4f(1, 1, 1, 1);
    // glRotatef(-playerRotX, 1, 0, 0);
    float zo = 0;
    t->begin();
    t->normal(0, 1, 0);

    float u0 = tex->getU0();
    float v0 = tex->getV0();
    float u1 = tex->getU1();
    float v1 = tex->getV1();

    float tmp = u1;
    u1 = u0;
    u0 = tmp;

    t->vertexUV((float)(0 - xo), (float)(0 - yo), (float)(0), (float)(u1),
                (float)(v1));
    t->vertexUV((float)(r - xo), (float)(0 - yo), (float)(0), (float)(u0),
                (float)(v1));
    t->vertexUV((float)(r - xo), (float)(1.4f - yo), (float)(0), (float)(u0),
                (float)(v0));
    t->vertexUV((float)(0 - xo), (float)(1.4f - yo), (float)(0), (float)(u1),
                (float)(v0));

    t->end();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

ResourceLocation* FireballRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &TextureAtlas::LOCATION_ITEMS;
}
