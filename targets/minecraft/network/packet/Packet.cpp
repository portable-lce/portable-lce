#include "minecraft/util/Log.h"
#include "Packet.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "app/linux/Stubs/winapi_stubs.h"
#include "java/Exceptions.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/System.h"
#include "minecraft/network/packet/AddEntityPacket.h"
#include "minecraft/network/packet/AddExperienceOrbPacket.h"
#include "minecraft/network/packet/AddGlobalEntityPacket.h"
#include "minecraft/network/packet/AddMobPacket.h"
#include "minecraft/network/packet/AddPaintingPacket.h"
#include "minecraft/network/packet/AddPlayerPacket.h"
#include "minecraft/network/packet/AnimatePacket.h"
#include "minecraft/network/packet/AwardStatPacket.h"
#include "minecraft/network/packet/BlockRegionUpdatePacket.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/network/packet/ChunkTilesUpdatePacket.h"
#include "minecraft/network/packet/ChunkVisibilityAreaPacket.h"
#include "minecraft/network/packet/ChunkVisibilityPacket.h"
#include "minecraft/network/packet/ClientCommandPacket.h"
#include "minecraft/network/packet/ComplexItemDataPacket.h"
#include "minecraft/network/packet/ContainerAckPacket.h"
#include "minecraft/network/packet/ContainerButtonClickPacket.h"
#include "minecraft/network/packet/ContainerClickPacket.h"
#include "minecraft/network/packet/ContainerClosePacket.h"
#include "minecraft/network/packet/ContainerOpenPacket.h"
#include "minecraft/network/packet/ContainerSetContentPacket.h"
#include "minecraft/network/packet/ContainerSetDataPacket.h"
#include "minecraft/network/packet/ContainerSetSlotPacket.h"
#include "minecraft/network/packet/CraftItemPacket.h"
#include "minecraft/network/packet/CustomPayloadPacket.h"
#include "minecraft/network/packet/DebugOptionsPacket.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/EntityActionAtPositionPacket.h"
#include "minecraft/network/packet/EntityEventPacket.h"
#include "minecraft/network/packet/ExplodePacket.h"
#include "minecraft/network/packet/GameCommandPacket.h"
#include "minecraft/network/packet/GameEventPacket.h"
#include "minecraft/network/packet/GetInfoPacket.h"
#include "minecraft/network/packet/InteractPacket.h"
#include "minecraft/network/packet/KeepAlivePacket.h"
#include "minecraft/network/packet/KickPlayerPacket.h"
#include "minecraft/network/packet/LevelEventPacket.h"
#include "minecraft/network/packet/LevelParticlesPacket.h"
#include "minecraft/network/packet/LevelSoundPacket.h"
#include "minecraft/network/packet/LoginPacket.h"
#include "minecraft/network/packet/MoveEntityPacket.h"
#include "minecraft/network/packet/MoveEntityPacketSmall.h"
#include "minecraft/network/packet/MovePlayerPacket.h"
#include "minecraft/network/packet/PlayerAbilitiesPacket.h"
#include "minecraft/network/packet/PlayerActionPacket.h"
#include "minecraft/network/packet/PlayerCommandPacket.h"
#include "minecraft/network/packet/PlayerInfoPacket.h"
#include "minecraft/network/packet/PlayerInputPacket.h"
#include "minecraft/network/packet/PreLoginPacket.h"
#include "minecraft/network/packet/RemoveEntitiesPacket.h"
#include "minecraft/network/packet/RemoveMobEffectPacket.h"
#include "minecraft/network/packet/RespawnPacket.h"
#include "minecraft/network/packet/RotateHeadPacket.h"
#include "minecraft/network/packet/ServerSettingsChangedPacket.h"
#include "minecraft/network/packet/SetCarriedItemPacket.h"
#include "minecraft/network/packet/SetCreativeModeSlotPacket.h"
#include "minecraft/network/packet/SetDisplayObjectivePacket.h"
#include "minecraft/network/packet/SetEntityDataPacket.h"
#include "minecraft/network/packet/SetEntityLinkPacket.h"
#include "minecraft/network/packet/SetEntityMotionPacket.h"
#include "minecraft/network/packet/SetEquippedItemPacket.h"
#include "minecraft/network/packet/SetExperiencePacket.h"
#include "minecraft/network/packet/SetHealthPacket.h"
#include "minecraft/network/packet/SetObjectivePacket.h"
#include "minecraft/network/packet/SetPlayerTeamPacket.h"
#include "minecraft/network/packet/SetScorePacket.h"
#include "minecraft/network/packet/SetSpawnPositionPacket.h"
#include "minecraft/network/packet/SetTimePacket.h"
#include "minecraft/network/packet/SignUpdatePacket.h"
#include "minecraft/network/packet/TakeItemEntityPacket.h"
#include "minecraft/network/packet/TeleportEntityPacket.h"
#include "minecraft/network/packet/TextureAndGeometryChangePacket.h"
#include "minecraft/network/packet/TextureAndGeometryPacket.h"
#include "minecraft/network/packet/TextureChangePacket.h"
#include "minecraft/network/packet/TexturePacket.h"
#include "minecraft/network/packet/TileDestructionPacket.h"
#include "minecraft/network/packet/TileEditorOpenPacket.h"
#include "minecraft/network/packet/TileEntityDataPacket.h"
#include "minecraft/network/packet/TileEventPacket.h"
#include "minecraft/network/packet/TileUpdatePacket.h"
#include "minecraft/network/packet/TradeItemPacket.h"
#include "minecraft/network/packet/UpdateAttributesPacket.h"
#include "minecraft/network/packet/UpdateGameRuleProgressPacket.h"
#include "minecraft/network/packet/UpdateMobEffectPacket.h"
#include "minecraft/network/packet/UpdateProgressPacket.h"
#include "minecraft/network/packet/UseItemPacket.h"
#include "minecraft/network/packet/XZPacket.h"
#include "minecraft/world/item/ItemInstance.h"
#include "nbt/NbtIo.h"

