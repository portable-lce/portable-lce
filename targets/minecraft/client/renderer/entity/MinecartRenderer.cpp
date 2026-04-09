#include "MinecartRenderer.h"

#include <math.h>
#include <stdint.h>

#include <numbers>
#include <optional>

#include "minecraft/client/model/MinecartModel.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/item/Minecart.h"
#include "minecraft/world/phys/Vec3.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

class Tile;

ResourceLocation MinecartRenderer::MINECART_LOCATION(TN_ITEM_CART);

MinecartRenderer::MinecartRenderer() {
    this->shadowRadius = 0.5f;
    model = new MinecartModel();
    renderer = new TileRenderer();
}

void MinecartRenderer::render(std::shared_ptr<Entity> _cart, double x, double y,
                              double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Minecart> cart = std::dynamic_pointer_cast<Minecart>(_cart);

    glPushMatrix();

    bindTexture(cart);

    // 4jcraft added a bunch of casts to prever overflow
    int64_t seed = (int64_t)((uint64_t)cart->entityId * 493286711ULL);
    seed = (int64_t)(((uint64_t)seed * (uint64_t)seed * 4392167121ULL) +
                     ((uint64_t)seed * 98761ULL));

    float xo = ((((seed >> 16) & 0x7) + 0.5f) / 8.0f - 0.5f) * 0.004f;
    float yo = ((((seed >> 20) & 0x7) + 0.5f) / 8.0f - 0.5f) * 0.004f;
    float zo = ((((seed >> 24) & 0x7) + 0.5f) / 8.0f - 0.5f) * 0.004f;

    glTranslatef(xo, yo, zo);

    double xx = cart->xOld + (cart->x - cart->xOld) * a;
    double yy = cart->yOld + (cart->y - cart->yOld) * a;
    double zz = cart->zOld + (cart->z - cart->zOld) * a;

    double r = 0.3f;

    std::optional<Vec3> p = cart->getPos(xx, yy, zz);

    float xRot = cart->xRotO + (cart->xRot - cart->xRotO) * a;

    if (p.has_value()) {
        auto p0 = cart->getPosOffs(xx, yy, zz, r);
        auto p1 = cart->getPosOffs(xx, yy, zz, -r);
        if (!p0.has_value()) p0 = p;
        if (!p1.has_value()) p1 = p;

        x += p->x - xx;
        y += (p0->y + p1->y) / 2 - yy;
        z += p->z - zz;

        Vec3 dir(-p0->x, -p0->y, -p0->z);
        dir = dir.add(p1->x, p1->y, p1->z);
        if (dir.length() == 0) {
        } else {
            dir = dir.normalize();
            rot = (float)(atan2(dir.z, dir.x) * 180 / std::numbers::pi);
            xRot = (float)(atan(dir.y) * 73);
        }
    }
    glTranslatef((float)x, (float)y, (float)z);

    glRotatef(180 - rot, 0, 1, 0);
    glRotatef(-xRot, 0, 0, 1);
    float hurt = cart->getHurtTime() - a;
    float dmg = cart->getDamage() - a;
    if (dmg < 0) dmg = 0;
    if (hurt > 0) {
        glRotatef(sinf(hurt) * hurt * dmg / 10 * cart->getHurtDir(), 1, 0, 0);
    }

    int yOffset = cart->getDisplayOffset();
    Tile* tile = cart->getDisplayTile();
    int tileData = cart->getDisplayData();

    if (tile != nullptr) {
        glPushMatrix();

        bindTexture(&TextureAtlas::LOCATION_BLOCKS);
        float ss = 12 / 16.0f;

        glScalef(ss, ss, ss);
        glTranslatef(0 / 16.f, yOffset / 16.f, 0 / 16.f);
        renderMinecartContents(cart, a, tile, tileData);

        glPopMatrix();
        glColor4f(1, 1, 1, 1);
        bindTexture(cart);
    }

    glScalef(-1, -1, 1);
    model->render(cart, 0, 0, -0.1f, 0, 0, 1 / 16.0f, true);
    glPopMatrix();

    /*
    if (cart->type != Minecart::RIDEABLE)
    {
    glPushMatrix();
    bindTexture(TN_TERRAIN);	// 4J was "/terrain.png"
    float ss = 12 / 16.0f;
    glScalef(ss, ss, ss);

    // 4J - changes here brought forward from 1.2.3
    if (cart->type == Minecart::CHEST)
    {
    glTranslatef(0 / 16.0f, 8 / 16.0f, 0 / 16.0f);
    TileRenderer *tr = new TileRenderer();
    tr->renderTile(Tile::chest, 0, cart->getBrightness(a));
    delete tr;
    }
    else if (cart->type == Minecart::FURNACE)
    {
    glTranslatef(0, 6 / 16.0f, 0);
    TileRenderer *tr = new TileRenderer();
    tr->renderTile(Tile::furnace, 0, cart->getBrightness(a));
    delete tr;
    }
    glPopMatrix();
    glColor4f(1, 1, 1, 1);
    }

    bindTexture(TN_ITEM_CART);		// 4J - was "/item/cart.png"
    glScalef(-1, -1, 1);
    // model.render(0, 0, cart->getLootContent() * 7.1f - 0.1f, 0, 0, 1 /
    // 16.0f);
    model->render(cart, 0, 0, -0.1f, 0, 0, 1 / 16.0f, true);
    glPopMatrix();
    */
}

ResourceLocation* MinecartRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &MINECART_LOCATION;
}

void MinecartRenderer::renderMinecartContents(std::shared_ptr<Minecart> cart,
                                              float a, Tile* tile,
                                              int tileData) {
    float brightness = cart->getBrightness(a);

    glPushMatrix();
    renderer->renderTile(tile, tileData, brightness);
    glPopMatrix();
}
