#include "GameRenderer.h"

#include <float.h>

#include <algorithm>
#include <cmath>
#include <numbers>

#include "BossMobGuiInfo.h"
#include "Chunk.h"
#include "ItemInHandRenderer.h"
#include "LevelRenderer.h"
#include "Tesselator.h"
#include "java/Class.h"
#include "java/FloatBuffer.h"
#include "java/JavaMath.h"
#include "java/Random.h"
#include "java/System.h"
#include "minecraft/Facing.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/Camera.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/MemoryTracker.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/gui/ScreenSizeCalculator.h"
#include "minecraft/client/gui/particle/GuiParticles.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/particle/ParticleEngine.h"
#include "minecraft/client/particle/SmokeParticle.h"
#include "minecraft/client/particle/WaterDropParticle.h"
#include "minecraft/client/player/LocalPlayer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/culling/Culler.h"
#include "minecraft/client/renderer/culling/Frustum.h"
#include "minecraft/client/renderer/culling/FrustumCuller.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/util/Log.h"
#include "minecraft/util/SmoothFloat.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/EnchantmentHelper.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/biome/BiomeSource.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/chunk/CompressedTileStorage.h"
#include "minecraft/world/level/chunk/SparseDataStorage.h"
#include "minecraft/world/level/chunk/SparseLightStorage.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/HitResult.h"
#include "minecraft/world/phys/Vec3.h"
#include "platform/PlatformTypes.h"
#include "platform/ShutdownManager.h"
#include "platform/input/input.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "util/FrameProfiler.h"

bool GameRenderer::anaglyph3d = false;
int GameRenderer::anaglyphPass = 0;

#if defined(MULTITHREAD_ENABLE)
C4JThread* GameRenderer::m_updateThread;
C4JThread::EventArray* GameRenderer::m_updateEvents;
bool GameRenderer::nearThingsToDo = false;
bool GameRenderer::updateRunning = false;
std::vector<uint8_t*> GameRenderer::m_deleteStackByte;
std::vector<SparseLightStorage*> GameRenderer::m_deleteStackSparseLightStorage;
std::vector<CompressedTileStorage*>
    GameRenderer::m_deleteStackCompressedTileStorage;
std::vector<SparseDataStorage*> GameRenderer::m_deleteStackSparseDataStorage;
#endif
std::mutex GameRenderer::m_csDeleteStack;

ResourceLocation GameRenderer::RAIN_LOCATION =
    ResourceLocation(TN_ENVIRONMENT_RAIN);
ResourceLocation GameRenderer::SNOW_LOCATION =
    ResourceLocation(TN_ENVIRONMENT_SNOW);

// dirty light tracking
static bool s_lightTexDirty[XUSER_MAX_COUNT] = {true, true, true, true};
static uint64_t s_lightTexKey[XUSER_MAX_COUNT] = {};
static bool s_lightTexKeyValid[XUSER_MAX_COUNT] = {};
static inline uint8_t light_q8(float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    return (uint8_t)(v * 255.0f + 0.5f);
}

GameRenderer::GameRenderer(Minecraft* mc) {
    // 4J - added this block of initialisers
    renderDistance = 0;
    _tick = 0;
    hovered = nullptr;
    thirdDistance = 4;
    thirdDistanceO = 4;
    thirdRotation = 0;
    thirdRotationO = 0;
    thirdTilt = 0;
    thirdTiltO = 0;

    accumulatedSmoothXO = 0;
    accumulatedSmoothYO = 0;
    tickSmoothXO = 0;
    tickSmoothYO = 0;
    lastTickA = 0;

    cameraPos = Vec3(0.0f, 0.0f, 0.0f);

    fovOffset = 0;
    fovOffsetO = 0;
    cameraRoll = 0;
    cameraRollO = 0;
    for (int i = 0; i < 4; i++) {
        fov[i] = 0.0f;
        oFov[i] = 0.0f;
        tFov[i] = 0.0f;
    }
    isInClouds = false;
    zoom = 1;
    zoom_x = 0;
    zoom_y = 0;
    rainXa = nullptr;
    rainZa = nullptr;
    lastActiveTime = Minecraft::currentTimeMillis();
    lastNsTime = 0;
    random = new Random();
    rainSoundTime = 0;
    xMod = 0;
    yMod = 0;
    lb = MemoryTracker::createFloatBuffer(16);
    fr = 0.0f;
    fg = 0.0f;
    fb = 0.0f;
    fogBrO = 0.0f;
    fogBr = 0.0f;
    cameraFlip = 0;
    _updateLightTexture = false;
    blr = 0.0f;
    blrt = 0.0f;
    blg = 0.0f;
    blgt = 0.0f;

    darkenWorldAmount = 0.0f;
    darkenWorldAmountO = 0.0f;

    m_fov = 70.0f;

    // 4J Stu - Init these so they are setup before the tick
    for (int i = 0; i < 4; i++) {
        fov[i] = oFov[i] = 1.0f;
    }

    this->mc = mc;
    itemInHandRenderer = nullptr;

    // 4J-PB - set up the local players iteminhand renderers here - needs to be
    // done with lighting enabled so that the render geometry gets compiled
    // correctly
    glEnable(GL_LIGHTING);
    mc->localitemInHandRenderers[0] =
        new ItemInHandRenderer(mc);  // itemInHandRenderer;
    mc->localitemInHandRenderers[1] = new ItemInHandRenderer(mc);
    mc->localitemInHandRenderers[2] = new ItemInHandRenderer(mc);
    mc->localitemInHandRenderers[3] = new ItemInHandRenderer(mc);
    glDisable(GL_LIGHTING);

    // 4J - changes brought forward from 1.8.2
    BufferedImage* img = new BufferedImage(16, 16, BufferedImage::TYPE_INT_RGB);
    for (int i = 0; i < NUM_LIGHT_TEXTURES; i++) {
        lightTexture[i] =
            mc->textures->getTexture(img);  // 4J - changed to one light texture
                                            // per level to support split screen
    }
    delete img;
    for (int i = 0; i < NUM_LIGHT_TEXTURES; i++)
        lightPixels[i] = std::vector<int>(16 * 16);

#if defined(MULTITHREAD_ENABLE)
    m_updateEvents = new C4JThread::EventArray(
        eUpdateEventCount, C4JThread::EventArray::Mode::AutoClear);
    m_updateEvents->set(eUpdateEventIsFinished);

    m_updateThread = new C4JThread(runUpdate, nullptr, "Chunk update");
    m_updateThread->run();
#endif
}

// 4J Stu Added to go with 1.8.2 change
GameRenderer::~GameRenderer() {
    if (rainXa != nullptr) delete[] rainXa;
    if (rainZa != nullptr) delete[] rainZa;
}

void GameRenderer::tick(bool first)  // 4J - add bFirst
{
    tickFov();
    tickLightTexture();  // 4J - change brought forward from 1.8.2
    fogBrO = fogBr;
    thirdDistanceO = thirdDistance;
    thirdRotationO = thirdRotation;
    thirdTiltO = thirdTilt;
    fovOffsetO = fovOffset;
    cameraRollO = cameraRoll;

    if (mc->options->smoothCamera) {
        // update player view in tick() instead of render() to maintain
        // camera movement regardless of FPS
        float ss = mc->options->sensitivity * 0.6f + 0.2f;
        float sens = (ss * ss * ss) * 8;
        tickSmoothXO =
            smoothTurnX.getNewDeltaValue(accumulatedSmoothXO, 0.05f * sens);
        tickSmoothYO =
            smoothTurnY.getNewDeltaValue(accumulatedSmoothYO, 0.05f * sens);
        lastTickA = 0;

        accumulatedSmoothXO = 0;
        accumulatedSmoothYO = 0;
    }

    if (mc->cameraTargetPlayer == nullptr) {
        mc->cameraTargetPlayer = std::dynamic_pointer_cast<Mob>(mc->player);
    }

    float brr = mc->level->getBrightness(std::floor(mc->cameraTargetPlayer->x),
                                         std::floor(mc->cameraTargetPlayer->y),
                                         std::floor(mc->cameraTargetPlayer->z));
    float whiteness = (3 - mc->options->viewDistance) / 3.0f;
    float fogBrT = brr * (1 - whiteness) + whiteness;
    fogBr += (fogBrT - fogBr) * 0.1f;

    itemInHandRenderer->tick();

    tickRain();

    darkenWorldAmountO = darkenWorldAmount;
    if (BossMobGuiInfo::darkenWorld) {
        darkenWorldAmount +=
            1.0f / ((float)SharedConstants::TICKS_PER_SECOND * 1);
        if (darkenWorldAmount > 1) {
            darkenWorldAmount = 1;
        }
        BossMobGuiInfo::darkenWorld = false;
    } else if (darkenWorldAmount > 0) {
        darkenWorldAmount -=
            1.0f / ((float)SharedConstants::TICKS_PER_SECOND * 4);
    }

    if (mc->player != mc->localplayers[PlatformInput.GetPrimaryPad()])
        return;  // 4J added for split screen - only do rest of processing for
                 // once per frame

    _tick++;
}

