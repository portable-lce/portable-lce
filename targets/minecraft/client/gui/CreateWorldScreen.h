#pragma once
#include <string>

#include "Screen.h"

class EditBox;
class LevelStorageSource;
class Button;

class CreateWorldScreen : public Screen {
private:
    Screen* lastScreen;
    EditBox* nameEdit;
    EditBox* seedEdit;
    std::string resultFolder;
    bool done;

    bool moreOptions;
    std::string gameMode;
    bool generateStructures;
    bool bonusChest;
    bool cheatsEnabled;
    bool flatWorld;

    Button* gameModeButton;
    Button* moreWorldOptionsButton;
    Button* generateStructuresButton;
    Button* bonusChestButton;
    Button* worldTypeButton;
    Button* cheatsEnabledButton;

    std::string gameModeDescriptionLine1;
    std::string gameModeDescriptionLine2;
    std::string seed;

public:
    CreateWorldScreen(Screen* lastScreen);
    virtual void tick() override;
    virtual void init() override;

private:
    void updateResultFolder();
    void updateStrings();

public:
    static std::string findAvailableFolderName(LevelStorageSource* levelSource,
                                               const std::string& folder);
    virtual void removed() override;

protected:
    virtual void buttonClicked(Button* button) override;
    virtual void keyPressed(char ch, int eventKey) override;
    virtual void mouseClicked(int x, int y, int buttonNum) override;

public:
    virtual void render(int xm, int ym, float a) override;
    virtual void tabPressed() override;

private:
    int m_iGameModeId;
    bool m_bGameModeCreative;

    struct MoreOptionsParams {
        bool bGenerateOptions;
        bool bStructures;
        bool bFlatWorld;
        bool bBonusChest;
        bool bPVP;
        bool bTrust;
        bool bFireSpreads;
        bool bHostPrivileges;
        bool bTNT;
        bool bMobGriefing;
        bool bKeepInventory;
        bool bDoMobSpawning;
        bool bDoMobLoot;
        bool bDoTileDrops;
        bool bNaturalRegeneration;
        bool bDoDaylightCycle;
        bool bOnlineGame;
        bool bInviteOnly;
        bool bAllowFriendsOfFriends;
        bool bOnlineSettingChangedBySystem;
        bool bCheatsEnabled;
        int dwTexturePack;
        int iPad;
        std::string worldName;
        std::string seed;
    } m_MoreOptionsParams;
};