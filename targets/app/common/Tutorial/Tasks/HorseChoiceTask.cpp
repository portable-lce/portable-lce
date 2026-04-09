#include "HorseChoiceTask.h"

#include <memory>

#include "app/common/Tutorial/Tasks/ChoiceTask.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "java/Class.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/animal/EntityHorse.h"

class Tutorial;

HorseChoiceTask::HorseChoiceTask(Tutorial* tutorial, int iDescHorse,
                                 int iDescDonkey, int iDescMule, int iPromptId,
                                 bool requiresUserInput, int iConfirmMapping,
                                 int iCancelMapping,
                                 eTutorial_CompletionAction cancelAction)

    : ChoiceTask(tutorial, -1, iPromptId, requiresUserInput, iConfirmMapping,
                 iCancelMapping, cancelAction) {
    m_eHorseType = -1;
    m_iDescMule = iDescMule;
    m_iDescDonkey = iDescDonkey;
    m_iDescHorse = iDescHorse;
}

int HorseChoiceTask::getDescriptionId() {
    switch (m_eHorseType) {
        case EntityHorse::TYPE_HORSE:
            return m_iDescHorse;
        case EntityHorse::TYPE_DONKEY:
            return m_iDescDonkey;
        case EntityHorse::TYPE_MULE:
            return m_iDescMule;
        default:
            return -1;
    }
    return -1;
}

void HorseChoiceTask::onLookAtEntity(std::shared_ptr<Entity> entity) {
    if ((m_eHorseType < 0) && entity->instanceof (eTYPE_HORSE)) {
        std::shared_ptr<EntityHorse> horse =
            std::dynamic_pointer_cast<EntityHorse>(entity);
        if (horse->isAdult()) m_eHorseType = horse->getType();
    }
}