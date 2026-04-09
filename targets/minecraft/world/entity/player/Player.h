#pragma once

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "Abilities.h"
#include "java/Class.h"
#include "minecraft/commands/CommandSender.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/SkinTypes.h"
#include "minecraft/world/food/FoodData.h"
#include "minecraft/world/inventory/PlayerEnderChestContainer.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/scores/ScoreHolder.h"
#include "platform/PlatformTypes.h"

class AbstractContainerMenu;
class Stats;
class FishingHook;
class EntityHorse;
class ItemEntity;
class Slot;
class Pos;
class TileEntity;
class BeaconTileEntity;
class FurnaceTileEntity;
class DispenserTileEntity;
class SignTileEntity;
class BrewingStandTileEntity;
class HopperTileEntity;
class MinecartHopper;
class Inventory;
class Container;
class FoodData;
class DamageSource;
class Merchant;
class PlayerEnderChestContainer;
class GameType;
class Scoreboard;
class Level;
class ModelPart;
class Stat;
class Tile;

class Player : public LivingEntity, public CommandSender, public ScoreHolder {
public:
    static const int MAX_NAME_LENGTH = 16 + 4;
    static const int MAX_HEALTH = 20;
    static const int SLEEP_DURATION = 100;
    static const int WAKE_UP_DURATION = 10;

    static const int CHAT_VISIBILITY_FULL = 0;
    static const int CHAT_VISIBILITY_SYSTEM = 1;
    static const int CHAT_VISIBILITY_HIDDEN = 2;

    // 4J-PB - added for a red death fade in the gui
    static const int DEATHFADE_DURATION = 21;

private:
    static const int FLY_ACHIEVEMENT_SPEED = 25;

    static const int DATA_PLAYER_FLAGS_ID = 16;
    static const int DATA_PLAYER_ABSORPTION_ID = 17;
    static const int DATA_SCORE_ID = 18;

protected:
    static const int FLAG_HIDE_CAPE = 1;

public:
    std::shared_ptr<Inventory> inventory;

private:
    std::shared_ptr<PlayerEnderChestContainer> enderChestInventory;

public:
    AbstractContainerMenu* inventoryMenu;
    AbstractContainerMenu* containerMenu;

protected:
    FoodData foodData;
    int jumpTriggerTime;

public:
    std::uint8_t userType;
    float oBob, bob;

    std::string name;
    int takeXpDelay;

    // 4J-PB - track custom skin
    std::string customTextureUrl;
    std::string customTextureUrl2;
    unsigned int m_uiPlayerCurrentSkin;
    void ChangePlayerSkin();

    // 4J-PB - not needed, since cutomtextureurl2 is the same thing string
    // cloakTexture;

    double xCloakO, yCloakO, zCloakO;
    double xCloak, yCloak, zCloak;

    // 4J-HG: store display name, added for Xbox One "game display name"
    std::string m_displayName;

protected:
    // player sleeping in bed?
    bool m_isSleeping;

public:
    Pos* bedPosition;

private:
    int sleepCounter;      // animation timer
    int deathFadeCounter;  // animation timer

public:
    float bedOffsetX, bedOffsetY, bedOffsetZ;
    Stats* stats;

private:
    Pos* respawnPosition;
    bool respawnForced;
    Pos* minecartAchievementPos;

    // 4J Gordon: These are in cms, every time they go > 1m they are entered
    // into the stats
    int distanceWalk, distanceSwim, distanceFall, distanceClimb,
        distanceMinecart, distanceBoat, distancePig;

public:
    Abilities abilities;

    int experienceLevel, totalExperience;
    float experienceProgress;

    // 4J Stu - Made protected so that we can access it from
    // MultiPlayerLocalPlayer
protected:
    std::shared_ptr<ItemInstance> useItem;
    int useItemDuration;

protected:
    float defaultWalkSpeed;
    float defaultFlySpeed;

private:
    int lastLevelUpTime;

public:
    eINSTANCEOF GetType() { return eTYPE_PLAYER; }

    // 4J Added to default init
    void _init();

