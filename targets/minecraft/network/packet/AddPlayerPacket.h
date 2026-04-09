#pragma once

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "Packet.h"
#include "minecraft/network/packet/Packet.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "platform/PlatformTypes.h"

class Player;

class AddPlayerPacket : public Packet,
                        public std::enable_shared_from_this<AddPlayerPacket> {
private:
    std::shared_ptr<SynchedEntityData> entityData;
    std::vector<std::shared_ptr<SynchedEntityData::DataItem> >* unpack;

public:
    int id;
    std::string name;
    int x, y, z;
    char yRot, xRot;
    int carriedItem;
    PlayerUID xuid;                   // 4J Added
    PlayerUID OnlineXuid;             // 4J Added
    std::uint8_t m_playerIndex;       // 4J Added
    std::uint32_t m_skinId;           // 4J Added
    std::uint32_t m_capeId;           // 4J Added
    unsigned int m_uiGamePrivileges;  // 4J Added
    std::uint8_t yHeadRot;            // 4J Added

    AddPlayerPacket();
    ~AddPlayerPacket();
    AddPlayerPacket(std::shared_ptr<Player> player, PlayerUID xuid,
                    PlayerUID OnlineXuid, int xp, int yp, int zp, int yRotp,
                    int xRotp, int yHeadRotp);

    virtual void read(DataInputStream* dis);
    virtual void write(DataOutputStream* dos);
    virtual void handle(PacketListener* listener);
    virtual int getEstimatedSize();

    std::vector<std::shared_ptr<SynchedEntityData::DataItem> >*
    getUnpackedData();

public:
    static std::shared_ptr<Packet> create() {
        return std::make_shared<AddPlayerPacket>();
    }
    virtual int getId() { return 20; }
};
