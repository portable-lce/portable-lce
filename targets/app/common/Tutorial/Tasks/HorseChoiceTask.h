#pragma once

#include "ChoiceTask.h"
#include "minecraft/world/tutorial/TutorialEnum.h"

class Tutorial;

// Same as choice task, but switches description based on horse type.
class HorseChoiceTask : public ChoiceTask {
protected:
    int m_eHorseType;

    int m_iDescHorse, m_iDescDonkey, m_iDescMule;

public:
    HorseChoiceTask(
        Tutorial* tutorial, int iDescHorse, int iDescDonkey, int iDescMule,
        int iPromptId = -1, bool requiresUserInput = false,
        int iConfirmMapping = 0, int iCancelMapping = 0,
        eTutorial_CompletionAction cancelAction = e_Tutorial_Completion_None);

    virtual int getDescriptionId();

    virtual void onLookAtEntity(std::shared_ptr<Entity> entity);
};