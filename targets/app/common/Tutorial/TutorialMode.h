#pragma once
// using namespace std;

#include "Tutorial.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"

class ClientConnection;
class Minecraft;
class Tutorial;

class TutorialMode : public MultiPlayerGameMode {
protected:
    Tutorial* tutorial;
    int m_iPad;

    // Function to make this an abstract class
    virtual bool isImplemented() = 0;

public:
    TutorialMode(int iPad, Minecraft* minecraft, ClientConnection* connection);
    virtual ~TutorialMode();

    void startDestroyBlock(int x, int y, int z, int face) override;
    bool destroyBlock(int x, int y, int z, int face) override;
    void tick() override;
    bool useItemOn(std::shared_ptr<Player> player, Level* level,
                   std::shared_ptr<ItemInstance> item, int x, int y, int z,
                   int face, Vec3* hit, bool bTestUseOnly = false,
                   bool* pbUsedItem = nullptr) override;
    void attack(std::shared_ptr<Player> player,
                std::shared_ptr<Entity> entity) override;

    bool isInputAllowed(int mapping) override;

    Tutorial* getTutorial() override { return tutorial; }
};