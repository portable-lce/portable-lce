#include "ItemInHandRenderer.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "Tesselator.h"
#include "Textures.h"
#include "TileRenderer.h"
#include "java/System.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Lighting.h"
#include "minecraft/client/MemoryTracker.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Minimap.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/player/LocalPlayer.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/renderer/entity/PlayerRenderer.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/BowItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/MapItem.h"
#include "minecraft/world/item/UseAnim.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/FireTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "platform/renderer/IRenderPath.h"


class EntityRenderer;
class MapItemSavedData;

ResourceLocation ItemInHandRenderer::ENCHANT_GLINT_LOCATION =
    ResourceLocation(TN__BLUR__MISC_GLINT);
ResourceLocation ItemInHandRenderer::MAP_BACKGROUND_LOCATION =
    ResourceLocation(TN_MISC_MAPBG);
ResourceLocation ItemInHandRenderer::UNDERWATER_LOCATION =
    ResourceLocation(TN_MISC_WATER);

int ItemInHandRenderer::listItem = -1;
int ItemInHandRenderer::listTerrain = -1;
int ItemInHandRenderer::listGlint = -1;

ItemInHandRenderer::ItemInHandRenderer(Minecraft* minecraft,
                                       bool optimisedMinimap) {
    // 4J - added
    height = 0;
    oHeight = 0;
    selectedItem = nullptr;
    tileRenderer = new TileRenderer();
    lastSlot = -1;

    this->minecraft = minecraft;
    minimap = new Minimap(minecraft->font, minecraft->options,
                          minecraft->textures, optimisedMinimap);

    // 4J - replaced mesh that is used to render held items with individual
    // cubes, so we can make it all join up properly without seams. This has a
    // lot more quads in it than the original, so is now precompiled with a UV
    // matrix offset to put it in the final place for the current icon. Compile
    // it on demand for the first ItemInHandRenderer (list is static)
    if (listItem == -1) {
        listItem = MemoryTracker::genLists(1);
        float dd = 1 / 16.0f;

        RenderPath.CBuffStart(listItem);
        Tesselator* t = Tesselator::getInstance();
        t->begin();
        for (int yp = 0; yp < 16; yp++)
            for (int xp = 0; xp < 16; xp++) {
                float u = (15 - xp) / 256.0f;
                float v = (15 - yp) / 256.0f;
                u += 0.5f / 256.0f;
                v += 0.5f / 256.0f;
                float x0 = xp / 16.0f;
                float x1 = x0 + 1.0f / 16.0f;
                float y0 = yp / 16.0f;
                float y1 = y0 + 1.0f / 16.0f;
                float z0 = 0.0f;
                float z1 = -dd;

                t->normal(0, 0, 1);
                t->vertexUV(x0, y0, z0, u, v);
                t->vertexUV(x1, y0, z0, u, v);
                t->vertexUV(x1, y1, z0, u, v);
                t->vertexUV(x0, y1, z0, u, v);
                t->normal(0, 0, -1);
                t->vertexUV(x0, y1, z1, u, v);
                t->vertexUV(x1, y1, z1, u, v);
                t->vertexUV(x1, y0, z1, u, v);
                t->vertexUV(x0, y0, z1, u, v);
                t->normal(-1, 0, 0);
                t->vertexUV(x0, y0, z1, u, v);
                t->vertexUV(x0, y0, z0, u, v);
                t->vertexUV(x0, y1, z0, u, v);
                t->vertexUV(x0, y1, z1, u, v);
                t->normal(1, 0, 0);
                t->vertexUV(x1, y1, z1, u, v);
                t->vertexUV(x1, y1, z0, u, v);
                t->vertexUV(x1, y0, z0, u, v);
                t->vertexUV(x1, y0, z1, u, v);
                t->normal(0, 1, 0);
                t->vertexUV(x1, y0, z0, u, v);
                t->vertexUV(x0, y0, z0, u, v);
                t->vertexUV(x0, y0, z1, u, v);
                t->vertexUV(x1, y0, z1, u, v);
                t->normal(0, -1, 0);
                t->vertexUV(x1, y1, z1, u, v);
                t->vertexUV(x0, y1, z1, u, v);
                t->vertexUV(x0, y1, z0, u, v);
                t->vertexUV(x1, y1, z0, u, v);
            }
        t->end();
        RenderPath.CBuffEnd();
    }

    // Terrain texture is a different layout from the item texture
    if (listTerrain == -1) {
        listTerrain = MemoryTracker::genLists(1);
        float dd = 1 / 16.0f;

        RenderPath.CBuffStart(listTerrain);
        Tesselator* t = Tesselator::getInstance();
        t->begin();
        for (int yp = 0; yp < 16; yp++)
            for (int xp = 0; xp < 16; xp++) {
                float u = (15 - xp) / 256.0f;
                float v = (15 - yp) / 512.0f;
                u += 0.5f / 256.0f;
                v += 0.5f / 512.0f;
                float x0 = xp / 16.0f;
                float x1 = x0 + 1.0f / 16.0f;
                float y0 = yp / 16.0f;
                float y1 = y0 + 1.0f / 16.0f;
                float z0 = 0.0f;
                float z1 = -dd;

                t->normal(0, 0, 1);
                t->vertexUV(x0, y0, z0, u, v);
                t->vertexUV(x1, y0, z0, u, v);
                t->vertexUV(x1, y1, z0, u, v);
                t->vertexUV(x0, y1, z0, u, v);
                t->normal(0, 0, -1);
                t->vertexUV(x0, y1, z1, u, v);
                t->vertexUV(x1, y1, z1, u, v);
                t->vertexUV(x1, y0, z1, u, v);
                t->vertexUV(x0, y0, z1, u, v);
                t->normal(-1, 0, 0);
                t->vertexUV(x0, y0, z1, u, v);
                t->vertexUV(x0, y0, z0, u, v);
                t->vertexUV(x0, y1, z0, u, v);
                t->vertexUV(x0, y1, z1, u, v);
                t->normal(1, 0, 0);
                t->vertexUV(x1, y1, z1, u, v);
                t->vertexUV(x1, y1, z0, u, v);
                t->vertexUV(x1, y0, z0, u, v);
                t->vertexUV(x1, y0, z1, u, v);
                t->normal(0, 1, 0);
                t->vertexUV(x1, y0, z0, u, v);
                t->vertexUV(x0, y0, z0, u, v);
                t->vertexUV(x0, y0, z1, u, v);
                t->vertexUV(x1, y0, z1, u, v);
                t->normal(0, -1, 0);
                t->vertexUV(x1, y1, z1, u, v);
                t->vertexUV(x0, y1, z1, u, v);
                t->vertexUV(x0, y1, z0, u, v);
                t->vertexUV(x1, y1, z0, u, v);
            }
        t->end();
        RenderPath.CBuffEnd();
    }

    // Also create special object for glint overlays - this is the same as the
    // previous one, with a different UV scalings, and depth test set to equal
    if (listGlint == -1) {
        listGlint = MemoryTracker::genLists(1);
        float dd = 1 / 16.0f;

        RenderPath.CBuffStart(listGlint);
        RenderPath.StateSetDepthFunc(rp::DepthTest::equal);
        Tesselator* t = Tesselator::getInstance();
        t->begin();
        for (int yp = 0; yp < 16; yp++)
            for (int xp = 0; xp < 16; xp++) {
                float u0 = (15 - xp) / 16.0f;
                float v0 = (15 - yp) / 16.0f;
                float u1 = u0 - (1.0f / 16.0f);
                float v1 = v0 - (1.0f / 16.0f);
                ;

                float x0 = xp / 16.0f;
                float x1 = x0 + 1.0f / 16.0f;
                float y0 = yp / 16.0f;
                float y1 = y0 + 1.0f / 16.0f;
                float z0 = 0.0f;
                float z1 = -dd;

                float br = 0.76f;
                t->color(0.5f * br, 0.25f * br, 0.8f * br,
                         1.0f);  // MGH - added the color here, as the glColour
                                 // below wasn't making it through to render

                t->normal(0, 0, 1);
                t->vertexUV(x0, y0, z0, u0, v0);
                t->vertexUV(x1, y0, z0, u1, v0);
                t->vertexUV(x1, y1, z0, u1, v1);
                t->vertexUV(x0, y1, z0, u0, v1);
                t->normal(0, 0, -1);
                t->vertexUV(x0, y1, z1, u0, v1);
                t->vertexUV(x1, y1, z1, u1, v1);
                t->vertexUV(x1, y0, z1, u1, v0);
                t->vertexUV(x0, y0, z1, u0, v0);
                t->normal(-1, 0, 0);
                t->vertexUV(x0, y0, z1, u0, v0);
                t->vertexUV(x0, y0, z0, u0, v0);
                t->vertexUV(x0, y1, z0, u0, v1);
                t->vertexUV(x0, y1, z1, u0, v1);
                t->normal(1, 0, 0);
                t->vertexUV(x1, y1, z1, u1, v1);
                t->vertexUV(x1, y1, z0, u1, v1);
                t->vertexUV(x1, y0, z0, u1, v0);
                t->vertexUV(x1, y0, z1, u1, v0);
                t->normal(0, 1, 0);
                t->vertexUV(x1, y0, z0, u1, v0);
                t->vertexUV(x0, y0, z0, u0, v0);
                t->vertexUV(x0, y0, z1, u0, v0);
                t->vertexUV(x1, y0, z1, u1, v0);
                t->normal(0, -1, 0);
                t->vertexUV(x1, y1, z1, u1, v1);
                t->vertexUV(x0, y1, z1, u0, v1);
                t->vertexUV(x0, y1, z0, u0, v1);
                t->vertexUV(x1, y1, z0, u1, v1);
            }
        t->end();
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
        RenderPath.CBuffEnd();
    }
}

