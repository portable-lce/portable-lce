#include "minecraft/util/Log.h"
#include "GameCommandPacket.h"

#include <limits>

#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/commands/CommandsEnum.h"

GameCommandPacket::GameCommandPacket() { length = 0; }

GameCommandPacket::~GameCommandPacket() {}

GameCommandPacket::GameCommandPacket(EGameCommand command,
                                     std::vector<uint8_t> data) {
    this->command = command;
    this->data = data;
    length = 0;

    if (!data.empty()) {
        length = data.size();

        if (length > std::numeric_limits<short>::max()) {
            Log::info("Payload may not be larger than 32K\n");
#ifndef _CONTENT_PACKAGE
            assert(0);
#endif
            // throw new IllegalArgumentException("Payload may not be larger
            // than 32k");
        }
    }
}

void GameCommandPacket::read(DataInputStream* dis) {
    command = (EGameCommand)dis->readInt();
    length = dis->readShort();

    if (length > 0 && length < std::numeric_limits<short>::max()) {
        data = std::vector<uint8_t>(length);
        dis->readFully(data);
    }
}

void GameCommandPacket::write(DataOutputStream* dos) {
    dos->writeInt(command);
    dos->writeShort((short)length);
    if (!data.empty()) {
        dos->write(data);
    }
}

void GameCommandPacket::handle(PacketListener* listener) {
    listener->handleGameCommand(shared_from_this());
}

int GameCommandPacket::getEstimatedSize() { return 2 + 2 + length; }
