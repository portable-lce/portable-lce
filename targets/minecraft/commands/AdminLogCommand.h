#pragma once

#include "minecraft/network/packet/ChatPacket.h"

class CommandSender;

class AdminLogCommand {
public:
    static const int LOGTYPE_DONT_SHOW_TO_SELF = 1;

    virtual void logAdminCommand(std::shared_ptr<CommandSender> source,
                                 int type,
                                 ChatPacket::EChatPacketMessage messageType,
                                 const std::string& message = "",
                                 int customData = -1,
                                 const std::string& additionalMessage = "") = 0;
};