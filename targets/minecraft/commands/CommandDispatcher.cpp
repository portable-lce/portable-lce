#include "CommandDispatcher.h"

#include <string>
#include <utility>

#include "minecraft/commands/Command.h"
#include "minecraft/commands/CommandSender.h"
#include "minecraft/commands/CommandsEnum.h"
#include "minecraft/util/Log.h"

int CommandDispatcher::performCommand(std::shared_ptr<CommandSender> sender,
                                      EGameCommand command,
                                      std::vector<uint8_t>& commandData) {
    auto it = commandsById.find(command);

    if (it != commandsById.end()) {
        Command* command = it->second;
        if (command->canExecute(sender)) {
            command->execute(sender, commandData);
        } else {
#ifndef _CONTENT_PACKAGE
            sender->sendMessage(
                "\u00A7cYou do not have permission to use this command.");
#endif
        }
    } else {
        Log::info("Command %d not found!\n", command);
    }

    return 0;
}

Command* CommandDispatcher::addCommand(Command* command) {
    commandsById[command->getId()] = command;
    commands.insert(command);
    return command;
}