    Player(Level* level, const std::string& name);
    virtual ~Player();

protected:
    virtual void registerAttributes();
    virtual void defineSynchedData();

public:
    std::shared_ptr<ItemInstance> getUseItem();
    int getUseItemDuration();
    bool isUsingItem();
    int getTicksUsingItem();
    void releaseUsingItem();
    void stopUsingItem();
    virtual bool isBlocking();

    // 4J Stu - Added for things that should only be ticked once per simulation
    // frame
    virtual void updateFrameTick();

    virtual void tick();
    virtual int getPortalWaitTime();
    virtual int getDimensionChangingDelay();
    virtual void playSound(int iSound, float volume, float pitch);

protected:
    void spawnEatParticles(std::shared_ptr<ItemInstance> useItem, int count);
    virtual void completeUsingItem();

public:
    virtual void handleEntityEvent(std::uint8_t id);

protected:
    bool isImmobile();
    virtual void closeContainer();

public:
    virtual void ride(std::shared_ptr<Entity> e);
    void prepareCustomTextures();
    virtual void rideTick();
    virtual void resetPos();

protected:
    virtual void serverAiStep();

public:
    virtual void aiStep();

private:
    virtual void touch(std::shared_ptr<Entity> entity);

public:
    virtual int getScore();
    virtual void setScore(int value);
    virtual void increaseScore(int amount);
    virtual void die(DamageSource* source);
    virtual void awardKillScore(std::shared_ptr<Entity> victim,
                                int awardPoints);
    virtual bool isShootable();
    bool isCreativeModeAllowed();
    virtual std::shared_ptr<ItemEntity> drop(bool all);
    std::shared_ptr<ItemEntity> drop(std::shared_ptr<ItemInstance> item);
    std::shared_ptr<ItemEntity> drop(std::shared_ptr<ItemInstance> item,
                                     bool randomly);

protected:
    virtual void reallyDrop(std::shared_ptr<ItemEntity> thrownItem);

public:
    float getDestroySpeed(Tile* tile, bool hasProperTool);
    bool canDestroy(Tile* tile);
    virtual void readAdditionalSaveData(CompoundTag* entityTag);
    virtual void addAdditonalSaveData(CompoundTag* entityTag);
    virtual bool openContainer(
        std::shared_ptr<Container> container);  // 4J - added bool return
    virtual bool openHopper(std::shared_ptr<HopperTileEntity> container);
    virtual bool openHopper(std::shared_ptr<MinecartHopper> container);
    virtual bool openHorseInventory(std::shared_ptr<EntityHorse> horse,
                                    std::shared_ptr<Container> container);
    virtual bool startEnchanting(
        int x, int y, int z,
        const std::string& name);                      // 4J - added bool return
    virtual bool startRepairing(int x, int y, int z);  // 4J - added bool return
    virtual bool startCrafting(int x, int y, int z);   // 4J - added bool return
    virtual bool openFireworks(int x, int y, int z);   // 4J - added
    virtual float getHeadHeight();

    // 4J-PB - added to keep the code happy with the change to make the third
    // person view per player
    virtual int ThirdPersonView() { return 0; }
    virtual void SetThirdPersonView(int val) {}

protected:
    virtual void setDefaultHeadHeight();

public:
    std::shared_ptr<FishingHook> fishing;

    virtual bool hurt(DamageSource* source, float dmg);
    virtual bool canHarmPlayer(std::shared_ptr<Player> target);
    virtual bool canHarmPlayer(
        std::string targetName);  // 4J: Added for ServerPlayer when only
                                  // player name is provided

protected:
    virtual void hurtArmor(float damage);

public:
    virtual int getArmorValue();
    virtual float getArmorCoverPercentage();

protected:
    virtual void actuallyHurt(DamageSource* source, float dmg);

public:
    using Entity::interact;

