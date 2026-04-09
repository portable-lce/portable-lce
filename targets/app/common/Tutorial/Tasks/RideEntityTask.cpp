#include "RideEntityTask.h"

#include <memory>

#include "app/common/Tutorial/Tasks/TutorialTask.h"
#include "java/Class.h"
#include "minecraft/world/entity/Entity.h"

class Tutorial;
class TutorialConstraint;

RideEntityTask::RideEntityTask(const int eType, Tutorial* tutorial,
                               int descriptionId, bool enablePreCompletion,
                               std::vector<TutorialConstraint*>* inConstraints,
                               bool bShowMinimumTime, bool bAllowFade,
                               bool bTaskReminders)
    : TutorialTask(tutorial, descriptionId, enablePreCompletion, inConstraints,
                   bShowMinimumTime, bAllowFade, bTaskReminders),
      m_eType(eType) {}

bool RideEntityTask::isCompleted() { return bIsCompleted; }

void RideEntityTask::onRideEntity(std::shared_ptr<Entity> entity) {
    if (entity->instanceof ((eINSTANCEOF)m_eType)) {
        bIsCompleted = true;
    }
}