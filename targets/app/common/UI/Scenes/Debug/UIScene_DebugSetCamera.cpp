
#include "app/common/UI/Scenes/Debug/UIScene_DebugSetCamera.h"

#include <wchar.h>

#include <memory>

#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/world/phys/Vec3.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"

class UILayer;
#ifdef _DEBUG_MENUS_ENABLED
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "util/StringHelpers.h"

UIScene_DebugSetCamera::UIScene_DebugSetCamera(int iPad, void* initData,
                                               UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    int playerNo = 0;
    currentPosition = new DebugSetCameraPosition();
    currentPosition->player = playerNo;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft != nullptr) {
        Vec3 vec = pMinecraft->localplayers[playerNo]->getPos(1.0);

        currentPosition->m_camX = vec.x;
        currentPosition->m_camY =
            vec.y -
            1.62;  // pMinecraft->localplayers[playerNo]->getHeadHeight();
        currentPosition->m_camZ = vec.z;

        currentPosition->m_yRot = pMinecraft->localplayers[playerNo]->yRot;
        currentPosition->m_elev = pMinecraft->localplayers[playerNo]->xRot;
    }

    char TempString[256];

    snprintf(TempString, 256, "%f", currentPosition->m_camX);
    m_textInputX.init(TempString, eControl_CamX);

    snprintf(TempString, 256, "%f", currentPosition->m_camY);
    m_textInputY.init(TempString, eControl_CamY);

    snprintf(TempString, 256, "%f", currentPosition->m_camZ);
    m_textInputZ.init(TempString, eControl_CamZ);

    snprintf(TempString, 256, "%f", currentPosition->m_yRot);
    m_textInputYRot.init(TempString, eControl_YRot);

    snprintf(TempString, 256, "%f", currentPosition->m_elev);
    m_textInputElevation.init(TempString, eControl_Elevation);

    m_checkboxLockPlayer.init("Lock Player", eControl_LockPlayer,
                              app.GetFreezePlayers());

    m_buttonTeleport.init("Teleport", eControl_Teleport);

    m_labelTitle.init("Set Camera Position");
    m_labelCamX.init("CamX");
    m_labelCamY.init("CamY");
    m_labelCamZ.init("CamZ");
    m_labelYRotElev.init("Y-Rot & Elevation (Degs)");
}

std::string UIScene_DebugSetCamera::getMoviePath() { return "DebugSetCamera"; }

void UIScene_DebugSetCamera::handleInput(int iPad, int key, bool repeat,
                                         bool pressed, bool released,
                                         bool& handled) {
    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_DebugSetCamera::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Teleport: {
            std::unique_ptr<minecraft::XuiActionOwnedPayload> payload(
                currentPosition);
            currentPosition = nullptr;  // ownership transferred
            app.SetXuiServerAction(PlatformProfile.GetPrimaryPad(),
                                   eXuiServerAction_SetCameraLocation,
                                   std::move(payload));
        } break;
        case eControl_CamX:
        case eControl_CamY:
        case eControl_CamZ:
        case eControl_YRot:
        case eControl_Elevation:
            m_keyboardCallbackControl = (eControls)((int)controlId);
            PlatformInput.RequestKeyboard(
                "Enter something", "", 0, 25,
                [this](bool bRes) -> int {
                    return handleKeyboardComplete(bRes);
                },
                IPlatformInput::EKeyboardMode_Default);
            break;
    };
}

void UIScene_DebugSetCamera::handleCheckboxToggled(F64 controlId,
                                                   bool selected) {
    switch ((int)controlId) {
        case eControl_LockPlayer:
            app.SetFreezePlayers(selected);
            break;
    }
}

int UIScene_DebugSetCamera::handleKeyboardComplete(bool bRes) {
    const char* text = PlatformInput.GetText();
    if (text[0] != '\0') {
        std::string value = text;
        double val = 0;
        if (!value.empty()) val = fromWString<double>(value);
        switch (m_keyboardCallbackControl) {
            case eControl_CamX:
                m_textInputX.setLabel(value);
                currentPosition->m_camX = val;
                break;
            case eControl_CamY:
                m_textInputY.setLabel(value);
                currentPosition->m_camY = val;
                break;
            case eControl_CamZ:
                m_textInputZ.setLabel(value);
                currentPosition->m_camZ = val;
                break;
            case eControl_YRot:
                m_textInputYRot.setLabel(value);
                currentPosition->m_yRot = val;
                break;
            case eControl_Elevation:
                m_textInputElevation.setLabel(value);
                currentPosition->m_elev = val;
                break;
            default:
                break;
        }
    }

    return 0;
}
#endif
