#include "FishingHookRenderer.h"
#include "platform/stubs.h"

#include <cmath>
#include <memory>
#include <numbers>

#include "platform/renderer/renderer.h"
#include "EntityRenderDispatcher.h"

#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/FishingHook.h"
#include "minecraft/world/phys/Vec3.h"

ResourceLocation FishingHookRenderer::PARTICLE_LOCATION =
    ResourceLocation(TN_PARTICLES);

void FishingHookRenderer::render(std::shared_ptr<Entity> _hook, double x,
                                 double y, double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<FishingHook> hook =
        std::dynamic_pointer_cast<FishingHook>(_hook);

    glPushMatrix();

    glTranslatef((float)x, (float)y, (float)z);
    glEnable(GL_RESCALE_NORMAL);
    glScalef(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
    int xi = 1;
    int yi = 2;
    bindTexture(hook);  // 4J was "/particles.png"
    Tesselator* t = Tesselator::getInstance();

    float u0 = (xi * 8 + 0) / 128.0f;
    float u1 = (xi * 8 + 8) / 128.0f;
    float v0 = (yi * 8 + 0) / 128.0f;
    float v1 = (yi * 8 + 8) / 128.0f;

    float r = 1.0f;
    float xo = 0.5f;
    float yo = 0.5f;

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

    if (hook->owner != nullptr) {
        float swing = hook->owner->getAttackAnim(a);
        float swing2 = (float)sinf(sqrt(swing) * std::numbers::pi);

        Vec3 vv(-0.5, 0.03, 0.8);
        vv.xRot(-(hook->owner->xRotO +
                  (hook->owner->xRot - hook->owner->xRotO) * a) *
                std::numbers::pi / 180);
        vv.yRot(-(hook->owner->yRotO +
                  (hook->owner->yRot - hook->owner->yRotO) * a) *
                std::numbers::pi / 180);
        vv.yRot(swing2 * 0.5f);
        vv.xRot(-swing2 * 0.7f);

        double xp =
            hook->owner->xo + (hook->owner->x - hook->owner->xo) * a + vv.x;
        double yp =
            hook->owner->yo + (hook->owner->y - hook->owner->yo) * a + vv.y;
        double zp =
            hook->owner->zo + (hook->owner->z - hook->owner->zo) * a + vv.z;
        double yOffset = hook->owner == std::dynamic_pointer_cast<Player>(
                                            Minecraft::GetInstance()->player)
                             ? 0
                             : hook->owner->getHeadHeight();

        // 4J-PB - changing this to be per player
        // if (this->entityRenderDispatcher->options->thirdPersonView)
        if (hook->owner->ThirdPersonView() > 0) {
            float rr =
                (float)(hook->owner->yBodyRotO +
                        (hook->owner->yBodyRot - hook->owner->yBodyRotO) * a) *
                std::numbers::pi / 180;
            double ss = sinf((float)rr);
            double cc = cosf((float)rr);
            xp = hook->owner->xo + (hook->owner->x - hook->owner->xo) * a -
                 cc * 0.35 - ss * 0.85;
            yp = hook->owner->yo + yOffset +
                 (hook->owner->y - hook->owner->yo) * a - 0.45;
            zp = hook->owner->zo + (hook->owner->z - hook->owner->zo) * a -
                 ss * 0.35 + cc * 0.85;
        }

        double xh = hook->xo + (hook->x - hook->xo) * a;
        double yh = hook->yo + (hook->y - hook->yo) * a + 4 / 16.0f;
        double zh = hook->zo + (hook->z - hook->zo) * a;

        double xa = (float)(xp - xh);
        double ya = (float)(yp - yh);
        double za = (float)(zp - zh);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        t->begin(GL_LINE_STRIP);
        t->color(0x000000);
        int steps = 16;
        for (int i = 0; i <= steps; i++) {
            float aa = i / (float)steps;
            t->vertex((float)(x + xa * aa),
                      (float)(y + ya * (aa * aa + aa) * 0.5 + 4 / 16.0f),
                      (float)(z + za * aa));
        }
        t->end();
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }
}

ResourceLocation* FishingHookRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &PARTICLE_LOCATION;
}