void GameRenderer::pick(float a) {
    if (mc->cameraTargetPlayer == nullptr) return;
    if (mc->level == nullptr) return;

    mc->crosshairPickMob = nullptr;

    double range = mc->gameMode->getPickRange();
    delete mc->hitResult;
    mc->hitResult = mc->cameraTargetPlayer->pick(range, a);

    // 4J - added - stop blocks right at the edge of the world from being
    // pickable so we shouldn't be able to directly destroy or create anything
    // there
    if (mc->hitResult) {
        int maxxz = ((mc->level->chunkSource->m_XZSize / 2) * 16) - 2;
        int minxz = (-(mc->level->chunkSource->m_XZSize / 2) * 16) + 1;

        // Don't select the tops of the very edge blocks, or the sides of the
        // next blocks in 4J Stu - Only block the sides that are facing an
        // outside block
        int hitx = mc->hitResult->x;
        int hitz = mc->hitResult->z;
        int face = mc->hitResult->f;
        if (face == Facing::WEST && hitx < 0) hitx -= 1;
        if (face == Facing::EAST && hitx > 0) hitx += 1;
        if (face == Facing::NORTH && hitz < 0) hitz -= 1;
        if (face == Facing::SOUTH && hitz > 0) hitz += 1;

        if ((hitx < minxz) || (hitx > maxxz) || (hitz < minxz) ||
            (hitz > maxxz)) {
            delete mc->hitResult;
            mc->hitResult = nullptr;
        }
    }

    double dist = range;
    Vec3 from = mc->cameraTargetPlayer->getPos(a);

    if (mc->gameMode->hasFarPickRange()) {
        dist = range = 6;
    } else {
        if (dist > 3) dist = 3;
        range = dist;
    }

    if (mc->hitResult != nullptr) {
        dist = mc->hitResult->pos.distanceTo(from);
    }

    Vec3 b = mc->cameraTargetPlayer->getViewVector(a);
    Vec3 to(b.x * range, b.y * range, b.z * range);
    to = to.add(from.x, from.y, from.z);
    hovered = nullptr;
    float overlap = 1;
    AABB grown = mc->cameraTargetPlayer->bb
                     .expand(b.x * (range), b.y * (range), b.z * (range))
                     .grow(overlap, overlap, overlap);

    std::vector<std::shared_ptr<Entity> >* objects =
        mc->level->getEntities(mc->cameraTargetPlayer, &grown);
    double nearest = dist;

    auto itEnd = objects->end();
    for (auto it = objects->begin(); it != itEnd; it++) {
        std::shared_ptr<Entity> e = *it;  // objects->at(i);
        if (!e->isPickable()) continue;

        float rr = e->getPickRadius();
        AABB bb = e->bb.grow(rr, rr, rr);
        HitResult* p = bb.clip(from, to);
        if (bb.contains(from)) {
            if (0 < nearest || nearest == 0) {
                hovered = e;
                nearest = 0;
            }
        } else if (p != nullptr) {
            double dd = from.distanceTo(p->pos);
            std::shared_ptr<Entity> ridingEntity =
                mc->cameraTargetPlayer->riding;
            // 4jcraft: compare the mounted entity explicitly so riding the hit
            // target does not get collapsed into a bool/null comparison.
            if (ridingEntity != nullptr && e == ridingEntity) {
                if (nearest == 0) {
                    hovered = e;
                }
            } else {
                hovered = e;
                nearest = dd;
            }
        }
        delete p;
    }

    if (hovered != nullptr) {
        if (nearest < dist || (mc->hitResult == nullptr)) {
            if (mc->hitResult != nullptr) delete mc->hitResult;
            mc->hitResult = new HitResult(hovered);
            if (hovered->instanceof (eTYPE_LIVINGENTITY)) {
                mc->crosshairPickMob =
                    std::dynamic_pointer_cast<LivingEntity>(hovered);
            }
        }
    }
}

void GameRenderer::SetFovVal(float fov) { m_fov = fov; }

float GameRenderer::GetFovVal() { return m_fov; }

void GameRenderer::tickFov() {
    std::shared_ptr<LocalPlayer> player =
        std::dynamic_pointer_cast<LocalPlayer>(mc->cameraTargetPlayer);

    int playerIdx = player ? player->GetXboxPad() : 0;
    tFov[playerIdx] = player->getFieldOfViewModifier();

    oFov[playerIdx] = fov[playerIdx];
    fov[playerIdx] += (tFov[playerIdx] - fov[playerIdx]) * 0.5f;

    if (fov[playerIdx] > 1.5f) fov[playerIdx] = 1.5f;
    if (fov[playerIdx] < 0.1f) fov[playerIdx] = 0.1f;
}

float GameRenderer::getFov(float a, bool applyEffects) {
    if (cameraFlip > 0) return 90;

    std::shared_ptr<LocalPlayer> player =
        std::dynamic_pointer_cast<LocalPlayer>(mc->cameraTargetPlayer);
    int playerIdx = player ? player->GetXboxPad() : 0;
    float fov = m_fov;  // 70;
    if (applyEffects) {
        fov += mc->options->fov * 40;
        fov *= oFov[playerIdx] + (this->fov[playerIdx] - oFov[playerIdx]) * a;
    }
    if (player->getHealth() <= 0) {
        float duration = player->deathTime + a;

        fov /= ((1 - 500 / (duration + 500)) * 2.0f + 1);
    }

    int t = Camera::getBlockAt(mc->level, player, a);
    if (t != 0 && Tile::tiles[t]->material == Material::water)
        fov = fov * 60 / 70;

    return fov + fovOffsetO + (fovOffset - fovOffsetO) * a;
}

void GameRenderer::bobHurt(float a) {
    std::shared_ptr<LivingEntity> player = mc->cameraTargetPlayer;

    float hurt = player->hurtTime - a;

    if (player->getHealth() <= 0) {
        float duration = player->deathTime + a;

        glRotatef(40 - (40 * 200) / (duration + 200), 0, 0, 1);
    }

    if (hurt < 0) return;
    hurt /= player->hurtDuration;
    hurt = (float)sinf(hurt * hurt * hurt * hurt * std::numbers::pi);

    float rr = player->hurtDir;

    glRotatef(-rr, 0, 1, 0);
    glRotatef(-hurt * 14, 0, 0, 1);
    glRotatef(+rr, 0, 1, 0);
}

void GameRenderer::bobView(float a) {
    if (!mc->cameraTargetPlayer->instanceof (eTYPE_LIVINGENTITY)) return;

    std::shared_ptr<Player> player =
        std::dynamic_pointer_cast<Player>(mc->cameraTargetPlayer);

    float wda = player->walkDist - player->walkDistO;
    float b = -(player->walkDist + wda * a);
    float bob = player->oBob + (player->bob - player->oBob) * a;
    float tilt = player->oTilt + (player->tilt - player->oTilt) * a;
    glTranslatef((float)sinf(b * std::numbers::pi) * bob * 0.5f,
                 -(float)std::abs(cosf(b * std::numbers::pi) * bob), 0);
    glRotatef((float)sinf(b * std::numbers::pi) * bob * 3, 0, 0, 1);
    glRotatef((float)std::abs(cosf(b * std::numbers::pi - 0.2f) * bob) * 5, 1,
              0, 0);
    glRotatef((float)tilt, 1, 0, 0);
}

void GameRenderer::moveCameraToPlayer(float a) {
    std::shared_ptr<LivingEntity> player = mc->cameraTargetPlayer;
    std::shared_ptr<LocalPlayer> localplayer =
        std::dynamic_pointer_cast<LocalPlayer>(mc->cameraTargetPlayer);
    float heightOffset = player->heightOffset - 1.62f;

    double x = player->xo + (player->x - player->xo) * a;
    double y = player->yo + (player->y - player->yo) * a - heightOffset;
    double z = player->zo + (player->z - player->zo) * a;

    glRotatef(cameraRollO + (cameraRoll - cameraRollO) * a, 0, 0, 1);

    if (player->isSleeping()) {
        heightOffset += 1.0;
        glTranslatef(0.0f, 0.3f, 0);
        if (!mc->options->fixedCamera) {
            int t =
                mc->level->getTile(std::floor(player->x), std::floor(player->y),
                                   std::floor(player->z));
            if (t == Tile::bed_Id) {
                int data = mc->level->getData(std::floor(player->x),
                                              std::floor(player->y),
                                              std::floor(player->z));

                int direction = data & 3;
                glRotatef((float)direction * 90, 0.0f, 1.0f, 0.0f);
            }
            glRotatef(player->yRotO + (player->yRot - player->yRotO) * a + 180,
                      0, -1, 0);
            glRotatef(player->xRotO + (player->xRot - player->xRotO) * a, -1, 0,
                      0);
        }
    }
    // 4J-PB - changing this to be per player
    // else if (mc->options->thirdPersonView)
    else if (localplayer->ThirdPersonView()) {
        double cameraDist =
            thirdDistanceO + (thirdDistance - thirdDistanceO) * a;

        if (mc->options->fixedCamera) {
            float rotationY =
                thirdRotationO + (thirdRotation - thirdRotationO) * a;
            float xRot = thirdTiltO + (thirdTilt - thirdTiltO) * a;

            glTranslatef(0, 0, (float)-cameraDist);
            glRotatef(xRot, 1, 0, 0);
            glRotatef(rotationY, 0, 1, 0);
        } else {
            // 4J - corrected bug where this used to just take player->xRot &
            // yRot directly and so wasn't taking into account interpolation,
            // allowing camera to go through walls
            float playerYRot =
                player->yRotO + (player->yRot - player->yRotO) * a;
            float playerXRot =
                player->xRotO + (player->xRot - player->xRotO) * a;
            float yRot = playerYRot;
            float xRot = playerXRot;

            // Thirdperson view values are now 0 for disabled, 1 for original
            // mode, 2 for reversed.
            if (localplayer->ThirdPersonView() == 2) {
                // Reverse x rotation - note that this is only used in doing
                // collision to calculate our view distance, the actual rotation
                // itself is just below this else {} block
                xRot += 180.0f;
            }

            double xd = -sinf(yRot / 180 * std::numbers::pi) *
                        cosf(xRot / 180 * std::numbers::pi) * cameraDist;
            double zd = cosf(yRot / 180 * std::numbers::pi) *
                        cosf(xRot / 180 * std::numbers::pi) * cameraDist;
            double yd = -sinf(xRot / 180 * std::numbers::pi) * cameraDist;

            for (int i = 0; i < 8; i++) {
                float xo = (float)((i & 1) * 2 - 1);
                float yo = (float)(((i >> 1) & 1) * 2 - 1);
                float zo = (float)(((i >> 2) & 1) * 2 - 1);

                xo *= 0.1f;
                yo *= 0.1f;
                zo *= 0.1f;

                // 4J - corrected bug here where zo was also added to x
                // component
                Vec3 a(x + xo, y + yo, z + zo);
                Vec3 b(x - xd + xo, y - yd + yo, z - zd + zo);
                HitResult* hr = mc->level->clip(&a, &b);
                if (hr != nullptr) {
                    Vec3 p(x, y, z);
                    double dist = hr->pos.distanceTo(p);
                    if (dist < cameraDist) cameraDist = dist;
                    delete hr;
                }
            }

            if (localplayer->ThirdPersonView() == 2) {
                glRotatef(180, 0, 1, 0);
            }

            glRotatef(playerXRot - xRot, 1, 0, 0);
            glRotatef(playerYRot - yRot, 0, 1, 0);
            glTranslatef(0, 0, (float)-cameraDist);
            glRotatef(yRot - playerYRot, 0, 1, 0);
            glRotatef(xRot - playerXRot, 1, 0, 0);
        }
    } else {
        glTranslatef(0, 0, -0.1f);
    }

    if (!mc->options->fixedCamera) {
        glRotatef(player->xRotO + (player->xRot - player->xRotO) * a, 1, 0, 0);
        glRotatef(player->yRotO + (player->yRot - player->yRotO) * a + 180, 0,
                  1, 0);
    }

    glTranslatef(0, heightOffset, 0);

    x = player->xo + (player->x - player->xo) * a;
    y = player->yo + (player->y - player->yo) * a - heightOffset;
    z = player->zo + (player->z - player->zo) * a;

    isInClouds = mc->levelRenderer->isInCloud(x, y, z, a);
}

