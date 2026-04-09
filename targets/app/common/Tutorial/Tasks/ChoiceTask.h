#pragma once
// using namespace std;

#include "TutorialTask.h"
#include "app/common/Tutorial/TutorialEnum.h"

class Tutorial;

// Information messages with a choice
class ChoiceTask : public TutorialTask {
private:
    int m_iConfirmMapping, m_iCancelMapping;
    bool m_bConfirmMappingComplete, m_bCancelMappingComplete;
    eTutorial_CompletionAction m_cancelAction;

    bool CompletionMaskIsValid();

public:
    ChoiceTask(
        Tutorial* tutorial, int descriptionId, int promptId = -1,
        bool requiresUserInput = false, int iConfirmMapping = 0,
        int iCancelMapping = 0,
        eTutorial_CompletionAction cancelAction = e_Tutorial_Completion_None);
    virtual bool isCompleted();
    virtual eTutorial_CompletionAction getCompletionAction();
    virtual int getPromptId();
    virtual void setAsCurrentTask(bool active = true);
    virtual void handleUIInput(int iAction);
};