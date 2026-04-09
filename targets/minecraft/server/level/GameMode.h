#pragma once

#include <memory>

class Minecraft;
class Level;
class Player;
class ItemInstance;
class Entity;

class ITutorial;

class GameMode {
protected:
    Minecraft* minecraft;

public:
    bool instaBuild;

    GameMode(Minecraft* minecraft);
    virtual ~GameMode() {}

    virtual void initLevel(Level* level);
    virtual void startDestroyBlock(int x, int y, int z, int face) = 0;
    virtual bool destroyBlock(int x, int y, int z, int face);
    virtual void continueDestroyBlock(int x, int y, int z, int face) = 0;
    virtual void stopDestroyBlock() = 0;
    virtual void render(float a);
    virtual float getPickRange() = 0;
    virtual void initPlayer(std::shared_ptr<Player> player);
    virtual void tick();
    virtual bool canHurtPlayer() = 0;
    virtual void adjustPlayer(std::shared_ptr<Player> player);
    virtual bool useItem(std::shared_ptr<Player> player, Level* level,
                         std::shared_ptr<ItemInstance> item,
                         bool bTestUseOnly = false);
    virtual bool useItemOn(std::shared_ptr<Player> player, Level* level,
                           std::shared_ptr<ItemInstance> item, int x, int y,
                           int z, int face, bool bTestUseOnOnly = false,
                           bool* pbUsedItem = nullptr) = 0;

    virtual std::shared_ptr<Player> createPlayer(Level* level);
    virtual bool interact(std::shared_ptr<Player> player,
                          std::shared_ptr<Entity> entity);
    virtual void attack(std::shared_ptr<Player> player,
                        std::shared_ptr<Entity> entity);
    virtual std::shared_ptr<ItemInstance> handleInventoryMouseClick(
        int containerId, int slotNum, int buttonNum, bool quickKeyHeld,
        std::shared_ptr<Player> player);
    virtual void handleCloseInventory(int containerId,
                                      std::shared_ptr<Player> player);
    virtual void handleInventoryButtonClick(int containerId, int buttonId);

    virtual bool isCutScene();
    virtual void releaseUsingItem(std::shared_ptr<Player> player);
    virtual bool hasExperience();
    virtual bool hasMissTime();
    virtual bool hasInfiniteItems();
    virtual bool hasFarPickRange();
    virtual void handleCreativeModeItemAdd(
        std::shared_ptr<ItemInstance> clicked, int i);
    virtual void handleCreativeModeItemDrop(
        std::shared_ptr<ItemInstance> clicked);

    // 4J Stu - Added so we can send packets for this in the network game
    virtual bool handleCraftItem(int recipe, std::shared_ptr<Player> player);
    virtual void handleDebugOptions(unsigned int uiVal,
                                    std::shared_ptr<Player> player);

    // 4J Stu - Added for tutorial checks
    virtual bool isInputAllowed(int mapping) { return true; }
    virtual bool isTutorial() { return false; }
    virtual ITutorial* getTutorial() { return nullptr; }
};
