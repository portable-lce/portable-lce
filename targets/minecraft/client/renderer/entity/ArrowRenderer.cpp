#include "ArrowRenderer.h"

#include <math.h>

#include <memory>
#include <numbers>

#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

ResourceLocation ArrowRenderer::ARROW_LOCATION =
    ResourceLocation(TN_ITEM_ARROWS);

void ArrowRenderer::render(std::shared_ptr<Entity> _arrow, double x, double y,
                           double z, float rot, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Arrow rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<Arrow> arrow = std::dynamic_pointer_cast<Arrow>(_arrow);
    bindTexture(_arrow);  // 4J - was "/item/arrows.png"

    RenderPath.MatrixPush();

    float yRot = arrow->yRot;
    float xRot = arrow->xRot;
    float yRotO = arrow->yRotO;
    float xRotO = arrow->xRotO;
    if ((yRot - yRotO) > 180.0f)
        yRot -= 360.0f;
    else if ((yRot - yRotO) < -180.0f)
        yRot += 360.0f;
    if ((xRot - xRotO) > 180.0f)
        xRot -= 360.0f;
    else if ((xRot - xRotO) < -180.0f)
        xRot += 360.0f;

    RenderPath.MatrixTranslate((float)x, (float)y, (float)z);
    RenderPath.MatrixRotate((yRotO + (yRot - yRotO) * a - 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    RenderPath.MatrixRotate((xRotO + (xRot - xRotO) * a)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

    Tesselator* t = Tesselator::getInstance();
    int type = 0;

    float u0 = 0 / 32.0f;
    float u1 = 16 / 32.0f;
    float v0 = (0 + type * 10) / 32.0f;
    float v1 = (5 + type * 10) / 32.0f;

    float u02 = 0 / 32.0f;
    float u12 = 5 / 32.0f;
    float v02 = (5 + type * 10) / 32.0f;
    float v12 = (10 + type * 10) / 32.0f;
    float ss = 0.9f / 16.0f;
    (void)0;
    float shake = arrow->shakeTime - a;
    if (shake > 0) {
        float pow = -sinf(shake * 3) * shake;
        RenderPath.MatrixRotate((pow)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
    }
    RenderPath.MatrixRotate((45)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
    RenderPath.MatrixScale(ss, ss, ss);

    RenderPath.MatrixTranslate(-4, 0, 0);

    //    (void)0;		// 4J - changed to use tesselator
    t->begin();
    t->normal(1, 0, 0);
    t->vertexUV((float)(-7), (float)(-2), (float)(-2), (float)(u02),
                (float)(v02));
    t->vertexUV((float)(-7), (float)(-2), (float)(+2), (float)(u12),
                (float)(v02));
    t->vertexUV((float)(-7), (float)(+2), (float)(+2), (float)(u12),
                (float)(v12));
    t->vertexUV((float)(-7), (float)(+2), (float)(-2), (float)(u02),
                (float)(v12));
    t->end();

    //    (void)0;	// 4J - changed to use tesselator
    t->begin();
    t->normal(-1, 0, 0);
    t->vertexUV((float)(-7), (float)(+2), (float)(-2), (float)(u02),
                (float)(v02));
    t->vertexUV((float)(-7), (float)(+2), (float)(+2), (float)(u12),
                (float)(v02));
    t->vertexUV((float)(-7), (float)(-2), (float)(+2), (float)(u12),
                (float)(v12));
    t->vertexUV((float)(-7), (float)(-2), (float)(-2), (float)(u02),
                (float)(v12));
    t->end();

    for (int i = 0; i < 4; i++) {
        RenderPath.MatrixRotate((90)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
        //        (void)0;		// 4J - changed to use
        //        tesselator
        t->begin();
        t->normal(0, 0, 1);
        t->vertexUV((float)(-8), (float)(-2), (float)(0), (float)(u0),
                    (float)(v0));
        t->vertexUV((float)(+8), (float)(-2), (float)(0), (float)(u1),
                    (float)(v0));
        t->vertexUV((float)(+8), (float)(+2), (float)(0), (float)(u1),
                    (float)(v1));
        t->vertexUV((float)(-8), (float)(+2), (float)(0), (float)(u0),
                    (float)(v1));
        t->end();
    }
    (void)0;
    RenderPath.MatrixPop();
}

ResourceLocation* ArrowRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &ARROW_LOCATION;
}