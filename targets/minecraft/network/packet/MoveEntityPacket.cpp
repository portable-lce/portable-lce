#include "MoveEntityPacket.h"

#include <stdint.h>

#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/network/packet/Packet.h"

MoveEntityPacket::MoveEntityPacket() {
    hasRot = false;

    id = -1;
    xa = 0;
    ya = 0;
    za = 0;
    yRot = 0;
    xRot = 0;
}

MoveEntityPacket::MoveEntityPacket(int id) {
    this->id = id;
    hasRot = false;

    xa = 0;
    ya = 0;
    za = 0;
    yRot = 0;
    xRot = 0;
}

void MoveEntityPacket::read(DataInputStream* dis)  // throws IOException
{
    id = dis->readShort();
}

void MoveEntityPacket::write(DataOutputStream* dos)  // throws IOException
{
    if ((id < 0) || (id >= 2048)) {
        // We shouln't be tracking an entity that doesn't have a short type of
        // id
        assert(0);
    }
    dos->writeShort((short)id);
}

void MoveEntityPacket::handle(PacketListener* listener) {
    listener->handleMoveEntity(shared_from_this());
}

int MoveEntityPacket::getEstimatedSize() { return 2; }

bool MoveEntityPacket::canBeInvalidated() { return true; }

bool MoveEntityPacket::isInvalidatedBy(std::shared_ptr<Packet> packet) {
    std::shared_ptr<MoveEntityPacket> target =
        std::dynamic_pointer_cast<MoveEntityPacket>(packet);
    return target != nullptr && target->id == id;
}

MoveEntityPacket::PosRot::PosRot() { hasRot = true; }

MoveEntityPacket::PosRot::PosRot(int id, char xa, char ya, char za, char yRot,
                                 char xRot)
    : MoveEntityPacket(id) {
    this->xa = xa;
    this->ya = ya;
    this->za = za;
    this->yRot = yRot;
    this->xRot = xRot;
    hasRot = true;
}

void MoveEntityPacket::PosRot::read(DataInputStream* dis)  // throws IOException
{
    MoveEntityPacket::read(dis);
    xa = (int)dis->readByte();
    ya = (int)dis->readByte();
    za = (int)dis->readByte();
    yRot = (int)dis->readByte();
    xRot = (int)dis->readByte();
}

void MoveEntityPacket::PosRot::write(
    DataOutputStream* dos)  // throws IOException
{
    MoveEntityPacket::write(dos);
    dos->writeByte((uint8_t)xa);
    dos->writeByte((uint8_t)ya);
    dos->writeByte((uint8_t)za);
    dos->writeByte((uint8_t)yRot);
    dos->writeByte((uint8_t)xRot);
}

int MoveEntityPacket::PosRot::getEstimatedSize() { return 2 + 5; }

MoveEntityPacket::Pos::Pos() {}

MoveEntityPacket::Pos::Pos(int id, char xa, char ya, char za)
    : MoveEntityPacket(id) {
    this->xa = xa;
    this->ya = ya;
    this->za = za;
}

void MoveEntityPacket::Pos::read(DataInputStream* dis)  // throws IOException
{
    MoveEntityPacket::read(dis);
    xa = (int)dis->readByte();
    ya = (int)dis->readByte();
    za = (int)dis->readByte();
}

void MoveEntityPacket::Pos::write(DataOutputStream* dos)  // throws IOException
{
    MoveEntityPacket::write(dos);
    dos->writeByte((uint8_t)xa);
    dos->writeByte((uint8_t)ya);
    dos->writeByte((uint8_t)za);
}

int MoveEntityPacket::Pos::getEstimatedSize() { return 2 + 3; }

MoveEntityPacket::Rot::Rot() { hasRot = true; }

MoveEntityPacket::Rot::Rot(int id, char yRot, char xRot)
    : MoveEntityPacket(id) {
    this->yRot = yRot;
    this->xRot = xRot;
    hasRot = true;
}

void MoveEntityPacket::Rot::read(DataInputStream* dis)  // throws IOException
{
    MoveEntityPacket::read(dis);
    yRot = (int)dis->readByte();
    xRot = (int)dis->readByte();
}

void MoveEntityPacket::Rot::write(DataOutputStream* dos)  // throws IOException
{
    MoveEntityPacket::write(dos);
    dos->writeByte((uint8_t)yRot);
    dos->writeByte((uint8_t)xRot);
}

int MoveEntityPacket::Rot::getEstimatedSize() { return 2 + 2; }
