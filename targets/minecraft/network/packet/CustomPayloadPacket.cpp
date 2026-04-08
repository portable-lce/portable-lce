#include "minecraft/util/Log.h"
#include "CustomPayloadPacket.h"

#include <limits>

#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"

// Mojang-defined custom packets
const std::string CustomPayloadPacket::CUSTOM_BOOK_PACKET = "MC|BEdit";
const std::string CustomPayloadPacket::CUSTOM_BOOK_SIGN_PACKET = "MC|BSign";
const std::string CustomPayloadPacket::TEXTURE_PACK_PACKET = "MC|TPack";
const std::string CustomPayloadPacket::TRADER_LIST_PACKET = "MC|TrList";
const std::string CustomPayloadPacket::TRADER_SELECTION_PACKET = "MC|TrSel";
const std::string CustomPayloadPacket::SET_ADVENTURE_COMMAND_PACKET =
    "MC|AdvCdm";
const std::string CustomPayloadPacket::SET_BEACON_PACKET = "MC|Beacon";
const std::string CustomPayloadPacket::SET_ITEM_NAME_PACKET = "MC|ItemName";

CustomPayloadPacket::CustomPayloadPacket() {}

CustomPayloadPacket::CustomPayloadPacket(const std::string& identifier,
                                         std::vector<uint8_t> data) {
    this->identifier = identifier;
    this->data = data;

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

void CustomPayloadPacket::read(DataInputStream* dis) {
    identifier = readUtf(dis, 20);
    length = dis->readShort();

    if (length > 0 && length < std::numeric_limits<short>::max()) {
        data = std::vector<uint8_t>(length);
        dis->readFully(data);
    }
}

void CustomPayloadPacket::write(DataOutputStream* dos) {
    writeUtf(identifier, dos);
    dos->writeShort((short)length);
    if (!data.empty()) {
        dos->write(data);
    }
}

void CustomPayloadPacket::handle(PacketListener* listener) {
    listener->handleCustomPayload(shared_from_this());
}

int CustomPayloadPacket::getEstimatedSize() {
    return 2 + identifier.length() * 2 + 2 + length;
}
