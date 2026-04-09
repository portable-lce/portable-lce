#pragma once
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "minecraft/network/Connection.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/PacketListener.h"
#include "minecraft/world/entity/Entity.h"
#include "platform/storage/storage.h"
#include "util/Timer.h"

class Minecraft;
class MultiPlayerLevel;
class SavedDataStorage;
class Socket;
class MultiplayerLocalPlayer;
class INetworkPlayer;
class Packet;
class Random;
class SetEntityLinkPacket;

class ClientConnection : public PacketListener {
private:
    enum eClientConnectionConnectingState {
        eCCPreLoginSent = 0,
        eCCPreLoginReceived,
        eCCLoginSent,
        eCCLoginReceived,
        eCCConnected
    };

private:
    bool done;
    Connection* connection;

public:
    std::string message;
    bool createdOk;  // 4J added
private:
    Minecraft* minecraft;
    MultiPlayerLevel* level;
    bool started;

    // 4J Stu - I don't think we are interested in the PlayerInfo data, so I'm
    // not going to use it at the moment
    // Map<String, PlayerInfo> playerInfoMap = new HashMap<String,
    // PlayerInfo>();
public:
    // List<PlayerInfo> playerInfos = new ArrayList<PlayerInfo>();

    int maxPlayers;

public:
    bool isStarted() { return started; }                     // 4J Added
    bool isClosed() { return done; }                         // 4J Added
    Socket* getSocket() { return connection->getSocket(); }  // 4J Added

private:
    int m_userIndex;  // 4J Added
public:
    SavedDataStorage* savedDataStorage;
    ClientConnection(Minecraft* minecraft, const std::string& ip, int port);
    ClientConnection(Minecraft* minecraft, Socket* socket, int iUserIndex = -1);
    ~ClientConnection();
    void tick();
    INetworkPlayer* getNetworkPlayer();
    virtual void handleLogin(std::shared_ptr<LoginPacket> packet);
    virtual void handleAddEntity(std::shared_ptr<AddEntityPacket> packet);
    virtual void handleAddExperienceOrb(
        std::shared_ptr<AddExperienceOrbPacket> packet);
    virtual void handleAddGlobalEntity(
        std::shared_ptr<AddGlobalEntityPacket> packet);
    virtual void handleAddPainting(std::shared_ptr<AddPaintingPacket> packet);
    virtual void handleSetEntityMotion(
        std::shared_ptr<SetEntityMotionPacket> packet);
    virtual void handleSetEntityData(
        std::shared_ptr<SetEntityDataPacket> packet);
    virtual void handleAddPlayer(std::shared_ptr<AddPlayerPacket> packet);
    virtual void handleTeleportEntity(
        std::shared_ptr<TeleportEntityPacket> packet);
    virtual void handleSetCarriedItem(
        std::shared_ptr<SetCarriedItemPacket> packet);
    virtual void handleMoveEntity(std::shared_ptr<MoveEntityPacket> packet);
    virtual void handleRotateMob(std::shared_ptr<RotateHeadPacket> packet);
    virtual void handleMoveEntitySmall(
        std::shared_ptr<MoveEntityPacketSmall> packet);
    virtual void handleRemoveEntity(
        std::shared_ptr<RemoveEntitiesPacket> packet);
    virtual void handleMovePlayer(std::shared_ptr<MovePlayerPacket> packet);

    Random* random;

    // 4J Added
    virtual void handleChunkVisibilityArea(
        std::shared_ptr<ChunkVisibilityAreaPacket> packet);

