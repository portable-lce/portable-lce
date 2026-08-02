#include "GuiComponent.h"

#include <math.h>

#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "platform/renderer/IRenderPath.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"


void GuiComponent::hLine(int x0, int x1, int y, int col) {
    if (x1 < x0) {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    fill(x0, y, x1 + 1, y + 1, col);
}

void GuiComponent::vLine(int x, int y0, int y1, int col) {
    if (y1 < y0) {
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    fill(x, y0 + 1, x + 1, y1, col);
}

void GuiComponent::fill(int x0, int y0, int x1, int y1, int col) {
    if (x0 < x1) {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (y0 < y1) {
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    float a = ((col >> 24) & 0xff) / 255.0f;
    float r = ((col >> 16) & 0xff) / 255.0f;
    float g = ((col >> 8) & 0xff) / 255.0f;
    float b = ((col) & 0xff) / 255.0f;

    auto [tvb, span] = RenderPath.alloc_transient_vertices(
        4, rp::VertexLayout::world_standard, rp::PrimitiveType::triangle_fan);
    if (span.empty()) {
        return;
    }
    auto* v = reinterpret_cast<rp::WorldStandardVertex*>(span.data());
    v[0] = {{(float)x0, (float)y1, 0}, {0, 0}, 0, 0, 0xfe00fe00};
    v[1] = {{(float)x1, (float)y1, 0}, {0, 0}, 0, 0, 0xfe00fe00};
    v[2] = {{(float)x1, (float)y0, 0}, {0, 0}, 0, 0, 0xfe00fe00};
    v[3] = {{(float)x0, (float)y0, 0}, {0, 0}, 0, 0, 0xfe00fe00};

    rp::DrawCall dc{};
    dc.source = rp::VertexSource::transient;
    dc.transient = tvb;
    dc.material = Gui::gui_mat_untextured_alpha_;
    dc.tint_color[0] = r;
    dc.tint_color[1] = g;
    dc.tint_color[2] = b;
    dc.tint_color[3] = a;

    RenderPath.StateSetBlendEnable(true);
    RenderPath.StateSetTextureEnable(false);
    RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
    RenderPath.submit_immediate(dc);
    RenderPath.StateSetTextureEnable(true);
    RenderPath.StateSetBlendEnable(false);
}

static uint32_t pack_color(float r, float g, float b, float a) {
    return (uint32_t(r * 255) << 24) | (uint32_t(g * 255) << 16) |
           (uint32_t(b * 255) << 8) | uint32_t(a * 255);
}

void GuiComponent::fillGradient(int x0, int y0, int x1, int y1, int col1,
                                int col2) {
    float a1 = ((col1 >> 24) & 0xff) / 255.0f;
    float r1 = ((col1 >> 16) & 0xff) / 255.0f;
    float g1 = ((col1 >> 8) & 0xff) / 255.0f;
    float b1 = ((col1) & 0xff) / 255.0f;

    float a2 = ((col2 >> 24) & 0xff) / 255.0f;
    float r2 = ((col2 >> 16) & 0xff) / 255.0f;
    float g2 = ((col2 >> 8) & 0xff) / 255.0f;
    float b2 = ((col2) & 0xff) / 255.0f;

    uint32_t c1 = pack_color(r1, g1, b1, a1);
    uint32_t c2 = pack_color(r2, g2, b2, a2);

    auto [tvb, span] = RenderPath.alloc_transient_vertices(
        4, rp::VertexLayout::world_standard, rp::PrimitiveType::triangle_fan);
    if (span.empty()) return;

    auto* v = reinterpret_cast<rp::WorldStandardVertex*>(span.data());
    v[0] = {{(float)x1, (float)y0, blitOffset}, {0, 0}, c1, 0, 0xfe00fe00};
    v[1] = {{(float)x0, (float)y0, blitOffset}, {0, 0}, c1, 0, 0xfe00fe00};
    v[2] = {{(float)x0, (float)y1, blitOffset}, {0, 0}, c2, 0, 0xfe00fe00};
    v[3] = {{(float)x1, (float)y1, blitOffset}, {0, 0}, c2, 0, 0xfe00fe00};

    rp::DrawCall dc{};
    dc.source = rp::VertexSource::transient;
    dc.transient = tvb;
    dc.material = Gui::gui_mat_untextured_alpha_;

    RenderPath.StateSetTextureEnable(false);
    RenderPath.StateSetBlendEnable(true);
    RenderPath.StateSetAlphaTestEnable(false);
    RenderPath.StateSetBlendFunc(rp::BlendFactor::src_alpha, rp::BlendFactor::one_minus_src_alpha);
    (void)0;
    RenderPath.submit_immediate(dc);
    (void)0;
    RenderPath.StateSetBlendEnable(false);
    RenderPath.StateSetAlphaTestEnable(true);
    RenderPath.StateSetTextureEnable(true);
}

GuiComponent::GuiComponent() { blitOffset = 0; }

void GuiComponent::drawCenteredString(Font* font, const std::string& str, int x,
                                      int y, int color) {
    font->drawShadow(str, x - (font->width(str)) / 2, y, color);
}

void GuiComponent::drawString(Font* font, const std::string& str, int x, int y,
                              int color) {
    font->drawShadow(str, x, y, color);
}

void GuiComponent::blit(int x, int y, int sx, int sy, int w, int h) {
    float us = 1 / 256.0f;
    float vs = 1 / 256.0f;

    const float extraShift = 0.75f;
    float dx = (extraShift * (float)Minecraft::GetInstance()->width) /
               (float)Minecraft::GetInstance()->width_phys;
    dx /= Gui::currentGuiScaleFactor;
    float dy = extraShift / Gui::currentGuiScaleFactor;
    float fx = (floorf((float)x * Gui::currentGuiScaleFactor)) /
               Gui::currentGuiScaleFactor;
    float fy = (floorf((float)y * Gui::currentGuiScaleFactor)) /
               Gui::currentGuiScaleFactor;
    float fw = (floorf((float)w * Gui::currentGuiScaleFactor)) /
               Gui::currentGuiScaleFactor;
    float fh = (floorf((float)h * Gui::currentGuiScaleFactor)) /
               Gui::currentGuiScaleFactor;

    float u0 = (sx + 0) * us;
    float u1 = (sx + w) * us;
    float v0 = (sy + 0) * vs;
    float v1 = (sy + h) * vs;

    auto [tvb, span] = RenderPath.alloc_transient_vertices(
        4, rp::VertexLayout::world_standard, rp::PrimitiveType::triangle_fan);
    if (span.empty()) return;
    auto* v = reinterpret_cast<rp::WorldStandardVertex*>(span.data());
    v[0] = {{fx + 0  - dx, fy + fh - dy, blitOffset}, {u0, v1}, 0, 0, 0xfe00fe00};
    v[1] = {{fx + fw - dx, fy + fh - dy, blitOffset}, {u1, v1}, 0, 0, 0xfe00fe00};
    v[2] = {{fx + fw - dx, fy + 0  - dy, blitOffset}, {u1, v0}, 0, 0, 0xfe00fe00};
    v[3] = {{fx + 0  - dx, fy + 0  - dy, blitOffset}, {u0, v0}, 0, 0, 0xfe00fe00};

    rp::DrawCall dc{};
    dc.source = rp::VertexSource::transient;
    dc.transient = tvb;
    RenderPath.submit_immediate(dc);
}

