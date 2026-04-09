#include "ChangeStateConstraint.h"

#include <memory>

#include "app/common/Tutorial/Constraints/TutorialConstraint.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/Tutorial/TutorialEnum.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/network/packet/PlayerInfoPacket.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/LevelSettings.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/Vec3.h"

ChangeStateConstraint::ChangeStateConstraint(
    Tutorial* tutorial, eTutorial_State targetState,
    eTutorial_State sourceStates[], std::size_t sourceStatesCount, double x0,
    double y0, double z0, double x1, double y1, double z1,
    bool contains /*= true*/, bool changeGameMode /*= false*/,
    GameType* targetGameMode /*= 0*/)
    : TutorialConstraint(-1) {
    movementArea = AABB(x0, y0, z0, x1, y1, z1);

    this->contains = contains;

    m_changeGameMode = changeGameMode;
    m_targetGameMode = targetGameMode;
    m_changedFromGameMode = 0;

    m_tutorial = tutorial;
    m_targetState = targetState;
    m_sourceStatesCount = sourceStatesCount;

    m_bHasChanged = false;
    m_changedFromState = e_Tutorial_State_None;

    m_bComplete = false;

    m_sourceStates = new eTutorial_State[m_sourceStatesCount];
    for (unsigned int i = 0; i < m_sourceStatesCount; i++) {
        m_sourceStates[i] = sourceStates[i];
    }
}

ChangeStateConstraint::~ChangeStateConstraint() {
    if (m_sourceStatesCount > 0) delete[] m_sourceStates;
}

void ChangeStateConstraint::tick(int iPad) {
    if (m_bComplete) return;

    if (m_tutorial->isStateCompleted(m_targetState)) {
        Minecraft* minecraft = Minecraft::GetInstance();
        if (m_changeGameMode) {
            unsigned int playerPrivs =
                minecraft->localplayers[iPad]->getAllPlayerGamePrivileges();
            Player::setPlayerGamePrivilege(
                playerPrivs, Player::ePlayerGamePrivilege_CreativeMode,
                m_changedFromGameMode == GameType::CREATIVE);

            unsigned int originalPrivileges =
                minecraft->localplayers[iPad]->getAllPlayerGamePrivileges();
            if (originalPrivileges != playerPrivs) {
                // Send update settings packet to server
                Minecraft* pMinecraft = Minecraft::GetInstance();
                std::shared_ptr<MultiplayerLocalPlayer> player =
                    minecraft->localplayers[iPad];
                if (player != nullptr && player->connection &&
                    player->connection->getNetworkPlayer() != nullptr) {
                    player->connection->send(
                        std::shared_ptr<PlayerInfoPacket>(new PlayerInfoPacket(
                            player->connection->getNetworkPlayer()
                                ->GetSmallId(),
                            -1, playerPrivs)));
                }
            }
        }
        m_bComplete = true;
        return;
    }

    bool inASourceState = false;
    Minecraft* minecraft = Minecraft::GetInstance();
    for (std::size_t i = 0; i < m_sourceStatesCount; ++i) {
        if (m_sourceStates[i] == m_tutorial->getCurrentState()) {
            inASourceState = true;
            break;
        }
    }

    // TODO: check if this can be elided
    Vec3 ipad_player = minecraft->localplayers[iPad]->getPos(1);
    if (!m_bHasChanged && inASourceState &&
        movementArea.contains(ipad_player) == contains) {
        m_bHasChanged = true;
        m_changedFromState = m_tutorial->getCurrentState();
        m_tutorial->changeTutorialState(m_targetState);

        if (m_changeGameMode) {
            if (minecraft->localgameModes[iPad] != nullptr) {
                m_changedFromGameMode =
                    minecraft->localplayers[iPad]->abilities.instabuild
                        ? GameType::CREATIVE
                        : GameType::SURVIVAL;

                unsigned int playerPrivs =
                    minecraft->localplayers[iPad]->getAllPlayerGamePrivileges();
                Player::setPlayerGamePrivilege(
                    playerPrivs, Player::ePlayerGamePrivilege_CreativeMode,
                    m_targetGameMode == GameType::CREATIVE);

                unsigned int originalPrivileges =
                    minecraft->localplayers[iPad]->getAllPlayerGamePrivileges();
                if (originalPrivileges != playerPrivs) {
                    // Send update settings packet to server
                    Minecraft* pMinecraft = Minecraft::GetInstance();
                    std::shared_ptr<MultiplayerLocalPlayer> player =
                        minecraft->localplayers[iPad];
                    if (player != nullptr && player->connection &&
                        player->connection->getNetworkPlayer() != nullptr) {
                        player->connection->send(
                            std::shared_ptr<PlayerInfoPacket>(
                                new PlayerInfoPacket(
                                    player->connection->getNetworkPlayer()
                                        ->GetSmallId(),
                                    -1, playerPrivs)));
                    }
                }
            }
        }
    } else if (m_bHasChanged &&
               movementArea.contains(ipad_player) != contains) {
        m_bHasChanged = false;
        m_tutorial->changeTutorialState(m_changedFromState);

        if (m_changeGameMode) {
            unsigned int playerPrivs =
                minecraft->localplayers[iPad]->getAllPlayerGamePrivileges();
            Player::setPlayerGamePrivilege(
                playerPrivs, Player::ePlayerGamePrivilege_CreativeMode,
                m_changedFromGameMode == GameType::CREATIVE);

            unsigned int originalPrivileges =
                minecraft->localplayers[iPad]->getAllPlayerGamePrivileges();
            if (originalPrivileges != playerPrivs) {
                // Send update settings packet to server
                Minecraft* pMinecraft = Minecraft::GetInstance();
                std::shared_ptr<MultiplayerLocalPlayer> player =
                    minecraft->localplayers[iPad];
                if (player != nullptr && player->connection &&
                    player->connection->getNetworkPlayer() != nullptr) {
                    player->connection->send(
                        std::shared_ptr<PlayerInfoPacket>(new PlayerInfoPacket(
                            player->connection->getNetworkPlayer()
                                ->GetSmallId(),
                            -1, playerPrivs)));
                }
            }
        }
    }
}
