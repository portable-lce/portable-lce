#include "TextureAndGeometryPacket.h"

#include <sstream>
#include <vector>

#include "minecraft/Minecraft_Macros.h"
#include "app/common/DLC/DLCSkinFile.h"
#include "PacketListener.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"

TextureAndGeometryPacket::TextureAndGeometryPacket() {
    this->textureName = "";
    this->dwTextureBytes = 0;
    this->pbData = nullptr;
    this->dwBoxC = 0;
    this->BoxDataA = nullptr;
    uiAnimOverrideBitmask = 0;
}

TextureAndGeometryPacket::~TextureAndGeometryPacket() {
    // can't free these - they're used elsewhere
    // 	if(this->BoxDataA!=nullptr)
    // 	{
    // 		delete [] this->BoxDataA;
    // 	}
    //
    // 	if(this->pbData!=nullptr)
    // 	{
    // 		delete [] this->pbData;
    // 	}
}

TextureAndGeometryPacket::TextureAndGeometryPacket(
    const std::string& textureName, std::uint8_t* pbData,
    std::uint32_t dataBytes) {
    this->textureName = textureName;

    std::string skinValue = textureName.substr(7, textureName.size());
    skinValue = skinValue.substr(0, skinValue.find_first_of('.'));
    std::stringstream ss;
    ss << std::dec << skinValue.c_str();
    ss >> this->dwSkinID;
    this->dwSkinID = MAKE_SKIN_BITMASK(true, this->dwSkinID);
    this->pbData = pbData;
    this->dwTextureBytes = dataBytes;
    this->dwBoxC = 0;
    this->BoxDataA = nullptr;
    this->uiAnimOverrideBitmask = 0;
}

TextureAndGeometryPacket::TextureAndGeometryPacket(
    const std::string& textureName, std::uint8_t* pbData,
    std::uint32_t dataBytes, DLCSkinFile* pDLCSkinFile) {
    this->textureName = textureName;

    std::string skinValue = textureName.substr(7, textureName.size());
    skinValue = skinValue.substr(0, skinValue.find_first_of('.'));
    std::stringstream ss;
    ss << std::dec << skinValue.c_str();
    ss >> this->dwSkinID;
    this->dwSkinID = MAKE_SKIN_BITMASK(true, this->dwSkinID);

    this->pbData = pbData;
    this->dwTextureBytes = dataBytes;
    this->uiAnimOverrideBitmask = pDLCSkinFile->getAnimOverrideBitmask();
    this->dwBoxC = pDLCSkinFile->getAdditionalBoxesCount();
    if (this->dwBoxC != 0) {
        this->BoxDataA = new SKIN_BOX[this->dwBoxC];
        std::vector<SKIN_BOX*>* pSkinBoxes = pDLCSkinFile->getAdditionalBoxes();
        int iCount = 0;

        for (auto it = pSkinBoxes->begin(); it != pSkinBoxes->end(); ++it) {
            SKIN_BOX* pSkinBox = *it;
            this->BoxDataA[iCount++] = *pSkinBox;
        }
    } else {
        this->BoxDataA = nullptr;
    }
}

TextureAndGeometryPacket::TextureAndGeometryPacket(
    const std::string& textureName, std::uint8_t* pbData,
    std::uint32_t dataBytes, std::vector<SKIN_BOX*>* pvSkinBoxes,
    unsigned int uiAnimOverrideBitmask) {
    this->textureName = textureName;

    std::string skinValue = textureName.substr(7, textureName.size());
    skinValue = skinValue.substr(0, skinValue.find_first_of('.'));
    std::stringstream ss;
    ss << std::dec << skinValue.c_str();
    ss >> this->dwSkinID;
    this->dwSkinID = MAKE_SKIN_BITMASK(true, this->dwSkinID);

    this->pbData = pbData;
    this->dwTextureBytes = dataBytes;
    this->uiAnimOverrideBitmask = uiAnimOverrideBitmask;
    if (pvSkinBoxes == nullptr) {
        this->dwBoxC = 0;
        this->BoxDataA = nullptr;
    } else {
        this->dwBoxC = (std::uint32_t)pvSkinBoxes->size();
        this->BoxDataA = new SKIN_BOX[this->dwBoxC];
        int iCount = 0;

        for (auto it = pvSkinBoxes->begin(); it != pvSkinBoxes->end(); ++it) {
            SKIN_BOX* pSkinBox = *it;
            this->BoxDataA[iCount++] = *pSkinBox;
        }
    }
}

void TextureAndGeometryPacket::handle(PacketListener* listener) {
    listener->handleTextureAndGeometry(shared_from_this());
}

void TextureAndGeometryPacket::read(DataInputStream* dis)  // throws IOException
{
    textureName = dis->readUTF();
    dwSkinID = static_cast<std::uint32_t>(dis->readInt());
    dwTextureBytes = (std::uint32_t)dis->readShort();

    if (dwTextureBytes > 0) {
        this->pbData = new std::uint8_t[dwTextureBytes];

        for (std::uint32_t i = 0; i < dwTextureBytes; i++) {
            this->pbData[i] = dis->readByte();
        }
    }
    uiAnimOverrideBitmask = dis->readInt();

    dwBoxC = (std::uint32_t)dis->readShort();

    if (dwBoxC > 0) {
        this->BoxDataA = new SKIN_BOX[dwBoxC];
    }

    for (std::uint32_t i = 0; i < dwBoxC; i++) {
        this->BoxDataA[i].ePart = (eBodyPart)dis->readShort();
        this->BoxDataA[i].fX = dis->readFloat();
        this->BoxDataA[i].fY = dis->readFloat();
        this->BoxDataA[i].fZ = dis->readFloat();
        this->BoxDataA[i].fH = dis->readFloat();
        this->BoxDataA[i].fW = dis->readFloat();
        this->BoxDataA[i].fD = dis->readFloat();
        this->BoxDataA[i].fU = dis->readFloat();
        this->BoxDataA[i].fV = dis->readFloat();
    }
}

void TextureAndGeometryPacket::write(
    DataOutputStream* dos)  // throws IOException
{
    dos->writeUTF(textureName);
    dos->writeInt(static_cast<int>(dwSkinID));
    dos->writeShort((short)dwTextureBytes);
    for (std::uint32_t i = 0; i < dwTextureBytes; i++) {
        dos->writeByte(this->pbData[i]);
    }
    dos->writeInt(uiAnimOverrideBitmask);

    dos->writeShort((short)dwBoxC);
    for (std::uint32_t i = 0; i < dwBoxC; i++) {
        dos->writeShort((short)this->BoxDataA[i].ePart);
        dos->writeFloat(this->BoxDataA[i].fX);
        dos->writeFloat(this->BoxDataA[i].fY);
        dos->writeFloat(this->BoxDataA[i].fZ);
        dos->writeFloat(this->BoxDataA[i].fH);
        dos->writeFloat(this->BoxDataA[i].fW);
        dos->writeFloat(this->BoxDataA[i].fD);
        dos->writeFloat(this->BoxDataA[i].fU);
        dos->writeFloat(this->BoxDataA[i].fV);
    }
}

int TextureAndGeometryPacket::getEstimatedSize() {
    return 4096 + +sizeof(int) + sizeof(float) * 8 * 4;
}