    virtual bool openFurnace(std::shared_ptr<FurnaceTileEntity>
                                 container);  // 4J - added bool return
    virtual bool openTrap(std::shared_ptr<DispenserTileEntity>
                              container);  // 4J - added bool return
    virtual void openTextEdit(std::shared_ptr<TileEntity> sign);
    virtual bool openBrewingStand(std::shared_ptr<BrewingStandTileEntity>
                                      brewingStand);  // 4J - added bool return
    virtual bool openBeacon(std::shared_ptr<BeaconTileEntity> beacon);
    virtual bool openTrading(
        std::shared_ptr<Merchant> traderTarget,
        const std::string& name);  // 4J - added bool return
    virtual void openItemInstanceGui(
        std::shared_ptr<ItemInstance> itemInstance);
    virtual bool interact(std::shared_ptr<Entity> entity);
    virtual std::shared_ptr<ItemInstance> getSelectedItem();
    void removeSelectedItem();
    virtual double getRidingHeight();
    virtual void attack(std::shared_ptr<Entity> entity);
    virtual void crit(std::shared_ptr<Entity> entity);
    virtual void magicCrit(std::shared_ptr<Entity> entity);
    virtual void respawn();

protected:
    static void animateRespawn(std::shared_ptr<Player> player, Level* level);

public:
    Slot* getInventorySlot(int slotId);
    virtual void remove();
    virtual bool isInWall();
    virtual bool isLocalPlayer();

    enum BedSleepingResult {
        OK,
        NOT_POSSIBLE_HERE,
        NOT_POSSIBLE_NOW,
        TOO_FAR_AWAY,
        OTHER_PROBLEM,
        NOT_SAFE
    };

    virtual BedSleepingResult startSleepInBed(int x, int y, int z,
                                              bool bTestUse = false);

private:
    void setBedOffset(int bedDirection);

public:
    /**
     *
     * @param forcefulWakeUp
     *            If the player has been forced to wake up. When this happens,
     *            the client will skip the wake-up animation. For example, when
     *            the player is hurt or the bed is destroyed.
     * @param updateLevelList
     *            If the level's sleeping player list needs to be updated. This
     *            is usually the case.
     * @param saveRespawnPoint
     *            TODO
     */
    virtual void stopSleepInBed(bool forcefulWakeUp, bool updateLevelList,
                                bool saveRespawnPoint);

private:
    bool checkBed();

public:
    static Pos* checkBedValidRespawnPosition(Level* level, Pos* pos,
                                             bool forced);
    float getSleepRotation();
    bool isSleeping();
    bool isSleepingLongEnough();
    int getSleepTimer();
    int getDeathFadeTimer();

protected:
    bool getPlayerFlag(int flag);
    void setPlayerFlag(int flag, bool value);

public:
    /**
     * This method is currently only relevant to client-side players. It will
     * try to load the messageId from the language file and display it to the
     * client.
     */
    virtual void displayClientMessage(int messageId);
    virtual Pos* getRespawnPosition();
    virtual bool isRespawnForced();
    virtual void setRespawnPosition(Pos* respawnPosition, bool forced);
    virtual void awardStat(Stat* stat, const std::vector<uint8_t>& param);

protected:
    void jumpFromGround();

public:
    virtual void travel(float xa, float ya);
    virtual float getSpeed();
    virtual void checkMovementStatistiscs(double dx, double dy, double dz);

private:
    void checkRidingStatistiscs(double dx, double dy, double dz);

    bool m_bAwardedOnARail;

protected:
    virtual void causeFallDamage(float distance);

public:
    virtual void killed(std::shared_ptr<LivingEntity> mob);
    virtual void makeStuckInWeb();
    virtual Icon* getItemInHandIcon(std::shared_ptr<ItemInstance> item,
                                    int layer);
    virtual std::shared_ptr<ItemInstance> getArmor(int pos);
    virtual void increaseXp(int i);
    virtual void giveExperienceLevels(int amount);
    int getXpNeededForNextLevel();
    void causeFoodExhaustion(float amount);
    FoodData* getFoodData();
    bool canEat(bool magicalItem);
    bool isHurt();
    virtual void startUsingItem(std::shared_ptr<ItemInstance> instance,
                                int duration);
    virtual bool mayDestroyBlockAt(int x, int y, int z);
    virtual bool mayUseItemAt(int x, int y, int z, int face,
                              std::shared_ptr<ItemInstance> item);

protected:
    virtual int getExperienceReward(std::shared_ptr<Player> killedBy);
    virtual bool isAlwaysExperienceDropper();

public:
    virtual std::string getAName();
    virtual bool shouldShowName();
    virtual void restoreFrom(std::shared_ptr<Player> oldPlayer,
                             bool restoreAll);

protected:
    bool makeStepSound();

public:
    void onUpdateAbilities();
    void setGameMode(GameType* mode);
    std::string getName();
    virtual std::string getDisplayName();
    virtual std::string getNetworkName();  // 4J: Added

