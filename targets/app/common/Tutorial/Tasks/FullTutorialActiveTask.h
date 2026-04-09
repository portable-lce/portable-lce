#pragma once
// using namespace std;

#include "TutorialTask.h"
#include "minecraft/world/tutorial/TutorialEnum.h"

class Tutorial;

// Information messages with a choice
class FullTutorialActiveTask : public TutorialTask {
private:
    eTutorial_CompletionAction m_completeAction;

    bool CompletionMaskIsValid();

public:
    FullTutorialActiveTask(
        Tutorial* tutorial,
        eTutorial_CompletionAction completeAction = e_Tutorial_Completion_None);
    virtual bool isCompleted();
    virtual eTutorial_CompletionAction getCompletionAction();
};