void ItemInHandRenderer::renderItem(std::shared_ptr<LivingEntity> mob,
                                    std::shared_ptr<ItemInstance> item,
                                    int layer, bool setColor /* = true*/) {
    // 4J - code borrowed from render method below, although not factoring in
    // brightness as that should already be being taken into account by texture
    // lighting. This is for colourising things held in 3rd person view.
    if ((setColor) && (item != nullptr)) {
        int col = Item::items[item->id]->getColor(item, 0);
        float red = ((col >> 16) & 0xff) / 255.0f;
        float g = ((col >> 8) & 0xff) / 255.0f;
        float b = ((col) & 0xff) / 255.0f;

        RenderPath.StateSetColour(red, g, b, 1);
    }

    RenderPath.MatrixPush();
    Tile* tile = Tile::tiles[item->id];
    if (item->getIconType() == Icon::TYPE_TERRAIN && tile != nullptr &&
        TileRenderer::canRender(tile->getRenderShape())) {
        minecraft->textures->bindTexture(
            minecraft->textures->getTextureLocation(Icon::TYPE_TERRAIN));
        tileRenderer->renderTile(
            Tile::tiles[item->id], item->getAuxValue(),
            SharedConstants::TEXTURE_LIGHTING
                ? 1.0f
                : mob->getBrightness(
                      1));  // 4J - change brought forward from 1.8.2
    } else {
        Icon* icon = mob->getItemInHandIcon(item, layer);
        if (icon == nullptr) {
            RenderPath.MatrixPop();
            return;
        }

        bool bIsTerrain = item->getIconType() == Icon::TYPE_TERRAIN;
        minecraft->textures->bindTexture(
            minecraft->textures->getTextureLocation(item->getIconType()));

        Tesselator* t = Tesselator::getInstance();

        // Consider forcing the mipmap LOD level to use, if this is to be
        // rendered from a larger than standard source texture.
        int iconWidth = icon->getWidth();
        int LOD = -1;  // Default to not doing anything special with LOD forcing
        if (iconWidth == 32) {
            LOD = 1;  // Force LOD level 1 to achieve texture reads from 256x256
                      // map
        } else if (iconWidth == 64) {
            LOD = 2;  // Force LOD level 2 to achieve texture reads from 256x256
                      // map
        }
        RenderPath.StateSetForceLOD(LOD);

        // 4J Original comment
        // Yes, these are backwards.
        // No, I don't know why.
        // 4J Stu - Make them the right way round...u coords were swapped
        float u0 = icon->getU0();
        float u1 = icon->getU1();
        float v0 = icon->getV0();
        float v1 = icon->getV1();

        float xo = 0.0f;
        float yo = 0.3f;

        (void)0;
        RenderPath.MatrixTranslate(-xo, -yo, 0);
        float s = 1.5f;
        RenderPath.MatrixScale(s, s, s);

        RenderPath.MatrixRotate((50)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        RenderPath.MatrixRotate((45 + 290)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        RenderPath.MatrixTranslate(-15 / 16.0f, -1 / 16.0f, 0);
        float dd = 1 / 16.0f;

        renderItem3D(t, u0, v0, u1, v1, icon->getSourceWidth(),
                     icon->getSourceHeight(), 1 / 16.0f, false, bIsTerrain);

        if (item != nullptr && item->isFoil() && layer == 0) {
            RenderPath.StateSetDepthFunc(rp::DepthTest::equal);
            RenderPath.StateSetLightingEnable(false);
            minecraft->textures->bindTexture(&ENCHANT_GLINT_LOCATION);
            RenderPath.StateSetBlendEnable(true);
            RenderPath.StateSetBlendFunc(rp::BlendFactor::src_color, rp::BlendFactor::one);
            float br = 0.76f;
            RenderPath.StateSetColour(0.5f * br, 0.25f * br, 0.8f * br,
                      1);  // MGH - for some reason this colour isn't making it
                           // through to the render, so I've added to the
                           // tesselator for the glint geom above
            RenderPath.MatrixMode(rp::MatrixStack::texture);
            RenderPath.MatrixPush();
            float ss = 1 / 8.0f;
            RenderPath.MatrixScale(ss, ss, ss);
            float sx = Minecraft::currentTimeMillis() % (3000) / (3000.0f) * 8;
            RenderPath.MatrixTranslate(sx, 0, 0);
            RenderPath.MatrixRotate((-50)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

            renderItem3D(t, 0, 0, 1, 1, 256, 256, 1 / 16.0f, true, bIsTerrain);
            RenderPath.MatrixPop();
            RenderPath.MatrixPush();
            RenderPath.MatrixScale(ss, ss, ss);
            sx = System::currentTimeMillis() % (3000 + 1873) /
                 (3000 + 1873.0f) * 8;
            RenderPath.MatrixTranslate(-sx, 0, 0);
            RenderPath.MatrixRotate((10)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            renderItem3D(t, 0, 0, 1, 1, 256, 256, 1 / 16.0f, true, bIsTerrain);
            RenderPath.MatrixPop();
            RenderPath.MatrixMode(rp::MatrixStack::modelview);
            RenderPath.StateSetBlendEnable(false);
            RenderPath.StateSetLightingEnable(true);
            RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
        }

        RenderPath.StateSetForceLOD(-1);

        (void)0;
    }
    RenderPath.MatrixPop();
}

// 4J added useList parameter
void ItemInHandRenderer::renderItem3D(Tesselator* t, float u0, float v0,
                                      float u1, float v1, int width, int height,
                                      float depth, bool isGlint,
                                      bool isTerrain) {
    float r = 1.0f;

    // 4J - replaced mesh that is used to render held items with individual
    // cubes, so we can make it all join up properly without seams. This has a
    // lot more quads in it than the original, so is now precompiled with a UV
    // matrix offset to put it in the final place for the current icon

    if (isGlint) {
        ((void)RenderPath.CBuffCall(listGlint));
    } else {
        // 4J - replaced mesh that is used to render held items with individual
        // cubes, so we can make it all join up properly without seams. This has
        // a lot more quads in it than the original, so is now precompiled with
        // a UV matrix offset to put it in the final place for the current icon

        RenderPath.MatrixMode(rp::MatrixStack::texture);
        RenderPath.MatrixSetIdentity();
        RenderPath.MatrixTranslate(u0, v0, 0);
        ((void)RenderPath.CBuffCall(isTerrain ? listTerrain : listItem));
        RenderPath.MatrixSetIdentity();
        RenderPath.MatrixMode(rp::MatrixStack::modelview);
    }
    // 4J added since we are setting the colour to other values at the start of
    // the function now
    RenderPath.StateSetColour(1.0f, 1.0f, 1.0f, 1.0f);
}

void ItemInHandRenderer::render(float a) {
    float h = oHeight + (height - oHeight) * a;
    std::shared_ptr<Player> player = minecraft->player;

    // 4J - added so we can adjust the position of the hands for horizontal &
    // vertical split screens
    float fudgeX = 0.0f;
    float fudgeY = 0.0f;
    float fudgeZ = 0.0f;
    bool splitHoriz = false;
    std::shared_ptr<LocalPlayer> localPlayer =
        std::dynamic_pointer_cast<LocalPlayer>(player);
    if (localPlayer) {
        if (localPlayer->m_iScreenSection ==
                2 ||
            localPlayer->m_iScreenSection ==
                1) {
            fudgeY = 0.08f;
            splitHoriz = true;
        } else if (localPlayer->m_iScreenSection ==
                       3 ||
                   localPlayer->m_iScreenSection ==
                       4) {
            fudgeX = -0.18f;
        }
    }

    float xr = player->xRotO + (player->xRot - player->xRotO) * a;

    RenderPath.MatrixPush();
    RenderPath.MatrixRotate((xr)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
    RenderPath.MatrixRotate((player->yRotO + (player->yRot - player->yRotO) * a)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    Lighting::turnOn();
    RenderPath.MatrixPop();

    if (localPlayer) {
        float xrr =
            localPlayer->xBobO + (localPlayer->xBob - localPlayer->xBobO) * a;
        float yrr =
            localPlayer->yBobO + (localPlayer->yBob - localPlayer->yBobO) * a;
        // 4J - was using player->xRot and yRot directly here rather than
        // interpolating between old & current with a
        float yr = player->yRotO + (player->yRot - player->yRotO) * a;
        RenderPath.MatrixRotate(((xr - xrr) * 0.1f)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
        RenderPath.MatrixRotate(((yr - yrr) * 0.1f)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    }

    std::shared_ptr<ItemInstance> item = selectedItem;

    float br = minecraft->level->getBrightness(
        std::floor(player->x), std::floor(player->y), std::floor(player->z));
    // 4J - change brought forward from 1.8.2
    if (SharedConstants::TEXTURE_LIGHTING) {
        br = 1;
        int col = minecraft->level->getLightColor(std::floor(player->x),
                                                  std::floor(player->y),
                                                  std::floor(player->z), 0);
        int u = col % 65536;
        int v = col / 65536;

        // 4jcraft
        static int lightmapLogCount = 0;
        if (lightmapLogCount < 8) {
            ++lightmapLogCount;
            Log::info("[4jcraft-lightmap] item-hand raw=0x%08x uv=(%d,%d)\n",
                      col, u, v);
        }

        RenderPath.StateSetVertexTextureUV(u / 1.0f, v / 1.0f);
        RenderPath.StateSetColour(1, 1, 1, 1);
    }
    if (item != nullptr) {
        int col = Item::items[item->id]->getColor(item, 0);
        float red = ((col >> 16) & 0xff) / 255.0f;
        float g = ((col >> 8) & 0xff) / 255.0f;
        float b = ((col) & 0xff) / 255.0f;

        RenderPath.StateSetColour(br * red, br * g, br * b, 1);
    } else {
        RenderPath.StateSetColour(br, br, br, 1);
    }

    if (item != nullptr && item->id == Item::map->id) {
        RenderPath.MatrixPush();
        float d = 0.8f;

        // 4J - move the map away a bit if we're in horizontal split screen, so
        // it doesn't clip out of the save zone
        if (splitHoriz) {
            RenderPath.MatrixTranslate(0.0f, 0.0f, -0.3f);
        }

        {
            float swing = player->getAttackAnim(a);

            float swing1 = sinf(swing * std::numbers::pi);
            float swing2 = sinf((sqrt(swing)) * std::numbers::pi);
            RenderPath.MatrixTranslate(-swing2 * 0.4f,
                         sinf(sqrt(swing) * std::numbers::pi * 2) * 0.2f,
                         -swing1 * 0.2f);
        }

        float tilt = 1 - xr / 45.0f + 0.1f;
        if (tilt < 0) tilt = 0;
        if (tilt > 1) tilt = 1;
        tilt = -cosf(tilt * std::numbers::pi) * 0.5f + 0.5f;

        RenderPath.MatrixTranslate(0.0f, 0.0f * d - (1 - h) * 1.2f - tilt * 0.5f + 0.04f,
                     -0.9f * d);

        RenderPath.MatrixRotate((90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        RenderPath.MatrixRotate(((tilt) * -85)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        (void)0;

        {
            // 4J-PB - if we've got a player texture, use that
            // RenderPath.TextureBind(            // minecraft->textures->loadHttpTexture(minecraft->player->customTextureUrl,
            // minecraft->player->getTexture()));
            RenderPath.TextureBind(                          minecraft->textures->loadMemTexture(
                              minecraft->player->customTextureUrl,
                              minecraft->player->getTexture()));
            minecraft->textures->clearLastBoundId();
            for (int i = 0; i < 2; i++) {
                int flip = i * 2 - 1;
                RenderPath.MatrixPush();

                RenderPath.MatrixTranslate(-0.0f, -0.6f, 1.1f * flip);
                RenderPath.MatrixRotate(((float)(-45 * flip))*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
                RenderPath.MatrixRotate((-90)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
                RenderPath.MatrixRotate((59)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
                RenderPath.MatrixRotate(((float)(-65 * flip))*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

                EntityRenderer* er =
                    EntityRenderDispatcher::instance->getRenderer(
                        minecraft->player);
                PlayerRenderer* playerRenderer = (PlayerRenderer*)er;
                float ss = 1;
                RenderPath.MatrixScale(ss, ss, ss);

                // Can't turn off the hand if the player is holding a map
                std::shared_ptr<ItemInstance> itemInstance =
                    player->inventory->getSelected();
                if ((itemInstance &&
                     (itemInstance->getItem()->id == Item::map_Id)) ||
                    gameServices().getGameSettings(localPlayer->GetXboxPad(),
                                                   eGameSetting_DisplayHand) !=
                        0) {
                    playerRenderer->renderHand();
                }
                RenderPath.MatrixPop();
            }
        }

        {
            float swing = player->getAttackAnim(a);
            float swing3 = sinf(swing * swing * std::numbers::pi);
            float swing2 = sinf(sqrt(swing) * std::numbers::pi);
            RenderPath.MatrixRotate((-swing3 * 20)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
            RenderPath.MatrixRotate((-swing2 * 20)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            RenderPath.MatrixRotate((-swing2 * 80)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
        }

        float ss = 0.38f;
        RenderPath.MatrixScale(ss, ss, ss);

        RenderPath.MatrixRotate((90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

        RenderPath.MatrixTranslate(-1, -1, +0);

        float s = 2 / 128.0f;
        RenderPath.MatrixScale(s, s, s);

        minecraft->textures->bindTexture(
            &MAP_BACKGROUND_LOCATION);  // 4J was "/misc/mapbg.png"
        Tesselator* t = Tesselator::getInstance();

        //        (void)0;	// 4J - changed to use tesselator
        t->begin();
        int vo = 7;
        t->normal(0, 0, -1);
        t->vertexUV((float)(0 - vo), (float)(128 + vo), (float)(0), (float)(0),
                    (float)(1));
        t->vertexUV((float)(128 + vo), (float)(128 + vo), (float)(0),
                    (float)(1), (float)(1));
        t->vertexUV((float)(128 + vo), (float)(0 - vo), (float)(0), (float)(1),
                    (float)(0));
        t->vertexUV((float)(0 - vo), (float)(0 - vo), (float)(0), (float)(0),
                    (float)(0));
        t->end();

        std::shared_ptr<MapItemSavedData> data =
            Item::map->getSavedData(item, minecraft->level);
        if (data != nullptr)
            minimap->render(minecraft->player, minecraft->textures, data,
                            minecraft->player->entityId);

        RenderPath.MatrixPop();
    } else if (item != nullptr) {
        RenderPath.MatrixPush();
        float d = 0.8f;

        static const float swingPowFactor =
            4.0f;  // 4J added, to slow the swing down when nearest the player
                   // for avoiding luminance flash issues
        if (player->getUseItemDuration() > 0) {
            UseAnim anim = item->getUseAnimation();
            if ((anim == UseAnim_eat) || (anim == UseAnim_drink)) {
                float t = (player->getUseItemDuration() - a + 1);
                float swing = 1 - (t / item->getUseDuration());

                float is = 1 - swing;
                is = is * is * is;
                is = is * is * is;
                is = is * is * is;
                float iss = 1 - is;
                RenderPath.MatrixTranslate(0,
                             std::abs(cosf(t / 4 * std::numbers::pi) * 0.1f) *
                                 (swing > 0.2 ? 1 : 0),
                             0);
                RenderPath.MatrixTranslate(iss * 0.6f, -iss * 0.5f, 0);
                RenderPath.MatrixRotate((iss * 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
                RenderPath.MatrixRotate((iss * 10)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
                RenderPath.MatrixRotate((iss * 30)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            }
        } else {
            float swing = powf(player->getAttackAnim(a), swingPowFactor);

            float swing1 = sinf(swing * std::numbers::pi);
            float swing2 = sinf((sqrt(swing)) * std::numbers::pi);
            RenderPath.MatrixTranslate(-swing2 * 0.4f,
                         sinf(sqrt(swing) * std::numbers::pi * 2) * 0.2f,
                         -swing1 * 0.2f);
        }

        RenderPath.MatrixTranslate(0.7f * d, -0.65f * d - (1 - h) * 0.6f, -0.9f * d);
        RenderPath.MatrixTranslate(fudgeX, fudgeY, fudgeZ);  // 4J added

        RenderPath.MatrixRotate((45)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        (void)0;

        float swing = powf(player->getAttackAnim(a), swingPowFactor);
        float swing3 = sinf(swing * swing * std::numbers::pi);
        float swing2 = sinf(sqrt(swing) * std::numbers::pi);
        RenderPath.MatrixRotate((-swing3 * 20)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        RenderPath.MatrixRotate((-swing2 * 20)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        RenderPath.MatrixRotate((-swing2 * 80)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

        float ss = 0.4f;
        RenderPath.MatrixScale(ss, ss, ss);

        if (player->getUseItemDuration() > 0) {
            UseAnim anim = item->getUseAnimation();
            if (anim == UseAnim_block) {
                RenderPath.MatrixTranslate(-0.5f, 0.2f, 0.0f);
                RenderPath.MatrixRotate((30)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
                RenderPath.MatrixRotate((-80)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
                RenderPath.MatrixRotate((60)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
            } else if (anim == UseAnim_bow) {
                RenderPath.MatrixRotate((-18)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
                RenderPath.MatrixRotate((-12)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
                RenderPath.MatrixRotate((-8)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
                RenderPath.MatrixTranslate(-0.9f, 0.2f, 0.0f);
                float timeHeld = (item->getUseDuration() -
                                  (player->getUseItemDuration() - a + 1));
                float pow = timeHeld / (float)(BowItem::MAX_DRAW_DURATION);
                pow = ((pow * pow) + pow * 2) / 3;
                if (pow > 1) pow = 1;
                if (pow > 0.1f) {
                    RenderPath.MatrixTranslate(
                        0,
                        sinf((timeHeld - 0.1f) * 1.3f) * 0.01f * (pow - 0.1f),
                        0);
                }
                RenderPath.MatrixTranslate(0, 0, pow * 0.1f);

                RenderPath.MatrixRotate((-45 - 290)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
                RenderPath.MatrixRotate((-50)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
                RenderPath.MatrixTranslate(0, 0.5f, 0);
                float ys = 1 + pow * 0.2f;
                RenderPath.MatrixScale(1, 1, ys);
                RenderPath.MatrixTranslate(0, -0.5f, 0);
                RenderPath.MatrixRotate((50)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
                RenderPath.MatrixRotate((45 + 290)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
            }
        }

        if (item->getItem()->isMirroredArt()) {
            RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        }

        if (item->getItem()->hasMultipleSpriteLayers()) {
            // special case for potions, refactor this when we get more
            // items that have two layers
            renderItem(player, item, 0, false);

            int col = Item::items[item->id]->getColor(item, 1);
            float red = ((col >> 16) & 0xff) / 255.0f;
            float g = ((col >> 8) & 0xff) / 255.0f;
            float b = ((col) & 0xff) / 255.0f;

            RenderPath.StateSetColour(br * red, br * g, br * b, 1);

            renderItem(player, item, 1, false);
        } else {
            renderItem(player, item, 0, false);
        }
        RenderPath.MatrixPop();
    } else if (!player->isInvisible()) {
        RenderPath.MatrixPush();
        float d = 0.8f;

        {
            float swing = player->getAttackAnim(a);

            float swing1 = sinf(swing * std::numbers::pi);
            float swing2 = sinf((sqrt(swing)) * std::numbers::pi);
            RenderPath.MatrixTranslate(-swing2 * 0.3f,
                         sinf(sqrt(swing) * std::numbers::pi * 2) * 0.4f,
                         -swing1 * 0.4f);
        }

        RenderPath.MatrixTranslate(0.8f * d, -0.75f * d - (1 - h) * 0.6f, -0.9f * d);
        RenderPath.MatrixTranslate(fudgeX, fudgeY, fudgeZ);  // 4J added

        RenderPath.MatrixRotate((45)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        (void)0;
        {
            float swing = player->getAttackAnim(a);
            float swing3 = sinf(swing * swing * std::numbers::pi);
            float swing2 = sinf(sqrt(swing) * std::numbers::pi);
            RenderPath.MatrixRotate((swing2 * 70)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
            RenderPath.MatrixRotate((-swing3 * 20)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        }

        // 4J-PB - if we've got a player texture, use that

        // RenderPath.TextureBind(        // minecraft->textures->loadHttpTexture(minecraft->player->customTextureUrl,
        // minecraft->player->getTexture()));

        RenderPath.TextureBind(minecraft->textures->loadMemTexture(
                                         minecraft->player->customTextureUrl,
                                         minecraft->player->getTexture()));
        minecraft->textures->clearLastBoundId();
        RenderPath.MatrixTranslate(-1.0f, +3.6f, +3.5f);
        RenderPath.MatrixRotate((120)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
        RenderPath.MatrixRotate((180 + 20)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
        RenderPath.MatrixRotate((-90 - 45)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        RenderPath.MatrixScale(1.5f / 24.0f * 16, 1.5f / 24.0f * 16, 1.5f / 24.0f * 16);
        RenderPath.MatrixTranslate(5.6f, 0, 0);

        EntityRenderer* er =
            EntityRenderDispatcher::instance->getRenderer(minecraft->player);
        PlayerRenderer* playerRenderer = (PlayerRenderer*)er;
        float ss = 1;
        RenderPath.MatrixScale(ss, ss, ss);
        // Can't turn off the hand if the player is holding a map
        std::shared_ptr<ItemInstance> itemInstance =
            player->inventory->getSelected();

        if ((itemInstance && (itemInstance->getItem()->id == Item::map_Id)) ||
            gameServices().getGameSettings(localPlayer->GetXboxPad(),
                                           eGameSetting_DisplayHand) != 0) {
            playerRenderer->renderHand();
        }
        RenderPath.MatrixPop();
    }

    (void)0;
    Lighting::turnOff();
}

void ItemInHandRenderer::renderScreenEffect(float a) {
    RenderPath.StateSetAlphaTestEnable(false);
    if (minecraft->player->isOnFire()) {
        renderFire(a);
    }

    if (minecraft->player->isInWall())  // Inside a tile
    {
        int x = std::floor(minecraft->player->x);
        int y = std::floor(minecraft->player->y);
        int z = std::floor(minecraft->player->z);

        int tile = minecraft->level->getTile(x, y, z);
        if (minecraft->level->isSolidBlockingTile(x, y, z)) {
            renderTex(a, Tile::tiles[tile]->getTexture(2));
        } else {
            for (int i = 0; i < 8; i++) {
                float xo =
                    ((i >> 0) % 2 - 0.5f) * minecraft->player->bbWidth * 0.9f;
                float yo =
                    ((i >> 1) % 2 - 0.5f) * minecraft->player->bbHeight * 0.2f;
                float zo =
                    ((i >> 2) % 2 - 0.5f) * minecraft->player->bbWidth * 0.9f;
                int xt = std::floor(x + xo);
                int yt = std::floor(y + yo);
                int zt = std::floor(z + zo);
                if (minecraft->level->isSolidBlockingTile(xt, yt, zt)) {
                    tile = minecraft->level->getTile(xt, yt, zt);
                }
            }
        }

        if (Tile::tiles[tile] != nullptr)
            renderTex(a, Tile::tiles[tile]->getTexture(2));
    }

    if (minecraft->player->isUnderLiquid(Material::water)) {
        minecraft->textures->bindTexture(
            &UNDERWATER_LOCATION);  // 4J was "/misc/water.png"
        renderWater(a);
    }
    RenderPath.StateSetAlphaTestEnable(true);
}

void ItemInHandRenderer::renderTex(float a, Icon* slot) {
    minecraft->textures->bindTexture(
        &TextureAtlas::LOCATION_BLOCKS);  // TODO: get this data from Icon

    Tesselator* t = Tesselator::getInstance();

    float br = 0.1f;
    br = 0.1f;
    RenderPath.StateSetColour(br, br, br, 0.5f);

    RenderPath.MatrixPush();

    float x0 = -1;
    float x1 = +1;
    float y0 = -1;
    float y1 = +1;
    float z0 = -0.5f;

    float r = 2 / 256.0f;
    float u0 = slot->getU0();
    float u1 = slot->getU1();
    float v0 = slot->getV0();
    float v1 = slot->getV1();

    t->begin();
    t->vertexUV((float)(x0), (float)(y0), (float)(z0), (float)(u1),
                (float)(v1));
    t->vertexUV((float)(x1), (float)(y0), (float)(z0), (float)(u0),
                (float)(v1));
    t->vertexUV((float)(x1), (float)(y1), (float)(z0), (float)(u0),
                (float)(v0));
    t->vertexUV((float)(x0), (float)(y1), (float)(z0), (float)(u1),
                (float)(v0));
    t->end();
    RenderPath.MatrixPop();

    RenderPath.StateSetColour(1, 1, 1, 1);
}

void ItemInHandRenderer::renderWater(float a) {
    minecraft->textures->bindTexture(&UNDERWATER_LOCATION);

    Tesselator* t = Tesselator::getInstance();

    float br = minecraft->player->getBrightness(a);
    RenderPath.StateSetColour(br, br, br, 0.5f);
    RenderPath.StateSetBlendEnable(true);
    RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);

    RenderPath.MatrixPush();

    float size = 4;

    float x0 = -1;
    float x1 = +1;
    float y0 = -1;
    float y1 = +1;
    float z0 = -0.5f;

    float uo = -minecraft->player->yRot / 64.0f;
    float vo = +minecraft->player->xRot / 64.0f;

    t->begin();
    t->vertexUV((float)(x0), (float)(y0), (float)(z0), (float)(size + uo),
                (float)(size + vo));
    t->vertexUV((float)(x1), (float)(y0), (float)(z0), (float)(0 + uo),
                (float)(size + vo));
    t->vertexUV((float)(x1), (float)(y1), (float)(z0), (float)(0 + uo),
                (float)(0 + vo));
    t->vertexUV((float)(x0), (float)(y1), (float)(z0), (float)(size + uo),
                (float)(0 + vo));
    t->end();
    RenderPath.MatrixPop();

    RenderPath.StateSetColour(1, 1, 1, 1);
    RenderPath.StateSetBlendEnable(false);
}

void ItemInHandRenderer::renderFire(float a) {
    Tesselator* t = Tesselator::getInstance();

    unsigned int col = Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_Fire_Overlay);
    float aCol = ((col >> 24) & 0xFF) / 255.0f;
    float rCol = ((col >> 16) & 0xFF) / 255.0f;
    float gCol = ((col >> 8) & 0xFF) / 255.0;
    float bCol = (col & 0xFF) / 255.0;

    RenderPath.StateSetColour(rCol, gCol, bCol, aCol);
    RenderPath.StateSetBlendEnable(true);
    RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);

    float size = 1;
    for (int i = 0; i < 2; i++) {
        RenderPath.MatrixPush();
        Icon* slot = Tile::fire->getTextureLayer(1);
        minecraft->textures->bindTexture(
            &TextureAtlas::LOCATION_BLOCKS);  // TODO: Get this from Icon

        float u0 = slot->getU0(true);
        float u1 = slot->getU1(true);
        float v0 = slot->getV0(true);
        float v1 = slot->getV1(true);

        float x0 = (0 - size) / 2;
        float x1 = x0 + size;
        float y0 = 0 - size / 2;
        float y1 = y0 + size;
        float z0 = -0.5f;
        RenderPath.MatrixTranslate(-(i * 2 - 1) * 0.24f, -0.3f, 0);
        RenderPath.MatrixRotate(((i * 2 - 1) * 10.0f)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

        t->begin();
        t->vertexUV((float)(x0), (float)(y0), (float)(z0), (float)(u1),
                    (float)(v1));
        t->vertexUV((float)(x1), (float)(y0), (float)(z0), (float)(u0),
                    (float)(v1));
        t->vertexUV((float)(x1), (float)(y1), (float)(z0), (float)(u0),
                    (float)(v0));
        t->vertexUV((float)(x0), (float)(y1), (float)(z0), (float)(u1),
                    (float)(v0));
        t->end();
        RenderPath.MatrixPop();
    }
    RenderPath.StateSetColour(1, 1, 1, 1);
    RenderPath.StateSetBlendEnable(false);
}

void ItemInHandRenderer::tick() {
    oHeight = height;

    std::shared_ptr<Player> player = minecraft->player;
    std::shared_ptr<ItemInstance> nextTile = player->inventory->getSelected();

    bool matches =
        lastSlot == player->inventory->selected && nextTile == selectedItem;
    if (selectedItem == nullptr && nextTile == nullptr) {
        matches = true;
    }
    if (nextTile != nullptr && selectedItem != nullptr &&
        nextTile != selectedItem && nextTile->id == selectedItem->id &&
        nextTile->getAuxValue() == selectedItem->getAuxValue()) {
        selectedItem = nextTile;
        matches = true;
    }

    float max = 0.4f;
    float tHeight = matches ? 1.0f : 0;
    float dd = tHeight - height;
    if (dd < -max) dd = -max;
    if (dd > max) dd = max;

    height += dd;
    if (height < 0.1f) {
        selectedItem = nextTile;
        lastSlot = player->inventory->selected;
    }
}

void ItemInHandRenderer::itemPlaced() { height = 0; }

void ItemInHandRenderer::itemUsed() { height = 0; }
