#include "ChoiceTask.h"

#include <memory>
#include <vector>

#include "app/common/Tutorial/Constraints/InputConstraint.h"
#include "app/common/Tutorial/Tasks/TutorialTask.h"
#include "app/common/Tutorial/Tutorial.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/world/level/material/Material.h"
#include "platform/input/input.h"

ChoiceTask::ChoiceTask(
    Tutorial* tutorial, int descriptionId, int promptId /*= -1*/,
    bool requiresUserInput /*= false*/, int iConfirmMapping /*= 0*/,
    int iCancelMapping /*= 0*/,
    eTutorial_CompletionAction cancelAction /*= e_Tutorial_Completion_None*/)
    : TutorialTask(tutorial, descriptionId, false, nullptr, true, false,
                   false) {
    if (requiresUserInput == true) {
        constraints.push_back(new InputConstraint(iConfirmMapping));
        constraints.push_back(new InputConstraint(iCancelMapping));
    }
    m_iConfirmMapping = iConfirmMapping;
    m_iCancelMapping = iCancelMapping;
    m_bConfirmMappingComplete = false;
    m_bCancelMappingComplete = false;

    m_cancelAction = cancelAction;

    m_promptId = promptId;
    tutorial->addMessage(m_promptId);
}

bool ChoiceTask::isCompleted() {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (m_bConfirmMappingComplete || m_bCancelMappingComplete) {
        enableConstraints(false, true);
        return true;
    }

    if (ui.GetMenuDisplayed(tutorial->getPad())) {
        // If a menu is displayed, then we use the handleUIInput to complete the
        // task
    } else {
        // If the player is under water then allow all keypresses so they can
        // jump out
        if (pMinecraft->localplayers[tutorial->getPad()]->isUnderLiquid(
                Material::water))
            return false;

        if (!m_bConfirmMappingComplete &&
            PlatformInput.GetValue(pMinecraft->player->GetXboxPad(),
                                   m_iConfirmMapping) > 0) {
            m_bConfirmMappingComplete = true;
        }
        if (!m_bCancelMappingComplete &&
            PlatformInput.GetValue(pMinecraft->player->GetXboxPad(),
                                   m_iCancelMapping) > 0) {
            m_bCancelMappingComplete = true;
        }
    }

    if (m_bConfirmMappingComplete || m_bCancelMappingComplete) {
        enableConstraints(false, true);
    }
    return m_bConfirmMappingComplete || m_bCancelMappingComplete;
}

eTutorial_CompletionAction ChoiceTask::getCompletionAction() {
    if (m_bCancelMappingComplete) {
        return m_cancelAction;
    } else {
        return e_Tutorial_Completion_None;
    }
}

int ChoiceTask::getPromptId() {
    if (m_bShownForMinimumTime)
        return m_promptId;
    else
        return -1;
}

void ChoiceTask::setAsCurrentTask(bool active /*= true*/) {
    enableConstraints(active);
    TutorialTask::setAsCurrentTask(active);
}

void ChoiceTask::handleUIInput(int iAction) {
    if (bHasBeenActivated && m_bShownForMinimumTime) {
        if (iAction == m_iConfirmMapping) {
            m_bConfirmMappingComplete = true;
        } else if (iAction == m_iCancelMapping) {
            m_bCancelMappingComplete = true;
        }
    }
}
