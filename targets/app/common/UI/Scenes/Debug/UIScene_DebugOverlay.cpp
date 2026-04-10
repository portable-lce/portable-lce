
#include "app/common/UI/Scenes/Debug/UIScene_DebugOverlay.h"

#include <wchar.h>

#include <memory>

#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#include "minecraft/GameEnums.h"
#include "platform/profile/profile.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/commands/common/EnchantItemCommand.h"
#include "minecraft/commands/common/GiveItemCommand.h"
#include "minecraft/commands/common/TimeCommand.h"
#include "minecraft/commands/common/ToggleDownfallCommand.h"
#include "minecraft/network/packet/GameCommandPacket.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/level/storage/LevelData.h"

class Player;
class UILayer;
#ifdef _DEBUG_MENUS_ENABLED
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/ServerAction.h"
#include "util/StringHelpers.h"

UIScene_DebugOverlay::UIScene_DebugOverlay(int iPad, void* initData,
                                           UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    Minecraft* pMinecraft = Minecraft::GetInstance();
    char TempString[256];
    snprintf(TempString, 256, "Set fov (%d)",
             (int)pMinecraft->gameRenderer->GetFovVal());
    m_sliderFov.init(TempString, eControl_FOV, 0, 100,
                     (int)pMinecraft->gameRenderer->GetFovVal());

    float currentTime =
        pMinecraft->level->getLevelData()->getGameTime() % 24000;
    snprintf((char*)TempString, 256, "Set time (unsafe) (%d)",
             (int)currentTime);
    m_sliderTime.init(TempString, eControl_Time, 0, 240, currentTime / 100);

    m_buttonRain.init("Toggle Rain", eControl_Rain);
    m_buttonThunder.init("Toggle Thunder", eControl_Thunder);
    m_buttonSchematic.init("Create Schematic", eControl_Schematic);
    m_buttonResetTutorial.init("Reset profile tutorial progress",
                               eControl_ResetTutorial);
    m_buttonSetCamera.init("Set camera", eControl_SetCamera);
    m_buttonSetDay.init("Set Day", eControl_SetDay);
    m_buttonSetNight.init("Set Night", eControl_SetNight);

    m_buttonListItems.init(eControl_Items);

    int listId = 0;
    for (unsigned int i = 0; i < Item::items.size(); ++i) {
        if (Item::items[i] != nullptr) {
            m_itemIds.push_back(i);
            m_buttonListItems.addItem(
                app.GetString(Item::items[i]->getDescriptionId()), listId);
            ++listId;
        }
    }

    m_buttonListEnchantments.init(eControl_Enchantments);

    for (unsigned int i = 0; i < Enchantment::validEnchantments.size(); ++i) {
        Enchantment* ench = Enchantment::validEnchantments.at(i);

        for (unsigned int level = ench->getMinLevel();
             level <= ench->getMaxLevel(); ++level) {
            m_enchantmentIdAndLevels.push_back(
                std::pair<int, int>(ench->id, level));
            m_buttonListEnchantments.addItem(
                app.GetString(ench->getDescriptionId()) +
                toWString<int>(level));
        }
    }

    m_buttonListMobs.init(eControl_Mobs);
    m_buttonListMobs.addItem("Chicken");
    m_mobFactories.push_back(eTYPE_CHICKEN);
    m_buttonListMobs.addItem("Cow");
    m_mobFactories.push_back(eTYPE_COW);
    m_buttonListMobs.addItem("Pig");
    m_mobFactories.push_back(eTYPE_PIG);
    m_buttonListMobs.addItem("Sheep");
    m_mobFactories.push_back(eTYPE_SHEEP);
    m_buttonListMobs.addItem("Squid");
    m_mobFactories.push_back(eTYPE_SQUID);
    m_buttonListMobs.addItem("Wolf");
    m_mobFactories.push_back(eTYPE_WOLF);
    m_buttonListMobs.addItem("Creeper");
    m_mobFactories.push_back(eTYPE_CREEPER);
    m_buttonListMobs.addItem("Ghast");
    m_mobFactories.push_back(eTYPE_GHAST);
    m_buttonListMobs.addItem("Pig Zombie");
    m_mobFactories.push_back(eTYPE_PIGZOMBIE);
    m_buttonListMobs.addItem("Skeleton");
    m_mobFactories.push_back(eTYPE_SKELETON);
    m_buttonListMobs.addItem("Slime");
    m_mobFactories.push_back(eTYPE_SLIME);
    m_buttonListMobs.addItem("Spider");
    m_mobFactories.push_back(eTYPE_SPIDER);
    m_buttonListMobs.addItem("Zombie");
    m_mobFactories.push_back(eTYPE_ZOMBIE);
    m_buttonListMobs.addItem("Enderman");
    m_mobFactories.push_back(eTYPE_ENDERMAN);
    m_buttonListMobs.addItem("Silverfish");
    m_mobFactories.push_back(eTYPE_SILVERFISH);
    m_buttonListMobs.addItem("Cave Spider");
    m_mobFactories.push_back(eTYPE_CAVESPIDER);
    m_buttonListMobs.addItem("Mooshroom");
    m_mobFactories.push_back(eTYPE_MUSHROOMCOW);
    m_buttonListMobs.addItem("Snow Golem");
    m_mobFactories.push_back(eTYPE_SNOWMAN);
    m_buttonListMobs.addItem("Ender Dragon");
    m_mobFactories.push_back(eTYPE_ENDERDRAGON);
    m_buttonListMobs.addItem("Blaze");
    m_mobFactories.push_back(eTYPE_BLAZE);
    m_buttonListMobs.addItem("Magma Cube");
    m_mobFactories.push_back(eTYPE_LAVASLIME);
}

