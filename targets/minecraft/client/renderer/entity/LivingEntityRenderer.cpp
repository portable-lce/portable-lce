#include "LivingEntityRenderer.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "EntityRenderDispatcher.h"
#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/model/geom/Cube.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "platform/renderer/IRenderPath.h"


ResourceLocation LivingEntityRenderer::ENCHANT_GLINT_LOCATION =
    ResourceLocation(TN__BLUR__MISC_GLINT);
int LivingEntityRenderer::MAX_ARMOR_LAYERS = 4;

LivingEntityRenderer::LivingEntityRenderer(Model* model, float shadow) {
    this->model = model;
    shadowRadius = shadow;
    armor = nullptr;
}

void LivingEntityRenderer::setArmor(Model* armor) { this->armor = armor; }

float LivingEntityRenderer::rotlerp(float from, float to, float a) {
    float diff = to - from;
    while (diff < -180) diff += 360;
    while (diff >= 180) diff -= 360;
    return from + a * diff;
}

void LivingEntityRenderer::render(std::shared_ptr<Entity> _mob, double x,
                                  double y, double z, float rot, float a) {
    std::shared_ptr<LivingEntity> mob =
        std::dynamic_pointer_cast<LivingEntity>(_mob);

    RenderPath.MatrixPush();
    RenderPath.StateSetFaceCull(false);

    model->attackTime = getAttackAnim(mob, a);
    if (armor != nullptr) armor->attackTime = model->attackTime;
    model->riding = mob->isRiding();
    if (armor != nullptr) armor->riding = model->riding;
    model->young = mob->isBaby();
    if (armor != nullptr) armor->young = model->young;

    /*try*/
    {
        float bodyRot = rotlerp(mob->yBodyRotO, mob->yBodyRot, a);
        float headRot = rotlerp(mob->yHeadRotO, mob->yHeadRot, a);

        if (mob->isRiding() && mob->riding->instanceof(eTYPE_LIVINGENTITY)) {
            std::shared_ptr<LivingEntity> riding =
                std::dynamic_pointer_cast<LivingEntity>(mob->riding);
            bodyRot = rotlerp(riding->yBodyRotO, riding->yBodyRot, a);

            float headDiff = Mth::wrapDegrees(headRot - bodyRot);
            if (headDiff < -85) headDiff = -85;
            if (headDiff >= 85) headDiff = +85;
            bodyRot = headRot - headDiff;
            if (headDiff * headDiff > 50 * 50) {
                bodyRot += headDiff * 0.2f;
            }
        }

        float headRotx = (mob->xRotO + (mob->xRot - mob->xRotO) * a);

        setupPosition(mob, x, y, z);

        float bob = getBob(mob, a);
        setupRotations(mob, bob, bodyRot, a);

        float fScale = 1 / 16.0f;
        (void)0;
        RenderPath.MatrixScale(-1, -1, 1);

        scale(mob, a);
        RenderPath.MatrixTranslate(0, -24 * fScale - 0.125f / 16.0f, 0);

        float ws = mob->walkAnimSpeedO +
                   (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a;
        float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1 - a);
        if (mob->isBaby()) {
            wp *= 3.0f;
        }

        if (ws > 1) ws = 1;

        RenderPath.StateSetAlphaTestEnable(true);
        model->prepareMobModel(mob, wp, ws, a);
        renderModel(mob, wp, ws, bob, headRot - bodyRot, headRotx, fScale);

        for (int i = 0; i < MAX_ARMOR_LAYERS; i++) {
            int armorType = prepareArmor(mob, i, a);
            if (armorType > 0) {
                armor->prepareMobModel(mob, wp, ws, a);
                armor->render(mob, wp, ws, bob, headRot - bodyRot, headRotx,
                              fScale, true);
                if ((armorType & 0xf0) == 16) {
                    prepareSecondPassArmor(mob, i, a);
                    armor->render(mob, wp, ws, bob, headRot - bodyRot, headRotx,
                                  fScale, true);
                }
                // 4J - added condition here for rendering player as part of the
                // gui. Avoiding rendering the glint here as it involves using
                // its own blending, and for gui rendering we are globally
                // blending to be able to offer user configurable gui opacity.
                // Note that I really don't know why GL_BLEND is turned off at
                // the end of the first armour layer anyway, or why alpha
                // testing is turned on... but we definitely don't want to be
                // turning blending off during the gui render.
                if (!entityRenderDispatcher->isGuiRender) {
                    if ((armorType & 0xf) == 0xf) {
                        float time = mob->tickCount + a;
                        bindTexture(&ENCHANT_GLINT_LOCATION);
                        RenderPath.StateSetBlendEnable(true);
                        float br = 0.5f;
                        RenderPath.StateSetColour(br, br, br, 1);
                        RenderPath.StateSetDepthFunc(rp::DepthTest::equal);
                        RenderPath.StateSetDepthMask(false);

                        for (int j = 0; j < 2; j++) {
                            RenderPath.StateSetLightingEnable(false);
                            float brr = 0.76f;
                            RenderPath.StateSetColour(0.5f * brr, 0.25f * brr, 0.8f * brr, 1);
                            RenderPath.StateSetBlendFunc(rp::BlendFactor::src_color, rp::BlendFactor::one);
                            RenderPath.MatrixMode(rp::MatrixStack::texture);
                            RenderPath.MatrixSetIdentity();
                            float uo = time * (0.001f + j * 0.003f) * 20;
                            float ss = 1 / 3.0f;
                            RenderPath.MatrixScale(ss, ss, ss);
                            RenderPath.MatrixRotate((30 - (j) * 60.0f)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
                            RenderPath.MatrixTranslate(0, uo, 0);
                            RenderPath.MatrixMode(rp::MatrixStack::modelview);
                            armor->render(mob, wp, ws, bob, headRot - bodyRot,
                                          headRotx, fScale, false);
                        }

                        RenderPath.StateSetColour(1, 1, 1, 1);
                        RenderPath.MatrixMode(rp::MatrixStack::texture);
                        RenderPath.StateSetDepthMask(true);
                        RenderPath.MatrixSetIdentity();
                        RenderPath.MatrixMode(rp::MatrixStack::modelview);
                        RenderPath.StateSetLightingEnable(true);
                        RenderPath.StateSetBlendEnable(false);
                        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
                    }
                    RenderPath.StateSetBlendEnable(false);
                }
                RenderPath.StateSetAlphaTestEnable(true);
            }
        }
        RenderPath.StateSetDepthMask(true);

        additionalRendering(mob, a);
        float br = mob->getBrightness(a);
        int overlayColor = getOverlayColor(mob, br, a);
        RenderPath.StateSetActiveTexture(0x84C1);
        RenderPath.StateSetTextureEnable(false);
        RenderPath.StateSetActiveTexture(0x84C0);

        if (((overlayColor >> 24) & 0xff) > 0 || mob->hurtTime > 0 ||
            mob->deathTime > 0) {
            RenderPath.StateSetTextureEnable(false);
            RenderPath.StateSetAlphaTestEnable(false);
            RenderPath.StateSetBlendEnable(true);
            RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
            RenderPath.StateSetDepthFunc(rp::DepthTest::equal);

            // 4J - changed these renders to not use the compiled version of
            // their models, because otherwise the render states set about (in
            // particular the depth & alpha test) don't work with our command
            // buffer versions
            if (mob->hurtTime > 0 || mob->deathTime > 0) {
                RenderPath.StateSetColour(br, 0, 0, 0.4f);
                model->render(mob, wp, ws, bob, headRot - bodyRot, headRotx,
                              fScale, false);
                for (int i = 0; i < MAX_ARMOR_LAYERS; i++) {
                    if (prepareArmorOverlay(mob, i, a) >= 0) {
                        RenderPath.StateSetColour(br, 0, 0, 0.4f);
                        armor->render(mob, wp, ws, bob, headRot - bodyRot,
                                      headRotx, fScale, false);
                    }
                }
            }

            if (((overlayColor >> 24) & 0xff) > 0) {
                float r = ((overlayColor >> 16) & 0xff) / 255.0f;
                float g = ((overlayColor >> 8) & 0xff) / 255.0f;
                float b = ((overlayColor) & 0xff) / 255.0f;
                float aa = ((overlayColor >> 24) & 0xff) / 255.0f;
                RenderPath.StateSetColour(r, g, b, aa);
                model->render(mob, wp, ws, bob, headRot - bodyRot, headRotx,
                              fScale, false);
                for (int i = 0; i < MAX_ARMOR_LAYERS; i++) {
                    if (prepareArmorOverlay(mob, i, a) >= 0) {
                        RenderPath.StateSetColour(r, g, b, aa);
                        armor->render(mob, wp, ws, bob, headRot - bodyRot,
                                      headRotx, fScale, false);
                    }
                }
            }

            RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
            RenderPath.StateSetBlendEnable(false);
            RenderPath.StateSetAlphaTestEnable(true);
            RenderPath.StateSetTextureEnable(true);
        }
        (void)0;
    }
    /* catch (Exception e)
    {
    e.printStackTrace();
    }*/

    RenderPath.StateSetActiveTexture(0x84C1);
    RenderPath.StateSetTextureEnable(true);
    RenderPath.StateSetActiveTexture(0x84C0);
    RenderPath.StateSetFaceCull(true);

    RenderPath.MatrixPop();

    renderName(mob, x, y, z);
}

void LivingEntityRenderer::renderModel(std::shared_ptr<LivingEntity> mob,
                                       float wp, float ws, float bob,
                                       float headRotMinusBodyRot,
                                       float headRotx, float scale) {
    bindTexture(mob);
    if (!mob->isInvisible()) {
        model->render(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale,
                      true);
    } else if (!mob->isInvisibleTo(std::dynamic_pointer_cast<Player>(
                   Minecraft::GetInstance()->player))) {
        RenderPath.MatrixPush();
        RenderPath.StateSetColour(1, 1, 1, 0.15f);
        RenderPath.StateSetDepthMask(false);
        RenderPath.StateSetBlendEnable(true);
        RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
        RenderPath.StateSetAlphaFunc(rp::AlphaTest::greater, 1.0f / 255.0f);
        model->render(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale,
                      true);
        RenderPath.StateSetBlendEnable(false);
        RenderPath.StateSetAlphaFunc(rp::AlphaTest::greater, .1f);
        RenderPath.MatrixPop();
        RenderPath.StateSetDepthMask(true);
    } else {
        model->setupAnim(wp, ws, bob, headRotMinusBodyRot, headRotx, scale,
                         mob);
    }
}

void LivingEntityRenderer::setupPosition(std::shared_ptr<LivingEntity> mob,
                                         double x, double y, double z) {
    RenderPath.MatrixTranslate((float)x, (float)y, (float)z);
}

void LivingEntityRenderer::setupRotations(std::shared_ptr<LivingEntity> mob,
                                          float bob, float bodyRot, float a) {
    RenderPath.MatrixRotate((180 - bodyRot)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    if (mob->deathTime > 0) {
        float fall = (mob->deathTime + a - 1) / 20.0f * 1.6f;
        fall = sqrt(fall);
        if (fall > 1) fall = 1;
        RenderPath.MatrixRotate((fall * getFlipDegrees(mob))*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
    } else {
        std::string name = mob->getAName();
        if (name == "Dinnerbone" || name == "Grumm") {
            if (!mob->instanceof(eTYPE_PLAYER) ||
                !std::dynamic_pointer_cast<Player>(mob)->isCapeHidden()) {
                RenderPath.MatrixTranslate(0, mob->bbHeight + 0.1f, 0);
                RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            }
        }
    }
}

float LivingEntityRenderer::getAttackAnim(std::shared_ptr<LivingEntity> mob,
                                          float a) {
    return mob->getAttackAnim(a);
}

float LivingEntityRenderer::getBob(std::shared_ptr<LivingEntity> mob, float a) {
    return (mob->tickCount + a);
}

void LivingEntityRenderer::additionalRendering(
    std::shared_ptr<LivingEntity> mob, float a) {}

void LivingEntityRenderer::renderArrows(std::shared_ptr<LivingEntity> mob,
                                        float a) {
    int arrowCount = mob->getArrowCount();
    if (arrowCount > 0) {
        std::shared_ptr<Entity> arrow = std::shared_ptr<Entity>(
            new Arrow(mob->level, mob->x, mob->y, mob->z));
        Random random = Random(mob->entityId);
        Lighting::turnOff();
        for (int i = 0; i < arrowCount; i++) {
            RenderPath.MatrixPush();
            ModelPart* modelPart = model->getRandomModelPart(random);
            Cube* cube =
                modelPart->cubes[random.nextInt(modelPart->cubes.size())];
            modelPart->translateTo(1 / 16.0f);
            float xd = random.nextFloat();
            float yd = random.nextFloat();
            float zd = random.nextFloat();
            float xo = (cube->x0 + (cube->x1 - cube->x0) * xd) / 16.0f;
            float yo = (cube->y0 + (cube->y1 - cube->y0) * yd) / 16.0f;
            float zo = (cube->z0 + (cube->z1 - cube->z0) * zd) / 16.0f;
            RenderPath.MatrixTranslate(xo, yo, zo);
            xd = xd * 2 - 1;
            yd = yd * 2 - 1;
            zd = zd * 2 - 1;
            if (true) {
                xd *= -1;
                yd *= -1;
                zd *= -1;
            }
            float sd = (float)sqrt(xd * xd + zd * zd);
            arrow->yRotO = arrow->yRot =
                (float)(atan2(xd, zd) * 180 / std::numbers::pi);
            arrow->xRotO = arrow->xRot =
                (float)(atan2(yd, sd) * 180 / std::numbers::pi);
            double x = 0;
            double y = 0;
            double z = 0;
            float yRot = 0;
            entityRenderDispatcher->render(arrow, x, y, z, yRot, a);
            RenderPath.MatrixPop();
        }
        Lighting::turnOn();
    }
}

int LivingEntityRenderer::prepareArmorOverlay(std::shared_ptr<LivingEntity> mob,
                                              int layer, float a) {
    return prepareArmor(mob, layer, a);
}

int LivingEntityRenderer::prepareArmor(std::shared_ptr<LivingEntity> mob,
                                       int layer, float a) {
    return -1;
}

void LivingEntityRenderer::prepareSecondPassArmor(
    std::shared_ptr<LivingEntity> mob, int layer, float a) {}

float LivingEntityRenderer::getFlipDegrees(std::shared_ptr<LivingEntity> mob) {
    return 90;
}

int LivingEntityRenderer::getOverlayColor(std::shared_ptr<LivingEntity> mob,
                                          float br, float a) {
    return 0;
}

void LivingEntityRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {}

void LivingEntityRenderer::renderName(std::shared_ptr<LivingEntity> mob,
                                      double x, double y, double z) {
    if (shouldShowName(mob) || Minecraft::renderDebug()) {
        float size = 1.60f;
        float s = 1 / 60.0f * size;
        double dist = mob->distanceToSqr(entityRenderDispatcher->cameraEntity);

        float maxDist = mob->isSneaking() ? 32 : 64;

        if (dist < maxDist * maxDist) {
            std::string msg = mob->getDisplayName();

            if (!msg.empty()) {
                if (mob->isSneaking()) {
                    if (gameServices().getGameSettings(
                            eGameSetting_DisplayHUD) == 0) {
                        // 4J-PB - turn off gamertag render
                        return;
                    }

                    if (gameServices().getGameHostOption(
                            eGameHostOption_Gamertags) == 0) {
                        // turn off gamertags if the host has set them off
                        return;
                    }

                    Font* font = getFont();
                    RenderPath.MatrixPush();
                    RenderPath.MatrixTranslate((float)x + 0, (float)y + mob->bbHeight + 0.5f,
                                 (float)z);
                    (void)0;

                    RenderPath.MatrixRotate((-entityRenderDispatcher->playerRotY)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
                    RenderPath.MatrixRotate((entityRenderDispatcher->playerRotX)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

                    RenderPath.MatrixScale(-s, -s, s);
                    RenderPath.StateSetLightingEnable(false);

                    RenderPath.MatrixTranslate(0, 0.25f / s, 0);
                    RenderPath.StateSetDepthMask(false);
                    RenderPath.StateSetBlendEnable(true);
                    RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
                    Tesselator* t = Tesselator::getInstance();

                    RenderPath.StateSetTextureEnable(false);
                    t->begin();
                    int w = font->width(msg) / 2;
                    t->color(0.f, 0.f, 0.f, 0.25f);
                    t->vertex(-w - 1, -1, 0);
                    t->vertex(-w - 1, +8, 0);
                    t->vertex(+w + 1, +8, 0);
                    t->vertex(+w + 1, -1, 0);
                    t->end();
                    RenderPath.StateSetTextureEnable(true);
                    RenderPath.StateSetDepthMask(true);
                    font->draw(msg, -font->width(msg) / 2, 0, 0x20ffffff);
                    RenderPath.StateSetLightingEnable(true);
                    RenderPath.StateSetBlendEnable(false);
                    RenderPath.StateSetColour(1, 1, 1, 1);
                    RenderPath.MatrixPop();
                } else {
                    renderNameTags(mob, x, y, z, msg, s, dist);
                }
            }
        }
    }
}

bool LivingEntityRenderer::shouldShowName(std::shared_ptr<LivingEntity> mob) {
    return Minecraft::renderNames() &&
           mob != entityRenderDispatcher->cameraEntity &&
           !mob->isInvisibleTo(Minecraft::GetInstance()->player) &&
           mob->rider.lock() == nullptr;
}

void LivingEntityRenderer::renderNameTags(std::shared_ptr<LivingEntity> mob,
                                          double x, double y, double z,
                                          const std::string& msg, float scale,
                                          double dist) {
    if (mob->isSleeping()) {
        renderNameTag(mob, msg, x, y - 1.5f, z, 64);
    } else {
        renderNameTag(mob, msg, x, y, z, 64);
    }
}

// 4J Added parameter for color here so that we can colour players names
void LivingEntityRenderer::renderNameTag(std::shared_ptr<LivingEntity> mob,
                                         const std::string& name, double x,
                                         double y, double z, int maxDist,
                                         int color /*= 0xff000000*/) {
    if (gameServices().getGameSettings(eGameSetting_DisplayHUD) == 0) {
        // 4J-PB - turn off gamertag render
        return;
    }

    if (gameServices().getGameHostOption(eGameHostOption_Gamertags) == 0) {
        // turn off gamertags if the host has set them off
        return;
    }

    float dist = mob->distanceTo(entityRenderDispatcher->cameraEntity);

    if (dist > maxDist) {
        return;
    }

    Font* font = getFont();

    float size = 1.60f;
    float s = 1 / 60.0f * size;

    RenderPath.MatrixPush();
    RenderPath.MatrixTranslate((float)x + 0, (float)y + 2.3f, (float)z);
    (void)0;

    RenderPath.MatrixRotate((-this->entityRenderDispatcher->playerRotY)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    RenderPath.MatrixRotate((this->entityRenderDispatcher->playerRotX)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

    RenderPath.MatrixScale(-s, -s, s);
    RenderPath.StateSetLightingEnable(false);

    // 4J Stu - If it's beyond readable distance, then just render a coloured
    // box
    int readableDist = PLAYER_NAME_READABLE_FULLSCREEN;
    if (!RenderPath.framebuffer().is_hi_def) {
        readableDist = PLAYER_NAME_READABLE_DISTANCE_SD;
    } else if (gameServices().getLocalPlayerCount() > 2) {
        readableDist = PLAYER_NAME_READABLE_DISTANCE_SPLITSCREEN;
    }

    float textOpacity = 1.0f;
    if (dist >= readableDist) {
        int diff = dist - readableDist;

        textOpacity /= (diff / 2);

        if (diff > readableDist) textOpacity = 0.0f;
    }

    if (textOpacity < 0.0f) textOpacity = 0.0f;
    if (textOpacity > 1.0f) textOpacity = 1.0f;

    RenderPath.StateSetBlendEnable(true);
    RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
    Tesselator* t = Tesselator::getInstance();

    int offs = 0;

    std::string playerName;
    char wchName[2];

    if (mob->instanceof(eTYPE_PLAYER)) {
        std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(mob);

        if (gameServices().isXuidDeadmau5(player->getXuid())) offs = -10;

        playerName = name;
    } else {
        playerName = name;
    }

    if (textOpacity > 0.0f) {
        RenderPath.StateSetColour(1.0f, 1.0f, 1.0f, textOpacity);

        RenderPath.StateSetDepthMask(false);
        RenderPath.StateSetDepthTestEnable(false);

        RenderPath.StateSetTextureEnable(false);

        t->begin();
        int w = font->width(playerName) / 2;

        if (textOpacity < 1.0f) {
            t->color(color, 255 * textOpacity);
        } else {
            t->color(0.0f, 0.0f, 0.0f, 0.25f);
        }
        t->vertex((float)(-w - 1), (float)(-1 + offs), (float)(0));
        t->vertex((float)(-w - 1), (float)(+8 + offs + 1), (float)(0));
        t->vertex((float)(+w + 1), (float)(+8 + offs + 1), (float)(0));
        t->vertex((float)(+w + 1), (float)(-1 + offs), (float)(0));
        t->end();

        RenderPath.StateSetDepthTestEnable(true);
        RenderPath.StateSetDepthMask(true);
        RenderPath.StateSetDepthFunc(rp::DepthTest::always);
        RenderPath.StateSetLineWidth(2.0f);
        t->begin(0x0003);
        t->color(color, 255 * textOpacity);
        t->vertex((float)(-w - 1), (float)(-1 + offs), (float)(0));
        t->vertex((float)(-w - 1), (float)(+8 + offs + 1), (float)(0));
        t->vertex((float)(+w + 1), (float)(+8 + offs + 1), (float)(0));
        t->vertex((float)(+w + 1), (float)(-1 + offs), (float)(0));
        t->vertex((float)(-w - 1), (float)(-1 + offs), (float)(0));
        t->end();
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
        RenderPath.StateSetDepthMask(false);
        RenderPath.StateSetDepthTestEnable(false);

        RenderPath.StateSetTextureEnable(true);
        font->draw(playerName, -font->width(playerName) / 2, offs, 0x20ffffff);
        RenderPath.StateSetDepthTestEnable(true);

        RenderPath.StateSetDepthMask(true);
    }

    if (textOpacity < 1.0f) {
        RenderPath.StateSetColour(1.0f, 1.0f, 1.0f, 1.0f);
        RenderPath.StateSetTextureEnable(false);
        RenderPath.StateSetDepthFunc(rp::DepthTest::always);
        t->begin();
        int w = font->width(playerName) / 2;
        t->color(color, 255);
        t->vertex((float)(-w - 1), (float)(-1 + offs), (float)(0));
        t->vertex((float)(-w - 1), (float)(+8 + offs), (float)(0));
        t->vertex((float)(+w + 1), (float)(+8 + offs), (float)(0));
        t->vertex((float)(+w + 1), (float)(-1 + offs), (float)(0));
        t->end();
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
        RenderPath.StateSetTextureEnable(true);

        RenderPath.MatrixTranslate(0.0f, 0.0f, -0.04f);
    }

    if (textOpacity > 0.0f) {
        int textColor = (((int)(textOpacity * 255) << 24) | 0xffffff);
        font->draw(playerName, -font->width(playerName) / 2, offs, textColor);
    }

    RenderPath.StateSetLightingEnable(true);
    RenderPath.StateSetBlendEnable(false);
    RenderPath.StateSetColour(1, 1, 1, 1);
    RenderPath.MatrixPop();
}
