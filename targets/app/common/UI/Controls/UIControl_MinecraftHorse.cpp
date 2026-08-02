#include "app/common/UI/Controls/UIControl_MinecraftHorse.h"

#include <cmath>
#include <memory>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_HorseInventoryMenu.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include <numbers>

#include "minecraft/client/Lighting.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/ScreenSizeCalculator.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/EntityHorse.h"
// #include
// "../../../minecraft/net.minecraft.world.entity.animal.EntityHorse.h"

UIControl_MinecraftHorse::UIControl_MinecraftHorse() {
    UIControl::setControlType(UIControl::eMinecraftHorse);

    Minecraft* pMinecraft = Minecraft::GetInstance();

    ScreenSizeCalculator ssc(pMinecraft->options, pMinecraft->width_phys,
                             pMinecraft->height_phys);
    m_fScreenWidth = (float)pMinecraft->width_phys;
    m_fRawWidth = (float)ssc.rawWidth;
    m_fScreenHeight = (float)pMinecraft->height_phys;
    m_fRawHeight = (float)ssc.rawHeight;
}

void UIControl_MinecraftHorse::render(IggyCustomDrawCallbackRegion* region) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    (void)0;
    (void)0;
    RenderPath.MatrixPush();

    float width = region->x1 - region->x0;
    float height = region->y1 - region->y0;
    float xo = width / 2;
    float yo = height;

    // dynamic y offset according to region height
    RenderPath.MatrixTranslate(xo, yo - (height / 7.5f), 50.0f);

    // UIScene_InventoryMenu *containerMenu = (UIScene_InventoryMenu
    // *)m_parentScene;
    UIScene_HorseInventoryMenu* containerMenu =
        (UIScene_HorseInventoryMenu*)m_parentScene;

    std::shared_ptr<LivingEntity> entityHorse = containerMenu->m_horse;

    // Base scale on height of this control
    // Potentially we might want separate x & y scales here
    float ss = width / (m_fScreenWidth / m_fScreenHeight) * 0.71f;

    RenderPath.MatrixScale(-ss, ss, ss);
    RenderPath.MatrixRotate((180)*(std::numbers::pi_v<float>/180.f), 0, 0, 1);

    float oybr = entityHorse->yBodyRot;
    float oyr = entityHorse->yRot;
    float oxr = entityHorse->xRot;
    float oyhr = entityHorse->yHeadRot;

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

    entityHorse->yBodyRot = (float)atan(xd / 40.0f) * 20;
    entityHorse->yRot = (float)atan(xd / 40.0f) * 40;
    entityHorse->xRot = -(float)atan(yd / 40.0f) * 20;
    entityHorse->yHeadRot = entityHorse->yRot;
    // entityHorse->glow = 1;
    RenderPath.MatrixTranslate(0, entityHorse->heightOffset, 0);
    EntityRenderDispatcher::instance->playerRotY = 180;

    // 4J Stu - Turning on hideGui while we do this stops the name rendering in
    // split-screen
    bool wasHidingGui = pMinecraft->options->hideGui;
    pMinecraft->options->hideGui = true;
    EntityRenderDispatcher::instance->render(entityHorse, 0, 0, 0, 0, 1, false,
                                             false);
    pMinecraft->options->hideGui = wasHidingGui;
    // entityHorse->glow = 0;

    entityHorse->yBodyRot = oybr;
    entityHorse->yRot = oyr;
    entityHorse->xRot = oxr;
    entityHorse->yHeadRot = oyhr;
    RenderPath.MatrixPop();
    Lighting::turnOff();
    (void)0;
}
