#include "DragonModel.h"
#include "platform/stubs.h"

#include <math.h>

#include <memory>
#include <numbers>
#include <string>
#include <vector>


#include "platform/renderer/renderer.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/boss/enderdragon/EnderDragon.h"

DragonModel::DragonModel(float g) : Model() {
    // 4J-PB
    texWidth = 256;
    texHeight = 256;

    setMapTex("body.body", 0, 0);
    setMapTex("wing.skin", -56, 88);
    setMapTex("wingtip.skin", -56, 144);
    setMapTex("rearleg.main", 0, 0);
    setMapTex("rearfoot.main", 112, 0);
    setMapTex("rearlegtip.main", 196, 0);
    setMapTex("head.upperhead", 112, 30);
    setMapTex("wing.bone", 112, 88);
    setMapTex("head.upperlip", 176, 44);
    setMapTex("jaw.jaw", 176, 65);
    setMapTex("frontleg.main", 112, 104);
    setMapTex("wingtip.bone", 112, 136);
    setMapTex("frontfoot.main", 144, 104);
    setMapTex("neck.box", 192, 104);
    setMapTex("frontlegtip.main", 226, 138);
    setMapTex("body.scale", 220, 53);
    setMapTex("head.scale", 0, 0);
    setMapTex("neck.scale", 48, 0);
    setMapTex("head.nostril", 112, 0);

    float zo = -16;
    head = new ModelPart(this, "head");
    head->addBox("upperlip", -6, -1, -8 + zo, 12, 5, 16);
    head->addBox("upperhead", -8, -8, 6 + zo, 16, 16, 16);
    head->bMirror = true;
    head->addBox("scale", -1 - 4, -12, 12 + zo, 2, 4, 6);
    head->addBox("nostril", -1 - 4, -3, -6 + zo, 2, 2, 4);
    head->bMirror = false;
    head->addBox("scale", -1 + 4, -12, 12 + zo, 2, 4, 6);
    head->addBox("nostril", -1 + 4, -3, -6 + zo, 2, 2, 4);

    jaw = new ModelPart(this, "jaw");
    jaw->setPos(0, 4, 8 + zo);
    jaw->addBox("jaw", -6, 0, -16, 12, 4, 16);
    head->addChild(jaw);

    neck = new ModelPart(this, "neck");
    neck->addBox("box", -5, -5, -5, 10, 10, 10);
    neck->addBox("scale", -1, -9, -5 + 2, 2, 4, 6);

    body = new ModelPart(this, "body");
    body->setPos(0, 4, 8);
    body->addBox("body", -12, 0, -16, 24, 24, 64);
    body->addBox("scale", -1, -6, -10 + 20 * 0, 2, 6, 12);
    body->addBox("scale", -1, -6, -10 + 20 * 1, 2, 6, 12);
    body->addBox("scale", -1, -6, -10 + 20 * 2, 2, 6, 12);

    wing = new ModelPart(this, "wing");
    wing->setPos(-12, 5, 2);
    wing->addBox("bone", -56, -4, -4, 56, 8, 8);
    wing->addBox("skin", -56, 0, +2, 56, 0, 56);
    wingTip = new ModelPart(this, "wingtip");
    wingTip->setPos(-56, 0, 0);
    wingTip->addBox("bone", -56, -2, -2, 56, 4, 4);
    wingTip->addBox("skin", -56, 0, +2, 56, 0, 56);
    wing->addChild(wingTip);

    frontLeg = new ModelPart(this, "frontleg");
    frontLeg->setPos(-12, 20, 2);
    frontLeg->addBox("main", -4, -4, -4, 8, 24, 8);
    frontLegTip = new ModelPart(this, "frontlegtip");
    frontLegTip->setPos(0, 20, -1);
    frontLegTip->addBox("main", -3, -1, -3, 6, 24, 6);
    frontLeg->addChild(frontLegTip);
    frontFoot = new ModelPart(this, "frontfoot");
    frontFoot->setPos(0, 23, 0);
    frontFoot->addBox("main", -4, 0, -12, 8, 4, 16);
    frontLegTip->addChild(frontFoot);

    rearLeg = new ModelPart(this, "rearleg");
    rearLeg->setPos(-12 - 4, 16, 2 + 40);
    rearLeg->addBox("main", -8, -4, -8, 16, 32, 16);
    rearLegTip = new ModelPart(this, "rearlegtip");
    rearLegTip->setPos(0, 32, -4);
    rearLegTip->addBox("main", -6, -2, 0, 12, 32, 12);
    rearLeg->addChild(rearLegTip);
    rearFoot = new ModelPart(this, "rearfoot");
    rearFoot->setPos(0, 31, 4);
    rearFoot->addBox("main", -9, 0, -20, 18, 6, 24);
    rearLegTip->addChild(rearFoot);

    // 4J added - compile now to avoid random performance hit first time cubes
    // are rendered 4J Stu - Not just performance, but alpha+depth tests don't
    // work right unless we compile here
    head->compile(1.0f / 16.0f);
    jaw->compile(1.0f / 16.0f);
    neck->compile(1.0f / 16.0f);
    body->compile(1.0f / 16.0f);
    wing->compile(1.0f / 16.0f);
    wingTip->compile(1.0f / 16.0f);
    frontLeg->compile(1.0f / 16.0f);
    frontLegTip->compile(1.0f / 16.0f);
    frontFoot->compile(1.0f / 16.0f);
    rearLeg->compile(1.0f / 16.0f);
    rearLegTip->compile(1.0f / 16.0f);
    rearFoot->compile(1.0f / 16.0f);
}

