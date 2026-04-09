#include "StatTask.h"

#include "app/common/Tutorial/Tasks/TutorialTask.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/stats/StatsCounter.h"
#include "platform/profile/profile.h"

class Tutorial;

StatTask::StatTask(Tutorial* tutorial, int descriptionId,
                   bool enablePreCompletion, Stat* stat, int variance /*= 1*/)
    : TutorialTask(tutorial, descriptionId, enablePreCompletion, nullptr) {
    this->stat = stat;

    Minecraft* minecraft = Minecraft::GetInstance();
    targetValue =
        minecraft->stats[PlatformProfile.GetPrimaryPad()]->getTotalValue(stat) +
        variance;
}

bool StatTask::isCompleted() {
    if (bIsCompleted) return true;

    Minecraft* minecraft = Minecraft::GetInstance();
    bIsCompleted =
        minecraft->stats[PlatformProfile.GetPrimaryPad()]->getTotalValue(
            stat) >= (unsigned int)targetValue;
    return bIsCompleted;
}