class CompoundTag;

void Packet::staticCtor() {
    // nextPrint = 0;

    // 4J - Note that item IDs are now defined in virtual method for each packet
    // type

    // 4J Stu - The values for canSendToAnyClient may not necessarily be the
    // correct choices
    map(0, true, true, true, false, typeid(KeepAlivePacket),
        KeepAlivePacket::create);
    map(1, true, true, true, false, typeid(LoginPacket), LoginPacket::create);
    map(2, true, true, true, false, typeid(PreLoginPacket),
        PreLoginPacket::create);
    map(3, true, true, true, false, typeid(ChatPacket), ChatPacket::create);
    map(4, true, false, false, true, typeid(SetTimePacket),
        SetTimePacket::create);
    map(5, true, false, false, true, typeid(SetEquippedItemPacket),
        SetEquippedItemPacket::create);
    map(6, true, false, true, true, typeid(SetSpawnPositionPacket),
        SetSpawnPositionPacket::create);
    map(7, false, true, false, false, typeid(InteractPacket),
        InteractPacket::create);
    map(8, true, false, true, true, typeid(SetHealthPacket),
        SetHealthPacket::create);
    map(9, true, true, true, false, typeid(RespawnPacket),
        RespawnPacket::create);

    map(10, true, true, true, false, typeid(MovePlayerPacket),
        MovePlayerPacket::create);
    map(11, true, true, true, true, typeid(MovePlayerPacket::Pos),
        MovePlayerPacket::Pos::create);
    map(12, true, true, true, true, typeid(MovePlayerPacket::Rot),
        MovePlayerPacket::Rot::create);
    map(13, true, true, true, true, typeid(MovePlayerPacket::PosRot),
        MovePlayerPacket::PosRot::create);

    map(14, false, true, false, false, typeid(PlayerActionPacket),
        PlayerActionPacket::create);
    map(15, false, true, false, false, typeid(UseItemPacket),
        UseItemPacket::create);
    map(16, true, true, true, false, typeid(SetCarriedItemPacket),
        SetCarriedItemPacket::create);
    // 4J-PB - we need to send to any client for the sleep in bed
    // map(17, true, false, false, false, EntityActionAtPositionPacket));
    map(17, true, false, true, false, typeid(EntityActionAtPositionPacket),
        EntityActionAtPositionPacket::create);
    // 4J-PB - we need to send to any client for the wake up from sleeping
    // map(18, true, true, false, false, AnimatePacket));
    map(18, true, true, true, false, typeid(AnimatePacket),
        AnimatePacket::create);
    map(19, false, true, false, false, typeid(PlayerCommandPacket),
        PlayerCommandPacket::create);

    map(20, true, false, false, true, typeid(AddPlayerPacket),
        AddPlayerPacket::create);
    map(22, true, false, true, true, typeid(TakeItemEntityPacket),
        TakeItemEntityPacket::create);
    map(23, true, false, false, true, typeid(AddEntityPacket),
        AddEntityPacket::create);
    map(24, true, false, false, true, typeid(AddMobPacket),
        AddMobPacket::create);
    map(25, true, false, false, false, typeid(AddPaintingPacket),
        AddPaintingPacket::create);
    map(26, true, false, false, false, typeid(AddExperienceOrbPacket),
        AddExperienceOrbPacket::create);  // TODO New for 1.8.2 - Needs
                                          // sendToAny?
    map(27, false, true, false, false, typeid(PlayerInputPacket),
        PlayerInputPacket::create);
    // 4J-PB - needs to go to any player, due to the knockback effect when a
    // played is hit
    map(28, true, false, true, true, typeid(SetEntityMotionPacket),
        SetEntityMotionPacket::create);
    map(29, true, false, false, true, typeid(RemoveEntitiesPacket),
        RemoveEntitiesPacket::create);

    map(30, true, false, false, false, typeid(MoveEntityPacket),
        MoveEntityPacket::create);
    map(31, true, false, false, true, typeid(MoveEntityPacket::Pos),
        MoveEntityPacket::Pos::create);
    map(32, true, false, false, true, typeid(MoveEntityPacket::Rot),
        MoveEntityPacket::Rot::create);
    map(33, true, false, false, true, typeid(MoveEntityPacket::PosRot),
        MoveEntityPacket::PosRot::create);
    map(34, true, false, false, true, typeid(TeleportEntityPacket),
        TeleportEntityPacket::create);
    map(35, true, false, false, false, typeid(RotateHeadPacket),
        RotateHeadPacket::create);

    // 4J - needs to go to any player, to create sound effect when a player is
    // hit
    map(38, true, false, true, true, typeid(EntityEventPacket),
        EntityEventPacket::create);
    map(39, true, false, true, false, typeid(SetEntityLinkPacket),
        SetEntityLinkPacket::create);
    map(40, true, false, true, true, typeid(SetEntityDataPacket),
        SetEntityDataPacket::create);
    map(41, true, false, true, false, typeid(UpdateMobEffectPacket),
        UpdateMobEffectPacket::create);
    map(42, true, false, true, false, typeid(RemoveMobEffectPacket),
        RemoveMobEffectPacket::create);
    map(43, true, false, true, false, typeid(SetExperiencePacket),
        SetExperiencePacket::create);
    map(44, true, false, true, false, typeid(UpdateAttributesPacket),
        UpdateAttributesPacket::create);

    map(50, true, false, true, true, typeid(ChunkVisibilityPacket),
        ChunkVisibilityPacket::create);
    map(51, true, false, true, true, typeid(BlockRegionUpdatePacket),
        BlockRegionUpdatePacket::create);  // Changed to LevelChunkPacket in
                                           // Java but we aren't using that
    map(52, true, false, true, true, typeid(ChunkTilesUpdatePacket),
        ChunkTilesUpdatePacket::create);
    map(53, true, false, true, true, typeid(TileUpdatePacket),
        TileUpdatePacket::create);
    map(54, true, false, true, true, typeid(TileEventPacket),
        TileEventPacket::create);
    map(55, true, false, false, false, typeid(TileDestructionPacket),
        TileDestructionPacket::create);

    map(60, true, false, true, false, typeid(ExplodePacket),
        ExplodePacket::create);
    map(61, true, false, true, false, typeid(LevelEventPacket),
        LevelEventPacket::create);
    // 4J-PB - don't see the need for this, we can use 61
    map(62, true, false, true, false, typeid(LevelSoundPacket),
        LevelSoundPacket::create);
    map(63, true, false, true, false, typeid(LevelParticlesPacket),
        LevelParticlesPacket::create);

    map(70, true, false, false, false, typeid(GameEventPacket),
        GameEventPacket::create);
    map(71, true, false, false, false, typeid(AddGlobalEntityPacket),
        AddGlobalEntityPacket::create);

    map(100, true, false, true, false, typeid(ContainerOpenPacket),
        ContainerOpenPacket::create);
    map(101, true, true, true, false, typeid(ContainerClosePacket),
        ContainerClosePacket::create);
    map(102, false, true, false, false, typeid(ContainerClickPacket),
        ContainerClickPacket::create);
#if !defined(_CONTENT_PACKAGE)
    // 4J Stu - We have some debug code that uses this packet to send data back
    // to the server from the client We may wish to add this into the real game
    // at some point
    map(103, true, true, true, false, typeid(ContainerSetSlotPacket),
        ContainerSetSlotPacket::create);
#else
    map(103, true, false, true, false, typeid(ContainerSetSlotPacket),
        ContainerSetSlotPacket::create);
#endif
    map(104, true, false, true, false, typeid(ContainerSetContentPacket),
        ContainerSetContentPacket::create);
    map(105, true, false, true, false, typeid(ContainerSetDataPacket),
        ContainerSetDataPacket::create);
    map(106, true, true, true, false, typeid(ContainerAckPacket),
        ContainerAckPacket::create);
    map(107, true, true, true, false, typeid(SetCreativeModeSlotPacket),
        SetCreativeModeSlotPacket::create);
    map(108, false, true, false, false, typeid(ContainerButtonClickPacket),
        ContainerButtonClickPacket::create);

    map(130, true, true, true, false, typeid(SignUpdatePacket),
        SignUpdatePacket::create);
    map(131, true, false, true, false, typeid(ComplexItemDataPacket),
        ComplexItemDataPacket::create);
    map(132, true, false, false, false, typeid(TileEntityDataPacket),
        TileEntityDataPacket::create);
    map(133, true, false, true, false, typeid(TileEditorOpenPacket),
        TileEditorOpenPacket::create);

    // 4J Added
    map(150, false, true, false, false, typeid(CraftItemPacket),
        CraftItemPacket::create);
    map(151, false, true, true, false, typeid(TradeItemPacket),
        TradeItemPacket::create);
    map(152, false, true, false, false, typeid(DebugOptionsPacket),
        DebugOptionsPacket::create);
    map(153, true, true, false, false, typeid(ServerSettingsChangedPacket),
        ServerSettingsChangedPacket::create);
    map(154, true, true, true, false, typeid(TexturePacket),
        TexturePacket::create);
    map(155, true, false, true, true, typeid(ChunkVisibilityAreaPacket),
        ChunkVisibilityAreaPacket::create);
    map(156, true, false, false, true, typeid(UpdateProgressPacket),
        UpdateProgressPacket::create);
    map(157, true, true, true, false, typeid(TextureChangePacket),
        TextureChangePacket::create);
    map(158, true, false, true, false, typeid(UpdateGameRuleProgressPacket),
        UpdateGameRuleProgressPacket::create);
    map(159, false, true, false, false, typeid(KickPlayerPacket),
        KickPlayerPacket::create);
    map(160, true, true, true, false, typeid(TextureAndGeometryPacket),
        TextureAndGeometryPacket::create);
    map(161, true, true, true, false, typeid(TextureAndGeometryChangePacket),
        TextureAndGeometryChangePacket::create);

    map(162, true, false, false, false, typeid(MoveEntityPacketSmall),
        MoveEntityPacketSmall::create);
    map(163, true, false, false, true, typeid(MoveEntityPacketSmall::Pos),
        MoveEntityPacketSmall::Pos::create);
    map(164, true, false, false, true, typeid(MoveEntityPacketSmall::Rot),
        MoveEntityPacketSmall::Rot::create);
    map(165, true, false, false, true, typeid(MoveEntityPacketSmall::PosRot),
        MoveEntityPacketSmall::PosRot::create);
    map(166, true, true, false, false, typeid(XZPacket), XZPacket::create);
    map(167, false, true, false, false, typeid(GameCommandPacket),
        GameCommandPacket::create);

    map(200, true, false, true, false, typeid(AwardStatPacket),
        AwardStatPacket::create);
    map(201, true, true, false, false, typeid(PlayerInfoPacket),
        PlayerInfoPacket::create);  // TODO New for 1.8.2 - Repurposed by 4J
    map(202, true, true, true, false, typeid(PlayerAbilitiesPacket),
        PlayerAbilitiesPacket::create);
    // 4J Stu - These added 1.3.2, but don't think we need them
    // map(203, true, true, true, false, ChatAutoCompletePacket.class);
    // map(204, false, true, true, false, ClientInformationPacket.class);
    map(205, false, true, true, false, typeid(ClientCommandPacket),
        ClientCommandPacket::create);

    map(206, true, false, true, false, typeid(SetObjectivePacket),
        SetObjectivePacket::create);
    map(207, true, false, true, false, typeid(SetScorePacket),
        SetScorePacket::create);
    map(208, true, false, true, false, typeid(SetDisplayObjectivePacket),
        SetDisplayObjectivePacket::create);
    map(209, true, false, true, false, typeid(SetPlayerTeamPacket),
        SetPlayerTeamPacket::create);

    map(250, true, true, true, false, typeid(CustomPayloadPacket),
        CustomPayloadPacket::create);
    // 4J Stu - These added 1.3.2, but don't think we need them
    // map(252, true, true, SharedKeyPacket.class);
    // map(253, true, false, ServerAuthDataPacket.class);
    map(254, false, true, false, false, typeid(GetInfoPacket),
        GetInfoPacket::create);  // TODO New for 1.8.2 - Needs sendToAny?
    map(255, true, true, true, false, typeid(DisconnectPacket),
        DisconnectPacket::create);
}