    virtual void handleChunkVisibility(
        std::shared_ptr<ChunkVisibilityPacket> packet);
    virtual void handleChunkTilesUpdate(
        std::shared_ptr<ChunkTilesUpdatePacket> packet);
    virtual void handleBlockRegionUpdate(
        std::shared_ptr<BlockRegionUpdatePacket> packet);
    virtual void handleTileUpdate(std::shared_ptr<TileUpdatePacket> packet);
    virtual void handleDisconnect(std::shared_ptr<DisconnectPacket> packet);
    virtual void onDisconnect(DisconnectPacket::eDisconnectReason reason,
                              void* reasonObjects);
    void sendAndDisconnect(std::shared_ptr<Packet> packet);
    void send(std::shared_ptr<Packet> packet);
    virtual void handleTakeItemEntity(
        std::shared_ptr<TakeItemEntityPacket> packet);
    virtual void handleChat(std::shared_ptr<ChatPacket> packet);
    virtual void handleAnimate(std::shared_ptr<AnimatePacket> packet);
    virtual void handleEntityActionAtPosition(
        std::shared_ptr<EntityActionAtPositionPacket> packet);
    virtual void handlePreLogin(std::shared_ptr<PreLoginPacket> packet);
    void close();
    virtual void handleAddMob(std::shared_ptr<AddMobPacket> packet);
    virtual void handleSetTime(std::shared_ptr<SetTimePacket> packet);
    virtual void handleSetSpawn(std::shared_ptr<SetSpawnPositionPacket> packet);
    virtual void handleEntityLinkPacket(
        std::shared_ptr<SetEntityLinkPacket> packet);
    virtual void handleEntityEvent(std::shared_ptr<EntityEventPacket> packet);

private:
    std::shared_ptr<Entity> getEntity(int entityId);
    std::string GetDisplayNameByGamertag(std::string gamertag);

public:
    virtual void handleSetHealth(std::shared_ptr<SetHealthPacket> packet);
    virtual void handleSetExperience(
        std::shared_ptr<SetExperiencePacket> packet);
    virtual void handleRespawn(std::shared_ptr<RespawnPacket> packet);
    virtual void handleExplosion(std::shared_ptr<ExplodePacket> packet);
    virtual void handleContainerOpen(
        std::shared_ptr<ContainerOpenPacket> packet);
    virtual void handleContainerSetSlot(
        std::shared_ptr<ContainerSetSlotPacket> packet);
    virtual void handleContainerAck(std::shared_ptr<ContainerAckPacket> packet);
    virtual void handleContainerContent(
        std::shared_ptr<ContainerSetContentPacket> packet);
    virtual void handleTileEditorOpen(
        std::shared_ptr<TileEditorOpenPacket> packet);
    virtual void handleSignUpdate(std::shared_ptr<SignUpdatePacket> packet);
    virtual void handleTileEntityData(
        std::shared_ptr<TileEntityDataPacket> packet);
    virtual void handleContainerSetData(
        std::shared_ptr<ContainerSetDataPacket> packet);
    virtual void handleSetEquippedItem(
        std::shared_ptr<SetEquippedItemPacket> packet);
    virtual void handleContainerClose(
        std::shared_ptr<ContainerClosePacket> packet);
    virtual void handleTileEvent(std::shared_ptr<TileEventPacket> packet);
    virtual void handleTileDestruction(
        std::shared_ptr<TileDestructionPacket> packet);
    virtual bool canHandleAsyncPackets();
    virtual void handleGameEvent(
        std::shared_ptr<GameEventPacket> gameEventPacket);
    virtual void handleComplexItemData(
        std::shared_ptr<ComplexItemDataPacket> packet);
    virtual void handleLevelEvent(std::shared_ptr<LevelEventPacket> packet);
    virtual void handleAwardStat(std::shared_ptr<AwardStatPacket> packet);
    virtual void handleUpdateMobEffect(
        std::shared_ptr<UpdateMobEffectPacket> packet);
    virtual void handleRemoveMobEffect(
        std::shared_ptr<RemoveMobEffectPacket> packet);
    virtual bool isServerPacketListener();
    virtual void handlePlayerInfo(std::shared_ptr<PlayerInfoPacket> packet);
    virtual void handleKeepAlive(std::shared_ptr<KeepAlivePacket> packet);
    virtual void handlePlayerAbilities(
        std::shared_ptr<PlayerAbilitiesPacket> playerAbilitiesPacket);
    virtual void handleSoundEvent(std::shared_ptr<LevelSoundPacket> packet);
    virtual void handleCustomPayload(
        std::shared_ptr<CustomPayloadPacket> customPayloadPacket);
    virtual Connection* getConnection();

    // 4J Added
    virtual void handleServerSettingsChanged(
        std::shared_ptr<ServerSettingsChangedPacket> packet);
    virtual void handleTexture(std::shared_ptr<TexturePacket> packet);
    virtual void handleTextureAndGeometry(
        std::shared_ptr<TextureAndGeometryPacket> packet);
    virtual void handleUpdateProgress(
        std::shared_ptr<UpdateProgressPacket> packet);

    // 4J Added
    static int HostDisconnectReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result);
    static int ExitGameAndSaveReturned(void* pParam, int iPad,
                                       IPlatformStorage::EMessageResult result);
    virtual void handleTextureChange(
        std::shared_ptr<TextureChangePacket> packet);
    virtual void handleTextureAndGeometryChange(
        std::shared_ptr<TextureAndGeometryChangePacket> packet);
    virtual void handleUpdateGameRuleProgressPacket(
        std::shared_ptr<UpdateGameRuleProgressPacket> packet);
    virtual void handleXZ(std::shared_ptr<XZPacket> packet);

    void displayPrivilegeChanges(std::shared_ptr<MultiplayerLocalPlayer> player,
                                 unsigned int oldPrivileges);

    virtual void handleAddObjective(std::shared_ptr<SetObjectivePacket> packet);
    virtual void handleSetScore(std::shared_ptr<SetScorePacket> packet);
    virtual void handleSetDisplayObjective(
        std::shared_ptr<SetDisplayObjectivePacket> packet);
    virtual void handleSetPlayerTeamPacket(
        std::shared_ptr<SetPlayerTeamPacket> packet);
    virtual void handleParticleEvent(
        std::shared_ptr<LevelParticlesPacket> packet);
    virtual void handleUpdateAttributes(
        std::shared_ptr<UpdateAttributesPacket> packet);

private:
    // 4J: Entity link packet deferred
    class DeferredEntityLinkPacket {
    public:
        time_util::time_point m_recievedTick;
        std::shared_ptr<SetEntityLinkPacket> m_packet;

        DeferredEntityLinkPacket(std::shared_ptr<SetEntityLinkPacket> packet);
    };

    std::vector<DeferredEntityLinkPacket> deferredEntityLinkPackets;
    static const int MAX_ENTITY_LINK_DEFERRAL_INTERVAL = 1000;

    void checkDeferredEntityLinkPackets(int newEntityId);
};