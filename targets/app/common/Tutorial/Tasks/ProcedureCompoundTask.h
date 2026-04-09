#pragma once

#include <vector>

#include "TutorialTask.h"
#include "app/common/Tutorial/TutorialEnum.h"

class Tutorial;

// A tutorial task that requires each of the task to be completed in order until
// the last one is complete. If an earlier task that was complete is now not
// complete then it's hint should be shown.
class ProcedureCompoundTask : public TutorialTask {
public:
    ProcedureCompoundTask(Tutorial* tutorial)
        : TutorialTask(tutorial, -1, false, nullptr, false, true, false) {}

    ~ProcedureCompoundTask();

    void AddTask(TutorialTask* task);

    virtual int getDescriptionId();
    virtual int getPromptId();
    virtual bool isCompleted();
    virtual void onCrafted(std::shared_ptr<ItemInstance> item);
    virtual void handleUIInput(int iAction);
    virtual void setAsCurrentTask(bool active = true);
    virtual bool ShowMinimumTime();
    virtual bool hasBeenActivated();
    virtual void setShownForMinimumTime();
    virtual bool AllowFade();

    virtual void useItemOn(Level* level, std::shared_ptr<ItemInstance> item,
                           int x, int y, int z, bool bTestUseOnly = false);
    virtual void useItem(std::shared_ptr<ItemInstance> item,
                         bool bTestUseOnly = false);
    virtual void onTake(std::shared_ptr<ItemInstance> item,
                        unsigned int invItemCountAnyAux,
                        unsigned int invItemCountThisAux);
    virtual void onStateChange(eTutorial_State newState);

private:
    std::vector<TutorialTask*> m_taskSequence;
};