    virtual Level* getCommandSenderWorld();

    std::shared_ptr<PlayerEnderChestContainer> getEnderChestInventory();

    virtual std::shared_ptr<ItemInstance> getCarried(int slot);
    virtual std::shared_ptr<ItemInstance> getCarriedItem();
    virtual void setEquippedSlot(int slot, std::shared_ptr<ItemInstance> item);
    virtual bool isInvisibleTo(std::shared_ptr<Player> player);
    virtual std::vector<std::shared_ptr<ItemInstance>> getEquipmentSlots();
    virtual bool isCapeHidden();
    virtual bool isPushedByWater();
    virtual Scoreboard* getScoreboard();
    virtual Team* getTeam();
    virtual void setAbsorptionAmount(float absorptionAmount);
    virtual float getAbsorptionAmount();

    //////// 4J /////////////////

    static int hash_fnct(const std::shared_ptr<Player> k);
    static bool eq_test(const std::shared_ptr<Player> x,
                        const std::shared_ptr<Player> y);

    // 4J Stu - Added to allow callback to tutorial to stay within
    // Minecraft.Client Overidden in LocalPlayer
    virtual void onCrafted(std::shared_ptr<ItemInstance> item) {}

    // 4J Overriding this so that we can have some different default skins
    virtual int getTexture();  // 4J changed from std::string to int
    void setPlayerDefaultSkin(EDefaultSkins skin);
    EDefaultSkins getPlayerDefaultSkin() { return m_skinIndex; }
    virtual void setCustomSkin(std::uint32_t skinId);
    std::uint32_t getCustomSkin() { return m_dwSkinId; }
    virtual void setCustomCape(std::uint32_t capeId);
    std::uint32_t getCustomCape() { return m_dwCapeId; }

    static std::uint32_t getCapeIdFromPath(const std::string& cape);
    static std::string getCapePathFromId(std::uint32_t capeId);
    static unsigned int getSkinAnimOverrideBitmask(std::uint32_t skinId);

    // 4J Added
    void setXuid(PlayerUID xuid);
    PlayerUID getXuid() { return m_xuid; }
    void setOnlineXuid(PlayerUID xuid) { m_OnlineXuid = xuid; }
    PlayerUID getOnlineXuid() { return m_OnlineXuid; }

    void setPlayerIndex(std::uint8_t index) { m_playerIndex = index; }
    std::uint8_t getPlayerIndex() { return m_playerIndex; }

    void setIsGuest(bool bVal) { m_bIsGuest = bVal; }
    bool isGuest() { return m_bIsGuest; }

    void setShowOnMaps(bool bVal) { m_bShownOnMaps = bVal; }
    bool canShowOnMaps() {
        return m_bShownOnMaps &&
               !getPlayerGamePrivilege(ePlayerGamePrivilege_Invisible);
    }

    virtual void sendMessage(
        const std::string& message,
        ChatPacket::EChatPacketMessage type = ChatPacket::e_ChatCustom,
        int customData = -1, const std::string& additionalMessage = "") {}

private:
    PlayerUID m_xuid;
    PlayerUID m_OnlineXuid;

protected:
    bool m_bShownOnMaps;

    bool m_bIsGuest;

private:
    EDefaultSkins m_skinIndex;
    std::uint32_t m_dwSkinId, m_dwCapeId;

    // 4J Added - Used to show which colour the player is on the map/behind
    // their name
    std::uint8_t m_playerIndex;

    // 4J-PB - to track debug options from the server player
    unsigned int m_uiDebugOptions;

public:
    void SetDebugOptions(unsigned int uiVal) { m_uiDebugOptions = uiVal; }
    unsigned int GetDebugOptions(void) { return m_uiDebugOptions; }

