#include "Input.h"

#include <cmath>

#include "LocalPlayer.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "platform/input/input.h"

Input::Input() {
    xa = 0;
    ya = 0;
    wasJumping = false;
    jumping = false;
    sneaking = false;
    sprintKey = false;

    lReset = false;
    rReset = false;
}

void Input::tick(LocalPlayer* player) {
    // 4J Stu -  Assume that we only need one input class, even though the java
    // has subclasses for keyboard/controller This function is based on the
    // ControllerInput class in the Java, and will probably need changed
    // OutputDebugString("INPUT: Beginning input tick\n");

    Minecraft* pMinecraft = Minecraft::GetInstance();
    int iPad = player->GetXboxPad();

    // 4J-PB minecraft movement seems to be the wrong way round, so invert x!
    if (pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_LEFT) ||
        pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_RIGHT))
        xa = -PlatformInput.GetJoypadStick_LX(iPad);
    else
        xa = 0.0f;

    if (pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_FORWARD) ||
        pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_BACKWARD))
        ya = PlatformInput.GetJoypadStick_LY(iPad);
    else
        ya = 0.0f;

#ifndef _CONTENT_PACKAGE
    if (gameServices().debugFreezePlayers()) {
        xa = ya = 0.0f;
        player->abilities.flying = true;
    }
#endif

    if (!lReset) {
        if (xa * xa + ya * ya == 0.0f) {
            lReset = true;
        }
        xa = ya = 0.0f;
    }

    // 4J - in flying mode, don't actually toggle sneaking
    if (!player->abilities.flying) {
        if ((player->ullButtonsPressed &
             (1LL << MINECRAFT_ACTION_SNEAK_TOGGLE)) &&
            pMinecraft->localgameModes[iPad]->isInputAllowed(
                MINECRAFT_ACTION_SNEAK_TOGGLE)) {
            sneaking = !sneaking;
        }
    }

    if (sneaking) {
        xa *= 0.3f;
        ya *= 0.3f;
    }

    float turnSpeed = 50.0f;

    float tx = 0.0f;
    float ty = 0.0f;
    if (pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_LOOK_LEFT) ||
        pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_LOOK_RIGHT))
        tx = PlatformInput.GetJoypadStick_RX(iPad) *
             (((float)gameServices().getGameSettings(
                  iPad,
                  eGameSetting_Sensitivity_InGame)) /
              100.0f);  // apply sensitivity to look
    if (pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_LOOK_UP) ||
        pMinecraft->localgameModes[iPad]->isInputAllowed(
            MINECRAFT_ACTION_LOOK_DOWN))
        ty = PlatformInput.GetJoypadStick_RY(iPad) *
             (((float)gameServices().getGameSettings(
                  iPad,
                  eGameSetting_Sensitivity_InGame)) /
              100.0f);  // apply sensitivity to look

#ifndef _CONTENT_PACKAGE
    if (gameServices().debugFreezePlayers()) tx = ty = 0.0f;
#endif

    // 4J: WESTY : Invert look Y if required.
    if (gameServices().getGameSettings(iPad, eGameSetting_ControlInvertLook)) {
        ty = -ty;
    }

    if (!rReset) {
        if (tx * tx + ty * ty == 0.0f) {
            rReset = true;
        }
        tx = ty = 0.0f;
    }
    player->interpolateTurn(tx * std::abs(tx) * turnSpeed,
                            ty * std::abs(ty) * turnSpeed);

    // jumping = controller.isButtonPressed(0);

    sprintKey = PlatformInput.GetValue(iPad, MINECRAFT_ACTION_SPRINT) &&
                pMinecraft->localgameModes[iPad]->isInputAllowed(
                    MINECRAFT_ACTION_SPRINT);
    jumping =
        PlatformInput.GetValue(iPad, MINECRAFT_ACTION_JUMP) &&
        pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_JUMP);

#ifndef _CONTENT_PACKAGE
    if (gameServices().debugFreezePlayers()) jumping = false;
#endif

    // OutputDebugString("INPUT: End input tick\n");
}