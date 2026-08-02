#include "ItemRenderer.h"

#include <math.h>

#include <numbers>
#include <vector>

#include "EntityRenderDispatcher.h"
#include "java/JavaMath.h"
#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/renderer/IRenderPath.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

class ResourceLocation;

ItemRenderer::ItemRenderer() : EntityRenderer() {
    random = new Random();
    setColor = true;
    blitOffset = 0;

    shadowRadius = 0.15f;
    shadowStrength = 0.75f;

    // 4J added
    m_bItemFrame = false;
}

ItemRenderer::~ItemRenderer() { delete random; }

ResourceLocation* ItemRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    std::shared_ptr<ItemEntity> itemEntity =
        std::dynamic_pointer_cast<ItemEntity>(entity);
    return getTextureLocation(itemEntity->getItem()->getIconType());
}

ResourceLocation* ItemRenderer::getTextureLocation(int iconType) {
    if (iconType == Icon::TYPE_TERRAIN) {
        return &TextureAtlas::LOCATION_BLOCKS;  // "/terrain.png"));
    } else {
        return &TextureAtlas::LOCATION_ITEMS;  // "/gui/items.png"));
    }
}

void ItemRenderer::render(std::shared_ptr<Entity> _itemEntity, double x,
                          double y, double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<ItemEntity> itemEntity =
        std::dynamic_pointer_cast<ItemEntity>(_itemEntity);
    bindTexture(itemEntity);

    random->setSeed(187);
    std::shared_ptr<ItemInstance> item = itemEntity->getItem();
    if (item->getItem() == nullptr) return;

    RenderPath.MatrixPush();
    float bob =
        sinf((itemEntity->age + a) / 10.0f + itemEntity->bobOffs) * 0.1f + 0.1f;
    float spin =
        ((itemEntity->age + a) / 20.0f + itemEntity->bobOffs) * Mth::RAD_TO_DEG;

    int count = 1;
    if (itemEntity->getItem()->count > 1) count = 2;
    if (itemEntity->getItem()->count > 5) count = 3;
    if (itemEntity->getItem()->count > 20) count = 4;
    if (itemEntity->getItem()->count > 40) count = 5;

    RenderPath.MatrixTranslate((float)x, (float)y + bob, (float)z);
    (void)0;

    Tile* tile = Tile::tiles[item->id];

    if (item->getIconType() == Icon::TYPE_TERRAIN && tile != nullptr &&
        TileRenderer::canRender(tile->getRenderShape())) {
        RenderPath.MatrixRotate((spin)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

        if (m_bItemFrame) {
            RenderPath.MatrixScale(1.25f, 1.25f, 1.25f);
            RenderPath.MatrixTranslate(0, 0.05f, 0);
            RenderPath.MatrixRotate((-90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        }

        float s = 1 / 4.0f;
        int shape = tile->getRenderShape();
        if (shape == Tile::SHAPE_CROSS_TEXTURE || shape == Tile::SHAPE_STEM ||
            shape == Tile::SHAPE_LEVER || shape == Tile::SHAPE_TORCH) {
            s = 0.5f;
        }

        RenderPath.MatrixScale(s, s, s);
        for (int i = 0; i < count; i++) {
            RenderPath.MatrixPush();
            if (i > 0) {
                float xo = (random->nextFloat() * 2 - 1) * 0.2f / s;
                float yo = (random->nextFloat() * 2 - 1) * 0.2f / s;
                float zo = (random->nextFloat() * 2 - 1) * 0.2f / s;
                RenderPath.MatrixTranslate(xo, yo, zo);
            }
            // 4J - change brought forward from 1.8.2
            float br = SharedConstants::TEXTURE_LIGHTING
                           ? 1.0f
                           : itemEntity->getBrightness(a);
            tileRenderer->renderTile(tile, item->getAuxValue(), br);
            RenderPath.MatrixPop();
        }
    } else if (item->getIconType() == Icon::TYPE_ITEM &&
               item->getItem()->hasMultipleSpriteLayers()) {
        if (m_bItemFrame) {
            RenderPath.MatrixScale(1 / 1.95f, 1 / 1.95f, 1 / 1.95f);
            RenderPath.MatrixTranslate(0, -0.05f, 0);
            RenderPath.StateSetLightingEnable(false);
        } else {
            RenderPath.MatrixScale(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
        }

        bindTexture(&TextureAtlas::LOCATION_ITEMS);  // 4J was "/gui/items.png"

        for (int layer = 0; layer <= 1; layer++) {
            random->setSeed(187);
            Icon* icon =
                item->getItem()->getLayerIcon(item->getAuxValue(), layer);
            float brightness = SharedConstants::TEXTURE_LIGHTING
                                   ? 1
                                   : itemEntity->getBrightness(a);
            if (setColor) {
                int col = Item::items[item->id]->getColor(item, layer);
                float red = ((col >> 16) & 0xff) / 255.0f;
                float g = ((col >> 8) & 0xff) / 255.0f;
                float b = ((col) & 0xff) / 255.0f;

                RenderPath.StateSetColour(red * brightness, g * brightness, b * brightness, 1);
                renderItemBillboard(itemEntity, icon, count, a,
                                    red * brightness, g * brightness,
                                    b * brightness);
            } else {
                renderItemBillboard(itemEntity, icon, count, a, 1, 1, 1);
            }
        }
    } else {
        if (m_bItemFrame) {
            RenderPath.MatrixScale(1 / 1.95f, 1 / 1.95f, 1 / 1.95f);
            RenderPath.MatrixTranslate(0, -0.05f, 0);
            RenderPath.StateSetLightingEnable(false);
        } else {
            RenderPath.MatrixScale(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
        }

        // 4J Stu - For rendering the static compass, we give it a non-zero aux
        // value
        if (item->id == Item::compass_Id) item->setAuxValue(255);
        if (item->id == Item::compass_Id) item->setAuxValue(0);

        Icon* icon = item->getIcon();
        if (setColor) {
            int col = Item::items[item->id]->getColor(item, 0);
            float red = ((col >> 16) & 0xff) / 255.0f;
            float g = ((col >> 8) & 0xff) / 255.0f;
            float b = ((col) & 0xff) / 255.0f;
            float brightness = SharedConstants::TEXTURE_LIGHTING
                                   ? 1
                                   : itemEntity->getBrightness(a);

            RenderPath.StateSetColour(red * brightness, g * brightness, b * brightness, 1);
            renderItemBillboard(itemEntity, icon, count, a, red * brightness,
                                g * brightness, b * brightness);
        } else {
            renderItemBillboard(itemEntity, icon, count, a, 1, 1, 1);
        }
    }
    (void)0;
    RenderPath.MatrixPop();
    if (m_bItemFrame) {
        RenderPath.StateSetLightingEnable(true);
    }
}

void ItemRenderer::renderItemBillboard(std::shared_ptr<ItemEntity> entity,
                                       Icon* icon, int count, float a,
                                       float red, float green, float blue) {
    Tesselator* t = Tesselator::getInstance();

    if (icon == nullptr)
        icon = entityRenderDispatcher->textures->getMissingIcon(
            entity->getItem()->getIconType());
    float u0 = icon->getU0();
    float u1 = icon->getU1();
    float v0 = icon->getV0();
    float v1 = icon->getV1();

    float r = 1.0f;
    float xo = 0.5f;
    float yo = 0.25f;

    if (entityRenderDispatcher->options->fancyGraphics) {
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

        RenderPath.MatrixPush();
        if (m_bItemFrame) {
            RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        } else {
            RenderPath.MatrixRotate(
                (entity->age + a) / 20.0f + entity->bobOffs,
                                    0, 1, 0);
        }

        float width = 1 / 16.0f;
        float margin = 0.35f / 16.0f;
        std::shared_ptr<ItemInstance> item = entity->getItem();
        int items = item->count;

        if (items < 2) {
            count = 1;
        } else if (items < 16) {
            count = 2;
        } else if (items < 32) {
            count = 3;
        } else {
            count = 4;
        }

        RenderPath.MatrixTranslate(-xo, -yo, -((width + margin) * count / 2));

        for (int i = 0; i < count; i++) {
            RenderPath.MatrixTranslate(0, 0, width + margin);

            bool bIsTerrain = false;
            if (item->getIconType() == Icon::TYPE_TERRAIN &&
                Tile::tiles[item->id] != nullptr) {
                bIsTerrain = true;
                bindTexture(&TextureAtlas::LOCATION_BLOCKS);  // TODO: Do this
                                                              // sanely by Icon
            } else {
                bindTexture(&TextureAtlas::LOCATION_ITEMS);  // TODO: Do this
                                                             // sanely by Icon
            }

            RenderPath.StateSetColour(red, green, blue, 1);
            // 4J Stu - u coords were swapped in Java
            // ItemInHandRenderer::renderItem3D(t, u1, v0, u0, v1,
            // icon->getSourceWidth(), icon->getSourceHeight(), width, false);
            ItemInHandRenderer::renderItem3D(
                t, u0, v0, u1, v1, icon->getSourceWidth(),
                icon->getSourceHeight(), width, false, bIsTerrain);

            if (item != nullptr && item->isFoil()) {
                RenderPath.StateSetDepthFunc(rp::DepthTest::equal);
                RenderPath.StateSetLightingEnable(false);
                entityRenderDispatcher->textures->bindTexture(
                    &ItemInHandRenderer::ENCHANT_GLINT_LOCATION);
                RenderPath.StateSetBlendEnable(true);
                RenderPath.StateSetBlendFunc(rp::BlendFactor::src_color, rp::BlendFactor::one);
                float br = 0.76f;
                RenderPath.StateSetColour(0.5f * br, 0.25f * br, 0.8f * br, 1);
                RenderPath.MatrixMode(rp::MatrixStack::texture);
                RenderPath.MatrixPush();
                float ss = 1 / 8.0f;
                RenderPath.MatrixScale(ss, ss, ss);
                float sx =
                    Minecraft::currentTimeMillis() % (3000) / (3000.0f) * 8;
                RenderPath.MatrixTranslate(sx, 0, 0);
                RenderPath.MatrixRotate((-50)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

                ItemInHandRenderer::renderItem3D(t, 0, 0, 1, 1, 255, 255, width,
                                                 true, bIsTerrain);
                RenderPath.MatrixPop();
                RenderPath.MatrixPush();
                RenderPath.MatrixScale(ss, ss, ss);
                sx = Minecraft::currentTimeMillis() % (3000 + 1873) /
                     (3000 + 1873.0f) * 8;
                RenderPath.MatrixTranslate(-sx, 0, 0);
                RenderPath.MatrixRotate((10)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);
                ItemInHandRenderer::renderItem3D(t, 0, 0, 1, 1, 255, 255, width,
                                                 true, bIsTerrain);
                RenderPath.MatrixPop();
                RenderPath.MatrixMode(rp::MatrixStack::modelview);
                RenderPath.StateSetBlendEnable(false);
                RenderPath.StateSetLightingEnable(true);
                RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);
            }
        }

        RenderPath.MatrixPop();

        RenderPath.StateSetForceLOD(-1);
    } else {
        for (int i = 0; i < count; i++) {
            RenderPath.MatrixPush();
            if (i > 0) {
                float _xo = (random->nextFloat() * 2 - 1) * 0.3f;
                float _yo = (random->nextFloat() * 2 - 1) * 0.3f;
                float _zo = (random->nextFloat() * 2 - 1) * 0.3f;
                RenderPath.MatrixTranslate(_xo, _yo, _zo);
            }
            if (!m_bItemFrame)
                RenderPath.MatrixRotate((180 - entityRenderDispatcher->playerRotY)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
            RenderPath.StateSetColour(red, green, blue, 1);
            t->begin();
            t->normal(0, 1, 0);
            t->vertexUV((float)(0 - xo), (float)(0 - yo), (float)(0),
                        (float)(u0), (float)(v1));
            t->vertexUV((float)(r - xo), (float)(0 - yo), (float)(0),
                        (float)(u1), (float)(v1));
            t->vertexUV((float)(r - xo), (float)(1 - yo), (float)(0),
                        (float)(u1), (float)(v0));
            t->vertexUV((float)(0 - xo), (float)(1 - yo), (float)(0),
                        (float)(u0), (float)(v0));
            t->end();

            RenderPath.MatrixPop();
        }
    }
}

void ItemRenderer::renderGuiItem(Font* font, Textures* textures,
                                 std::shared_ptr<ItemInstance> item, float x,
                                 float y, float fScale, float fAlpha) {
    renderGuiItem(font, textures, item, x, y, fScale, fScale, fAlpha, true);
}

// 4J - this used to take x and y as ints, and no scale and alpha - but this
// interface is now implemented as a wrapper round this more fully featured one
void ItemRenderer::renderGuiItem(Font* font, Textures* textures,
                                 std::shared_ptr<ItemInstance> item, float x,
                                 float y, float fScaleX, float fScaleY,
                                 float fAlpha, bool useCompiled) {
    int itemId = item->id;
    int itemAuxValue = item->getAuxValue();
    Icon* itemIcon = item->getIcon();

    if (item->getIconType() == Icon::TYPE_TERRAIN &&
        TileRenderer::canRender(Tile::tiles[itemId]->getRenderShape())) {
        textures->bindTexture(&TextureAtlas::LOCATION_BLOCKS);

        Tile* tile = Tile::tiles[itemId];
        RenderPath.StateSetFaceCull(false);
        RenderPath.MatrixPush();
        // 4J - original code left here for reference
        // 4jcraft: original code reused for proper lighting
        RenderPath.MatrixTranslate((float)(x), (float)(y), 0.0f);
        RenderPath.MatrixScale(fScaleX, fScaleY, 1.0f);
        RenderPath.MatrixTranslate(-2.0f, 3.0f, -3.0f + blitOffset);
        RenderPath.MatrixScale(10.0f, 10.0f, 10.0f);
        RenderPath.MatrixTranslate(1.0f, 0.5f, 8.0f);
        RenderPath.MatrixScale(1.0f, 1.0f, -1.0f);
        RenderPath.MatrixRotate((180.0f + 30.0f)*(std::numbers::pi_v<float>/180.f), 1.0f, 0.0f, 0.0f);
        RenderPath.MatrixRotate((45.0f)*(std::numbers::pi_v<float>/180.f), 0.0f, 1.0f, 0.0f);
        // 4J-PB - pass the alpha value in - the grass block
        // render has the top surface coloured differently to
        // the rest of the block
        RenderPath.MatrixRotate((-90.0f)*(std::numbers::pi_v<float>/180.f), 0.0f, 1.0f, 0.0f);

        tileRenderer->renderTile(tile, itemAuxValue, 1, fAlpha, useCompiled);

        RenderPath.MatrixPop();

    } else if (Item::items[itemId]->hasMultipleSpriteLayers()) {
        // special double-layered
        RenderPath.StateSetLightingEnable(false);

        ResourceLocation* location = getTextureLocation(item->getIconType());
        textures->bindTexture(location);

        for (int layer = 0; layer <= 1; layer++) {
            Icon* fillingIcon =
                Item::items[itemId]->getLayerIcon(itemAuxValue, layer);

            int col = Item::items[itemId]->getColor(item, layer);
            float r = ((col >> 16) & 0xff) / 255.0f;
            float g = ((col >> 8) & 0xff) / 255.0f;
            float b = ((col) & 0xff) / 255.0f;

            if (setColor) RenderPath.StateSetColour(r, g, b, fAlpha);
            // scale the x and y by the scale factor
            if ((fScaleX != 1.0f) || (fScaleY != 1.0f)) {
                blit(x, y, fillingIcon, 16 * fScaleX, 16 * fScaleY);
            } else {
                blit((int)x, (int)y, fillingIcon, 16, 16);
            }
        }
        RenderPath.StateSetLightingEnable(true);

    } else {
        RenderPath.StateSetLightingEnable(false);
        if (item->getIconType() == Icon::TYPE_TERRAIN) {
            textures->bindTexture(
                &TextureAtlas::LOCATION_BLOCKS);  // "/terrain.png"));
        } else {
            textures->bindTexture(
                &TextureAtlas::LOCATION_ITEMS);  // "/gui/items.png"));
        }

        if (itemIcon == nullptr) {
            itemIcon = textures->getMissingIcon(item->getIconType());
        }

        int col = Item::items[itemId]->getColor(item, 0);
        float r = ((col >> 16) & 0xff) / 255.0f;
        float g = ((col >> 8) & 0xff) / 255.0f;
        float b = ((col) & 0xff) / 255.0f;

        if (setColor) RenderPath.StateSetColour(r, g, b, fAlpha);

        // scale the x and y by the scale factor
        if ((fScaleX != 1.0f) || (fScaleY != 1.0f)) {
            blit(x, y, itemIcon, 16 * fScaleX, 16 * fScaleY);
        } else {
            blit((int)x, (int)y, itemIcon, 16, 16);
        }
        RenderPath.StateSetLightingEnable(true);
    }
    RenderPath.StateSetFaceCull(true);
}

// 4J - original interface, now just a wrapper for preceding overload
void ItemRenderer::renderGuiItem(Font* font, Textures* textures,
                                 std::shared_ptr<ItemInstance> item, int x,
                                 int y) {
    renderGuiItem(font, textures, item, (float)x, (float)y, 1.0f, 1.0f);
}

// 4J - this used to take x and y as ints, and no scale, alpha or foil - but
// this interface is now implemented as a wrapper round this more fully featured
// one
void ItemRenderer::renderAndDecorateItem(
    Font* font, Textures* textures, const std::shared_ptr<ItemInstance> item,
    float x, float y, float fScale, float fAlpha, bool isFoil) {
    if (item == nullptr) return;
    renderAndDecorateItem(font, textures, item, x, y, fScale, fScale, fAlpha,
                          isFoil, true);
}

// 4J - added isConstantBlended and blendFactor parameters. This is true if the
// gui item is being rendered from a context where it already has blending
// enabled to do general interface fading (ie from the gui rather than xui). In
// this case we dno't want to enable/disable blending, and do need to restore
// the blend state when we are done.
void ItemRenderer::renderAndDecorateItem(
    Font* font, Textures* textures, const std::shared_ptr<ItemInstance> item,
    float x, float y, float fScaleX, float fScaleY, float fAlpha, bool isFoil,
    bool isConstantBlended, bool useCompiled) {
    if (item == nullptr) {
        return;
    }

    renderGuiItem(font, textures, item, x, y, fScaleX, fScaleY, fAlpha,
                  useCompiled);

    if (isFoil || item->isFoil()) {
        RenderPath.StateSetDepthFunc(rp::DepthTest::greater);
        RenderPath.StateSetLightingEnable(false);
        RenderPath.StateSetDepthMask(false);
        textures->bindTexture(
            &ItemInHandRenderer::
                ENCHANT_GLINT_LOCATION);  // 4J was "%blur%/misc/glint.png"
        blitOffset -= 50;
        if (!isConstantBlended) RenderPath.StateSetBlendEnable(true);

        RenderPath.StateSetBlendFunc(rp::BlendFactor::dst_color,
                    rp::BlendFactor::one);  // 4J - changed blend equation from rp::BlendFactor::dst_color,
                              // rp::BlendFactor::dst_color so we can fade this out

        float blendFactor =
            isConstantBlended ? Gui::currentGuiBlendFactor : 1.0f;

        RenderPath.StateSetColour(0.5f * blendFactor, 0.25f * blendFactor, 0.8f * blendFactor,
            1);  // 4J - scale back colourisation with blendFactor
        // scale the x and y by the scale factor
        if ((fScaleX != 1.0f) || (fScaleY != 1.0f)) {
            // 4J Stu - Scales were multiples of 20, making 16 to not overlap in
            // xui scenes
            blitGlint(x * 431278612.0f + y * 32178161.0f, x - 2, y - 2,
                      16 * fScaleX, 16 * fScaleY);
        } else {
            blitGlint(x * 431278612.0f + y * 32178161.0f, x - 2, y - 2, 20, 20);
        }
        RenderPath.StateSetColour(1.0f, 1.0f, 1.0f, 1);  // 4J added
        if (!isConstantBlended) RenderPath.StateSetBlendEnable(false);

        RenderPath.StateSetDepthMask(true);
        blitOffset += 50;
        RenderPath.StateSetLightingEnable(true);
        RenderPath.StateSetDepthFunc(rp::DepthTest::less_equal);

        if (isConstantBlended)
            RenderPath.StateSetBlendFunc(rp::BlendFactor::constant_alpha, rp::BlendFactor::one_minus_constant_alpha);
    }
}

// 4J - original interface, now just a wrapper for preceding overload
void ItemRenderer::renderAndDecorateItem(
    Font* font, Textures* textures, const std::shared_ptr<ItemInstance> item,
    int x, int y) {
    renderAndDecorateItem(font, textures, item, (float)x, (float)y, 1.0f, 1.0f,
                          item->isFoil());
}

// 4J - a few changes here to get x, y, w, h in as floats (for xui rendering
// accuracy), and to align final pixels to the final screen resolution
void ItemRenderer::blitGlint(int id, float x, float y, float w, float h) {
    float us = 1.0f / 64.0f / 4;
    float vs = 1.0f / 64.0f / 4;

    // 4J - calculate what the pixel coordinates will be in final screen
    // coordinates
    float sfx = (float)Minecraft::GetInstance()->width /
                (float)Minecraft::GetInstance()->width_phys;
    float sfy = (float)Minecraft::GetInstance()->height /
                (float)Minecraft::GetInstance()->height_phys;
    float xx0 = x * sfx;
    float xx1 = (x + w) * sfx;
    float yy0 = y * sfy;
    float yy1 = (y + h) * sfy;
    // Round to whole pixels - rounding inwards so that we don't overlap any
    // surrounding graphics
    xx0 = ceilf(xx0);
    xx1 = floorf(xx1);
    yy0 = ceilf(yy0);
    yy1 = floorf(yy1);
    // Offset by half to get actual centre of pixel - again moving inwards to
    // avoid overlap with surrounding graphics
    xx0 += 0.5f;
    xx1 -= 0.5f;
    yy0 += 0.5f;
    yy1 -= 0.5f;
    // Convert back to game coordinate space
    float xx0f = xx0 / sfx;
    float xx1f = xx1 / sfx;
    float yy0f = yy0 / sfy;
    float yy1f = yy1 / sfy;

    for (int i = 0; i < 2; i++) {
        if (i == 0) RenderPath.StateSetBlendFunc(rp::BlendFactor::src_color, rp::BlendFactor::one);
        if (i == 1) RenderPath.StateSetBlendFunc(rp::BlendFactor::src_color, rp::BlendFactor::one);
        float sx = Minecraft::currentTimeMillis() % (3000 + i * 1873) /
                   (3000.0f + i * 1873) * 256;
        float sy = 0;
        Tesselator* t = Tesselator::getInstance();
        float vv = 4;
        if (i == 1) vv = -1;
        t->begin();
        t->vertexUV(xx0f, yy1f, blitOffset, (sx + h * vv) * us, (sy + h) * vs);
        t->vertexUV(xx1f, yy1f, blitOffset, (sx + w + h * vv) * us,
                    (sy + h) * vs);
        t->vertexUV(xx1f, yy0f, blitOffset, (sx + w) * us, (sy + 0) * vs);
        t->vertexUV(xx0f, yy0f, blitOffset, (sx + 0) * us, (sy + 0) * vs);
        t->end();
    }
}

void ItemRenderer::renderGuiItemDecorations(Font* font, Textures* textures,
                                            std::shared_ptr<ItemInstance> item,
                                            int x, int y, float fAlpha) {
    renderGuiItemDecorations(font, textures, item, x, y, "", fAlpha);
}

void ItemRenderer::renderGuiItemDecorations(Font* font, Textures* textures,
                                            std::shared_ptr<ItemInstance> item,
                                            int x, int y,
                                            const std::string& countText,
                                            float fAlpha) {
    if (item == nullptr) {
        return;
    }

    if (item->count > 1 || !countText.empty() ||
        item->GetForceNumberDisplay()) {
        std::string amount = countText;
        if (amount.empty()) {
            int count = item->count;
            if (count > 64) {
                amount = toWString<int>(64) + "+";
            } else {
                amount = toWString<int>(item->count);
            }
        }
        RenderPath.StateSetLightingEnable(false);
        RenderPath.StateSetDepthTestEnable(false);
        font->drawShadow(amount, x + 19 - 2 - font->width(amount), y + 6 + 3,
                         0xffffff | (((unsigned int)(fAlpha * 0xff)) << 24));
        RenderPath.StateSetLightingEnable(true);
        RenderPath.StateSetDepthTestEnable(true);
    }

    if (item->isDamaged()) {
        int p = (int)Math::round(13.0 - (double)item->getDamageValue() * 13.0 /
                                            (double)item->getMaxDamage());
        int cc =
            (int)Math::round(255.0 - (double)item->getDamageValue() * 255.0 /
                                         (double)item->getMaxDamage());
        RenderPath.StateSetLightingEnable(false);
        RenderPath.StateSetDepthTestEnable(false);
        RenderPath.StateSetTextureEnable(false);

        Tesselator* t = Tesselator::getInstance();

        int ca = (255 - cc) << 16 | (cc) << 8;
        int cb = ((255 - cc) / 4) << 16 | (255 / 4) << 8;
        fillRect(t, x + 2, y + 13, 13, 2, 0x000000);
        fillRect(t, x + 2, y + 13, 12, 1, cb);
        fillRect(t, x + 2, y + 13, p, 1, ca);

        RenderPath.StateSetTextureEnable(true);
        RenderPath.StateSetLightingEnable(true);
        RenderPath.StateSetDepthTestEnable(true);
        RenderPath.StateSetColour(1, 1, 1, 1);
    } else if (item->hasPotionStrengthBar()) {
        RenderPath.StateSetLightingEnable(false);
        RenderPath.StateSetDepthTestEnable(false);
        RenderPath.StateSetTextureEnable(false);

        Tesselator* t = Tesselator::getInstance();

        fillRect(t, x + 3, y + 13, 11, 2, 0x000000);
        // fillRect(t, x + 2, y + 13, 13, 1, 0x1dabc0);
        fillRect(t, x + 3, y + 13,
                 m_iPotionStrengthBarWidth[item->GetPotionStrength()], 2,
                 0x00e1eb);
        fillRect(t, x + 2 + 3, y + 13, 1, 2, 0x000000);
        fillRect(t, x + 2 + 3 + 3, y + 13, 1, 2, 0x000000);
        fillRect(t, x + 2 + 3 + 3 + 3, y + 13, 1, 2, 0x000000);

        RenderPath.StateSetTextureEnable(true);
        RenderPath.StateSetLightingEnable(true);
        RenderPath.StateSetDepthTestEnable(true);
        RenderPath.StateSetColour(1, 1, 1, 1);
    }
    RenderPath.StateSetBlendEnable(false);
}

const int ItemRenderer::m_iPotionStrengthBarWidth[] = {3, 6, 9, 11};

void ItemRenderer::fillRect(Tesselator* t, int x, int y, int w, int h, int c) {
    t->begin();
    t->color(c);
    t->vertex((float)(x + 0), (float)(y + 0), (float)(0));
    t->vertex((float)(x + 0), (float)(y + h), (float)(0));
    t->vertex((float)(x + w), (float)(y + h), (float)(0));
    t->vertex((float)(x + w), (float)(y + 0), (float)(0));
    t->end();
}

// 4J - a few changes here to get x, y, w, h in as floats (for xui rendering
// accuracy), and to align final pixels to the final screen resolution
void ItemRenderer::blit(float x, float y, int sx, int sy, float w, float h) {
    float us = 1 / 256.0f;
    float vs = 1 / 256.0f;
    Tesselator* t = Tesselator::getInstance();
    t->begin();

    // 4J - calculate what the pixel coordinates will be in final screen
    // coordinates
    float sfx = (float)Minecraft::GetInstance()->width /
                (float)Minecraft::GetInstance()->width_phys;
    float sfy = (float)Minecraft::GetInstance()->height /
                (float)Minecraft::GetInstance()->height_phys;
    float xx0 = x * sfx;
    float xx1 = (x + w) * sfx;
    float yy0 = y * sfy;
    float yy1 = (y + h) * sfy;
    // Round to whole pixels - rounding inwards so that we don't overlap any
    // surrounding graphics
    xx0 = ceilf(xx0);
    xx1 = floorf(xx1);
    yy0 = ceilf(yy0);
    yy1 = floorf(yy1);
    // Offset by half to get actual centre of pixel - again moving inwards to
    // avoid overlap with surrounding graphics
    xx0 += 0.5f;
    xx1 -= 0.5f;
    yy0 += 0.5f;
    yy1 -= 0.5f;
    // Convert back to game coordinate space
    float xx0f = xx0 / sfx;
    float xx1f = xx1 / sfx;
    float yy0f = yy0 / sfy;
    float yy1f = yy1 / sfy;

    // 4J - subtracting 0.5f (actual screen pixels, so need to compensate for
    // physical & game width) from each x & y coordinate to compensate for
    // centre of pixels in directx vs openGL
    float f = (0.5f * (float)Minecraft::GetInstance()->width) /
              (float)Minecraft::GetInstance()->width_phys;

    t->vertexUV(xx0f, yy1f, (float)(blitOffset), (float)((sx + 0) * us),
                (float)((sy + 16) * vs));
    t->vertexUV(xx1f, yy1f, (float)(blitOffset), (float)((sx + 16) * us),
                (float)((sy + 16) * vs));
    t->vertexUV(xx1f, yy0f, (float)(blitOffset), (float)((sx + 16) * us),
                (float)((sy + 0) * vs));
    t->vertexUV(xx0f, yy0f, (float)(blitOffset), (float)((sx + 0) * us),
                (float)((sy + 0) * vs));
    t->end();
}

void ItemRenderer::blit(float x, float y, Icon* tex, float w, float h) {
    Tesselator* t = Tesselator::getInstance();
    t->begin();

    // 4J - calculate what the pixel coordinates will be in final screen
    // coordinates
    float sfx = (float)Minecraft::GetInstance()->width /
                (float)Minecraft::GetInstance()->width_phys;
    float sfy = (float)Minecraft::GetInstance()->height /
                (float)Minecraft::GetInstance()->height_phys;
    float xx0 = x * sfx;
    float xx1 = (x + w) * sfx;
    float yy0 = y * sfy;
    float yy1 = (y + h) * sfy;
    // Round to whole pixels - rounding inwards so that we don't overlap any
    // surrounding graphics
    xx0 = ceilf(xx0);
    xx1 = floorf(xx1);
    yy0 = ceilf(yy0);
    yy1 = floorf(yy1);
    // Offset by half to get actual centre of pixel - again moving inwards to
    // avoid overlap with surrounding graphics
    xx0 += 0.5f;
    xx1 -= 0.5f;
    yy0 += 0.5f;
    yy1 -= 0.5f;
    // Convert back to game coordinate space
    float xx0f = xx0 / sfx;
    float xx1f = xx1 / sfx;
    float yy0f = yy0 / sfy;
    float yy1f = yy1 / sfy;

    // 4J - subtracting 0.5f (actual screen pixels, so need to compensate for
    // physical & game width) from each x & y coordinate to compensate for
    // centre of pixels in directx vs openGL
    float f = (0.5f * (float)Minecraft::GetInstance()->width) /
              (float)Minecraft::GetInstance()->width_phys;

    t->vertexUV(xx0f, yy1f, blitOffset, tex->getU0(true), tex->getV1(true));
    t->vertexUV(xx1f, yy1f, blitOffset, tex->getU1(true), tex->getV1(true));
    t->vertexUV(xx1f, yy0f, blitOffset, tex->getU1(true), tex->getV0(true));
    t->vertexUV(xx0f, yy0f, blitOffset, tex->getU0(true), tex->getV0(true));
    t->end();
}
