#include "CompleteAllRuleDefinition.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "app/common/GameRules/LevelRules/RuleDefinitions/CompoundGameRuleDefinition.h"
#include "app/common/Game.h"
#include "minecraft/network/Connection.h"
#include "minecraft/network/packet/UpdateGameRuleProgressPacket.h"
#include "minecraft/world/level/GameRules/GameRule.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "util/StringHelpers.h"

void CompleteAllRuleDefinition::getChildren(
    std::vector<GameRuleDefinition*>* children) {
    CompoundGameRuleDefinition::getChildren(children);
}

bool CompleteAllRuleDefinition::onUseTile(GameRule* rule, int tileId, int x,
                                          int y, int z) {
    bool statusChanged =
        CompoundGameRuleDefinition::onUseTile(rule, tileId, x, y, z);
    if (statusChanged) updateStatus(rule);
    return statusChanged;
}

bool CompleteAllRuleDefinition::onCollectItem(
    GameRule* rule, std::shared_ptr<ItemInstance> item) {
    bool statusChanged = CompoundGameRuleDefinition::onCollectItem(rule, item);
    if (statusChanged) updateStatus(rule);
    return statusChanged;
}

void CompleteAllRuleDefinition::updateStatus(GameRule* rule) {
    int goal = 0;
    int progress = 0;
    for (auto it = rule->m_parameters.begin(); it != rule->m_parameters.end();
         ++it) {
        if (it->second.isPointer) {
            goal += it->second.gr->getGameRuleDefinition()->getGoal();
            progress += it->second.gr->getGameRuleDefinition()->getProgress(
                it->second.gr);
        }
    }
    if (rule->getConnection() != nullptr) {
        PacketData data;
        data.goal = goal;
        data.progress = progress;

        int icon = -1;
        int auxValue = 0;

        if (m_lastRuleStatusChanged != nullptr) {
            icon = m_lastRuleStatusChanged->getIcon();
            auxValue = m_lastRuleStatusChanged->getAuxValue();
            m_lastRuleStatusChanged = nullptr;
        }
        rule->getConnection()->send(
            std::shared_ptr<UpdateGameRuleProgressPacket>(
                new UpdateGameRuleProgressPacket(
                    getActionType(), this->m_descriptionId, icon, auxValue, 0,
                    &data, sizeof(PacketData))));
    }
    app.DebugPrintf("Updated CompleteAllRule - Completed %d of %d\n", progress,
                    goal);
}

std::string CompleteAllRuleDefinition::generateDescriptionString(
    const std::string& description, void* data, int dataLength) {
    PacketData* values = (PacketData*)data;
    std::string newDesc = description;
    newDesc =
        replaceAll(newDesc, "{*progress*}", toWString<int>(values->progress));
    newDesc = replaceAll(newDesc, "{*goal*}", toWString<int>(values->goal));
    return newDesc;
}