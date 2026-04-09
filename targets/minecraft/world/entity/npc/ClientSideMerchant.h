#pragma once

#include <memory>
#include <string>

#include "minecraft/world/item/trading/Merchant.h"

class MerchantContainer;
class MerchantRecipeList;
class MerchantRecipe;
class Container;
class Player;

class ClientSideMerchant
    : public Merchant,
      public std::enable_shared_from_this<ClientSideMerchant> {
private:
    MerchantContainer* container;
    std::shared_ptr<Player> source;
    MerchantRecipeList* currentOffers;
    std::string m_name;

public:
    ClientSideMerchant(std::shared_ptr<Player> source, const std::string& name);
    ~ClientSideMerchant();

    void createContainer();  // 4J Added
    Container* getContainer();
    std::shared_ptr<Player> getTradingPlayer();
    void setTradingPlayer(std::shared_ptr<Player> player);
    MerchantRecipeList* getOffers(std::shared_ptr<Player> forPlayer);
    void overrideOffers(MerchantRecipeList* recipeList);
    void notifyTrade(MerchantRecipe* activeRecipe);
    void notifyTradeUpdated(std::shared_ptr<ItemInstance> item);
    std::string getDisplayName();
};