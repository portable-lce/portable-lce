#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "app/common/App_structs.h"
#include "minecraft/XuiActionPayload.h"
#include "platform/storage/storage.h"
#include "platform/XboxStubs.h"

class Player;
class Inventory;
class Level;
class FurnaceTileEntity;
class Container;
class DispenserTileEntity;
class SignTileEntity;
class BrewingStandTileEntity;
class HopperTileEntity;
class MinecartHopper;
class EntityHorse;
class BeaconTileEntity;
class LocalPlayer;
class Merchant;
class CommandBlockEntity;

class MenuController {
public:
    MenuController();

    // Load menu methods
    bool loadInventoryMenu(int iPad, std::shared_ptr<LocalPlayer> player,
                           bool bNavigateBack = false);
    bool loadCreativeMenu(int iPad, std::shared_ptr<LocalPlayer> player,
                          bool bNavigateBack = false);
    bool loadEnchantingMenu(int iPad, std::shared_ptr<Inventory> inventory,
                            int x, int y, int z, Level* level,
                            const std::string& name);
    bool loadFurnaceMenu(int iPad, std::shared_ptr<Inventory> inventory,
                         std::shared_ptr<FurnaceTileEntity> furnace);
    bool loadBrewingStandMenu(
        int iPad, std::shared_ptr<Inventory> inventory,
        std::shared_ptr<BrewingStandTileEntity> brewingStand);
    bool loadContainerMenu(int iPad, std::shared_ptr<Container> inventory,
                           std::shared_ptr<Container> container);
    bool loadTrapMenu(int iPad, std::shared_ptr<Container> inventory,
                      std::shared_ptr<DispenserTileEntity> trap);
    bool loadCrafting2x2Menu(int iPad, std::shared_ptr<LocalPlayer> player);
    bool loadCrafting3x3Menu(int iPad, std::shared_ptr<LocalPlayer> player,
                             int x, int y, int z);
    bool loadFireworksMenu(int iPad, std::shared_ptr<LocalPlayer> player,
                           int x, int y, int z);
    bool loadSignEntryMenu(int iPad, std::shared_ptr<SignTileEntity> sign);
    bool loadRepairingMenu(int iPad, std::shared_ptr<Inventory> inventory,
                           Level* level, int x, int y, int z);
    bool loadTradingMenu(int iPad, std::shared_ptr<Inventory> inventory,
                         std::shared_ptr<Merchant> trader, Level* level,
                         const std::string& name);
    bool loadHopperMenu(int iPad, std::shared_ptr<Inventory> inventory,
                        std::shared_ptr<HopperTileEntity> hopper);
    bool loadHopperMenu(int iPad, std::shared_ptr<Inventory> inventory,
                        std::shared_ptr<MinecartHopper> hopper);
    bool loadHorseMenu(int iPad, std::shared_ptr<Inventory> inventory,
                       std::shared_ptr<Container> container,
                       std::shared_ptr<EntityHorse> horse);
    bool loadBeaconMenu(int iPad, std::shared_ptr<Inventory> inventory,
                        std::shared_ptr<BeaconTileEntity> beacon);

    // Action management
    void setAction(int iPad, eXuiAction action, void* param = nullptr);
    eXuiAction getXuiAction(int iPad) { return m_eXuiAction[iPad]; }
    void setXuiServerAction(int iPad, eXuiServerAction action,
                            XuiActionPayload param = {}) {
        m_eXuiServerAction[iPad] = action;
        m_eXuiServerActionParam[iPad] = std::move(param);
    }
    eXuiServerAction getXuiServerAction(int iPad) {
        return m_eXuiServerAction[iPad];
    }
    const XuiActionPayload& getXuiServerActionParam(int iPad) {
        return m_eXuiServerActionParam[iPad];
    }
    eXuiAction getGlobalXuiAction() { return m_eGlobalXuiAction; }
    void setGlobalXuiAction(eXuiAction action) { m_eGlobalXuiAction = action; }
    eXuiServerAction getGlobalXuiServerAction() {
        return m_eGlobalXuiServerAction;
    }
    void setGlobalXuiServerAction(eXuiServerAction action) {
        m_eGlobalXuiServerAction = action;
    }

    // TMS action
    void setTMSAction(int iPad, eTMSAction action) {
        m_eTMSAction[iPad] = action;
    }
    eTMSAction getTMSAction(int iPad) { return m_eTMSAction[iPad]; }

    // Dialog callbacks
    static int texturePackDialogReturned(void* pParam, int iPad,
                                         IPlatformStorage::EMessageResult result);
    static int fatalErrorDialogReturned(void* pParam, int iPad,
                                        IPlatformStorage::EMessageResult result);
    static int trialOverReturned(void* pParam, int iPad,
                                 IPlatformStorage::EMessageResult result);
    static int unlockFullExitReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result);
    static int unlockFullSaveReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result);
    static int unlockFullInviteReturned(void* pParam, int iPad,
                                        IPlatformStorage::EMessageResult result);

    // Remote save
    static int remoteSaveThreadProc(void* lpParameter);
    static void exitGameFromRemoteSave(void* lpParameter);
    static int exitGameFromRemoteSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);

    // Image text data
    void getImageTextData(std::uint8_t* imageData, unsigned int imageBytes,
                          unsigned char* seedText, unsigned int& uiHostOptions,
                          bool& bHostOptionsRead, std::uint32_t& uiTexturePack);
    unsigned int createImageTextData(std::uint8_t* textMetadata, int64_t seed,
                                     bool hasSeed, unsigned int uiHostOptions,
                                     unsigned int uiTexturePackId);

    // Opacity timer
    unsigned int getOpacityTimer(int iPad) {
        return m_uiOpacityCountDown[iPad];
    }
    void setOpacityTimer(int iPad) { m_uiOpacityCountDown[iPad] = 120; }
    void tickOpacityTimer(int iPad) {
        if (m_uiOpacityCountDown[iPad] > 0) m_uiOpacityCountDown[iPad]--;
    }

    // Action param accessor (needed by HandleXuiActions)
    void* getXuiActionParam(int iPad) { return m_eXuiActionParam[iPad]; }

private:
    eXuiAction m_eXuiAction[XUSER_MAX_COUNT];
    eTMSAction m_eTMSAction[XUSER_MAX_COUNT];
    void* m_eXuiActionParam[XUSER_MAX_COUNT];
    eXuiAction m_eGlobalXuiAction;
    eXuiServerAction m_eXuiServerAction[XUSER_MAX_COUNT];
    XuiActionPayload m_eXuiServerActionParam[XUSER_MAX_COUNT];
    eXuiServerAction m_eGlobalXuiServerAction;

    unsigned int m_uiOpacityCountDown[XUSER_MAX_COUNT];

    static unsigned char m_szPNG[8];
    unsigned int fromBigEndian(unsigned int uiValue);
};