IllegalArgumentException::IllegalArgumentException(
    const std::string& information) {
    this->information = information;
}

IOException::IOException(const std::string& information) {
    this->information = information;
}

Packet::Packet() : createTime(System::currentTimeMillis()) {
    shouldDelay = false;
}

std::unordered_map<int, packetCreateFn> Packet::idToCreateMap;

std::unordered_set<int> Packet::clientReceivedPackets =
    std::unordered_set<int>();
std::unordered_set<int> Packet::serverReceivedPackets =
    std::unordered_set<int>();
std::unordered_set<int> Packet::sendToAnyClientPackets =
    std::unordered_set<int>();

// 4J Added
std::unordered_map<int, Packet::PacketStatistics*> Packet::outgoingStatistics =
    std::unordered_map<int, Packet::PacketStatistics*>();
std::vector<Packet::PacketStatistics*> Packet::renderableStats =
    std::vector<Packet::PacketStatistics*>();
int Packet::renderPos = 0;

// sendToAnyClient - true - send to anyone, false - Sends to one person per
// dimension per machine
void Packet::map(int id, bool receiveOnClient, bool receiveOnServer,
                 bool sendToAnyClient, bool renderStats,
                 const std::type_info& clazz, packetCreateFn createFn) {
    idToCreateMap.insert(
        std::unordered_map<int, packetCreateFn>::value_type(id, createFn));

#if !defined(_CONTENT_PACKAGE)
#if PACKET_ENABLE_STAT_TRACKING
    Packet::PacketStatistics* packetStatistics = new PacketStatistics(id);
    outgoingStatistics[id] = packetStatistics;

    if (renderStats) {
        renderableStats.push_back(packetStatistics);
    }
#endif
#endif

    if (receiveOnClient) {
        clientReceivedPackets.insert(id);
    }
    if (receiveOnServer) {
        serverReceivedPackets.insert(id);
    }
    if (sendToAnyClient) {
        sendToAnyClientPackets.insert(id);
    }
}

