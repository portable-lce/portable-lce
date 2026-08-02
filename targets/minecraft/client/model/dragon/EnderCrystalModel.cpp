#include "EnderCrystalModel.h"

#include <memory>
#include <numbers>
#include <string>

#include "minecraft/client/model/geom/ModelPart.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

EnderCrystalModel::EnderCrystalModel(float g) {
    glass = new ModelPart(this, "glass");
    glass->texOffs(0, 0)->addBox(-4, -4, -4, 8, 8, 8);

    cube = new ModelPart(this, "cube");
    cube->texOffs(32, 0)->addBox(-4, -4, -4, 8, 8, 8);

    base = new ModelPart(this, "base");
    base->texOffs(0, 16)->addBox(-6, 0, -6, 12, 4, 12);

    // 4J added - compile now to avoid random performance hit first time cubes
    // are rendered
    glass->compile(1.0f / 16.0f);
    cube->compile(1.0f / 16.0f);
    base->compile(1.0f / 16.0f);
}

void EnderCrystalModel::render(std::shared_ptr<Entity> entity, float time,
                               float r, float bob, float yRot, float xRot,
                               float scale, bool usecompiled) {
    RenderPath.MatrixPush();
    RenderPath.MatrixScale(2, 2, 2);
    RenderPath.MatrixTranslate(0, -0.5f, 0);
    base->render(scale, usecompiled);
    RenderPath.MatrixRotate((r)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    RenderPath.MatrixTranslate(0, 0.8f + bob, 0);
    RenderPath.MatrixRotate((60)*(std::numbers::pi_v<float>/180.f), 0.7071f, 0, 0.7071f);
    glass->render(scale, usecompiled);
    float ss = 14 / 16.0f;
    RenderPath.MatrixScale(ss, ss, ss);
    RenderPath.MatrixRotate((60)*(std::numbers::pi_v<float>/180.f), 0.7071f, 0, 0.7071f);
    RenderPath.MatrixRotate((r)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    glass->render(scale, usecompiled);
    RenderPath.MatrixScale(ss, ss, ss);
    RenderPath.MatrixRotate((60)*(std::numbers::pi_v<float>/180.f), 0.7071f, 0, 0.7071f);
    RenderPath.MatrixRotate((r)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    cube->render(scale, usecompiled);
    RenderPath.MatrixPop();
}