#include "ExperienceOrbRenderer.h"

#include <math.h>

#include <memory>
#include <numbers>

#include "EntityRenderDispatcher.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/ExperienceOrb.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation ExperienceOrbRenderer::XP_ORB_LOCATION =
    ResourceLocation(TN_ITEM_EXPERIENCE_ORB);

ExperienceOrbRenderer::ExperienceOrbRenderer() {
    shadowRadius = 0.15f;
    shadowStrength = 0.75f;
}

void ExperienceOrbRenderer::render(std::shared_ptr<Entity> _orb, double x,
                                   double y, double z, float rot, float a) {
    std::shared_ptr<ExperienceOrb> orb =
        std::dynamic_pointer_cast<ExperienceOrb>(_orb);
    glPushMatrix();
    glTranslatef((float)x, (float)y, (float)z);

    int icon = orb->getIcon();
    bindTexture(orb);  // 4J was "/item/xporb.png"

    float u0 = ((icon % 4) * 16 + 0) / 64.0f;
    float u1 = ((icon % 4) * 16 + 16) / 64.0f;
    float v0 = ((icon / 4) * 16 + 0) / 64.0f;
    float v1 = ((icon / 4) * 16 + 16) / 64.0f;

    float r = 1.0f;
    float xo = 0.5f;
    float yo = 0.25f;

    if (SharedConstants::TEXTURE_LIGHTING) {
        int col = orb->getLightColor(a);
        int u = col % 65536;
        int v = col / 65536;
        glMultiTexCoord2f(GL_TEXTURE1, u / 1.0f, v / 1.0f);
        glColor4f(1, 1, 1, 1);
    } else {
        float br = orb->getBrightness(a);
        glColor4f(br, br, br, 1);
    }
    float br = 255.0f;
    float rr = (orb->tickCount + a) / 2;
    int rc = (int)((sinf(rr + 0 * std::numbers::pi * 2 / 3) + 1) * 0.5f * br);
    int gc = (int)(br);
    int bc = (int)((sinf(rr + 2 * std::numbers::pi * 2 / 3) + 1) * 0.1f * br);
    int col = rc << 16 | gc << 8 | bc;
    glRotatef(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
    glRotatef(-entityRenderDispatcher->playerRotX, 1, 0, 0);
    float s = 0.3f;
    glScalef(s, s, s);
    Tesselator* t = Tesselator::getInstance();
    t->begin();
    t->color(col, 128);
    t->normal(0, 1, 0);
    t->vertexUV(0 - xo, 0 - yo, 0, u0, v1);
    t->vertexUV(r - xo, 0 - yo, 0, u1, v1);
    t->vertexUV(r - xo, 1 - yo, 0, u1, v0);
    t->vertexUV(0 - xo, 1 - yo, 0, u0, v0);
    t->end();

    glDisable(GL_BLEND);
    glDisable(GL_RESCALE_NORMAL);
    glPopMatrix();
}

ResourceLocation* ExperienceOrbRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &XP_ORB_LOCATION;
}

void ExperienceOrbRenderer::blit(int x, int y, int sx, int sy, int w, int h) {
    float blitOffset = 0;
    float us = 1 / 256.0f;
    float vs = 1 / 256.0f;
    Tesselator* t = Tesselator::getInstance();
    t->begin();
    t->vertexUV(x + 0, y + h, blitOffset, (sx + 0) * us, (sy + h) * vs);
    t->vertexUV(x + w, y + h, blitOffset, (sx + w) * us, (sy + h) * vs);
    t->vertexUV(x + w, y + 0, blitOffset, (sx + w) * us, (sy + 0) * vs);
    t->vertexUV(x + 0, y + 0, blitOffset, (sx + 0) * us, (sy + 0) * vs);
    t->end();
}