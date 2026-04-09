#include "TntMinecartRenderer.h"

#include <memory>

#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/MinecartRenderer.h"
#include "minecraft/world/entity/item/Minecart.h"
#include "minecraft/world/entity/item/MinecartTNT.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

void TntMinecartRenderer::renderMinecartContents(
    std::shared_ptr<Minecart> _cart, float a, Tile* tile, int tileData) {
    std::shared_ptr<MinecartTNT> cart =
        std::dynamic_pointer_cast<MinecartTNT>(_cart);

    int fuse = cart->getFuse();

    if (fuse > -1) {
        if (fuse - a + 1 < 10) {
            float g = 1 - ((fuse - a + 1) / 10.0f);
            if (g < 0) g = 0;
            if (g > 1) g = 1;
            g *= g;
            g *= g;
            float s = 1.0f + g * 0.3f;
            glScalef(s, s, s);
        }
    }

    MinecartRenderer::renderMinecartContents(cart, a, tile, tileData);

    if (fuse > -1 && fuse / 5 % 2 == 0) {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
        glColor4f(1, 1, 1, (1 - ((fuse - a + 1) / 100.0f)) * 0.8f);

        glPushMatrix();
        renderer->renderTile(Tile::tnt, 0, 1);
        glPopMatrix();

        glColor4f(1, 1, 1, 1);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }
}