#pragma once

#include "TutorialHint.h"
#include "app/common/Tutorial/TutorialEnum.h"
#include "minecraft/world/phys/AABB.h"

class AABB;
class Tutorial;

class AreaHint : public TutorialHint {
private:
    AABB area;
    bool contains;  // If true we must stay in this area, if false must stay out
                    // of this area

    // Only display the hint if the game is in this state
    eTutorial_State m_displayState;

    // Only display the hint if this state is not completed
    eTutorial_State m_completeState;

public:
    AreaHint(eTutorial_Hint id, Tutorial* tutorial,
             eTutorial_State displayState, eTutorial_State completeState,
             int descriptionId, double x0, double y0, double z0, double x1,
             double y1, double z1, bool allowFade = true, bool contains = true);

    virtual int tick();
};
