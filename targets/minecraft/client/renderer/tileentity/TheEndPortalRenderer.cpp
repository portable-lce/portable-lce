#include "TheEndPortalRenderer.h"

#include <memory>
#include <numbers>

#include "TileEntityRenderDispatcher.h"
#include "java/FloatBuffer.h"
#include "java/Random.h"
#include "java/System.h"
#include "minecraft/client/Camera.h"
#include "minecraft/client/MemoryTracker.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/level/tile/entity/TheEndPortalTileEntity.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation TheEndPortalRenderer::END_SKY_LOCATION =
    ResourceLocation(TN_MISC_TUNNEL);
ResourceLocation TheEndPortalRenderer::END_PORTAL_LOCATION =
    ResourceLocation(TN_MISC_PARTICLEFIELD);
int TheEndPortalRenderer::RANDOM_SEED = 31100;
Random TheEndPortalRenderer::RANDOM = Random(RANDOM_SEED);

void TheEndPortalRenderer::render(std::shared_ptr<TileEntity> _table, double x,
                                  double y, double z, float a, bool setColor,
                                  float alpha, bool useCompiled) {
    // 4J Convert as we aren't using a templated class
    std::shared_ptr<TheEndPortalTileEntity> table =
        std::dynamic_pointer_cast<TheEndPortalTileEntity>(_table);
    float xx = (float)tileEntityRenderDispatcher->xPlayer;
    float yy = (float)tileEntityRenderDispatcher->yPlayer;
    float zz = (float)tileEntityRenderDispatcher->zPlayer;

    RenderPath.StateSetLightingEnable(false);

    RANDOM.setSeed(RANDOM_SEED);

    float hoff = 12 / 16.0f;
    for (int i = 0; i < 16; i++) {
        RenderPath.MatrixPush();

        float dist = (16 - (i));
        float sscale = 1 / 16.0f;

        float br = 1.0f / (dist + 1);
        if (i == 0) {
            this->bindTexture(&END_SKY_LOCATION);
            br = 0.1f;
            dist = 65;
            sscale = 1 / 8.0f;
            RenderPath.StateSetBlendEnable(true);
            RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
        }
        if (i == 1) {
            this->bindTexture(&END_PORTAL_LOCATION);
            RenderPath.StateSetBlendEnable(true);
            RenderPath.StateSetBlendFunc(rp::BlendFactor::one, rp::BlendFactor::one);
            sscale = 1 / 2.0f;
        }

        float dd = (float)-(y + hoff);
        {
            float ss1 = (float)(dd + Camera::yPlayerOffs);
            float ss2 = (float)(dd + dist + Camera::yPlayerOffs);
            float s = ss1 / ss2;
            s = (float)(y + hoff) + s;

            RenderPath.MatrixTranslate(xx, s, zz);
        }
        // 4J - note that the glTexGeni/glEnable calls don't actually do
        // anything in our opengl wrapper version, everything is currently just
        // inferred from the glTexGen calls.

        (void)0;
        (void)0;
        (void)0;
        (void)0;

        // texgen not supported in bgfx backend

        (void)0;
        (void)0;
        (void)0;
        (void)0;

        RenderPath.MatrixPop();
        RenderPath.MatrixMode(rp::MatrixStack::texture);

        RenderPath.MatrixPush();
        RenderPath.MatrixSetIdentity();

        RenderPath.MatrixTranslate(0, System::currentTimeMillis() % 700000 / 700000.0f, 0);
        RenderPath.MatrixScale(sscale, sscale, sscale);
        RenderPath.MatrixTranslate(0.5f, 0.5f, 0);
        RenderPath.MatrixRotate(((i * i * 4321 + i * 9) * 2.0f)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        RenderPath.MatrixTranslate(-0.5f, -0.5f, 0);
        RenderPath.MatrixTranslate(-xx, -zz, -yy);
        float ss1 = (float)(dd + Camera::yPlayerOffs);
        RenderPath.MatrixTranslate(Camera::xPlayerOffs * dist / ss1,
                                   Camera::zPlayerOffs * dist / ss1, -yy);

        Tesselator* t = Tesselator::getInstance();
        t->useProjectedTexture(
            true);  // 4J added - turns on both the generation of texture
                    // coordinates in the vertex shader & perspective divide of
                    // the texture coord in the pixel shader
        t->begin();

        float r = RANDOM.nextFloat() * 0.5f + 0.1f;
        float g = RANDOM.nextFloat() * 0.5f + 0.4f;
        float b = RANDOM.nextFloat() * 0.5f + 0.5f;
        if (i == 0) r = g = b = 1;
        t->color(r * br, g * br, b * br, 1.0f);
        t->vertex(x, y + hoff, z);
        t->vertex(x, y + hoff, z + 1);
        t->vertex(x + 1, y + hoff, z + 1);
        t->vertex(x + 1, y + hoff, z);
        t->end();

        t->useProjectedTexture(false);  // 4J added
        RenderPath.MatrixPop();
        RenderPath.MatrixMode(rp::MatrixStack::modelview);
    }
    RenderPath.StateSetBlendEnable(false);

    (void)0;
    (void)0;
    (void)0;
    (void)0;
    RenderPath.StateSetLightingEnable(true);
}

TheEndPortalRenderer::TheEndPortalRenderer() {
    lb = MemoryTracker::createFloatBuffer(16);
}

FloatBuffer* TheEndPortalRenderer::getBuffer(float a, float b, float c,
                                             float d) {
    lb->clear();
    lb->put(a)->put(b)->put(c)->put(d);
    lb->flip();
    return lb;
}
