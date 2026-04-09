#include "MobRenderer.h"

#include <math.h>

#include <numbers>

#include "EntityRenderDispatcher.h"
#include "LivingEntityRenderer.h"
#include "java/Class.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

class Model;

MobRenderer::MobRenderer(Model* model, float shadow)
    : LivingEntityRenderer(model, shadow) {}

void MobRenderer::render(std::shared_ptr<Entity> _mob, double x, double y,
                         double z, float rot, float a) {
    std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(_mob);

    LivingEntityRenderer::render(mob, x, y, z, rot, a);
    renderLeash(mob, x, y, z, rot, a);
}

bool MobRenderer::shouldShowName(std::shared_ptr<LivingEntity> mob) {
    return LivingEntityRenderer::shouldShowName(mob) &&
           (mob->shouldShowName() ||
            std::dynamic_pointer_cast<Mob>(mob)->hasCustomName() &&
                mob == entityRenderDispatcher->crosshairPickMob);
}

void MobRenderer::renderLeash(std::shared_ptr<Mob> entity, double x, double y,
                              double z, float rot, float a) {
    std::shared_ptr<Entity> roper = entity->getLeashHolder();
    // roper = entityRenderDispatcher.cameraEntity;
    if (roper != nullptr) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        y -= (1.6 - entity->bbHeight) * .5;
        Tesselator* tessellator = Tesselator::getInstance();
        double roperYRot =
            lerp(roper->yRotO, roper->yRot, a * .5f) * Mth::DEG_TO_RAD;
        double roperXRot =
            lerp(roper->xRotO, roper->xRot, a * .5f) * Mth::DEG_TO_RAD;
        double rotOffCos = cos(roperYRot);
        double rotOffSin = sin(roperYRot);
        double yOff = sin(roperXRot);
        if (roper->instanceof (eTYPE_HANGING_ENTITY)) {
            rotOffCos = 0;
            rotOffSin = 0;
            yOff = -1;
        }
        double swingOff = cos(roperXRot);
        double endX = lerp(roper->xo, roper->x, a) - (rotOffCos * 0.7) -
                      (rotOffSin * 0.5 * swingOff);
        double endY = lerp(roper->yo + roper->getHeadHeight() * .7,
                           roper->y + roper->getHeadHeight() * .7, a) -
                      (yOff * 0.5) - .25;
        double endZ = lerp(roper->zo, roper->z, a) - (rotOffSin * 0.7) +
                      (rotOffCos * 0.5 * swingOff);

        double entityYRot =
            lerp(entity->yBodyRotO, entity->yBodyRot, a) * Mth::DEG_TO_RAD +
            std::numbers::pi * .5;
        rotOffCos = cos(entityYRot) * entity->bbWidth * .4;
        rotOffSin = sin(entityYRot) * entity->bbWidth * .4;
        double startX = lerp(entity->xo, entity->x, a) + rotOffCos;
        double startY = lerp(entity->yo, entity->y, a);
        double startZ = lerp(entity->zo, entity->z, a) + rotOffSin;
        x += rotOffCos;
        z += rotOffSin;

        double dx = (float)(endX - startX);
        double dy = (float)(endY - startY);
        double dz = (float)(endZ - startZ);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);

        unsigned int lightCol =
            Minecraft::GetInstance()->getColourTable()->getColor(
                eMinecraftColour_Leash_Light_Colour);
        float rLightCol = ((lightCol >> 16) & 0xFF) / 255.0f;
        float gLightCol = ((lightCol >> 8) & 0xFF) / 255.0;
        float bLightCol = (lightCol & 0xFF) / 255.0;

        unsigned int darkCol =
            Minecraft::GetInstance()->getColourTable()->getColor(
                eMinecraftColour_Leash_Dark_Colour);
        float rDarkCol = ((darkCol >> 16) & 0xFF) / 255.0f;
        float gDarkCol = ((darkCol >> 8) & 0xFF) / 255.0;
        float bDarkCol = (darkCol & 0xFF) / 255.0;

        int steps = 24;
        double width = .025;
        tessellator->begin(GL_TRIANGLE_STRIP);
        for (int k = 0; k <= steps; k++) {
            if (k % 2 == 0) {
                tessellator->color(rLightCol, gLightCol, bLightCol, 1.0F);
            } else {
                tessellator->color(rDarkCol, gDarkCol, bDarkCol, 1.0F);
            }
            float aa = (float)k / (float)steps;
            tessellator->vertex(
                x + (dx * aa) + 0,
                y + (dy * ((aa * aa) + aa) * 0.5) +
                    ((((float)steps - (float)k) / (steps * 0.75F)) + 0.125F),
                z + (dz * aa));
            tessellator->vertex(
                x + (dx * aa) + width,
                y + (dy * ((aa * aa) + aa) * 0.5) +
                    ((((float)steps - (float)k) / (steps * 0.75F)) + 0.125F) +
                    width,
                z + (dz * aa));
        }
        tessellator->end();

        tessellator->begin(GL_TRIANGLE_STRIP);
        for (int k = 0; k <= steps; k++) {
            if (k % 2 == 0) {
                tessellator->color(rLightCol, gLightCol, bLightCol, 1.0F);
            } else {
                tessellator->color(rDarkCol, gDarkCol, bDarkCol, 1.0F);
            }
            float aa = (float)k / (float)steps;
            tessellator->vertex(
                x + (dx * aa) + 0,
                y + (dy * ((aa * aa) + aa) * 0.5) +
                    ((((float)steps - (float)k) / (steps * 0.75F)) + 0.125F) +
                    width,
                z + (dz * aa));
            tessellator->vertex(
                x + (dx * aa) + width,
                y + (dy * ((aa * aa) + aa) * 0.5) +
                    ((((float)steps - (float)k) / (steps * 0.75F)) + 0.125F),
                z + (dz * aa) + width);
        }
        tessellator->end();

        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
    }
}

double MobRenderer::lerp(double prev, double next, double a) {
    return prev + (next - prev) * a;
}