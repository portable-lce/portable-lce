#include "TextureAndGeometryChangePacket.h"

#include <sstream>

#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/Minecraft_Macros.h"
#include "minecraft/world/entity/Entity.h"

TextureAndGeometryChangePacket::TextureAndGeometryChangePacket() {
    id = -1;
    path = "";
    dwSkinID = 0;
}

TextureAndGeometryChangePacket::TextureAndGeometryChangePacket(
    std::shared_ptr<Entity> e, const std::string& path) {
    id = e->entityId;
    this->path = path;
    std::string skinValue = path.substr(7, path.size());
    skinValue = skinValue.substr(0, skinValue.find_first_of('.'));
    std::stringstream ss;
    ss << std::dec << skinValue.c_str();
    ss >> dwSkinID;
    dwSkinID = MAKE_SKIN_BITMASK(true, dwSkinID);
}

void TextureAndGeometryChangePacket::read(
    DataInputStream* dis)  // throws IOException
{
    id = dis->readInt();
    dwSkinID = static_cast<std::uint32_t>(dis->readInt());
    path = dis->readUTF();
}

void TextureAndGeometryChangePacket::write(
    DataOutputStream* dos)  // throws IOException
{
    dos->writeInt(id);
    dos->writeInt(static_cast<int>(dwSkinID));
    dos->writeUTF(path);
}

void TextureAndGeometryChangePacket::handle(PacketListener* listener) {
    listener->handleTextureAndGeometryChange(shared_from_this());
}

int TextureAndGeometryChangePacket::getEstimatedSize() {
    return 8 + (int)path.size();
}