// 4J Added to record data for outgoing packets
void Packet::recordOutgoingPacket(std::shared_ptr<Packet> packet,
                                  int playerIndex) {
#if !defined(_CONTENT_PACKAGE)
#if PACKET_ENABLE_STAT_TRACKING
    int idx = playerIndex;
    if (packet->getId() != 51) {
        idx = 100;
    }
    auto it = outgoingStatistics.find(idx);

    if (it == outgoingStatistics.end()) {
        Packet::PacketStatistics* packetStatistics = new PacketStatistics(idx);
        outgoingStatistics[idx] = packetStatistics;
        packetStatistics->addPacket(packet->getEstimatedSize());
    } else {
        it->second->addPacket(packet->getEstimatedSize());
    }
#endif
#endif
}

void Packet::updatePacketStatsPIX() {
#if !defined(_CONTENT_PACKAGE)
#if PACKET_ENABLE_STAT_TRACKING

    for (auto it = outgoingStatistics.begin(); it != outgoingStatistics.end();
         it++) {
        Packet::PacketStatistics* stat = it->second;
        int64_t count = stat->getRunningCount();
        char pixName[256];
        snprintf(pixName, "Packet count %d", stat->id);
        //		PIXReportCounter(pixName,(float)count);
        int64_t total = stat->getRunningTotal();
        snprintf(pixName, "Packet bytes %d", stat->id);
        PIXReportCounter(pixName, (float)total);
        stat->IncrementPos();
    }
#endif
#endif
}

