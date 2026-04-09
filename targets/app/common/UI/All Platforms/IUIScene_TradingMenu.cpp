#include "IUIScene_TradingMenu.h"

#include <limits.h>

#include <algorithm>

#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/network/packet/CustomPayloadPacket.h"
#include "minecraft/network/packet/TradeItemPacket.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/Rarity.h"
#include "minecraft/world/item/trading/MerchantRecipe.h"
#include "minecraft/world/item/trading/MerchantRecipeList.h"
#include "strings.h"

IUIScene_TradingMenu::IUIScene_TradingMenu() {
    m_validOffersCount = 0;
    m_selectedSlot = 0;
    m_offersStartIndex = 0;
    m_menu = nullptr;
    m_bHasUpdatedOnce = false;
}

std::shared_ptr<Merchant> IUIScene_TradingMenu::getMerchant() {
    return m_merchant;
}

bool IUIScene_TradingMenu::handleKeyDown(int iPad, int iAction, bool bRepeat) {
    bool handled = false;
    // MerchantRecipeList *offers =
    // m_merchant->getOffers(Minecraft::GetInstance()->localplayers[getPad()]);

    bool changed = false;

    Minecraft* pMinecraft = Minecraft::GetInstance();

    if (pMinecraft->localgameModes[getPad()] != nullptr) {
        Tutorial* tutorial = static_cast<Tutorial*>(
            pMinecraft->localgameModes[getPad()]->getTutorial());
        if (tutorial != nullptr) {
            tutorial->handleUIInput(iAction);
            if (ui.IsTutorialVisible(getPad()) &&
                !tutorial->isInputAllowed(iAction)) {
                return 0;
            }
        }
    }

    switch (iAction) {
        case ACTION_MENU_B:
            ui.ShowTooltip(iPad, eToolTipButtonX, false);
            ui.ShowTooltip(iPad, eToolTipButtonB, false);
            ui.ShowTooltip(iPad, eToolTipButtonA, false);
            ui.ShowTooltip(iPad, eToolTipButtonRB, false);
            // kill the crafting xui
            // ui.PlayUISFX(eSFX_Back);
            ui.CloseUIScenes(iPad);

            handled = true;
            break;
        case ACTION_MENU_A:
            if (!m_activeOffers.empty()) {
                int selectedShopItem = (m_selectedSlot + m_offersStartIndex);
                if (selectedShopItem < m_activeOffers.size()) {
                    MerchantRecipe* activeRecipe =
                        m_activeOffers.at(selectedShopItem).first;
                    if (!activeRecipe->isDeprecated()) {
                        // Do we have the ingredients?
                        std::shared_ptr<ItemInstance> buyAItem =
                            activeRecipe->getBuyAItem();
                        std::shared_ptr<ItemInstance> buyBItem =
                            activeRecipe->getBuyBItem();
                        std::shared_ptr<MultiplayerLocalPlayer> player =
                            Minecraft::GetInstance()->localplayers[getPad()];
                        int buyAMatches =
                            player->inventory->countMatches(buyAItem);
                        int buyBMatches =
                            player->inventory->countMatches(buyBItem);
                        if ((buyAItem != nullptr &&
                             buyAMatches >= buyAItem->count) &&
                            (buyBItem == nullptr ||
                             buyBMatches >= buyBItem->count)) {
                            // 4J-JEV: Fix for PS4 #7111: [PATCH 1.12] Trading
                            // Librarian villagers for multiple �Enchanted
                            // Books� will cause the title to crash.
                            int actualShopItem =
                                m_activeOffers.at(selectedShopItem).second;

                            m_merchant->notifyTrade(activeRecipe);

                            // Remove the items we are purchasing with
                            player->inventory->removeResources(buyAItem);
                            player->inventory->removeResources(buyBItem);

                            // Add the item we have purchased
                            std::shared_ptr<ItemInstance> result =
                                activeRecipe->getSellItem()->copy();
                            if (!player->inventory->add(result)) {
                                player->drop(result);
                            }

                            // Send a packet to the server
                            player->connection->send(
                                std::shared_ptr<TradeItemPacket>(
                                    new TradeItemPacket(m_menu->containerId,
                                                        actualShopItem)));

                            updateDisplay();
                        }
                    }
                }
            }
            handled = true;
            break;
        case ACTION_MENU_LEFT:
            handled = true;
            if (m_selectedSlot == 0) {
                if (m_offersStartIndex > 0) {
                    --m_offersStartIndex;
                    changed = true;
                }
            } else {
                --m_selectedSlot;
                changed = true;
                moveSelector(false);
            }
            break;
        case ACTION_MENU_RIGHT:
            handled = true;
            if (m_selectedSlot == (DISPLAY_TRADES_COUNT - 1)) {
                if ((m_offersStartIndex + DISPLAY_TRADES_COUNT) <
                    m_activeOffers.size()) {
                    ++m_offersStartIndex;
                    changed = true;
                }
            } else {
                ++m_selectedSlot;
                changed = true;
                moveSelector(true);
            }
            break;
    }
    if (changed) {
        updateDisplay();

        int selectedShopItem = (m_selectedSlot + m_offersStartIndex);
        if (selectedShopItem < m_activeOffers.size()) {
            int actualShopItem = m_activeOffers.at(selectedShopItem).second;
            m_menu->setSelectionHint(actualShopItem);

            ByteArrayOutputStream rawOutput;
            DataOutputStream output(&rawOutput);
            output.writeInt(actualShopItem);
            Minecraft::GetInstance()->getConnection(getPad())->send(
                std::shared_ptr<CustomPayloadPacket>(new CustomPayloadPacket(
                    CustomPayloadPacket::TRADER_SELECTION_PACKET,
                    rawOutput.toByteArray())));
        }
    }
    return handled;
}

