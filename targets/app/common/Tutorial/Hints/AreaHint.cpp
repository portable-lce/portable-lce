#include "AreaHint.h"

#include <memory>

#include "app/common/Tutorial/Hints/TutorialHint.h"
#include "app/common/Tutorial/Tutorial.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/Vec3.h"

AreaHint::AreaHint(eTutorial_Hint id, Tutorial* tutorial,
                   eTutorial_State displayState, eTutorial_State completeState,
                   int descriptionId, double x0, double y0, double z0,
                   double x1, double y1, double z1, bool allowFade /*= false*/,
                   bool contains /*= true*/)
    : TutorialHint(id, tutorial, descriptionId, e_Hint_Area, allowFade) {
    area = AABB(x0, y0, z0, x1, y1, z1);

    this->contains = contains;

    m_displayState = displayState;
    m_completeState = completeState;
}

int AreaHint::tick() {
    Minecraft* minecraft = Minecraft::GetInstance();
    Vec3 player_pos = minecraft->player->getPos(1);

    if ((m_displayState == e_Tutorial_State_Any ||
         m_tutorial->getCurrentState() == m_displayState) &&
        m_hintNeeded && area.contains(player_pos) == contains) {
        if (m_completeState == e_Tutorial_State_None) {
            m_hintNeeded = false;
        } else if (m_tutorial->isStateCompleted(m_completeState)) {
            m_hintNeeded = false;
            return -1;
        }

        return m_descriptionId;
    } else {
        return -1;
    }
}