    void StopSleeping() {}

public:
    // If you add things here, you should also add a message to
    // ClientConnection::displayPrivilegeChanges to alert players to changes
    enum EPlayerGamePrivileges {
        ePlayerGamePrivilege_CannotMine =
            0,                             // Only checked if trust system is on
        ePlayerGamePrivilege_CannotBuild,  // Only checked if trust system is on
        ePlayerGamePrivilege_CannotAttackMobs,  // Only checked if trust system
                                                // is on
        ePlayerGamePrivilege_CannotAttackPlayers,  // Only checked if trust
                                                   // system is on
        ePlayerGamePrivilege_Op,
        ePlayerGamePrivilege_CanFly,
        ePlayerGamePrivilege_ClassicHunger,
        ePlayerGamePrivilege_Invisible,
        ePlayerGamePrivilege_Invulnerable,

        ePlayerGamePrivilege_CreativeMode,  // Used only to transfer across
                                            // network, should never be used to
                                            // determine if a player is in
                                            // creative mode

        ePlayerGamePrivilege_CannotAttackAnimals,     // Only checked if trust
                                                      // system is on
        ePlayerGamePrivilege_CanUseDoorsAndSwitches,  // Only checked if trust
                                                      // system is on
        ePlayerGamePrivilege_CanUseContainers,  // Only checked if trust system
                                                // is on

        ePlayerGamePrivilege_CanToggleInvisible,
        ePlayerGamePrivilege_CanToggleFly,
        ePlayerGamePrivilege_CanToggleClassicHunger,
        ePlayerGamePrivilege_CanTeleport,

        // Currently enum is used to bitshift into an unsigned int
        ePlayerGamePrivilege_MAX = 32,
        ePlayerGamePrivilege_All = 33,
        ePlayerGamePrivilege_HOST,
    };

private:
    // 4J Added - Used to track what actions players have been allowed to
    // perform by the host
    unsigned int m_uiGamePrivileges;

    unsigned int getPlayerGamePrivilege(EPlayerGamePrivileges privilege);

public:
    unsigned int getAllPlayerGamePrivileges() {
        return getPlayerGamePrivilege(ePlayerGamePrivilege_All);
    }

    static unsigned int getPlayerGamePrivilege(unsigned int uiGamePrivileges,
                                               EPlayerGamePrivileges privilege);
    void setPlayerGamePrivilege(EPlayerGamePrivileges privilege,
                                unsigned int value);
    static void setPlayerGamePrivilege(unsigned int& uiGamePrivileges,
                                       EPlayerGamePrivileges privilege,
                                       unsigned int value);

    bool isAllowedToUse(Tile* tile);
    bool isAllowedToUse(std::shared_ptr<ItemInstance> item);
    bool isAllowedToInteract(std::shared_ptr<Entity> target);
    bool isAllowedToMine();
    bool isAllowedToAttackPlayers();
    bool isAllowedToAttackAnimals();
    bool isAllowedToHurtEntity(std::shared_ptr<Entity> target);
    bool isAllowedToFly();
    bool isAllowedToIgnoreExhaustion();
    bool isAllowedToTeleport();
    bool hasInvisiblePrivilege();
    bool hasInvulnerablePrivilege();
    bool isModerator();

    static void enableAllPlayerPrivileges(unsigned int& uigamePrivileges,
                                          bool enable);
    void enableAllPlayerPrivileges(bool enable);

    virtual bool canCreateParticles();

public:
    // 4J Stu - Added hooks for the game rules
    virtual void handleCollectItem(std::shared_ptr<ItemInstance> item) {}

    std::vector<ModelPart*>* GetAdditionalModelParts();
    void SetAdditionalModelParts(
        std::vector<ModelPart*>* ppAdditionalModelParts);

private:
    std::vector<ModelPart*>* m_ppAdditionalModelParts;
    bool m_bCheckedForModelParts;
    bool m_bCheckedDLCForModelParts;
};

struct PlayerKeyHash {
    int operator()(const std::shared_ptr<Player> k) const {
        return Player::hash_fnct(k);
    }
};

struct PlayerKeyEq {
    bool operator()(const std::shared_ptr<Player> x,
                    const std::shared_ptr<Player> y) const {
        return Player::eq_test(x, y);
    }
};
