#include "RemoveEntitiesPacket.h"

#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"

RemoveEntitiesPacket::RemoveEntitiesPacket() {}

RemoveEntitiesPacket::RemoveEntitiesPacket(std::vector<int>& ids) {
    this->ids = ids;
}

RemoveEntitiesPacket::~RemoveEntitiesPacket() {}

void RemoveEntitiesPacket::read(DataInputStream* dis)  // throws IOException
{
    ids = std::vector<int>(dis->readByte());
    for (unsigned int i = 0; i < ids.size(); ++i) {
        ids[i] = dis->readInt();
    }
}

void RemoveEntitiesPacket::write(DataOutputStream* dos)  // throws IOException
{
    dos->writeByte(ids.size());
    for (unsigned int i = 0; i < ids.size(); ++i) {
        dos->writeInt(ids[i]);
    }
}

void RemoveEntitiesPacket::handle(PacketListener* listener) {
    listener->handleRemoveEntity(shared_from_this());
}

int RemoveEntitiesPacket::getEstimatedSize() { return 1 + (ids.size() * 4); }

/*
        4J: These are necesary on the PS3.
                (and 4).
*/
#if 1
const int RemoveEntitiesPacket::MAX_PER_PACKET;
#endif