std::string UIScene_DebugOverlay::getMoviePath() { return "DebugMenu"; }

void UIScene_DebugOverlay::customDraw(IggyCustomDrawCallbackRegion* region) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft->localplayers[m_iPad] == nullptr ||
        pMinecraft->localgameModes[m_iPad] == nullptr)
        return;

    int itemId = -1;
    // 4jcraft TODO: UB on our platform since this casts char16_t* to char*
    sscanf((char*)region->name, "item_%d", &itemId);
    if (itemId == -1 || itemId > Item::ITEM_NUM_COUNT ||
        Item::items[itemId] == nullptr) {
        app.DebugPrintf("This is not the control we are looking for\n");
    } else {
        std::shared_ptr<ItemInstance> item =
            std::shared_ptr<ItemInstance>(new ItemInstance(itemId, 1, 0));
        if (item != nullptr)
            customDrawSlotControl(region, m_iPad, item, 1.0f, false, false);
    }
}

void UIScene_DebugOverlay::handleInput(int iPad, int key, bool repeat,
                                       bool pressed, bool released,
                                       bool& handled) {
    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT:
            if (pressed) {
                sendInputToMovie(key, repeat, pressed, released);
            }
            break;
    }
}

void UIScene_DebugOverlay::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Items: {
            app.DebugPrintf(
                "UIScene_DebugOverlay::handlePress for itemsList: %f\n",
                childId);
            int id = childId;
            // app.SetXuiServerAction(m_iPad, eXuiServerAction_DropItem, (void
            // *)m_itemIds[id]);
            ClientConnection* conn = Minecraft::GetInstance()->getConnection(
                PlatformProfile.GetPrimaryPad());
            conn->send(GiveItemCommand::preparePacket(
                std::dynamic_pointer_cast<Player>(
                    Minecraft::GetInstance()
                        ->localplayers[PlatformProfile.GetPrimaryPad()]),
                m_itemIds[id]));
        } break;
        case eControl_Mobs: {
            int id = childId;
            if (id < m_mobFactories.size()) {
                MinecraftServer::getInstance()->queueServerAction(
                    minecraft::server::SpawnDebugMob{
                        0, static_cast<int>(m_mobFactories[id])});
            }
        } break;
        case eControl_Enchantments: {
            int id = childId;
            ClientConnection* conn = Minecraft::GetInstance()->getConnection(
                PlatformProfile.GetPrimaryPad());
            conn->send(EnchantItemCommand::preparePacket(
                std::dynamic_pointer_cast<Player>(
                    Minecraft::GetInstance()
                        ->localplayers[PlatformProfile.GetPrimaryPad()]),
                m_enchantmentIdAndLevels[id].first,
                m_enchantmentIdAndLevels[id].second));
        } break;
        case eControl_Schematic: {
#ifndef _CONTENT_PACKAGE
            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_DebugCreateSchematic, nullptr,
                               eUILayer_Debug);
#endif
        } break;
        case eControl_SetCamera: {
#ifndef _CONTENT_PACKAGE
            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_DebugSetCamera, nullptr,
                               eUILayer_Debug);
#endif
        } break;
        case eControl_Rain: {
            // app.SetXuiServerAction(PlatformProfile.GetPrimaryPad(),eXuiServerAction_ToggleRain);
            ClientConnection* conn = Minecraft::GetInstance()->getConnection(
                PlatformProfile.GetPrimaryPad());
            conn->send(ToggleDownfallCommand::preparePacket());
        } break;
        case eControl_Thunder:
            MinecraftServer::getInstance()->queueServerAction(
                minecraft::server::ToggleThunder{});
            break;
        case eControl_ResetTutorial:
            Tutorial::debugResetPlayerSavedProgress(
                PlatformProfile.GetPrimaryPad());
            break;
        case eControl_SetDay: {
            ClientConnection* conn = Minecraft::GetInstance()->getConnection(
                PlatformProfile.GetPrimaryPad());
            conn->send(TimeCommand::preparePacket(false));
        } break;
        case eControl_SetNight: {
            ClientConnection* conn = Minecraft::GetInstance()->getConnection(
                PlatformProfile.GetPrimaryPad());
            conn->send(TimeCommand::preparePacket(true));
        } break;
    };
}

void UIScene_DebugOverlay::handleSliderMove(F64 sliderId, F64 currentValue) {
    switch ((int)sliderId) {
        case eControl_Time: {
            Minecraft* pMinecraft = Minecraft::GetInstance();

            // Need to set the time on both levels to stop the flickering as the
            // local level tries to predict the time Only works if we are on the
            // host machine, but shouldn't break if not
            MinecraftServer::SetTime(currentValue * 100);
            pMinecraft->level->getLevelData()->setGameTime(currentValue * 100);

            char TempString[256];
            float currentTime = currentValue * 100;
            snprintf(TempString, 256, "Set time (unsafe) (%d)",
                     (int)currentTime);
            m_sliderTime.setLabel(TempString);
        } break;
        case eControl_FOV: {
            Minecraft* pMinecraft = Minecraft::GetInstance();
            pMinecraft->gameRenderer->SetFovVal((float)currentValue);

            char TempString[256];
            snprintf(TempString, 256, "Set fov (%d)", (int)currentValue);
            m_sliderFov.setLabel(TempString);
        } break;
    };
}
#endif
