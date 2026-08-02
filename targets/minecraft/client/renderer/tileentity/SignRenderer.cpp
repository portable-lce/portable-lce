#include "SignRenderer.h"

#include <cstdint>
#include <memory>
#include <numbers>
#include <string>

#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/model/SignModel.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/SignTileEntity.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "platform/XboxStubs.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "strings.h"

ResourceLocation SignRenderer::SIGN_LOCATION = ResourceLocation(TN_ITEM_SIGN);

SignRenderer::SignRenderer() { signModel = new SignModel(); }

void SignRenderer::render(std::shared_ptr<TileEntity> _sign, double x, double y,
                          double z, float a, bool setColor, float alpha,
                          bool useCompiled) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<SignTileEntity> sign =
        std::dynamic_pointer_cast<SignTileEntity>(_sign);

    Tile* tile = sign->getTile();

    RenderPath.MatrixPush();
    float size = 16 / 24.0f;
    if (tile == Tile::sign) {
        RenderPath.MatrixTranslate((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);
        float rot = sign->getData() * 360 / 16.0f;
        RenderPath.MatrixRotate((-rot)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        signModel->cube2->visible = true;
    } else {
        int face = sign->getData();
        float rot = 0;

        if (face == 2) rot = 180;
        if (face == 4) rot = 90;
        if (face == 5) rot = -90;

        RenderPath.MatrixTranslate((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);
        RenderPath.MatrixRotate((-rot)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
        RenderPath.MatrixTranslate(0, -5 / 16.0f, -7 / 16.0f);

        signModel->cube2->visible = false;
    }

    bindTexture(&SIGN_LOCATION);  // 4J was "/item/sign.png"

    RenderPath.MatrixPush();
    RenderPath.MatrixScale(size, -size, -size);
    signModel->render(true);
    RenderPath.MatrixPop();
    Font* font = getFont();

    float s = 1 / 60.0f * size;
    RenderPath.MatrixTranslate(0, 0.5f * size, 0.07f * size);
    RenderPath.MatrixScale(s, -s, s);
    (void)0;
    RenderPath.StateSetDepthMask(false);

    int col = Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_Sign_Text);
    std::string msg;
    // need to send the new data
    // Get the current language setting from the console
    std::uint32_t dwLanguage = XGetLanguage();

    for (int i = 0; i < MAX_SIGN_LINES; i++)  // 4J - was sign.messages.size()
    {
        if (sign->IsVerified()) {
            if (sign->IsCensored()) {
                switch (dwLanguage) {
                    case XC_LANGUAGE_KOREAN:
                    case XC_LANGUAGE_JAPANESE:
                    case XC_LANGUAGE_TCHINESE:
                        msg = "Censored";  // In-game font, so English only
                        break;
                    default:
                        msg =
                            gameServices().getString(IDS_STRINGVERIFY_CENSORED);
                        break;
                }
            } else {
                msg = sign->GetMessage(i);
            }
        } else {
            switch (dwLanguage) {
                case XC_LANGUAGE_KOREAN:
                case XC_LANGUAGE_JAPANESE:
                case XC_LANGUAGE_TCHINESE:
                    msg = "Awaiting Approval";  // In-game font, so English only
                    break;
                default:
                    msg = gameServices().getString(
                        IDS_STRINGVERIFY_AWAITING_APPROVAL);
                    break;
            }
        }

        if (i == sign->GetSelectedLine()) {
            msg = "> " + msg + " <";
            font->draw(msg, -font->width(msg) / 2,
                       i * 10 - (MAX_SIGN_LINES) * 5,
                       col);  // 4J - (MAX_SIGN_LINES) was sign.messages.size()
        } else {
            font->draw(msg, -font->width(msg) / 2,
                       i * 10 - (MAX_SIGN_LINES) * 5,
                       col);  // 4J - (MAX_SIGN_LINES) was sign.messages.size()
        }
    }
    RenderPath.StateSetDepthMask(true);
    RenderPath.StateSetColour(1, 1, 1, 1);
    RenderPath.MatrixPop();
}