void GameRenderer::zoomRegion(double zoom, double xa, double ya) {
    zoom = zoom;
    zoom_x = xa;
    zoom_y = ya;
}

void GameRenderer::unZoomRegion() { zoom = 1; }

// 4J added as we have more complex adjustments to make for fov & aspect on
// account of viewports
void GameRenderer::getFovAndAspect(float& fov, float& aspect, float a,
                                   bool applyEffects) {
    // 4J - split out aspect ratio and fov here so we can adjust for viewports -
    // we might need to revisit these as they are maybe be too generous for
    // performance.
    aspect = mc->width / (float)mc->height;
    fov = getFov(a, applyEffects);

    if ((mc->player->m_iScreenSection ==
         IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP) ||
        (mc->player->m_iScreenSection ==
         IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM)) {
        aspect *= 2.0f;
        fov *= 0.7f;  // Reduce FOV to make things less fish-eye, at the expense
                      // of reducing vertical FOV from single player mode
    } else if ((mc->player->m_iScreenSection ==
                IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT) ||
               (mc->player->m_iScreenSection ==
                IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT)) {
        // Ideally I'd like to make the fov bigger here, but if I do then you an
        // see that the arm isn't very long...
        aspect *= 0.5f;
    }
}

void GameRenderer::setupCamera(float a, int eye) {
    renderDistance = (float)(16 * 16 >> (mc->options->viewDistance));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float stereoScale = 0.07f;
    if (mc->options->anaglyph3d)
        glTranslatef(-(eye * 2 - 1) * stereoScale, 0, 0);

    // 4J - have split out fov & aspect calculation so we can take into account
    // viewports
    float aspect, fov;
    getFovAndAspect(fov, aspect, a, true);

    if (zoom != 1) {
        glTranslatef((float)zoom_x, (float)-zoom_y, 0);
        glScaled(zoom, zoom, 1);
    }
    PlatformRenderer.MatrixPerspective(fov, aspect, 0.05f, renderDistance * 2);

    if (mc->gameMode->isCutScene()) {
        float s = 1 / 1.5f;
        glScalef(1, s, 1);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (mc->options->anaglyph3d) glTranslatef((eye * 2 - 1) * 0.10f, 0, 0);

    bobHurt(a);

    // 4J-PB - this is a per-player option
    // if (mc->options->bobView) bobView(a);

    bool bNoLegAnim = (mc->player->getAnimOverrideBitmask() &
                       (1 << HumanoidModel::eAnim_NoLegAnim)) != 0;
    bool bNoBobbingAnim = (mc->player->getAnimOverrideBitmask() &
                           (1 << HumanoidModel::eAnim_NoBobbing)) != 0;

    if (gameServices().getGameSettings(mc->player->GetXboxPad(),
                                       eGameSetting_ViewBob) &&
        !mc->player->abilities.flying && !bNoLegAnim && !bNoBobbingAnim)
        bobView(a);

    float pt = mc->player->oPortalTime +
               (mc->player->portalTime - mc->player->oPortalTime) * a;
    if (pt > 0) {
        int multiplier = 20;
        if (mc->player->hasEffect(MobEffect::confusion)) {
            multiplier = 7;
        }

        float skew = 5 / (pt * pt + 5) - pt * 0.04f;
        skew *= skew;
        glRotatef((_tick + a) * multiplier, 0, 1, 1);
        glScalef(1 / skew, 1, 1);
        glRotatef(-(_tick + a) * multiplier, 0, 1, 1);
    }

    moveCameraToPlayer(a);

    if (cameraFlip > 0) {
        int i = cameraFlip - 1;
        if (i == 1) glRotatef(90, 0, 1, 0);
        if (i == 2) glRotatef(180, 0, 1, 0);
        if (i == 3) glRotatef(-90, 0, 1, 0);
        if (i == 4) glRotatef(90, 1, 0, 0);
        if (i == 5) glRotatef(-90, 1, 0, 0);
    }
}

void GameRenderer::renderItemInHand(float a, int eye) {
    if (cameraFlip > 0) return;

    // 4jcraft: this function sometimes causes a segfault (was hell to catch
    // this in gdb) because of itemInHandRenderer not being initialized so let's
    // add a nullcheck
    if (itemInHandRenderer == nullptr) return;

    // 4J-JEV: I'm fairly confident this method would crash if the cameratarget
    // isnt a local player anyway, but oh well.
    std::shared_ptr<LocalPlayer> localplayer =
        mc->cameraTargetPlayer->instanceof
        (eTYPE_LOCALPLAYER)
            ? std::dynamic_pointer_cast<LocalPlayer>(mc->cameraTargetPlayer)
            : nullptr;

    bool renderHand = true;

    // 4J-PB - to turn off the hand for screenshots, but not when the item held
    // is a map
    if (localplayer != nullptr) {
        std::shared_ptr<ItemInstance> item =
            localplayer->inventory->getSelected();
        if (!(item && item->getItem()->id == Item::map_Id) &&
            gameServices().getGameSettings(localplayer->GetXboxPad(),
                                           eGameSetting_DisplayHand) == 0)
            renderHand = false;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float stereoScale = 0.07f;
    if (mc->options->anaglyph3d)
        glTranslatef(-(eye * 2 - 1) * stereoScale, 0, 0);

    // 4J - have split out fov & aspect calculation so we can take into account
    // viewports
    float fov, aspect;
    getFovAndAspect(fov, aspect, a, false);

    if (zoom != 1) {
        glTranslatef((float)zoom_x, (float)-zoom_y, 0);
        glScaled(zoom, zoom, 1);
    }
    PlatformRenderer.MatrixPerspective(fov, aspect, 0.05f, renderDistance * 2);

    if (mc->gameMode->isCutScene()) {
        float s = 1 / 1.5f;
        glScalef(1, s, 1);
    }

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    if (mc->options->anaglyph3d) glTranslatef((eye * 2 - 1) * 0.10f, 0, 0);

    glPushMatrix();
    bobHurt(a);

    // 4J-PB - changing this to be per player
    // if (mc->options->bobView) bobView(a);
    bool bNoLegAnim = (localplayer->getAnimOverrideBitmask() &
                       ((1 << HumanoidModel::eAnim_NoLegAnim) |
                        (1 << HumanoidModel::eAnim_NoBobbing))) != 0;
    if (gameServices().getGameSettings(localplayer->GetXboxPad(),
                                       eGameSetting_ViewBob) &&
        !localplayer->abilities.flying && !bNoLegAnim)
        bobView(a);

    // 4J: Skip hand rendering if render hand is off
    if (renderHand) {
        // 4J-PB - changing this to be per player
        // if (!mc->options->thirdPersonView &&
        // !mc->cameraTargetPlayer->isSleeping())
        if (!localplayer->ThirdPersonView() &&
            !mc->cameraTargetPlayer->isSleeping()) {
            if (!mc->options->hideGui && !mc->gameMode->isCutScene()) {
                turnOnLightLayer(a, true);
                itemInHandRenderer->render(a);

                turnOffLightLayer(a);
            }
        }
    }
    glPopMatrix();

    // 4J-PB - changing this to be per player
    // if (!mc->options->thirdPersonView &&
    // !mc->cameraTargetPlayer->isSleeping())
    if (!localplayer->ThirdPersonView() &&
        !mc->cameraTargetPlayer->isSleeping()) {
        itemInHandRenderer->renderScreenEffect(a);
        bobHurt(a);
    }

    // 4J-PB - changing this to be per player
    // if (mc->options->bobView) bobView(a);
    if (gameServices().getGameSettings(localplayer->GetXboxPad(),
                                       eGameSetting_ViewBob) &&
        !localplayer->abilities.flying && !bNoLegAnim)
        bobView(a);
}

// 4J - change brought forward from 1.8.2
void GameRenderer::turnOffLightLayer(double alpha) {  // 4J - TODO
    FRAME_PROFILE_SCOPE(Lightmap);
#if defined(__linux__)
    if (SharedConstants::TEXTURE_LIGHTING) {
        PlatformRenderer.TextureBindVertex(-1);
    }
#else
    // 4jcraft: manually handle this in order to ensure that the light layer is
    // turned off correctly
    if (SharedConstants::TEXTURE_LIGHTING) {
        glClientActiveTexture(GL_TEXTURE1);
        glActiveTexture(GL_TEXTURE1);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        glClientActiveTexture(GL_TEXTURE0);
        glActiveTexture(GL_TEXTURE0);
    }
#endif
}

// 4J - change brought forward from 1.8.2
void GameRenderer::turnOnLightLayer(
    double alpha,
    bool scaleLight) {  // 4jcraft: added scaleLight for entity lighting
    FRAME_PROFILE_SCOPE(Lightmap);
#if defined(__linux__)
    if (!SharedConstants::TEXTURE_LIGHTING) return;

    const int textureId = getLightTexture(mc->player->GetXboxPad(), mc->level);

    static int logCount = 0;
    if (logCount < 16) {
        ++logCount;
        Log::info("[linux-lightmap] turnOnLightLayer tex=%d scale=%d\n",
                  textureId, scaleLight ? 1 : 0);
    }

    PlatformRenderer.TextureBindVertex(textureId, scaleLight);
#else
    // 4jcraft: update light texture
    // todo: check implementation of getLightTexture.
    PlatformRenderer.TextureBindVertex(
        getLightTexture(mc->player->GetXboxPad(), mc->level), scaleLight);
#endif
}

// 4J - change brought forward from 1.8.2
void GameRenderer::tickLightTexture() {
    blrt += (float)((Math::random() - Math::random()) * Math::random() *
                    Math::random());
    blgt += (float)((Math::random() - Math::random()) * Math::random() *
                    Math::random());
    blrt *= 0.9;
    blgt *= 0.9;
    blr += (blrt - blr) * 1;
    blg += (blgt - blg) * 1;
    _updateLightTexture = true;

    // Mark all players dirty so updateLightTexture() knows when it actually
    // needs to tick, preventz unessesary player recompute
    for (int j = 0; j < XUSER_MAX_COUNT; j++) s_lightTexDirty[j] = true;
}

void GameRenderer::updateLightTexture(float a) {
    FRAME_PROFILE_SCOPE(Lightmap);
    // 4J-JEV: Now doing light textures on PER PLAYER basis.
    // 4J - we *had* added separate light textures for all dimensions, and this
    // loop to update them all here
    for (int j = 0; j < XUSER_MAX_COUNT; j++) {
        // Loop over all the players
        std::shared_ptr<MultiplayerLocalPlayer> player =
            Minecraft::GetInstance()->localplayers[j];
        if (player == nullptr) continue;

        if (!s_lightTexDirty[j]) continue;
        s_lightTexDirty[j] = false;

        Level* level = player->level;

        float skyDarken1 = level->getSkyDarken((float)1);
        float amount =
            darkenWorldAmountO + (darkenWorldAmount - darkenWorldAmountO) * a;
        bool hasNV = player->hasEffect(MobEffect::nightVision);
        float nvScale = hasNV ? getNightVisionScale(player, a) : 0.0f;

        uint64_t key = 0;
        key |= (uint64_t)light_q8(skyDarken1);
        key |= (uint64_t)light_q8(blr) << 8;
        key |= (uint64_t)light_q8(blg) << 16;
        key |= (uint64_t)light_q8(amount) << 24;
        key |= (uint64_t)light_q8(nvScale) << 32;
        key |= (uint64_t)(level->skyFlashTime > 0 ? 1 : 0) << 40;
        key |= (uint64_t)((unsigned int)level->dimension->id & 0xFF) << 48;

        if (s_lightTexKeyValid[j] && s_lightTexKey[j] == key) continue;
        s_lightTexKey[j] = key;
        s_lightTexKeyValid[j] = true;
        for (int i = 0; i < 256; i++) {
            float darken = skyDarken1 * 0.95f + 0.05f;
            float sky = level->dimension->brightnessRamp[i / 16] * darken;
            float block =
                level->dimension->brightnessRamp[i % 16] * (blr * 0.1f + 1.5f);

            if (level->skyFlashTime > 0) {
                sky = level->dimension->brightnessRamp[i / 16];
            }

            float rs = sky * (skyDarken1 * 0.65f + 0.35f);
            float gs = sky * (skyDarken1 * 0.65f + 0.35f);
            float bs = sky;

            float rb = block;
            float gb = block * ((block * 0.6f + 0.4f) * 0.6f + 0.4f);
            float bb = block * ((block * block) * 0.6f + 0.4f);

            float _r = (rs + rb);
            float _g = (gs + gb);
            float _b = (bs + bb);

            _r = _r * 0.96f + 0.03f;
            _g = _g * 0.96f + 0.03f;
            _b = _b * 0.96f + 0.03f;

            if (darkenWorldAmount > 0) {
                float amount = darkenWorldAmountO +
                               (darkenWorldAmount - darkenWorldAmountO) * a;
                _r = _r * (1.0f - amount) + (_r * .7f) * amount;
                _g = _g * (1.0f - amount) + (_g * .6f) * amount;
                _b = _b * (1.0f - amount) + (_b * .6f) * amount;
            }

            if (level->dimension->id == 1) {
                _r = (0.22f + rb * 0.75f);
                _g = (0.28f + gb * 0.75f);
                _b = (0.25f + bb * 0.75f);
            }

            if (player->hasEffect(MobEffect::nightVision)) {
                float scale = getNightVisionScale(player, a);
                {
                    float dist = 1.0f / _r;
                    if (dist > (1.0f / _g)) {
                        dist = (1.0f / _g);
                    }
                    if (dist > (1.0f / _b)) {
                        dist = (1.0f / _b);
                    }
                    _r = _r * (1.0f - scale) + (_r * dist) * scale;
                    _g = _g * (1.0f - scale) + (_g * dist) * scale;
                    _b = _b * (1.0f - scale) + (_b * dist) * scale;
                }
            }

            if (_r > 1) _r = 1;
            if (_g > 1) _g = 1;
            if (_b > 1) _b = 1;

            float brightness = 0.0f;  // 4J - TODO - was mc->options->gamma;

            float ir = 1 - _r;
            float ig = 1 - _g;
            float ib = 1 - _b;
            ir = 1 - (ir * ir * ir * ir);
            ig = 1 - (ig * ig * ig * ig);
            ib = 1 - (ib * ib * ib * ib);
            _r = _r * (1 - brightness) + ir * brightness;
            _g = _g * (1 - brightness) + ig * brightness;
            _b = _b * (1 - brightness) + ib * brightness;

            _r = _r * 0.96f + 0.03f;
            _g = _g * 0.96f + 0.03f;
            _b = _b * 0.96f + 0.03f;

            if (_r > 1) _r = 1;
            if (_r < 0) _r = 0;
            if (_g > 1) _g = 1;
            if (_g < 0) _g = 0;
            if (_b > 1) _b = 1;
            if (_b < 0) _b = 0;

            int alpha = 255;
            int r = (int)(_r * 255);
            int g = (int)(_g * 255);
            int b = (int)(_b * 255);

#if defined(_WIN64) || __linux__
            lightPixels[j][i] = alpha << 24 | b << 16 | g << 8 | r;
#else
            lightPixels[j][i] = r << 24 | g << 16 | b << 8 | alpha;
#endif
        }

        mc->textures->replaceTextureDirect(lightPixels[j], 16, 16,
                                           getLightTexture(j, level));
        // lightTexture->upload(); // 4J: not relevant

        //_updateLightTexture = false;
    }
}

float GameRenderer::getNightVisionScale(std::shared_ptr<Player> player,
                                        float a) {
    int duration = player->getEffect(MobEffect::nightVision)->getDuration();
    if (duration > (SharedConstants::TICKS_PER_SECOND * 10)) {
        return 1.0f;
    } else {
        float flash = std::max(0.0f, (float)duration - a);
        return .7f + sinf(flash * std::numbers::pi * .05f) *
                         .3f;  // was:  .7 + sin(flash*pi*0.2) * .3
    }
}

// 4J added, so we can have a light texture for each player to support split
// screen
int GameRenderer::getLightTexture(int iPad, Level* level) {
    // Turn the current dimenions id into an index from 0 to 2
    // int idx = level->dimension->id;
    // if( idx == -1 ) idx = 2;

    return lightTexture[iPad];  // 4J-JEV: Changing to Per Player lighting
                                // textures.
}

void GameRenderer::render(float a, bool bFirst) {
    FRAME_PROFILE_FRAME_SCOPE();

    if (_updateLightTexture && bFirst) updateLightTexture(a);
    if (Display::isActive()) {
        lastActiveTime = System::currentTimeMillis();
    } else {
        if (System::currentTimeMillis() - lastActiveTime > 500) {
            mc->pauseGame();
        }
    }

    if (mc->noRender) return;
    GameRenderer::anaglyph3d = mc->options->anaglyph3d;

    glViewport(0, 0, mc->width, mc->height);  // 4J - added
    ScreenSizeCalculator ssc(mc->options, mc->width, mc->height);
    int screenWidth = ssc.getWidth();
    int screenHeight = ssc.getHeight();
    int xMouse = PlatformInput.GetMouseX() * screenWidth / mc->width;
    int yMouse = PlatformInput.GetMouseY() * screenHeight / mc->height - 1;

    int maxFps = getFpsCap(mc->options->framerateLimit);

    if (mc->level != nullptr) {
        if (mc->options->framerateLimit == 0
#ifndef ENABLE_VSYNC
            || mc->options->framerateLimit == 3
#endif
        ) {
            renderLevel(a, 0);
        } else {
            renderLevel(a, lastNsTime + 1000000000 / maxFps);
        }

        lastNsTime = System::nanoTime();

        if (!mc->options->hideGui || mc->screen != nullptr) {
            FRAME_PROFILE_SCOPE(UIHud);
            mc->gui->render(a, mc->screen != nullptr, xMouse, yMouse);
        }
    } else {
        glViewport(0, 0, mc->width, mc->height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        setupGuiScreen();

        lastNsTime = System::nanoTime();
    }

    if (mc->screen != nullptr) {
        FRAME_PROFILE_SCOPE(UIHud);
        glClear(GL_DEPTH_BUFFER_BIT);
        mc->screen->render(xMouse, yMouse, a);
        if (mc->screen != nullptr && mc->screen->particles != nullptr)
            mc->screen->particles->render(a);
    }
}

void GameRenderer::renderLevel(float a) { renderLevel(a, 0); }

#if defined(MULTITHREAD_ENABLE)
// Request that an item be deleted, when it is safe to do so
void GameRenderer::AddForDelete(uint8_t* deleteThis) {
    m_csDeleteStack.lock();
    m_deleteStackByte.push_back(deleteThis);
}

void GameRenderer::AddForDelete(SparseLightStorage* deleteThis) {
    m_csDeleteStack.lock();
    m_deleteStackSparseLightStorage.push_back(deleteThis);
}

void GameRenderer::AddForDelete(CompressedTileStorage* deleteThis) {
    m_csDeleteStack.lock();
    m_deleteStackCompressedTileStorage.push_back(deleteThis);
}

void GameRenderer::AddForDelete(SparseDataStorage* deleteThis) {
    m_csDeleteStack.lock();
    m_deleteStackSparseDataStorage.push_back(deleteThis);
}

void GameRenderer::FinishedReassigning() { m_csDeleteStack.unlock(); }

int GameRenderer::runUpdate(void* lpParam) {
    Minecraft* minecraft = Minecraft::GetInstance();
    Tesselator::CreateNewThreadStorage(1024 * 1024);
    Compression::UseDefaultThreadStorage();
    PlatformRenderer.InitialiseContext();
#if defined(_LARGE_WORLDS)
    Chunk::CreateNewThreadStorage();
#endif
    Tile::CreateNewThreadStorage();

    ShutdownManager::HasStarted(ShutdownManager::eRenderChunkUpdateThread,
                                m_updateEvents);
    while (
        ShutdownManager::ShouldRun(ShutdownManager::eRenderChunkUpdateThread)) {
        // m_updateEvents->clear(eUpdateEventIsFinished);
        // m_updateEvents->waitForSingle(eUpdateCanRun,C4JThread::kInfiniteTimeout);
        //  4J Stu - We Need to have this happen atomically to avoid deadlocks
        m_updateEvents->waitForAll(C4JThread::kInfiniteTimeout);

        if (!ShutdownManager::ShouldRun(
                ShutdownManager::eRenderChunkUpdateThread)) {
            break;
        }

        m_updateEvents->set(eUpdateCanRun);

        // Update chunks atomically until there aren't any very near ones left -
        // they will be deferred for rendering until the call to
        // CBuffDeferredModeEnd if we have anything near to render here Now
        // limiting maximum number of updates that can be deferred as have
        // noticed that with redstone clock circuits, it is possible to create
        // things that need constant updating, so if you stand near them, the
        // render data Never gets updated and the game just keeps going until it
        // runs out of render memory...
        int count = 0;
        static const int MAX_DEFERRED_UPDATES = 10;
        bool shouldContinue = false;
        do {
            shouldContinue = minecraft->levelRenderer->updateDirtyChunks();
            count++;
        } while (shouldContinue && count < MAX_DEFERRED_UPDATES);

        //		while( minecraft->levelRenderer->updateDirtyChunks() )
        //			;
        PlatformRenderer.CBuffDeferredModeEnd();

        // If any renderable tile entities were flagged in this last block of
        // chunk(s) that were udpated, then change their flags to say that this
        // deferred chunk is over and they are actually safe to be removed now
        minecraft->levelRenderer->fullyFlagRenderableTileEntitiesToBeRemoved();

        // We've got stacks for things that can only safely be deleted whilst
        // this thread isn't updating things - delete those things now
        {
            std::lock_guard<std::mutex> lock(m_csDeleteStack);
            for (unsigned int i = 0; i < m_deleteStackByte.size(); i++)
                delete m_deleteStackByte[i];
            m_deleteStackByte.clear();
            for (unsigned int i = 0; i < m_deleteStackSparseLightStorage.size();
                 i++)
                delete m_deleteStackSparseLightStorage[i];
            m_deleteStackSparseLightStorage.clear();
            for (unsigned int i = 0;
                 i < m_deleteStackCompressedTileStorage.size(); i++)
                delete m_deleteStackCompressedTileStorage[i];
            m_deleteStackCompressedTileStorage.clear();
            for (unsigned int i = 0; i < m_deleteStackSparseDataStorage.size();
                 i++)
                delete m_deleteStackSparseDataStorage[i];
            m_deleteStackSparseDataStorage.clear();
        }

        //

        m_updateEvents->set(eUpdateEventIsFinished);
    }

    ShutdownManager::HasFinished(ShutdownManager::eRenderChunkUpdateThread);
    return 0;
}
#endif

void GameRenderer::EnableUpdateThread() {
    // #if 0 // MGH - disable the update on PS3 for now
    // 	return;
    // #endif
#if defined(MULTITHREAD_ENABLE)
    if (updateRunning) return;
    Log::info("------------------EnableUpdateThread--------------------\n");
    updateRunning = true;
    m_updateEvents->set(eUpdateCanRun);
    m_updateEvents->set(eUpdateEventIsFinished);
#endif
}

void GameRenderer::DisableUpdateThread() {
    // #if 0 // MGH - disable the update on PS3 for now
    // 	return;
    // #endif
#if defined(MULTITHREAD_ENABLE)
    if (!updateRunning) return;
    Log::info("------------------DisableUpdateThread--------------------\n");
    updateRunning = false;
    m_updateEvents->clear(eUpdateCanRun);
    m_updateEvents->waitForSingle(eUpdateEventIsFinished,
                                  C4JThread::kInfiniteTimeout);
#endif
}

void GameRenderer::renderLevel(float a, int64_t until) {
    FRAME_PROFILE_SCOPE(World);

    //	if (updateLightTexture) updateLightTexture();	// 4J - TODO -
    // Java 1.0.1 has this line enabled, should check why - don't want to put it
    // in now in case it breaks split-screen

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Is this the primary player? Only do the updating of chunks if it is. This
    // controls the creation of render data for each chunk - all of this we are
    // only going to do for the primary player, and the other players can just
    // view whatever they have loaded in - we're sharing render data between
    // players.
    bool updateChunks =
        (mc->player == mc->localplayers[PlatformInput.GetPrimaryPad()]);

    //	if (mc->cameraTargetPlayer == nullptr)	// 4J - removed condition as we
    // want to update this is mc->player changes for different local players
    { mc->cameraTargetPlayer = mc->player; }
    pick(a);

    std::shared_ptr<LivingEntity> cameraEntity = mc->cameraTargetPlayer;
    LevelRenderer* levelRenderer = mc->levelRenderer;
    ParticleEngine* particleEngine = mc->particleEngine;
    double xOff =
        cameraEntity->xOld + (cameraEntity->x - cameraEntity->xOld) * a;
    double yOff =
        cameraEntity->yOld + (cameraEntity->y - cameraEntity->yOld) * a;
    double zOff =
        cameraEntity->zOld + (cameraEntity->z - cameraEntity->zOld) * a;

    for (int i = 0; i < 2; i++) {
        if (mc->options->anaglyph3d) {
            GameRenderer::anaglyphPass = i;
            if (GameRenderer::anaglyphPass == 0)
                PlatformRenderer.StateSetWriteEnable(false, true, true, false);
            else
                PlatformRenderer.StateSetWriteEnable(true, false, false, false);
        }

        glViewport(0, 0, mc->width, mc->height);
        setupClearColor(a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        setupCamera(a, i);
        Camera::prepare(mc->player, mc->player->ThirdPersonView() == 2);

        Frustum::getFrustum();
        if (mc->options->viewDistance < 2) {
            setupFog(-1, a);
            {
                FRAME_PROFILE_SCOPE(WeatherSky);
                levelRenderer->renderSky(a);
                if (mc->skins->getSelected()->getId() == 1026)
                    levelRenderer->renderHaloRing(a);
            }
        }
        // 4jcraft: needs to be enabled for proper transparent texturing on low
        // render dists this was done in renderSky() for the far and normal
        // dists but was missing here, UPDATE: Also needed for the nether, so
        // just enable it unconditionally
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_FOG);
        setupFog(1, a);

        if (mc->options->ambientOcclusion) {
            glShadeModel(GL_SMOOTH);
        }

        //		Culler *frustum = new FrustumCuller();
        FrustumCuller frustObj;
        Culler* frustum = &frustObj;
        frustum->prepare(xOff, yOff, zOff);

        {
            FRAME_PROFILE_SCOPE(ChunkCull);
            mc->levelRenderer->cull(frustum, a);
        }

#if !defined(MULTITHREAD_ENABLE)
        if ((i == 0) && updateChunks)  // 4J - added updateChunks condition
        {
            int PIXPass = 0;
            do {
                bool retval =
                    mc->levelRenderer->updateDirtyChunks(cameraEntity, false);

                if (retval) break;

                if (until == 0) break;

                int64_t diff = until - System::nanoTime();
                if (diff < 0) break;
                if (diff > 1000000000) break;
            } while (true);
        }
#endif

        if (cameraEntity->y < Level::genDepth) {
            FRAME_PROFILE_SCOPE(WeatherSky);
            prepareAndRenderClouds(levelRenderer, a);
        }
        Frustum::getFrustum();  // 4J added - re-calculate frustum as rendering
                                // the clouds does a scale & recalculates one
                                // that isn't any good for the rest of the level
                                // rendering

        setupFog(0, a);
        glEnable(GL_FOG);
        mc->textures->bindTexture(
            &TextureAtlas::LOCATION_BLOCKS);  // 4J was "/terrain.png"
        Lighting::turnOff();
        levelRenderer->render(cameraEntity, 0, a, updateChunks);

        glShadeModel(GL_FLAT);

        if (cameraFlip == 0) {
            Lighting::turnOn();
            // 4J - for entities, don't include the "a" factor that interpolates
            // from the old to new position, as the AABBs for the entities are
            // already fully at the new position This fixes flickering
            // minecarts, and pigs that you are riding on
            frustum->prepare(cameraEntity->x, cameraEntity->y, cameraEntity->z);
            // 4J Stu - When rendering entities, in the end if the dragon is
            // hurt or we have a lot of entities we can end up wrapping our
            // index into the temp Vec3 cache and overwrite the one that was
            // storing the camera position Fix for #77745 - TU9: Content:
            // Gameplay: Items and mobs not belonging to end world are
            // disappearing when Enderdragon is damaged.
            Vec3 cameraPosTemp = cameraEntity->getPos(a);
            cameraPos.x = cameraPosTemp.x;
            cameraPos.y = cameraPosTemp.y;
            cameraPos.z = cameraPosTemp.z;
            {
                FRAME_PROFILE_SCOPE(Entity);
                levelRenderer->renderEntities(&cameraPos, frustum, a);
            }

            turnOnLightLayer(a);  // 4J - brought forward from 1.8.2
            {
                FRAME_PROFILE_SCOPE(Particle);
                particleEngine->renderLit(cameraEntity, a,
                                          ParticleEngine::OPAQUE_LIST);
            }
            Lighting::turnOff();
            setupFog(0, a);
            {
                FRAME_PROFILE_SCOPE(Particle);
                particleEngine->render(cameraEntity, a,
                                       ParticleEngine::OPAQUE_LIST);
            }

            turnOffLightLayer(a);  // 4J - brought forward from 1.8.2

            if ((mc->hitResult != nullptr) &&
                    cameraEntity->isUnderLiquid(Material::water) &&
                    cameraEntity->instanceof
                (eTYPE_PLAYER))  //&& !mc->options.hideGui)
            {
                std::shared_ptr<Player> player =
                    std::dynamic_pointer_cast<Player>(cameraEntity);
                glDisable(GL_ALPHA_TEST);
                levelRenderer->renderHit(player, mc->hitResult, 0,
                                         player->inventory->getSelected(), a);
                glEnable(GL_ALPHA_TEST);
            }
        }

        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        PlatformRenderer.StateSetBlendFunc(GL_SRC_ALPHA,
                                           GL_ONE_MINUS_SRC_ALPHA);
        PlatformRenderer.StateSetDepthMask(true);
        setupFog(0, a);
        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        mc->textures->bindTexture(
            &TextureAtlas::LOCATION_BLOCKS);  // 4J was "/terrain.png"
        // 4J - have changed this fancy rendering option to work with our
        // command buffers. The original used to use frame buffer flags to
        // disable writing to colour when doing the z-only pass, but that value
        // gets obliterated by our command buffers. Using alpha blend function
        // instead to achieve the same effect.
        if (true)  // (mc->options->fancyGraphics)
        {
            if (mc->options->ambientOcclusion) {
                glShadeModel(GL_SMOOTH);
            }

            PlatformRenderer.StateSetBlendFunc(GL_ZERO, GL_ONE);
            int visibleWaterChunks =
                levelRenderer->render(cameraEntity, 1, a, updateChunks);

            PlatformRenderer.StateSetBlendFunc(GL_SRC_ALPHA,
                                               GL_ONE_MINUS_SRC_ALPHA);

            if (visibleWaterChunks > 0) {
                levelRenderer->render(
                    cameraEntity, 1, a,
                    updateChunks);  // 4J - chanaged, used to be
                                    // renderSameAsLast but we don't support
                                    // that anymore
            }

            glShadeModel(GL_FLAT);
        } else {
            levelRenderer->render(cameraEntity, 1, a, updateChunks);
        }

        // 4J - added - have split out translucent particle rendering so that it
        // happens after the water is rendered, primarily for fireworks
        Lighting::turnOn();
        turnOnLightLayer(a);  // 4J - brought forward from 1.8.2
        {
            FRAME_PROFILE_SCOPE(Particle);
            particleEngine->renderLit(cameraEntity, a,
                                      ParticleEngine::TRANSLUCENT_LIST);
        }
        Lighting::turnOff();
        setupFog(0, a);
        {
            FRAME_PROFILE_SCOPE(Particle);
            particleEngine->render(cameraEntity, a,
                                   ParticleEngine::TRANSLUCENT_LIST);
        }

        turnOffLightLayer(a);  // 4J - brought forward from 1.8.2
        ////////////////////////// End of 4J added section

        PlatformRenderer.StateSetDepthMask(true);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        if ((zoom == 1) && cameraEntity->instanceof
            (eTYPE_PLAYER))  //&& !mc->options.hideGui)
        {
            if (mc->hitResult != nullptr &&
                !cameraEntity->isUnderLiquid(Material::water)) {
                std::shared_ptr<Player> player =
                    std::dynamic_pointer_cast<Player>(cameraEntity);
                glDisable(GL_ALPHA_TEST);
                levelRenderer->renderHitOutline(player, mc->hitResult, 0, a);
                glEnable(GL_ALPHA_TEST);
            }
        }

        /* 4J - moved rain rendering to after clouds so that it alpha blends
        onto them properly         renderSnowAndRain(a);

        glDisable(GL_FOG);
        */

        glEnable(GL_BLEND);
        PlatformRenderer.StateSetBlendFunc(GL_SRC_ALPHA, GL_ONE);
        {
            FRAME_PROFILE_SCOPE(WeatherSky);
            levelRenderer->renderDestroyAnimation(
                Tesselator::getInstance(),
                std::dynamic_pointer_cast<Player>(cameraEntity), a);
        }
        glDisable(GL_BLEND);

        if (cameraEntity->y >= Level::genDepth) {
            FRAME_PROFILE_SCOPE(WeatherSky);
            prepareAndRenderClouds(levelRenderer, a);
        }

        // 4J - rain rendering moved here so that it renders after clouds & can
        // blend properly onto them
        setupFog(0, a);
        glEnable(GL_FOG);
        {
            FRAME_PROFILE_SCOPE(WeatherSky);
            renderSnowAndRain(a);
        }

        glDisable(GL_FOG);

        if (zoom == 1) {
            glClear(GL_DEPTH_BUFFER_BIT);
            renderItemInHand(a, i);
        }

        if (!mc->options->anaglyph3d) {
            return;
        }
    }
    PlatformRenderer.StateSetWriteEnable(true, true, true, false);
}

void GameRenderer::prepareAndRenderClouds(LevelRenderer* levelRenderer,
                                          float a) {
    if (mc->options->isCloudsOn()) {
        glPushMatrix();
        setupFog(0, a);
        glEnable(GL_FOG);
        levelRenderer->renderClouds(a);

        glDisable(GL_FOG);
        setupFog(1, a);
        glPopMatrix();
    }
}

void GameRenderer::tickRain() {
    float rainLevel = mc->level->getRainLevel(1);

    if (!mc->options->fancyGraphics) rainLevel /= 2;
    if (rainLevel == 0) return;

    rainLevel /= (mc->levelRenderer->activePlayers() + 1);

    random->setSeed(_tick * 312987231l);
    std::shared_ptr<LivingEntity> player = mc->cameraTargetPlayer;
    Level* level = mc->level;

    int x0 = std::floor(player->x);
    int y0 = std::floor(player->y);
    int z0 = std::floor(player->z);

    int r = 10;

    double rainPosX = 0;
    double rainPosY = 0;
    double rainPosZ = 0;
    int rainPosSamples = 0;

    int rainCount = (int)(100 * rainLevel * rainLevel);
    if (mc->options->particles == 1) {
        rainCount >>= 1;
    } else if (mc->options->particles == 2) {
        rainCount = 0;
    }
    for (int i = 0; i < rainCount; i++) {
        int x = x0 + random->nextInt(r) - random->nextInt(r);
        int z = z0 + random->nextInt(r) - random->nextInt(r);
        int y = level->getTopRainBlock(x, z);
        int t = level->getTile(x, y - 1, z);
        Biome* biome = level->getBiome(x, z);
        if (y <= y0 + r && y >= y0 - r && biome->hasRain() &&
            biome->getTemperature() >= 0.2f) {
            float xa = random->nextFloat();
            float za = random->nextFloat();
            if (t > 0) {
                if (Tile::tiles[t]->material == Material::lava) {
                    mc->particleEngine->add(std::make_shared<SmokeParticle>(
                        level, x + xa, y + 0.1f - Tile::tiles[t]->getShapeY0(),
                        z + za, 0, 0, 0));
                } else {
                    if (random->nextInt(++rainPosSamples) == 0) {
                        rainPosX = x + xa;
                        rainPosY = y + 0.1f - Tile::tiles[t]->getShapeY0();
                        rainPosZ = z + za;
                    }
                    mc->particleEngine->add(std::shared_ptr<WaterDropParticle>(
                        new WaterDropParticle(
                            level, x + xa,
                            y + 0.1f - Tile::tiles[t]->getShapeY0(), z + za)));
                }
            }
        }
    }

    if (rainPosSamples > 0 && random->nextInt(3) < rainSoundTime++) {
        rainSoundTime = 0;
        if (rainPosY > player->y + 1 &&
            level->getTopRainBlock(std::floor(player->x),
                                   std::floor(player->z)) >
                std::floor(player->y)) {
            mc->level->playLocalSound(rainPosX, rainPosY, rainPosZ,
                                      eSoundType_AMBIENT_WEATHER_RAIN, 0.1f,
                                      0.5f);
        } else {
            mc->level->playLocalSound(rainPosX, rainPosY, rainPosZ,
                                      eSoundType_AMBIENT_WEATHER_RAIN, 0.2f,
                                      1.0f);
        }
    }
}

// 4J - this whole function updated from 1.8.2
void GameRenderer::renderSnowAndRain(float a) {
    float rainLevel = mc->level->getRainLevel(a);
    if (rainLevel <= 0) return;

    // 4J - rain is relatively low poly, but high fill-rate - better to clip it
    PlatformRenderer.StateSetEnableViewportClipPlanes(true);

    turnOnLightLayer(a);

    if (rainXa == nullptr) {
        rainXa = new float[32 * 32];
        rainZa = new float[32 * 32];

        for (int z = 0; z < 32; z++) {
            for (int x = 0; x < 32; x++) {
                float xa = x - 16;
                float za = z - 16;
                float d = std::sqrt(xa * xa + za * za);
                rainXa[z << 5 | x] = -za / d;
                rainZa[z << 5 | x] = xa / d;
            }
        }
    }

    std::shared_ptr<LivingEntity> player = mc->cameraTargetPlayer;
    Level* level = mc->level;

    int x0 = std::floor(player->x);
    int y0 = std::floor(player->y);
    int z0 = std::floor(player->z);

    Tesselator* t = Tesselator::getInstance();
    glDisable(GL_CULL_FACE);
    glNormal3f(0, 1, 0);
    glEnable(GL_BLEND);
    PlatformRenderer.StateSetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GREATER, 0.01f);

    mc->textures->bindTexture(
        &SNOW_LOCATION);  // 4J was "/environment/snow.png"

    double xo = player->xOld + (player->x - player->xOld) * a;
    double yo = player->yOld + (player->y - player->yOld) * a;
    double zo = player->zOld + (player->z - player->zOld) * a;

    int yMin = std::floor(yo);

    int r = 5;
    // 4J - was if(mc.options.fancyGraphics) r = 10;
    switch (mc->levelRenderer->activePlayers()) {
        case 1:
        default:
            r = 9;
            break;
        case 2:
            r = 7;
            break;
        case 3:
            r = 5;
            break;
        case 4:
            r = 5;
            break;
    }

    // 4J - some changes made here to access biome through new interface that
    // caches results in levelchunk flags, as an optimisation

    int mode = -1;
    float time = _tick + a;

    glColor4f(1, 1, 1, 1);

    // two snow/rain rendering
    mc->textures->bindTexture(&RAIN_LOCATION);
    t->begin();
    for (int x = x0 - r; x <= x0 + r; x++) {
        for (int z = z0 - r; z <= z0 + r; z++) {
            int rainSlot = (z - z0 + 16) * 32 + (x - x0 + 16);
            float xa = rainXa[rainSlot] * 0.5f;
            float za = rainZa[rainSlot] * 0.5f;

            Biome* b = level->getBiome(x, z);
            if (!b->hasRain() && !b->hasSnow()) continue;

            int floor = level->getTopRainBlock(x, z);
            int yy0 = y0 - r;
            int yy1 = y0 + r;
            if (yy0 < floor) yy0 = floor;
            if (yy1 < floor) yy1 = floor;
            if (yy0 == yy1) continue;

            int yl = floor;
            if (yl < yMin) yl = yMin;

            float temp = b->getTemperature();
            if (level->getBiomeSource()->scaleTemp(temp, floor) < 0.15f)
                continue;

            random->setSeed((x * x * 3121 + x * 45238971) ^
                            (z * z * 418711 + z * 13761));

            float ra = (((_tick + x * x * 3121 + x * 45238971 + z * z * 418711 +
                          z * 13761) &
                         31) +
                        a) /
                       32.0f * (3 + random->nextFloat());

            double xd = (x + 0.5f) - player->x;
            double zd = (z + 0.5f) - player->z;
            float dd = (float)std::sqrt(xd * xd + zd * zd) / r;

            float br = 1.0f;
            float s = 1.0f;
            t->offset(-xo, -yo, -zo);
            t->tex2(level->getLightColor(x, yl, z, 0));
            t->color(br, br, br, ((1 - dd * dd) * 0.5f + 0.5f) * rainLevel);
            t->vertexUV(x - xa + 0.5, yy0, z - za + 0.5, 0 * s,
                        yy0 * s / 4.0f + ra * s);
            t->vertexUV(x + xa + 0.5, yy0, z + za + 0.5, 1 * s,
                        yy0 * s / 4.0f + ra * s);
            t->vertexUV(x + xa + 0.5, yy1, z + za + 0.5, 1 * s,
                        yy1 * s / 4.0f + ra * s);
            t->vertexUV(x - xa + 0.5, yy1, z - za + 0.5, 0 * s,
                        yy1 * s / 4.0f + ra * s);
            t->offset(0, 0, 0);
        }
    }
    t->end();  // single submit for all rain geometry
    // sno time
    mc->textures->bindTexture(&SNOW_LOCATION);
    t->begin();
    for (int x = x0 - r; x <= x0 + r; x++) {
        for (int z = z0 - r; z <= z0 + r; z++) {
            int rainSlot = (z - z0 + 16) * 32 + (x - x0 + 16);
            float xa = rainXa[rainSlot] * 0.5f;
            float za = rainZa[rainSlot] * 0.5f;

            Biome* b = level->getBiome(x, z);
            if (!b->hasRain() && !b->hasSnow()) continue;

            int floor = level->getTopRainBlock(x, z);
            int yy0 = y0 - r;
            int yy1 = y0 + r;
            if (yy0 < floor) yy0 = floor;
            if (yy1 < floor) yy1 = floor;
            if (yy0 == yy1) continue;

            int yl = floor;
            if (yl < yMin) yl = yMin;

            float temp = b->getTemperature();
            // only draw snow (not rain) in this pass
            if (level->getBiomeSource()->scaleTemp(temp, floor) >= 0.15f)
                continue;

            random->setSeed((x * x * 3121 + x * 45238971) ^
                            (z * z * 418711 + z * 13761));

            float ra = (((_tick) & 511) + a) / 512.0f;
            float uo = random->nextFloat() +
                       time * 0.01f * (float)random->nextGaussian();
            float vo = random->nextFloat() +
                       time * (float)random->nextGaussian() * 0.001f;

            double xd = (x + 0.5f) - player->x;
            double zd = (z + 0.5f) - player->z;
            float dd = (float)sqrt(xd * xd + zd * zd) / r;

            float br = 1.0f;
            float s = 1.0f;
            t->offset(-xo, -yo, -zo);
#ifdef __PSVITA__
            float Alpha = ((1 - dd * dd) * 0.3f + 0.5f) * rainLevel;
            int tex2 = (level->getLightColor(x, yl, z, 0) * 3 + 0xf000f0) / 4;
            t->tileRainQuad(
                x - xa + 0.5, yy0, z - za + 0.5, 0 * s + uo,
                yy0 * s / 4.0f + ra * s + vo, x + xa + 0.5, yy0, z + za + 0.5,
                1 * s + uo, yy0 * s / 4.0f + ra * s + vo, x + xa + 0.5, yy1,
                z + za + 0.5, 1 * s + uo, yy1 * s / 4.0f + ra * s + vo,
                x - xa + 0.5, yy1, z - za + 0.5, 0 * s + uo,
                yy1 * s / 4.0f + ra * s + vo, br, br, br, Alpha, br, br, br,
                Alpha, tex2);
#else
            t->tex2((level->getLightColor(x, yl, z, 0) * 3 + 0xf000f0) / 4);
            t->color(br, br, br, ((1 - dd * dd) * 0.3f + 0.5f) * rainLevel);
            t->vertexUV(x - xa + 0.5, yy0, z - za + 0.5, 0 * s + uo,
                        yy0 * s / 4.0f + ra * s + vo);
            t->vertexUV(x + xa + 0.5, yy0, z + za + 0.5, 1 * s + uo,
                        yy0 * s / 4.0f + ra * s + vo);
            t->vertexUV(x + xa + 0.5, yy1, z + za + 0.5, 1 * s + uo,
                        yy1 * s / 4.0f + ra * s + vo);
            t->vertexUV(x - xa + 0.5, yy1, z - za + 0.5, 0 * s + uo,
                        yy1 * s / 4.0f + ra * s + vo);
#endif
            t->offset(0, 0, 0);
        }
    }
    t->end();  // single submit for all snow geometry

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.1f);
    turnOffLightLayer(a);

    PlatformRenderer.StateSetEnableViewportClipPlanes(false);
}

// 4J - added forceScale parameter
void GameRenderer::setupGuiScreen(int forceScale /*=-1*/) {
    int fbw, fbh;
    PlatformRenderer.GetFramebufferSize(fbw, fbh);

    // 4jcraft: use actual framebuffer dimensions instead of mc->width/height
    // to ensure GUI scales correctly after a window resize.
    ScreenSizeCalculator ssc(mc->options, fbw, fbh, forceScale);

    // 4jcraft: Java GUI screens still assume a clean 2D fixed-function style
    // state.
    PlatformRenderer.StateSetFaceCull(false);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glColor4f(1, 1, 1, 1);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glEnable(GL_DEPTH_TEST);
    PlatformRenderer.StateSetDepthFunc(GL_LEQUAL);
    PlatformRenderer.StateSetDepthMask(true);

    PlatformRenderer.TextureBindVertex(-1);

    glClientActiveTexture(GL_TEXTURE1);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glClientActiveTexture(GL_TEXTURE0);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (float)ssc.rawWidth, (float)ssc.rawHeight, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2000);
}

