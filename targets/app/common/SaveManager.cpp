#include "app/common/SaveManager.h"

#include <chrono>

#include "app/common/Game.h"
#include "app/common/Network/GameNetworkManager.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/ServerAction.h"
#include "platform/profile/profile.h"

void SaveManager::setAutosaveTimerTime(int settingValue) {
    m_uiAutosaveTimer =
        time_util::clock::now() + std::chrono::minutes(settingValue * 15);
}

bool SaveManager::autosaveDue() const {
    return (time_util::clock::now() > m_uiAutosaveTimer);
}

int64_t SaveManager::secondsToAutosave() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
               m_uiAutosaveTimer - time_util::clock::now())
        .count();
}

void SaveManager::lock() {
    std::lock_guard<std::mutex> lock(m_saveNotificationMutex);
    if (m_saveNotificationDepth++ == 0) {
        if (g_NetworkManager
                .IsInSession())  // this can be triggered from the front end if
                                 // we're downloading a save
        {
            MinecraftServer::getInstance()->broadcastStartSavingPacket();

            if (g_NetworkManager.IsLocalGame() &&
                g_NetworkManager.GetPlayerCount() == 1) {
                MinecraftServer::getInstance()->queueServerAction(
                    minecraft::server::PauseServer{true});
            }
        }
    }
}

void SaveManager::unlock() {
    std::lock_guard<std::mutex> lock(m_saveNotificationMutex);
    if (--m_saveNotificationDepth == 0) {
        if (g_NetworkManager
                .IsInSession())  // this can be triggered from the front end if
                                 // we're downloading a save
        {
            MinecraftServer::getInstance()->broadcastStopSavingPacket();

            if (g_NetworkManager.IsLocalGame() &&
                g_NetworkManager.GetPlayerCount() == 1) {
                MinecraftServer::getInstance()->queueServerAction(
                    minecraft::server::PauseServer{false});
            }
        }
    }
}
