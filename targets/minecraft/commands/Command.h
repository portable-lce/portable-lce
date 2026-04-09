#pragma once

// 4J Stu - Based loosely on the Java versions

#include <stdint.h>

#include <format>
#include <memory>
#include <string>
#include <vector>

#include "CommandsEnum.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "platform/PlatformTypes.h"

class AdminLogCommand;
class CommandSender;
class ServerPlayer;

class Command {
public:
    // commands such as "help" and "emote"
    static const int LEVEL_ALL = 0;
    // commands such as "mute"
    static const int LEVEL_MODERATORS = 1;
    // commands such as "seed", "tp", "spawnpoint" and "give"
    static const int LEVEL_GAMEMASTERS = 2;
    // commands such as "whitelist", "ban", etc
    static const int LEVEL_ADMINS = 3;
    // commands such as "stop", "save-all", etc
    static const int LEVEL_OWNERS = 4;

private:
    static AdminLogCommand* logger;

public:
    virtual EGameCommand getId() = 0;
    virtual int getPermissionLevel();
    virtual void execute(std::shared_ptr<CommandSender> source,
                         std::vector<uint8_t>& commandData) = 0;
    virtual bool canExecute(std::shared_ptr<CommandSender> source);

    static void logAdminAction(std::shared_ptr<CommandSender> source,
                               ChatPacket::EChatPacketMessage messageType,
                               const std::string& message = "",
                               int customData = -1,
                               const std::string& additionalMessage = "");
    static void logAdminAction(std::shared_ptr<CommandSender> source, int type,
                               ChatPacket::EChatPacketMessage messageType,
                               const std::string& message = "",
                               int customData = -1,
                               const std::string& additionalMessage = "");
    static void setLogger(AdminLogCommand* logger);

protected:
    std::shared_ptr<ServerPlayer> getPlayer(PlayerUID playerId);
};