void GameRenderer::setupClearColor(float a) {
    Level* level = mc->level;
    std::shared_ptr<LivingEntity> player = mc->cameraTargetPlayer;

    float whiteness = 1.0f / (4 - mc->options->viewDistance);
    whiteness = 1 - (float)pow((double)whiteness, 0.25);

    Vec3 skyColor = level->getSkyColor(mc->cameraTargetPlayer, a);
    float sr = (float)skyColor.x;
    float sg = (float)skyColor.y;
    float sb = (float)skyColor.z;

    Vec3 fogColor = level->getFogColor(a);
    fr = (float)fogColor.x;
    fg = (float)fogColor.y;
    fb = (float)fogColor.z;

    if (mc->options->viewDistance < 2) {
        Vec3 sunAngle =
            sinf(level->getSunAngle(a)) > 0 ? Vec3(-1, 0, 0) : Vec3(1, 0, 0);
        float d = (float)player->getViewVector(a).dot(sunAngle);
        if (d < 0) d = 0;
        if (d > 0) {
            float* c =
                level->dimension->getSunriseColor(level->getTimeOfDay(a), a);
            if (c != nullptr) {
                d *= c[3];
                fr = fr * (1 - d) + c[0] * d;
                fg = fg * (1 - d) + c[1] * d;
                fb = fb * (1 - d) + c[2] * d;
            }
        }
    }

    fr += (sr - fr) * whiteness;
    fg += (sg - fg) * whiteness;
    fb += (sb - fb) * whiteness;

    float rainLevel = level->getRainLevel(a);
    if (rainLevel > 0) {
        float ba = 1 - rainLevel * 0.5f;
        float bb = 1 - rainLevel * 0.4f;
        fr *= ba;
        fg *= ba;
        fb *= bb;
    }
    float thunderLevel = level->getThunderLevel(a);
    if (thunderLevel > 0) {
        float ba = 1 - thunderLevel * 0.5f;
        fr *= ba;
        fg *= ba;
        fb *= ba;
    }

    int t = Camera::getBlockAt(mc->level, player, a);
    if (isInClouds) {
        Vec3 cc = level->getCloudColor(a);
        fr = (float)cc.x;
        fg = (float)cc.y;
        fb = (float)cc.z;
    } else if (t != 0 && Tile::tiles[t]->material == Material::water) {
        float clearness = EnchantmentHelper::getOxygenBonus(player) * 0.2f;

        unsigned int colour =
            Minecraft::GetInstance()->getColourTable()->getColor(
                eMinecraftColour_Under_Water_Clear_Colour);
        uint8_t redComponent = ((colour >> 16) & 0xFF);
        uint8_t greenComponent = ((colour >> 8) & 0xFF);
        uint8_t blueComponent = ((colour) & 0xFF);

        fr = (float)redComponent / 256 + clearness;    // 0.02f;
        fg = (float)greenComponent / 256 + clearness;  // 0.02f;
        fb = (float)blueComponent / 256 + clearness;   // 0.2f;
    } else if (t != 0 && Tile::tiles[t]->material == Material::lava) {
        unsigned int colour =
            Minecraft::GetInstance()->getColourTable()->getColor(
                eMinecraftColour_Under_Lava_Clear_Colour);
        uint8_t redComponent = ((colour >> 16) & 0xFF);
        uint8_t greenComponent = ((colour >> 8) & 0xFF);
        uint8_t blueComponent = ((colour) & 0xFF);

        fr = (float)redComponent / 256;    // 0.6f;
        fg = (float)greenComponent / 256;  // 0.1f;
        fb = (float)blueComponent / 256;   // 0.00f;
    }

    float brr = fogBrO + (fogBr - fogBrO) * a;
    fr *= brr;
    fg *= brr;
    fb *= brr;

    double yy =
        (player->yOld + (player->y - player->yOld) * a) *
        level->dimension->getClearColorScale();  // 4J - getClearColorScale
                                                 // brought forward from 1.2.3

    if (player->hasEffect(MobEffect::blindness)) {
        int duration = player->getEffect(MobEffect::blindness)->getDuration();
        if (duration < 20) {
            yy = yy * (1.0f - (float)duration / 20.0f);
        } else {
            yy = 0;
        }
    }

    if (yy < 1) {
        if (yy < 0) yy = 0;
        yy = yy * yy;
        fr *= yy;
        fg *= yy;
        fb *= yy;
    }

    if (darkenWorldAmount > 0) {
        float amount =
            darkenWorldAmountO + (darkenWorldAmount - darkenWorldAmountO) * a;
        fr = fr * (1.0f - amount) + (fr * .7f) * amount;
        fg = fg * (1.0f - amount) + (fg * .6f) * amount;
        fb = fb * (1.0f - amount) + (fb * .6f) * amount;
    }

    if (player->hasEffect(MobEffect::nightVision)) {
        float scale = getNightVisionScale(mc->player, a);
        {
            float dist = FLT_MAX;  // MGH - changed this to avoid divide by zero
            if ((fr > 0) && (dist > (1.0f / fr))) {
                dist = (1.0f / fr);
            }
            if ((fg > 0) && (dist > (1.0f / fg))) {
                dist = (1.0f / fg);
            }
            if ((fb > 0) && (dist > (1.0f / fb))) {
                dist = (1.0f / fb);
            }
            fr = fr * (1.0f - scale) + (fr * dist) * scale;
            fg = fg * (1.0f - scale) + (fg * dist) * scale;
            fb = fb * (1.0f - scale) + (fb * dist) * scale;
        }
    }

    if (mc->options->anaglyph3d) {
        float frr = (fr * 30 + fg * 59 + fb * 11) / 100;
        float fgg = (fr * 30 + fg * 70) / (100);
        float fbb = (fr * 30 + fb * 70) / (100);

        fr = frr;
        fg = fgg;
        fb = fbb;
    }

    glClearColor(fr, fg, fb, 0.0f);
}

