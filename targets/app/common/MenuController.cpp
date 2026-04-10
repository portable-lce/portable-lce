#include "app/common/MenuController.h"

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#include "app/common/Game.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Scenes/UIScene_FullscreenProgress.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/item/MinecartHopper.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/crafting/Recipy.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/HopperTileEntity.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"

unsigned char MenuController::m_szPNG[8] = {137, 80, 78, 71, 13, 10, 26, 10};

MenuController::MenuController() {
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        m_eTMSAction[i] = eTMSAction_Idle;
        m_eXuiAction[i] = eAppAction_Idle;
        m_eXuiActionParam[i] = nullptr;
        m_uiOpacityCountDown[i] = 0;
    }
    m_eGlobalXuiAction = eAppAction_Idle;
}

void MenuController::setAction(int iPad, eXuiAction action, void* param) {
    if ((m_eXuiAction[iPad] == eAppAction_ReloadTexturePack) &&
        (action == eAppAction_EthernetDisconnected)) {
        app.DebugPrintf(
            "Invalid change of App action for pad %d from %d to %d, ignoring\n",
            iPad, m_eXuiAction[iPad], action);
    } else if ((m_eXuiAction[iPad] == eAppAction_ReloadTexturePack) &&
               (action == eAppAction_ExitWorld)) {
        app.DebugPrintf(
            "Invalid change of App action for pad %d from %d to %d, ignoring\n",
            iPad, m_eXuiAction[iPad], action);
    } else if (m_eXuiAction[iPad] == eAppAction_ExitWorldCapturedThumbnail &&
               action != eAppAction_Idle) {
        app.DebugPrintf(
            "Invalid change of App action for pad %d from %d to %d, ignoring\n",
            iPad, m_eXuiAction[iPad], action);
    } else {
        app.DebugPrintf("Changing App action for pad %d from %d to %d\n", iPad,
                        m_eXuiAction[iPad], action);
        m_eXuiAction[iPad] = action;
        m_eXuiActionParam[iPad] = param;
    }
}

bool MenuController::loadInventoryMenu(int iPad,
                                       std::shared_ptr<LocalPlayer> player,
                                       bool bNavigateBack) {
    bool success = true;

    InventoryScreenInput* initData = new InventoryScreenInput();
    initData->player = player;
    initData->bNavigateBack = bNavigateBack;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_InventoryMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_InventoryMenu, initData);
    }

    return success;
}

bool MenuController::loadCreativeMenu(int iPad,
                                      std::shared_ptr<LocalPlayer> player,
                                      bool bNavigateBack) {
    bool success = true;

    InventoryScreenInput* initData = new InventoryScreenInput();
    initData->player = player;
    initData->bNavigateBack = bNavigateBack;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_CreativeMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_CreativeMenu, initData);
    }

    return success;
}

bool MenuController::loadCrafting2x2Menu(int iPad,
                                         std::shared_ptr<LocalPlayer> player) {
    bool success = true;

    CraftingPanelScreenInput* initData = new CraftingPanelScreenInput();
    initData->player = player;
    initData->iContainerType = RECIPE_TYPE_2x2;
    initData->iPad = iPad;
    initData->x = 0;
    initData->y = 0;
    initData->z = 0;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_Crafting2x2Menu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_Crafting2x2Menu, initData);
    }

    return success;
}

bool MenuController::loadCrafting3x3Menu(int iPad,
                                         std::shared_ptr<LocalPlayer> player,
                                         int x, int y, int z) {
    bool success = true;

    CraftingPanelScreenInput* initData = new CraftingPanelScreenInput();
    initData->player = player;
    initData->iContainerType = RECIPE_TYPE_3x3;
    initData->iPad = iPad;
    initData->x = x;
    initData->y = y;
    initData->z = z;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_Crafting3x3Menu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_Crafting3x3Menu, initData);
    }

    return success;
}

bool MenuController::loadFireworksMenu(int iPad,
                                       std::shared_ptr<LocalPlayer> player,
                                       int x, int y, int z) {
    bool success = true;

    FireworksScreenInput* initData = new FireworksScreenInput();
    initData->player = player;
    initData->iPad = iPad;
    initData->x = x;
    initData->y = y;
    initData->z = z;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_FireworksMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_FireworksMenu, initData);
    }

    return success;
}

bool MenuController::loadEnchantingMenu(int iPad,
                                        std::shared_ptr<Inventory> inventory,
                                        int x, int y, int z, Level* level,
                                        const std::string& name) {
    bool success = true;

    EnchantingScreenInput* initData = new EnchantingScreenInput();
    initData->inventory = inventory;
    initData->level = level;
    initData->x = x;
    initData->y = y;
    initData->z = z;
    initData->iPad = iPad;
    initData->name = name;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_EnchantingMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_EnchantingMenu, initData);
    }

    return success;
}

