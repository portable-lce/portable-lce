#include "ControllerTask.h"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/common/Game.h"
#include "app/common/Tutorial/Constraints/InputConstraint.h"
#include "app/common/Tutorial/Tasks/TutorialTask.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "platform/input/input.h"

class Tutorial;

ControllerTask::ControllerTask(Tutorial* tutorial, int descriptionId,
                               bool enablePreCompletion, bool showMinimumTime,
                               int mappings[], unsigned int mappingsLength,
                               int iCompletionMaskA[],
                               int iCompletionMaskACount,
                               int iSouthpawMappings[],
                               unsigned int uiSouthpawMappingsCount)
    : TutorialTask(tutorial, descriptionId, enablePreCompletion, nullptr,
                   showMinimumTime) {
    for (unsigned int i = 0; i < mappingsLength; ++i) {
        constraints.push_back(new InputConstraint(mappings[i]));
        completedMappings[mappings[i]] = false;
    }
    if (uiSouthpawMappingsCount > 0) m_bHasSouthpaw = true;
    for (unsigned int i = 0; i < uiSouthpawMappingsCount; ++i) {
        southpawCompletedMappings[iSouthpawMappings[i]] = false;
    }

    m_iCompletionMaskA = new int[iCompletionMaskACount];
    for (int i = 0; i < iCompletionMaskACount; i++) {
        m_iCompletionMaskA[i] = iCompletionMaskA[i];
    }
    m_iCompletionMaskACount = iCompletionMaskACount;
    m_uiCompletionMask = 0;

    // If we don't want to be able to complete it early..then assume we want the
    // constraints active
    // if( !enablePreCompletion )
    //	enableConstraints( true );

    m_initialized = false;  // we can set yaw + pitch on the first tick
}

ControllerTask::~ControllerTask() { delete[] m_iCompletionMaskA; }

bool ControllerTask::isCompleted() {
    if (bIsCompleted) return true;

    Minecraft* pMinecraft = Minecraft::GetInstance();

    // mouse look check
    if (!m_initialized) {
        m_lastYaw = pMinecraft->player->yRot;
        m_lastPitch = pMinecraft->player->xRot;
        m_initialized = true;
    } else {
        float deltaYaw = fabs(pMinecraft->player->yRot - m_lastYaw);
        float deltaPitch = fabs(pMinecraft->player->xRot - m_lastPitch);
        m_lastYaw = pMinecraft->player->yRot;
        m_lastPitch = pMinecraft->player->xRot;

        const float LOOK_THRESHOLD = 0.1f;
        if (deltaYaw > LOOK_THRESHOLD || deltaPitch > LOOK_THRESHOLD)
            return true;
    }

    // check for controller button input
    bool bAllComplete = true;
    int iCurrent = 0;

    if (m_bHasSouthpaw && app.GetGameSettings(pMinecraft->player->GetXboxPad(),
                                              eGameSetting_ControlSouthPaw)) {
        for (auto it = southpawCompletedMappings.begin();
             it != southpawCompletedMappings.end(); ++it) {
            if (!it->second) {
                if (PlatformInput.GetValue(pMinecraft->player->GetXboxPad(),
                                           it->first) > 0) {
                    it->second = true;
                    m_uiCompletionMask |= 1 << iCurrent;
                } else {
                    bAllComplete = false;
                }
            }
            iCurrent++;
        }
    } else {
        for (auto it = completedMappings.begin(); it != completedMappings.end();
             ++it) {
            if (!it->second) {
                if (PlatformInput.GetValue(pMinecraft->player->GetXboxPad(),
                                           it->first) > 0) {
                    it->second = true;
                    m_uiCompletionMask |= 1 << iCurrent;
                } else {
                    bAllComplete = false;
                }
            }
            iCurrent++;
        }
    }

    // completion mask check
    if (m_iCompletionMaskA && CompletionMaskIsValid())
        bIsCompleted = true;
    else
        bIsCompleted = bAllComplete;

    return bIsCompleted;
}

bool ControllerTask::CompletionMaskIsValid() {
    for (int i = 0; i < m_iCompletionMaskACount; i++) {
        if (m_uiCompletionMask == m_iCompletionMaskA[i]) return true;
    }

    return false;
}
void ControllerTask::setAsCurrentTask(bool active /*= true*/) {
    TutorialTask::setAsCurrentTask(active);
    enableConstraints(!active);
}