std::shared_ptr<Packet> Packet::getPacket(int id) {
    // 4J: Removed try/catch
    return idToCreateMap[id]();
}

void Packet::writeBytes(DataOutputStream* dataoutputstream,
                        const std::vector<uint8_t>& bytes) {
    dataoutputstream->writeShort(bytes.size());
    dataoutputstream->write(bytes);
}

std::vector<uint8_t> Packet::readBytes(DataInputStream* datainputstream) {
    int size = datainputstream->readShort();
    if (size < 0) {
        Log::info("Key was smaller than nothing!  Weird key!");
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
        return std::vector<uint8_t>();
        // throw new IOException("Key was smaller than nothing!  Weird key!");
    }

    std::vector<uint8_t> bytes(size);
    datainputstream->readFully(bytes);

    return bytes;
}

bool Packet::canSendToAnyClient(std::shared_ptr<Packet> packet) {
    int packetId = packet->getId();

    return sendToAnyClientPackets.count(packetId) != 0;
}

// 4J - now a pure virtual method
/*
int Packet::getId()
{
return id;
}
*/

std::unordered_map<int, Packet::PacketStatistics*> Packet::statistics =
    std::unordered_map<int, Packet::PacketStatistics*>();

// int Packet::nextPrint = 0;

std::shared_ptr<Packet> Packet::readPacket(
    DataInputStream* dis, bool isServer)  // throws IOException TODO 4J JEV,
                                          // should this declare a throws?
{
    int id = 0;
    std::shared_ptr<Packet> packet = nullptr;

    // 4J - removed try/catch
    //    try
    //	{
    id = dis->read();
    if (id == -1) return nullptr;

    if ((isServer &&
         serverReceivedPackets.find(id) == serverReceivedPackets.end()) ||
        (!isServer &&
         clientReceivedPackets.find(id) == clientReceivedPackets.end())) {
        // Log::info("Bad packet id %d\n", id);
        assert(0);
        assert(false);
        //            throw new IOException(string("Bad packet id ") +
        //            toWString<int>(id));
    }

    packet = getPacket(id);
    if (packet == nullptr)
        assert(false);  // throw new IOException(string("Bad packet id ") +
                        // toWString<int>(id));

    // Log::info("%s reading packet %d\n", isServer ? "Server" : "Client",
    // packet->getId());
    packet->read(dis);
    //    }
    //	catch (EOFException e)
    //	{
    //       // reached end of stream
    //        OutputDebugString("Reached end of stream");
    //        return nullptr;
    //    }

    // 4J - Don't bother tracking stats in a content package
    // 4J Stu - This changes a bit in 1.0.1, but we don't really use it so stick
    // with what we have
#if !defined(_CONTENT_PACKAGE)
#if PACKET_ENABLE_STAT_TRACKING
    auto it = statistics.find(id);

    if (it == statistics.end()) {
        Packet::PacketStatistics* packetStatistics = new PacketStatistics(id);
        statistics[id] = packetStatistics;
        packetStatistics->addPacket(packet->getEstimatedSize());
    } else {
        it->second->addPacket(packet->getEstimatedSize());
    }
#endif
#endif

    return packet;
}