bool MenuController::loadFurnaceMenu(
    int iPad, std::shared_ptr<Inventory> inventory,
    std::shared_ptr<FurnaceTileEntity> furnace) {
    bool success = true;

    FurnaceScreenInput* initData = new FurnaceScreenInput();
    initData->furnace = furnace;
    initData->inventory = inventory;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_FurnaceMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_FurnaceMenu, initData);
    }

    return success;
}

bool MenuController::loadBrewingStandMenu(
    int iPad, std::shared_ptr<Inventory> inventory,
    std::shared_ptr<BrewingStandTileEntity> brewingStand) {
    bool success = true;

    BrewingScreenInput* initData = new BrewingScreenInput();
    initData->brewingStand = brewingStand;
    initData->inventory = inventory;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_BrewingStandMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_BrewingStandMenu, initData);
    }

    return success;
}

bool MenuController::loadContainerMenu(int iPad,
                                       std::shared_ptr<Container> inventory,
                                       std::shared_ptr<Container> container) {
    bool success = true;

    ContainerScreenInput* initData = new ContainerScreenInput();
    initData->inventory = inventory;
    initData->container = container;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;

        bool bLargeChest =
            (initData->container->getContainerSize() > 3 * 9) ? true : false;
        if (bLargeChest) {
            success =
                ui.NavigateToScene(iPad, eUIScene_LargeContainerMenu, initData);
        } else {
            success =
                ui.NavigateToScene(iPad, eUIScene_ContainerMenu, initData);
        }
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_ContainerMenu, initData);
    }

    return success;
}

bool MenuController::loadTrapMenu(int iPad,
                                  std::shared_ptr<Container> inventory,
                                  std::shared_ptr<DispenserTileEntity> trap) {
    bool success = true;

    TrapScreenInput* initData = new TrapScreenInput();
    initData->inventory = inventory;
    initData->trap = trap;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1) {
        initData->bSplitscreen = true;
        success = ui.NavigateToScene(iPad, eUIScene_DispenserMenu, initData);
    } else {
        initData->bSplitscreen = false;
        success = ui.NavigateToScene(iPad, eUIScene_DispenserMenu, initData);
    }

    return success;
}

bool MenuController::loadSignEntryMenu(int iPad,
                                       std::shared_ptr<SignTileEntity> sign) {
    bool success = true;

    SignEntryScreenInput* initData = new SignEntryScreenInput();
    initData->sign = sign;
    initData->iPad = iPad;

    success = ui.NavigateToScene(iPad, eUIScene_SignEntryMenu, initData);

    delete initData;

    return success;
}

bool MenuController::loadRepairingMenu(int iPad,
                                       std::shared_ptr<Inventory> inventory,
                                       Level* level, int x, int y, int z) {
    bool success = true;

    AnvilScreenInput* initData = new AnvilScreenInput();
    initData->inventory = inventory;
    initData->level = level;
    initData->x = x;
    initData->y = y;
    initData->z = z;
    initData->iPad = iPad;
    if (app.GetLocalPlayerCount() > 1)
        initData->bSplitscreen = true;
    else
        initData->bSplitscreen = false;

    success = ui.NavigateToScene(iPad, eUIScene_AnvilMenu, initData);

    return success;
}

bool MenuController::loadTradingMenu(int iPad,
                                     std::shared_ptr<Inventory> inventory,
                                     std::shared_ptr<Merchant> trader,
                                     Level* level, const std::string& name) {
    bool success = true;

    TradingScreenInput* initData = new TradingScreenInput();
    initData->inventory = inventory;
    initData->trader = trader;
    initData->level = level;
    initData->iPad = iPad;
    if (app.GetLocalPlayerCount() > 1)
        initData->bSplitscreen = true;
    else
        initData->bSplitscreen = false;

    success = ui.NavigateToScene(iPad, eUIScene_TradingMenu, initData);

    return success;
}

bool MenuController::loadHopperMenu(int iPad,
                                    std::shared_ptr<Inventory> inventory,
                                    std::shared_ptr<HopperTileEntity> hopper) {
    bool success = true;

    HopperScreenInput* initData = new HopperScreenInput();
    initData->inventory = inventory;
    initData->hopper = hopper;
    initData->iPad = iPad;
    if (app.GetLocalPlayerCount() > 1)
        initData->bSplitscreen = true;
    else
        initData->bSplitscreen = false;

    success = ui.NavigateToScene(iPad, eUIScene_HopperMenu, initData);

    return success;
}

bool MenuController::loadHopperMenu(int iPad,
                                    std::shared_ptr<Inventory> inventory,
                                    std::shared_ptr<MinecartHopper> hopper) {
    bool success = true;

    HopperScreenInput* initData = new HopperScreenInput();
    initData->inventory = inventory;
    initData->hopper = std::dynamic_pointer_cast<Container>(hopper);
    initData->iPad = iPad;
    if (app.GetLocalPlayerCount() > 1)
        initData->bSplitscreen = true;
    else
        initData->bSplitscreen = false;

    success = ui.NavigateToScene(iPad, eUIScene_HopperMenu, initData);

    return success;
}