void GameRenderer::setupFog(int i, float alpha) {
    std::shared_ptr<LivingEntity> player = mc->cameraTargetPlayer;

    // 4J - check for creative mode brought forward from 1.2.3
    bool creative = false;
    if (player->instanceof (eTYPE_PLAYER)) {
        creative =
            (std::dynamic_pointer_cast<Player>(player))->abilities.instabuild;
    }

    if (i == 999) {
        assert(0);
        // 4J TODO
        /*
        glFog(GL_FOG_COLOR, getBuffer(0, 0, 0, 1));
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 0);
        glFogf(GL_FOG_END, 8);

        if (GLContext.getCapabilities().GL_NV_fog_distance) {
        glFogi(NVFogDistance.GL_FOG_DISTANCE_MODE_NV,
        NVFogDistance.GL_EYE_RADIAL_NV);
        }

        glFogf(GL_FOG_START, 0);
        */
        return;
    }

    glFog(GL_FOG_COLOR, getBuffer(fr, fg, fb, 1));
    glNormal3f(0, -1, 0);
    glColor4f(1, 1, 1, 1);

    int t = Camera::getBlockAt(mc->level, player, alpha);

    if (player->hasEffect(MobEffect::blindness)) {
        float distance = 5.0f;
        int duration = player->getEffect(MobEffect::blindness)->getDuration();
        if (duration < 20) {
            distance = 5.0f + (renderDistance - 5.0f) *
                                  (1.0f - (float)duration / 20.0f);
        }

        glFogi(GL_FOG_MODE, GL_LINEAR);
        if (i < 0) {
            glFogf(GL_FOG_START, 0);
            glFogf(GL_FOG_END, distance * 0.8f);
        } else {
            glFogf(GL_FOG_START, distance * 0.25f);
            glFogf(GL_FOG_END, distance);
        }
        // 4J - TODO investigate implementing this
        //        if (GLContext.getCapabilities().GL_NV_fog_distance)
        //		{
        //            glFogi(NVFogDistance.GL_FOG_DISTANCE_MODE_NV,
        //            NVFogDistance.GL_EYE_RADIAL_NV);
        //        }
    } else if (isInClouds) {
        glFogi(GL_FOG_MODE, GL_EXP);
        glFogf(GL_FOG_DENSITY, 0.1f);  // was 0.06
    } else if (t > 0 && Tile::tiles[t]->material == Material::water) {
        glFogi(GL_FOG_MODE, GL_EXP);
        if (player->hasEffect(MobEffect::waterBreathing)) {
            glFogf(GL_FOG_DENSITY, 0.05f);  // was 0.06
        } else {
            glFogf(GL_FOG_DENSITY,
                   0.1f - (EnchantmentHelper::getOxygenBonus(player) *
                           0.03f));  // was 0.06
        }
    } else if (t > 0 && Tile::tiles[t]->material == Material::lava) {
        glFogi(GL_FOG_MODE, GL_EXP);
        glFogf(GL_FOG_DENSITY, 2.0f);  // was 0.06
    } else {
        float distance = renderDistance;
        if (!mc->level->dimension->hasCeiling) {
            // 4J - test for doing bedrockfog brought forward from 1.2.3
            if (mc->level->dimension->hasBedrockFog() && !creative) {
                double yy =
                    ((player->getLightColor(alpha) & 0xf00000) >> 20) / 16.0 +
                    (player->yOld + (player->y - player->yOld) * alpha + 4) /
                        32;
                if (yy < 1) {
                    if (yy < 0) yy = 0;
                    yy = yy * yy;
                    float dist = 100 * (float)yy;
                    if (dist < 5) dist = 5;
                    if (distance > dist) distance = dist;
                }
            }
        }

        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, distance * 0.25f);
        glFogf(GL_FOG_END, distance);
        if (i < 0) {
            glFogf(GL_FOG_START, 0);
            glFogf(GL_FOG_END, distance * 0.8f);
        } else {
            glFogf(GL_FOG_START, distance * 0.25f);
            glFogf(GL_FOG_END, distance);
        }
        /* 4J - removed - TODO investigate
        if (GLContext.getCapabilities().GL_NV_fog_distance)
        {
        glFogi(NVFogDistance.GL_FOG_DISTANCE_MODE_NV,
        NVFogDistance.GL_EYE_RADIAL_NV);
        }
        */

        if (mc->level->dimension->isFoggyAt((int)player->x, (int)player->z)) {
            glFogf(GL_FOG_START, distance * 0.05f);
            glFogf(GL_FOG_END, std::min(distance, 16 * 16 * .75f) * .5f);
        }
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT);
}

FloatBuffer* GameRenderer::getBuffer(float a, float b, float c, float d) {
    lb->clear();
    lb->put(a)->put(b)->put(c)->put(d);
    lb->flip();
    return lb;
}

int GameRenderer::getFpsCap(int option) {
    int maxFps = 200;
    if (option == 1) maxFps = 120;
    if (option == 2) maxFps = 35;
#ifndef ENABLE_VSYNC
    if (option == 3) maxFps = std::numeric_limits<int>::max();
#endif
    return maxFps;
}

void GameRenderer::updateAllChunks() {
    //	mc->levelRenderer->updateDirtyChunks(mc->cameraTargetPlayer, true);
}
