#include "FireballRenderer.h"

#include <memory>
#include <numbers>

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
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

FireballRenderer::FireballRenderer(float scale) { this->scale = scale; }

void FireballRenderer::render(std::shared_ptr<Entity> _fireball, double x,
                              double y, double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Fireball> fireball =
        std::dynamic_pointer_cast<Fireball>(_fireball);

    RenderPath.MatrixPush();

    RenderPath.MatrixTranslate((float)x, (float)y, (float)z);
    (void)0;
    float s = scale;
    RenderPath.MatrixScale(s / 1.0f, s / 1.0f, s / 1.0f);
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

    RenderPath.MatrixRotate((180 - entityRenderDispatcher->playerRotY)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    RenderPath.MatrixRotate((-entityRenderDispatcher->playerRotX)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
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

    (void)0;
    RenderPath.MatrixPop();
}

// 4J Added override. Based on EntityRenderer::renderFlame
void FireballRenderer::renderFlame(std::shared_ptr<Entity> e, double x,
                                   double y, double z, float a) {
    RenderPath.StateSetLightingEnable(false);
    Icon* tex = Tile::fire->getTextureLayer(0);

    RenderPath.MatrixPush();
    RenderPath.MatrixTranslate((float)x, (float)y, (float)z);

    float s = e->bbWidth * 1.4f;
    RenderPath.MatrixScale(s, s, s);
    bindTexture(&TextureAtlas::LOCATION_BLOCKS);
    Tesselator* t = Tesselator::getInstance();

    float r = 1.0f;
    float xo = 0.5f;
    //        float yo = 0.0f;

    float h = e->bbHeight / s;
    float yo = (float)(e->y - e->bb.y0);

    // RenderPath.MatrixRotate((-entityRenderDispatcher->playerRotY)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

    RenderPath.MatrixRotate((180 - entityRenderDispatcher->playerRotY)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    RenderPath.MatrixRotate((-entityRenderDispatcher->playerRotX)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
    RenderPath.MatrixTranslate(0, 0, 0.1f);
    // RenderPath.MatrixTranslate(0, 0, -0.3f + ((int) h) * 0.02f);
    RenderPath.StateSetColour(1, 1, 1, 1);
    // RenderPath.MatrixRotate((-playerRotX)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
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
    RenderPath.MatrixPop();
    RenderPath.StateSetLightingEnable(true);
}

ResourceLocation* FireballRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &TextureAtlas::LOCATION_ITEMS;
}