bool MenuController::loadHorseMenu(int iPad,
                                   std::shared_ptr<Inventory> inventory,
                                   std::shared_ptr<Container> container,
                                   std::shared_ptr<EntityHorse> horse) {
    bool success = true;

    HorseScreenInput* initData = new HorseScreenInput();
    initData->inventory = inventory;
    initData->container = container;
    initData->horse = horse;
    initData->iPad = iPad;
    if (app.GetLocalPlayerCount() > 1)
        initData->bSplitscreen = true;
    else
        initData->bSplitscreen = false;

    success = ui.NavigateToScene(iPad, eUIScene_HorseMenu, initData);

    return success;
}

bool MenuController::loadBeaconMenu(int iPad,
                                    std::shared_ptr<Inventory> inventory,
                                    std::shared_ptr<BeaconTileEntity> beacon) {
    bool success = true;

    BeaconScreenInput* initData = new BeaconScreenInput();
    initData->inventory = inventory;
    initData->beacon = beacon;
    initData->iPad = iPad;
    if (app.GetLocalPlayerCount() > 1)
        initData->bSplitscreen = true;
    else
        initData->bSplitscreen = false;

    success = ui.NavigateToScene(iPad, eUIScene_BeaconMenu, initData);

    return success;
}

int MenuController::texturePackDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    return 0;
}

int MenuController::unlockFullInviteReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    bool bNoPlayer;

    if (pMinecraft->player == nullptr) {
        bNoPlayer = true;
    }

    return 0;
}

int MenuController::unlockFullSaveReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    return 0;
}

int MenuController::unlockFullExitReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    Game* pApp = (Game*)pParam;
    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (result != IPlatformStorage::EMessage_ResultAccept) {
        pApp->SetAction(pMinecraft->player->GetXboxPad(),
                        eAppAction_ExitWorldTrial);
    }

    return 0;
}

int MenuController::trialOverReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result) {
    Game* pApp = (Game*)pParam;
    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (result != IPlatformStorage::EMessage_ResultAccept) {
        pApp->SetAction(pMinecraft->player->GetXboxPad(), eAppAction_ExitTrial);
    }

    return 0;
}

int MenuController::remoteSaveThreadProc(void* lpParameter) {
    Compression::UseDefaultThreadStorage();
    Tile::CreateNewThreadStorage();

    Minecraft* pMinecraft = Minecraft::GetInstance();

    pMinecraft->progressRenderer->progressStartNoAbort(
        IDS_PROGRESS_HOST_SAVING);
    pMinecraft->progressRenderer->progressStage(-1);
    pMinecraft->progressRenderer->progressStagePercentage(0);

    while (!app.GetGameStarted() &&
           app.GetXuiAction(PlatformProfile.GetPrimaryPad()) ==
               eAppAction_WaitRemoteServerSaveComplete) {
        pMinecraft->tickAllConnections();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (app.GetXuiAction(PlatformProfile.GetPrimaryPad()) !=
        eAppAction_WaitRemoteServerSaveComplete) {
        return 1223;  // ERROR_CANCELLED
    }
    app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_Idle);

    ui.UpdatePlayerBasePositions();

    Tile::ReleaseThreadStorage();

    return 0;
}

void MenuController::exitGameFromRemoteSave(void* lpParameter) {
    int primaryPad = PlatformProfile.GetPrimaryPad();

    unsigned int uiIDA[3];
    uiIDA[0] = IDS_CONFIRM_CANCEL;
    uiIDA[1] = IDS_CONFIRM_OK;

    ui.RequestAlertMessage(
        IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 2, primaryPad,
        &MenuController::exitGameFromRemoteSaveDialogReturned, nullptr);
}

int MenuController::exitGameFromRemoteSaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        app.SetAction(iPad, eAppAction_ExitWorld);
    } else {
        UIScene_FullscreenProgress* pScene =
            (UIScene_FullscreenProgress*)ui.FindScene(
                eUIScene_FullscreenProgress);
        if (pScene != nullptr) {
            pScene->SetWasCancelled(false);
        }
    }
    return 0;
}

#define PNG_TAG_tEXt 0x74455874

unsigned int MenuController::fromBigEndian(unsigned int uiValue) {
    unsigned int uiReturn =
        ((uiValue >> 24) & 0x000000ff) | ((uiValue >> 8) & 0x0000ff00) |
        ((uiValue << 8) & 0x00ff0000) | ((uiValue << 24) & 0xff000000);
    return uiReturn;
}