void Packet::writePacket(
    std::shared_ptr<Packet> packet,
    DataOutputStream*
        dos)  // throws IOException TODO 4J JEV, should this declare a throws?
{
    // Log::info("Writing packet %d\n", packet->getId());
    dos->write(packet->getId());
    packet->write(dos);
}

void Packet::writeUtf(const std::string& value,
                      DataOutputStream* dos)  // throws IOException TODO 4J JEV,
                                              // should this declare a throws?
{
    dos->writeShort((short)value.length());
    dos->writeChars(value);
}

std::string Packet::readUtf(DataInputStream* dis,
                             int maxLength)  // throws IOException TODO 4J JEV,
                                             // should this declare a throws?
{
    short stringLength = dis->readShort();
    if (stringLength > maxLength) {
        std::stringstream stream;
        stream << "Received string length longer than maximum allowed ("
               << stringLength << " > " << maxLength << ")";
        assert(false);
        //        throw new IOException( stream.str() );
    }
    if (stringLength < 0) {
        assert(false);
        //        throw new IOException("Received string length is less than
        //        zero! Weird string!");
    }

    std::string builder = "";
    for (int i = 0; i < stringLength; i++) {
        char rc = dis->readChar();
        builder.push_back(rc);
    }

    return builder;
}

Packet::PacketStatistics::PacketStatistics(int id)
    : id(id), count(0), totalSize(0), samplesPos(0) {
    memset(countSamples, 0, sizeof(countSamples));
    memset(sizeSamples, 0, sizeof(sizeSamples));
}

