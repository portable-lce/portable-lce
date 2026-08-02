#include "EnderDragonRenderer.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

#include "SharedConstants.h"
#include "java/Random.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/model/dragon/DragonModel.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/renderer/BossMobGuiInfo.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/boss/enderdragon/EnderCrystal.h"
#include "minecraft/world/entity/boss/enderdragon/EnderDragon.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation EnderDragonRenderer::DRAGON_EXPLODING_LOCATION =
    ResourceLocation(TN_MOB_ENDERDRAGON_SHUFFLE);
ResourceLocation EnderDragonRenderer::CRYSTAL_BEAM_LOCATION =
    ResourceLocation(TN_MOB_ENDERDRAGON_BEAM);
ResourceLocation EnderDragonRenderer::DRAGON_EYES_LOCATION =
    ResourceLocation(TN_MOB_ENDERDRAGON_ENDEREYES);
ResourceLocation EnderDragonRenderer::DRAGON_LOCATION =
    ResourceLocation(TN_MOB_ENDERDRAGON);

EnderDragonRenderer::EnderDragonRenderer()
    : MobRenderer(new DragonModel(0), 0.5f) {
    dragonModel = (DragonModel*)model;
    setArmor(model);  // TODO: Make second constructor that assigns this.
}

void EnderDragonRenderer::setupRotations(std::shared_ptr<LivingEntity> _mob,
                                         float bob, float bodyRot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<EnderDragon> mob =
        std::dynamic_pointer_cast<EnderDragon>(_mob);

    // 4J - reorganised a bit so we can free allocations
    double lpComponents[3];
    std::vector<double> lp =
        std::vector<double>(lpComponents, lpComponents + 3);
    mob->getLatencyPos(lp, 7, a);
    float yr = lp[0];
    // mob->getLatencyPos(lp, 5, a);
    // float rot2 = lp[1];
    // mob->getLatencyPos(lp, 10,a);
    // rot2 -= lp[1];
    float rot2 = mob->getTilt(a);

    RenderPath.MatrixRotate((-yr)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

    RenderPath.MatrixRotate((rot2)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
    // RenderPath.MatrixRotate((rot2 * 10)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

    RenderPath.MatrixTranslate(0, 0, 1);
    if (mob->deathTime > 0) {
        float fall = (mob->deathTime + a - 1) / 20.0f * 1.6f;
        fall = sqrt(fall);
        if (fall > 1) fall = 1;
        RenderPath.MatrixRotate((fall * getFlipDegrees(mob))*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
    }
}

void EnderDragonRenderer::renderModel(std::shared_ptr<LivingEntity> _mob,
                                      float wp, float ws, float bob,
                                      float headRotMinusBodyRot, float headRotx,
                                      float scale) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<EnderDragon> mob =
        std::dynamic_pointer_cast<EnderDragon>(_mob);

    if (mob->dragonDeathTime > 0) {
        float tt = (mob->dragonDeathTime / 200.0f);
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
        RenderPath.StateSetAlphaTestEnable(true);
        RenderPath.StateSetAlphaFunc(rp::AlphaTest::greater, tt);
        bindTexture(
            &DRAGON_EXPLODING_LOCATION);  // 4J was
                                          // "/mob/enderdragon/shuffle.png"
        model->render(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale,
                      true);
        RenderPath.StateSetAlphaFunc(rp::AlphaTest::greater, 0.1f);

        RenderPath.StateSetDepthFunc(rp::DepthTest::equal);
    }

    bindTexture(mob);
    model->render(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale, true);

    if (mob->hurtTime > 0) {
        RenderPath.StateSetDepthFunc(rp::DepthTest::equal);
        RenderPath.StateSetTextureEnable(false);
        RenderPath.StateSetBlendEnable(true);
        RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
        RenderPath.StateSetColour(1, 0, 0, 0.5f);
        model->render(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale,
                      false);
        RenderPath.StateSetTextureEnable(true);
        RenderPath.StateSetBlendEnable(false);
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
    }
}

void EnderDragonRenderer::render(std::shared_ptr<Entity> _mob, double x,
                                 double y, double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<EnderDragon> mob =
        std::dynamic_pointer_cast<EnderDragon>(_mob);
    BossMobGuiInfo::setBossHealth(mob, false);
    MobRenderer::render(mob, x, y, z, rot, a);
    if (mob->nearestCrystal != nullptr) {
        float tt = mob->nearestCrystal->time + a;
        float hh = sin(tt * 0.2f) / 2 + 0.5f;
        hh = (hh * hh + hh) * 0.2f;

        float xd = (float)(mob->nearestCrystal->x - mob->x -
                           (mob->xo - mob->x) * (1 - a));
        float yd = (float)(hh + mob->nearestCrystal->y - 1 - mob->y -
                           (mob->yo - mob->y) * (1 - a));
        float zd = (float)(mob->nearestCrystal->z - mob->z -
                           (mob->zo - mob->z) * (1 - a));

        float sdd = sqrt(xd * xd + zd * zd);
        float dd = sqrt(xd * xd + yd * yd + zd * zd);

        // this fixes a problem when the dragon is hit and the beam goes black
        // because the diffuse colour isn't being reset in MobRenderer::render
        RenderPath.StateSetColour(1, 1, 1, 1);

        RenderPath.MatrixPush();
        RenderPath.MatrixTranslate((float)x, (float)y + 2, (float)z);
        RenderPath.MatrixRotate((float)(-atan2(zd, xd)) - (90.0f * std::numbers::pi_v<float> / 180.f),
                  0, 1, 0);
        RenderPath.MatrixRotate((float)(-atan2(sdd, yd)) - (90.0f * std::numbers::pi_v<float> / 180.f),
                  1, 0, 0);

        // 4J-PB - Rotating the healing beam too
        static float fRot = 0.0f;
        RenderPath.MatrixRotate((fRot)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        fRot += 0.5f;  // 4J - rate of rotation changed from 5.0 to 0.5 for
                       // photosensitivity reasons
        if (fRot >= 360.0f) {
            fRot = 0.0f;
        }

        Tesselator* t = Tesselator::getInstance();
        Lighting::turnOff();
        RenderPath.StateSetFaceCull(false);

        RenderPath.StateSetBlendEnable(true);
        RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::dst_alpha);

        bindTexture(
            &CRYSTAL_BEAM_LOCATION);  // 4J was "/mob/enderdragon/beam.png"

        (void)0;

        float v0 = 0 - (mob->tickCount + a) *
                           0.005f;  // 4J - rate of movement changed from 0.01
                                    // to 0.005 for photosensitivity reasons
        float v1 = sqrt(xd * xd + yd * yd + zd * zd) / 32.0f -
                   (mob->tickCount + a) * 0.005f;

        t->begin(0x0005);

        int steps = 8;
        for (int i = 0; i <= steps; i++) {
            double d = i % steps * std::numbers::pi * 2 / steps;
            float s = sin(i % steps * std::numbers::pi * 2 / steps) * 0.75f;
            float c = cos(i % steps * std::numbers::pi * 2 / steps) * 0.75f;
            float u = i % steps * 1.0f / steps;
            // t->color(0x000000);
            t->vertexUV(s * 0.2f, c * 0.2f, 0, u, v1);
            // t->color(0xffffff);
            t->vertexUV(s, c, dd, u, v0);
        }

        t->end();
        RenderPath.StateSetFaceCull(true);
        (void)0;
        RenderPath.StateSetBlendEnable(false);

        RenderPath.MatrixPop();
        Lighting::turnOn();
    }
}

ResourceLocation* EnderDragonRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &DRAGON_LOCATION;
}

void EnderDragonRenderer::additionalRendering(
    std::shared_ptr<LivingEntity> _mob, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<EnderDragon> mob =
        std::dynamic_pointer_cast<EnderDragon>(_mob);
    MobRenderer::additionalRendering(mob, a);
    Tesselator* t = Tesselator::getInstance();

    if (mob->dragonDeathTime > 0) {
        Lighting::turnOff();
        float tt = ((mob->dragonDeathTime + a) / 200.0f);
        float overDrive = 0;
        if (tt > 0.8f) {
            overDrive = (tt - 0.8f) / 0.2f;
        }

        Random random(432);
        RenderPath.StateSetTextureEnable(false);
        (void)0;
        RenderPath.StateSetBlendEnable(true);
        RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one);
        RenderPath.StateSetAlphaTestEnable(false);
        RenderPath.StateSetFaceCull(true);
        RenderPath.StateSetDepthMask(false);
        RenderPath.MatrixPush();
        RenderPath.MatrixTranslate(0, -1, -2);
        for (int i = 0; i < (tt + tt * tt) / 2 * 60; i++) {
            RenderPath.MatrixRotate((random.nextFloat() * 360)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
            RenderPath.MatrixRotate((random.nextFloat() * 360)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
            RenderPath.MatrixRotate((random.nextFloat() * 360)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            RenderPath.MatrixRotate((random.nextFloat() * 360)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
            RenderPath.MatrixRotate((random.nextFloat() * 360)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
            RenderPath.MatrixRotate((random.nextFloat() * 360 + tt * 90)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            t->begin(0x0006);
            float dist = random.nextFloat() * 20 + 5 + overDrive * 10;
            float w = random.nextFloat() * 2 + 1 + overDrive * 2;
            t->color(0xffffff, (int)(255 * (1 - overDrive)));
            t->vertex(0, 0, 0);
            t->color(0xff00ff, 0);
            t->vertex(-0.866 * w, dist, -0.5f * w);
            t->vertex(+0.866 * w, dist, -0.5f * w);
            t->vertex(0, dist, 1 * w);
            t->vertex(-0.866 * w, dist, -0.5f * w);
            t->end();
        }
        RenderPath.MatrixPop();
        RenderPath.StateSetDepthMask(true);
        RenderPath.StateSetFaceCull(false);
        RenderPath.StateSetBlendEnable(false);
        (void)0;
        RenderPath.StateSetColour(1, 1, 1, 1);
        RenderPath.StateSetTextureEnable(true);
        RenderPath.StateSetAlphaTestEnable(true);
        Lighting::turnOn();
    }
}

int EnderDragonRenderer::prepareArmor(std::shared_ptr<LivingEntity> _mob,
                                      int layer, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<EnderDragon> mob =
        std::dynamic_pointer_cast<EnderDragon>(_mob);

    if (layer == 1) {
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
    }
    if (layer != 0) return -1;

    bindTexture(
        &DRAGON_EYES_LOCATION);  // 4J was "/mob/enderdragon/ender_eyes.png"
    float br = 1;
    RenderPath.StateSetBlendEnable(true);
    // 4J Stu - We probably don't need to do this on 360 either (as we force it
    // back on the renderer) However we do want it off for other platforms that
    // don't force it on in the render lib CBuff handling Several texture packs
    // have fully transparent bits that break if this is off
    RenderPath.StateSetBlendFunc(rp::BlendFactor::one, rp::BlendFactor::one);
    RenderPath.StateSetLightingEnable(false);
    RenderPath.StateSetDepthFunc(rp::DepthTest::equal);

    if (SharedConstants::TEXTURE_LIGHTING) {
        int col = 0xf0f0;
        int u = col % 65536;
        int v = col / 65536;

        RenderPath.StateSetVertexTextureUV(u / 1.0f, v / 1.0f);
        RenderPath.StateSetColour(1, 1, 1, 1);
    }

    RenderPath.StateSetLightingEnable(true);
    RenderPath.StateSetColour(1, 1, 1, br);
    return 1;
}