void DragonModel::prepareMobModel(std::shared_ptr<LivingEntity> mob, float time,
                                  float r, float a) {
    this->a = a;
}

void DragonModel::render(std::shared_ptr<Entity> entity, float time, float r,
                         float bob, float yRot, float xRot, float scale,
                         bool usecompiled) {
    glPushMatrix();
    std::shared_ptr<EnderDragon> dragon =
        std::dynamic_pointer_cast<EnderDragon>(entity);

    float ttt = dragon->oFlapTime + (dragon->flapTime - dragon->oFlapTime) * a;
    jaw->xRot = (float)(sinf(ttt * std::numbers::pi * 2) + 1) * 0.2f;

    float yo = (float)(sinf(ttt * std::numbers::pi * 2 - 1) + 1);
    yo = (yo * yo * 1 + yo * 2) * 0.05f;

    glTranslatef(0, yo - 2.0f, -3);
    glRotatef(yo * 2, 1, 0, 0);

    float yy = -30.0f;
    float zz = 22.0f;
    float xx = 0.0f;

    float rotScale = 1.5f;

    double startComponents[3];
    std::vector<double> start =
        std::vector<double>(startComponents, startComponents + 3);
    dragon->getLatencyPos(start, 6, a);

    double latencyPosAComponents[3], latencyPosBComponents[3];
    std::vector<double> latencyPosA =
        std::vector<double>(latencyPosAComponents, latencyPosAComponents + 3);
    std::vector<double> latencyPosB =
        std::vector<double>(latencyPosBComponents, latencyPosBComponents + 3);
    dragon->getLatencyPos(latencyPosA, 5, a);
    dragon->getLatencyPos(latencyPosB, 10, a);
    float rot2 = rotWrap(latencyPosA[0] - latencyPosB[0]);
    float rot = rotWrap(latencyPosA[0] + rot2 / 2);

    yy += 2.0f;

    float rr = 0;
    float roff = ttt * std::numbers::pi * 2.0f;
    yy = 20.0f;
    zz = -12.0f;
    double pComponents[3];
    std::vector<double> p = std::vector<double>(pComponents, pComponents + 3);

    for (int i = 0; i < 5; i++) {
        dragon->getLatencyPos(p, 5 - i, a);

        rr = (float)cosf(i * 0.45f + roff) * 0.15f;
        neck->yRot = rotWrap(dragon->getHeadPartYRotDiff(i, start, p)) *
                     std::numbers::pi / 180.0f *
                     rotScale;  // 4J replaced "p[0] - start[0] with
                                // call to getHeadPartYRotDiff
        neck->xRot = rr + (float)(dragon->getHeadPartYOffset(i, start, p)) *
                              std::numbers::pi / 180.0f * rotScale *
                              5.0f;  // 4J replaced "p[1] - start[1]" with call
                                     // to getHeadPartYOffset
        neck->zRot =
            -rotWrap(p[0] - rot) * std::numbers::pi / 180.0f * rotScale;

        neck->y = yy;
        neck->z = zz;
        neck->x = xx;
        yy += sinf(neck->xRot) * 10.0f;
        zz -= cosf(neck->yRot) * cosf(neck->xRot) * 10.0f;
        xx -= sinf(neck->yRot) * cosf(neck->xRot) * 10.0f;
        neck->render(scale, usecompiled);
    }

    head->y = yy;
    head->z = zz;
    head->x = xx;
    dragon->getLatencyPos(p, 0, a);
    head->yRot =
        rotWrap(dragon->getHeadPartYRotDiff(6, start, p)) * std::numbers::pi /
        180.0f *
        1;  // 4J replaced "p[0] - start[0] with call to getHeadPartYRotDiff
    head->xRot = (float)(dragon->getHeadPartYOffset(6, start, p)) *
                 std::numbers::pi / 180.0f * rotScale * 5.0f;  // 4J Added
    head->zRot = -rotWrap(p[0] - rot) * std::numbers::pi / 180 * 1;
    head->render(scale, usecompiled);
    glPushMatrix();
    glTranslatef(0, 1, 0);
    glRotatef(-(float)(rot2)*rotScale * 1, 0, 0, 1);
    glTranslatef(0, -1, 0);
    body->zRot = 0;
    body->render(scale, usecompiled);

    glEnable(GL_CULL_FACE);
    for (int i = 0; i < 2; i++) {
        float flapTime = ttt * std::numbers::pi * 2;
        wing->xRot = 0.125f - (float)(cosf(flapTime)) * 0.2f;
        wing->yRot = 0.25f;
        wing->zRot = (float)(sinf(flapTime) + 0.125f) * 0.8f;
        wingTip->zRot = -(float)(sinf(flapTime + 2.0f) + 0.5f) * 0.75f;

        rearLeg->xRot = 1.0f + yo * 0.1f;
        rearLegTip->xRot = 0.5f + yo * 0.1f;
        rearFoot->xRot = 0.75f + yo * 0.1f;

        frontLeg->xRot = 1.3f + yo * 0.1f;
        frontLegTip->xRot = -0.5f - yo * 0.1f;
        frontFoot->xRot = 0.75f + yo * 0.1f;
        wing->render(scale, usecompiled);
        frontLeg->render(scale, usecompiled);
        rearLeg->render(scale, usecompiled);
        glScalef(-1, 1, 1);
        if (i == 0) {
            glCullFace(GL_FRONT);
        }
    }
    glPopMatrix();
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    rr = -(float)sinf(ttt * std::numbers::pi * 2) * 0.0f;
    roff = ttt * std::numbers::pi * 2;
    yy = 10;
    zz = 60;
    xx = 0;
    dragon->getLatencyPos(start, 11, a);
    for (int i = 0; i < 12; i++) {
        dragon->getLatencyPos(p, 12 + i, a);
        rr += sinf(i * 0.45f + roff) * 0.05f;
        neck->yRot = (rotWrap(p[0] - start[0]) * rotScale + 180) *
                     std::numbers::pi / 180;
        neck->xRot = rr + (float)(p[1] - start[1]) * std::numbers::pi / 180 *
                              rotScale * 5;
        neck->zRot = rotWrap(p[0] - rot) * std::numbers::pi / 180 * rotScale;
        neck->y = yy;
        neck->z = zz;
        neck->x = xx;
        yy += sinf(neck->xRot) * 10;
        zz -= cosf(neck->yRot) * cosf(neck->xRot) * 10;
        xx -= sinf(neck->yRot) * cosf(neck->xRot) * 10;
        neck->render(scale, usecompiled);
    }
    glPopMatrix();
}
float DragonModel::rotWrap(double d) {
    while (d >= 180) d -= 360;
    while (d < -180) d += 360;
    return (float)d;
}