void MenuController::getImageTextData(std::uint8_t* imageData,
                                      unsigned int imageBytes,
                                      unsigned char* seedText,
                                      unsigned int& uiHostOptions,
                                      bool& bHostOptionsRead,
                                      std::uint32_t& uiTexturePack) {
    auto readPngUInt32 = [](const std::uint8_t* data) -> unsigned int {
        unsigned int value = 0;
        std::memcpy(&value, data, sizeof(value));
        return value;
    };

    std::uint8_t* ucPtr = imageData;
    unsigned int uiCount = 0;
    unsigned int uiChunkLen;
    unsigned int uiChunkType;
    unsigned int uiCRC;
    char szKeyword[80];

    for (int i = 0; i < 8; i++) {
        if (m_szPNG[i] != ucPtr[i]) return;
    }

    uiCount += 8;

    while (uiCount < imageBytes) {
        uiChunkLen = fromBigEndian(readPngUInt32(&ucPtr[uiCount]));
        uiCount += sizeof(int);
        uiChunkType = fromBigEndian(readPngUInt32(&ucPtr[uiCount]));
        uiCount += sizeof(int);

        if (uiChunkType == PNG_TAG_tEXt) {
            unsigned char* pszKeyword = &ucPtr[uiCount];
            while (pszKeyword < ucPtr + uiCount + uiChunkLen) {
                memset(szKeyword, 0, 80);
                unsigned int uiKeywordC = 0;
                while (*pszKeyword != 0) {
                    szKeyword[uiKeywordC++] = *pszKeyword;
                    pszKeyword++;
                }
                pszKeyword++;
                if (strcmp(szKeyword, "4J_SEED") == 0) {
                    unsigned int uiValueC = 0;
                    while (*pszKeyword != 0 &&
                           (pszKeyword < ucPtr + uiCount + uiChunkLen)) {
                        seedText[uiValueC++] = *pszKeyword;
                        pszKeyword++;
                    }
                } else if (strcmp(szKeyword, "4J_HOSTOPTIONS") == 0) {
                    bHostOptionsRead = true;
                    unsigned int uiValueC = 0;
                    unsigned char pszHostOptions[9];
                    memset(&pszHostOptions, 0, 9);
                    while (*pszKeyword != 0 &&
                           (pszKeyword < ucPtr + uiCount + uiChunkLen) &&
                           uiValueC < 8) {
                        pszHostOptions[uiValueC++] = *pszKeyword;
                        pszKeyword++;
                    }

                    uiHostOptions = 0;
                    std::stringstream ss;
                    ss << pszHostOptions;
                    ss >> std::hex >> uiHostOptions;
                } else if (strcmp(szKeyword, "4J_TEXTUREPACK") == 0) {
                    unsigned int uiValueC = 0;
                    unsigned char pszTexturePack[9];
                    memset(&pszTexturePack, 0, 9);
                    while (*pszKeyword != 0 &&
                           (pszKeyword < ucPtr + uiCount + uiChunkLen) &&
                           uiValueC < 8) {
                        pszTexturePack[uiValueC++] = *pszKeyword;
                        pszKeyword++;
                    }

                    std::stringstream ss;
                    ss << pszTexturePack;
                    ss >> std::hex >> uiTexturePack;
                }
            }
        }
        uiCount += uiChunkLen;
        uiCRC = fromBigEndian(readPngUInt32(&ucPtr[uiCount]));
        uiCount += sizeof(int);
    }

    return;
}

unsigned int MenuController::createImageTextData(std::uint8_t* textMetadata,
                                                 int64_t seed, bool hasSeed,
                                                 unsigned int uiHostOptions,
                                                 unsigned int uiTexturePackId) {
    int iTextMetadataBytes = 0;
    if (hasSeed) {
        strcpy((char*)textMetadata, "4J_SEED");
        snprintf((char*)&textMetadata[8], 42, "%lld", (long long)seed);

        iTextMetadataBytes += 8;
        while (textMetadata[iTextMetadataBytes] != 0) iTextMetadataBytes++;
        ++iTextMetadataBytes;
    }

    strcpy((char*)&textMetadata[iTextMetadataBytes], "4J_HOSTOPTIONS");
    snprintf((char*)&textMetadata[iTextMetadataBytes + 15], 9, "%X",
             uiHostOptions);

    iTextMetadataBytes += 15;
    while (textMetadata[iTextMetadataBytes] != 0) iTextMetadataBytes++;
    ++iTextMetadataBytes;

    strcpy((char*)&textMetadata[iTextMetadataBytes], "4J_TEXTUREPACK");
    snprintf((char*)&textMetadata[iTextMetadataBytes + 15], 9, "%X",
             uiHostOptions);

    iTextMetadataBytes += 15;
    while (textMetadata[iTextMetadataBytes] != 0) iTextMetadataBytes++;

    return iTextMetadataBytes;
}
