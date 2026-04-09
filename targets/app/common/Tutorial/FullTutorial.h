#pragma once
#include "Tutorial.h"
#include "app/common/Tutorial/TutorialEnum.h"

#define FULL_TUTORIAL_PROGRESS_2_X_2_Crafting 1
#define FULL_TUTORIAL_PROGRESS_3_X_3_Crafting 2
#define FULL_TUTORIAL_PROGRESS_CRAFT_FURNACE 4
#define FULL_TUTORIAL_PROGRESS_USE_FURNACE 8
#define EXTENDED_TUTORIAL_PROGRESS_USE_BREWING_STAND 16

class FullTutorial : public Tutorial {
private:
    bool m_isTrial;
    char m_progressFlags;
    bool m_completedStates[e_Tutorial_State_Max];

public:
    FullTutorial(int iPad, bool isTrial = false);

    virtual bool isStateCompleted(eTutorial_State state);
    virtual void setStateCompleted(eTutorial_State state);
};