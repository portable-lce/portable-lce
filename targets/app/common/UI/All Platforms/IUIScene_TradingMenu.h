#pragma once
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "app/common/Tutorial/Tutorial.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/world/inventory/MerchantMenu.h"
#include "minecraft/world/item/Rarity.h"
#include "minecraft/world/item/trading/Merchant.h"
#include "minecraft/world/tutorial/TutorialEnum.h"

class MerchantRecipe;
class HtmlString;
class ItemInstance;

class IUIScene_TradingMenu {
protected:
    MerchantMenu* m_menu;
    std::shared_ptr<Merchant> m_merchant;
    std::vector<std::pair<MerchantRecipe*, int> > m_activeOffers;

    int m_validOffersCount;
    int m_selectedSlot;
    int m_offersStartIndex;
    bool m_bHasUpdatedOnce;

    eTutorial_State m_previousTutorialState;

    static const int DISPLAY_TRADES_COUNT = 7;

    static const int BUY_A = MerchantMenu::USE_ROW_SLOT_END;
    static const int BUY_B = BUY_A + 1;
    static const int TRADES_START = BUY_B + 1;

protected:
    IUIScene_TradingMenu();

    bool handleKeyDown(int iPad, int iAction, bool bRepeat);
    void handleTick();

    virtual void showScrollRightArrow(bool show) = 0;
    virtual void showScrollLeftArrow(bool show) = 0;
    virtual void moveSelector(bool right) = 0;
    virtual void setRequest1Name(const std::string& name) = 0;
    virtual void setRequest2Name(const std::string& name) = 0;
    virtual void setTitle(const std::string& name) = 0;

    virtual void setRequest1RedBox(bool show) = 0;
    virtual void setRequest2RedBox(bool show) = 0;
    virtual void setTradeRedBox(int index, bool show) = 0;

    virtual void setOfferDescription(std::vector<HtmlString>* description) = 0;

    virtual void setRequest1Item(std::shared_ptr<ItemInstance> item);
    virtual void setRequest2Item(std::shared_ptr<ItemInstance> item);
    virtual void setTradeItem(int index, std::shared_ptr<ItemInstance> item);

    void updateDisplay();
    void HandleInventoryUpdated();

private:
    bool canMake(MerchantRecipe* recipe);

    std::vector<HtmlString>* GetItemDescription(
        std::shared_ptr<ItemInstance> item);

public:
    std::shared_ptr<Merchant> getMerchant();

    virtual int getPad() = 0;
};