void IUIScene_TradingMenu::handleTick() {
    int offerCount = 0;
    MerchantRecipeList* offers =
        m_merchant->getOffers(Minecraft::GetInstance()->localplayers[getPad()]);
    if (offers != nullptr) {
        offerCount = offers->size();

        if (!m_bHasUpdatedOnce) {
            updateDisplay();
        }
    }

    showScrollRightArrow((m_offersStartIndex + DISPLAY_TRADES_COUNT) <
                         m_activeOffers.size());
    showScrollLeftArrow(m_offersStartIndex > 0);
}

void IUIScene_TradingMenu::updateDisplay() {
    int iA = -1;

    MerchantRecipeList* unfilteredOffers =
        m_merchant->getOffers(Minecraft::GetInstance()->localplayers[getPad()]);
    if (unfilteredOffers != nullptr) {
        m_activeOffers.clear();
        int unfilteredIndex = 0;
        int firstValidTrade = INT_MAX;
        for (auto it = unfilteredOffers->begin(); it != unfilteredOffers->end();
             ++it) {
            MerchantRecipe* recipe = *it;
            if (!recipe->isDeprecated()) {
                m_activeOffers.push_back(
                    std::pair<MerchantRecipe*, int>(recipe, unfilteredIndex));
                firstValidTrade = std::min(firstValidTrade, unfilteredIndex);
            }
            ++unfilteredIndex;
        }

        if (!m_bHasUpdatedOnce) {
            if (firstValidTrade != 0 &&
                firstValidTrade < unfilteredOffers->size()) {
                m_menu->setSelectionHint(firstValidTrade);

                ByteArrayOutputStream rawOutput;
                DataOutputStream output(&rawOutput);
                output.writeInt(firstValidTrade);
                Minecraft::GetInstance()->getConnection(getPad())->send(
                    std::shared_ptr<CustomPayloadPacket>(
                        new CustomPayloadPacket(
                            CustomPayloadPacket::TRADER_SELECTION_PACKET,
                            rawOutput.toByteArray())));
            }
        }

        if ((m_offersStartIndex + DISPLAY_TRADES_COUNT) >
            m_activeOffers.size()) {
            m_offersStartIndex = m_activeOffers.size() - DISPLAY_TRADES_COUNT;
            if (m_offersStartIndex < 0) m_offersStartIndex = 0;
        }

        for (unsigned int i = 0; i < DISPLAY_TRADES_COUNT; ++i) {
            int offerIndex = i + m_offersStartIndex;
            bool showRedBox = false;
            if (offerIndex < m_activeOffers.size()) {
                showRedBox = !canMake(m_activeOffers.at(offerIndex).first);
                setTradeItem(
                    i, m_activeOffers.at(offerIndex).first->getSellItem());
            } else {
                setTradeItem(i, nullptr);
            }
            setTradeRedBox(i, showRedBox);
        }

        int selectedShopItem = (m_selectedSlot + m_offersStartIndex);
        if (selectedShopItem < m_activeOffers.size()) {
            MerchantRecipe* activeRecipe =
                m_activeOffers.at(selectedShopItem).first;

            std::string wsTemp;

            // 4J-PB - need to get the villager type here
            wsTemp = app.GetString(IDS_VILLAGER_OFFERS_ITEM);
            wsTemp = replaceAll(wsTemp, "{*VILLAGER_TYPE*}",
                                m_merchant->getDisplayName());
            int iPos = wsTemp.find("%s");
            wsTemp.replace(iPos, 2,
                           activeRecipe->getSellItem()->getHoverName());

            setTitle(wsTemp.c_str());

            std::vector<HtmlString>* offerDescription =
                GetItemDescription(activeRecipe->getSellItem());
            setOfferDescription(offerDescription);

            std::shared_ptr<ItemInstance> buyAItem =
                activeRecipe->getBuyAItem();
            std::shared_ptr<ItemInstance> buyBItem =
                activeRecipe->getBuyBItem();

            setRequest1Item(buyAItem);
            setRequest2Item(buyBItem);

            if (buyAItem != nullptr)
                setRequest1Name(buyAItem->getHoverName());
            else
                setRequest1Name("");

            if (buyBItem != nullptr)
                setRequest2Name(buyBItem->getHoverName());
            else
                setRequest2Name("");

            bool canMake = true;

            std::shared_ptr<MultiplayerLocalPlayer> player =
                Minecraft::GetInstance()->localplayers[getPad()];
            int buyAMatches = player->inventory->countMatches(buyAItem);
            if (buyAMatches > 0) {
                setRequest1RedBox(buyAMatches < buyAItem->count);
                canMake = buyAMatches > buyAItem->count;
            } else {
                setRequest1RedBox(true);
                canMake = false;
            }

            int buyBMatches = player->inventory->countMatches(buyBItem);
            if (buyBMatches > 0) {
                setRequest2RedBox(buyBMatches < buyBItem->count);
                canMake = canMake && buyBMatches > buyBItem->count;
            } else {
                if (buyBItem != nullptr) {
                    setRequest2RedBox(true);
                    canMake = false;
                } else {
                    setRequest2RedBox(buyBItem != nullptr);
                    canMake = canMake && buyBItem == nullptr;
                }
            }

            if (canMake) iA = IDS_TOOLTIPS_TRADE;
        } else {
            setTitle(m_merchant->getDisplayName());
            setRequest1Name("");
            setRequest2Name("");
            setRequest1RedBox(false);
            setRequest2RedBox(false);
            setRequest1Item(nullptr);
            setRequest2Item(nullptr);
            std::vector<HtmlString> offerDescription;
            setOfferDescription(&offerDescription);
        }

        m_bHasUpdatedOnce = true;
    }

    ui.SetTooltips(getPad(), iA, IDS_TOOLTIPS_EXIT);
}