void Packet::PacketStatistics::addPacket(int bytes) {
    countSamples[samplesPos]++;
    sizeSamples[samplesPos] += bytes;
    timeSamples[samplesPos] = System::currentTimeMillis();
    totalSize += bytes;
    count++;
}

int Packet::PacketStatistics::getCount() { return count; }

double Packet::PacketStatistics::getAverageSize() {
    if (count == 0) {
        return 0;
    }
    return (double)totalSize / count;
}

int Packet::PacketStatistics::getTotalSize() { return totalSize; }

int64_t Packet::PacketStatistics::getRunningTotal() {
    int64_t total = 0;
    int64_t currentTime = System::currentTimeMillis();
    for (int i = 0; i < TOTAL_TICKS; i++) {
        if (currentTime - timeSamples[i] <= 1000) {
            total += sizeSamples[i];
        }
    }
    return total;
}

int64_t Packet::PacketStatistics::getRunningCount() {
    int64_t total = 0;
    int64_t currentTime = System::currentTimeMillis();
    for (int i = 0; i < TOTAL_TICKS; i++) {
        if (currentTime - timeSamples[i] <= 1000) {
            total += countSamples[i];
        }
    }
    return total;
}

void Packet::PacketStatistics::IncrementPos() {
    samplesPos = (samplesPos + 1) % TOTAL_TICKS;
    countSamples[samplesPos] = 0;
    sizeSamples[samplesPos] = 0;
    timeSamples[samplesPos] = 0;
}

bool Packet::canBeInvalidated() { return false; }

bool Packet::isInvalidatedBy(std::shared_ptr<Packet> packet) { return false; }

bool Packet::isAync() { return false; }

// 4J Stu - Brought these functions forward for enchanting/game rules
std::shared_ptr<ItemInstance> Packet::readItem(DataInputStream* dis) {
    std::shared_ptr<ItemInstance> item = nullptr;
    int id = dis->readShort();
    if (id >= 0) {
        int count = dis->readByte();
        int damage = dis->readShort();

        item = std::make_shared<ItemInstance>(id, count, damage);
        // 4J Stu - Always read/write the tag
        // if (Item.items[id].canBeDepleted() ||
        // Item.items[id].shouldOverrideMultiplayerNBT())
        {
            item->tag = readNbt(dis);
        }
    }

    return item;
}

void Packet::writeItem(std::shared_ptr<ItemInstance> item,
                       DataOutputStream* dos) {
    if (item == nullptr) {
        dos->writeShort(-1);
    } else {
        dos->writeShort(item->id);
        dos->writeByte(item->count);
        dos->writeShort(item->getAuxValue());
        // 4J Stu - Always read/write the tag
        // if (item.getItem().canBeDepleted() ||
        // item.getItem().shouldOverrideMultiplayerNBT())
        {
            writeNbt(item->tag, dos);
        }
    }
}

CompoundTag* Packet::readNbt(DataInputStream* dis) {
    int size = dis->readShort();
    if (size < 0) return nullptr;
    std::vector<uint8_t> buff(size);
    dis->readFully(buff);
    CompoundTag* result = (CompoundTag*)NbtIo::decompress(buff);
    return result;
}

void Packet::writeNbt(CompoundTag* tag, DataOutputStream* dos) {
    if (tag == nullptr) {
        dos->writeShort(-1);
    } else {
        std::vector<uint8_t> buff = NbtIo::compress(tag);
        dos->writeShort((short)buff.size());
        dos->write(buff);
    }
}
