#include "CommandBlockEntity.h"

#include <assert.h>

#include <memory>

#include "minecraft/Pos.h"
#include "minecraft/commands/CommandsEnum.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/network/packet/TileEntityDataPacket.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "nbt/CompoundTag.h"

class Level;

CommandBlockEntity::CommandBlockEntity() {
    successCount = 0;
    command = "";
    name = "@";
}

void CommandBlockEntity::setCommand(const std::string& command) {
    this->command = command;
    setChanged();
}

std::string CommandBlockEntity::getCommand() { return command; }

int CommandBlockEntity::performCommand(Level* level) {
    // 4J-JEV: Cannot decide what to do with the command field.
    assert(false);
    return 0;
}

std::string CommandBlockEntity::getName() { return name; }

void CommandBlockEntity::setName(const std::string& name) { this->name = name; }

void CommandBlockEntity::sendMessage(const std::string& message,
                                     ChatPacket::EChatPacketMessage type,
                                     int customData,
                                     const std::string& additionalMessage) {}

bool CommandBlockEntity::hasPermission(EGameCommand command) { return false; }

void CommandBlockEntity::save(CompoundTag* tag) {
    TileEntity::save(tag);
    tag->putString("Command", command);
    tag->putInt("SuccessCount", successCount);
    tag->putString("CustomName", name);
}

void CommandBlockEntity::load(CompoundTag* tag) {
    TileEntity::load(tag);
    command = tag->getString("Command");
    successCount = tag->getInt("SuccessCount");
    if (tag->contains("CustomName")) name = tag->getString("CustomName");
}

Pos* CommandBlockEntity::getCommandSenderWorldPosition() {
    return new Pos(x, y, z);
}

Level* CommandBlockEntity::getCommandSenderWorld() { return getLevel(); }

std::shared_ptr<Packet> CommandBlockEntity::getUpdatePacket() {
    CompoundTag* tag = new CompoundTag();
    save(tag);
    return std::make_shared<TileEntityDataPacket>(
        x, y, z, TileEntityDataPacket::TYPE_ADV_COMMAND, tag);
}

int CommandBlockEntity::getSuccessCount() { return successCount; }

void CommandBlockEntity::setSuccessCount(int successCount) {
    this->successCount = successCount;
}

// 4J Added
std::shared_ptr<TileEntity> CommandBlockEntity::clone() {
    std::shared_ptr<CommandBlockEntity> result =
        std::make_shared<CommandBlockEntity>();
    TileEntity::clone(result);

    result->successCount = successCount;
    result->command = command;
    result->name = name;

    return result;
}