#include "UIControl_MinecraftPlayer.h"

#include <cmath>
#include <memory>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_InventoryMenu.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include <numbers>

#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/ScreenSizeCalculator.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"

UIControl_MinecraftPlayer::UIControl_MinecraftPlayer() {
    UIControl::setControlType(UIControl::eMinecraftPlayer);

    Minecraft* pMinecraft = Minecraft::GetInstance();

    ScreenSizeCalculator ssc(pMinecraft->options, pMinecraft->width_phys,
                             pMinecraft->height_phys);
    m_fScreenWidth = (float)pMinecraft->width_phys;
    m_fRawWidth = (float)ssc.rawWidth;
    m_fScreenHeight = (float)pMinecraft->height_phys;
    m_fRawHeight = (float)ssc.rawHeight;
}

void UIControl_MinecraftPlayer::render(IggyCustomDrawCallbackRegion* region) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    (void)0;
    (void)0;
    RenderPath.MatrixPush();

    float width = region->x1 - region->x0;
    float height = region->y1 - region->y0;
    float xo = width / 2;
    float yo = height;

    // dynamic y offset according to region height
    RenderPath.MatrixTranslate(xo, yo - (height / 9.0f), 50.0f);

    float ss;

    // Base scale on height of this control
    // Potentially we might want separate x & y scales here
    ss = width / (m_fScreenWidth / m_fScreenHeight);

    RenderPath.MatrixScale(-ss, ss, ss);
    RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

    UIScene_InventoryMenu* containerMenu =
        (UIScene_InventoryMenu*)m_parentScene;

    float oybr = pMinecraft->localplayers[containerMenu->getPad()]->yBodyRot;
    float oyr = pMinecraft->localplayers[containerMenu->getPad()]->yRot;
    float oxr = pMinecraft->localplayers[containerMenu->getPad()]->xRot;
    float oyhr = pMinecraft->localplayers[containerMenu->getPad()]->yHeadRot;

    // float xd = ( matrix._41 + ( (bwidth*matrix._11)/2) ) - m_pointerPos.x;
    float xd = (m_x + m_width / 2) - containerMenu->m_pointerPos.x;

    // Need to base Y on head position, not centre of mass
    // float yd = ( matrix._42 + ( (bheight*matrix._22) / 2) - 40 ) -
    // m_pointerPos.y;
    float yd = (m_y + m_height / 2 - 40) - containerMenu->m_pointerPos.y;

    RenderPath.MatrixRotate((45 + 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    Lighting::turnOn();
    RenderPath.MatrixRotate((-45 - 90)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);

    RenderPath.MatrixRotate((-(float)atan(yd / 40.0f) * 20)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);

    pMinecraft->localplayers[containerMenu->getPad()]->yBodyRot =
        (float)atan(xd / 40.0f) * 20;
    pMinecraft->localplayers[containerMenu->getPad()]->yRot =
        (float)atan(xd / 40.0f) * 40;
    pMinecraft->localplayers[containerMenu->getPad()]->xRot =
        -(float)atan(yd / 40.0f) * 20;
    pMinecraft->localplayers[containerMenu->getPad()]->yHeadRot =
        pMinecraft->localplayers[containerMenu->getPad()]->yRot;
    // pMinecraft->localplayers[m_iPad]->glow = 1;
    RenderPath.MatrixTranslate(
        0, pMinecraft->localplayers[containerMenu->getPad()]->heightOffset, 0);
    EntityRenderDispatcher::instance->playerRotY = 180;

    // 4J Stu - Turning on hideGui while we do this stops the name rendering in
    // split-screen
    bool wasHidingGui = pMinecraft->options->hideGui;
    pMinecraft->options->hideGui = true;
    EntityRenderDispatcher::instance->render(
        pMinecraft->localplayers[containerMenu->getPad()], 0, 0, 0, 0, 1, false,
        false);
    pMinecraft->options->hideGui = wasHidingGui;
    // pMinecraft->localplayers[m_iPad]->glow = 0;

    pMinecraft->localplayers[containerMenu->getPad()]->yBodyRot = oybr;
    pMinecraft->localplayers[containerMenu->getPad()]->yRot = oyr;
    pMinecraft->localplayers[containerMenu->getPad()]->xRot = oxr;
    pMinecraft->localplayers[containerMenu->getPad()]->yHeadRot = oyhr;
    RenderPath.MatrixPop();
    Lighting::turnOff();
    (void)0;
}