bool IUIScene_TradingMenu::canMake(MerchantRecipe* recipe) {
    bool canMake = false;
    if (recipe != nullptr) {
        if (recipe->isDeprecated()) return false;

        std::shared_ptr<ItemInstance> buyAItem = recipe->getBuyAItem();
        std::shared_ptr<ItemInstance> buyBItem = recipe->getBuyBItem();

        std::shared_ptr<MultiplayerLocalPlayer> player =
            Minecraft::GetInstance()->localplayers[getPad()];
        int buyAMatches = player->inventory->countMatches(buyAItem);
        if (buyAMatches > 0) {
            canMake = buyAMatches >= buyAItem->count;
        } else {
            canMake = buyAItem == nullptr;
        }

        int buyBMatches = player->inventory->countMatches(buyBItem);
        if (buyBMatches > 0) {
            canMake = canMake && buyBMatches >= buyBItem->count;
        } else {
            canMake = canMake && buyBItem == nullptr;
        }
    }
    return canMake;
}

void IUIScene_TradingMenu::setRequest1Item(std::shared_ptr<ItemInstance> item) {
}

void IUIScene_TradingMenu::setRequest2Item(std::shared_ptr<ItemInstance> item) {
}

void IUIScene_TradingMenu::setTradeItem(int index,
                                        std::shared_ptr<ItemInstance> item) {}

std::vector<HtmlString>* IUIScene_TradingMenu::GetItemDescription(
    std::shared_ptr<ItemInstance> item) {
    std::vector<HtmlString>* lines = item->getHoverText(nullptr, false);

    // Add rarity to first line
    if (lines->size() > 0) {
        lines->at(0).color = item->getRarity()->color;
    }

    return lines;
}

void IUIScene_TradingMenu::HandleInventoryUpdated() { updateDisplay(); }