/*package net.minecraft.world.item.crafting;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import net.minecraft.world.inventory.CraftingContainer;
import net.minecraft.world.item.CoalItem;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemInstance;
import net.minecraft.world.level.tile.StoneSlabTile;
import net.minecraft.world.level.tile.Tile;
*/

#include <format>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Recipy.h"
#include "java/Class.h"
#include "minecraft/world/item/ItemInstance.h"

class Item;
class Level;
class MapItem;
class Tile;

#pragma once

class CraftingContainer;
class FireTile;
class ArmorRecipes;
class ClothDyeRecipes;
class FoodRecipies;
class OreRecipies;
class StructureRecipies;
class ToolRecipies;
class WeaponRecipies;
class ShapedRecipy;
class FireworksRecipe;

typedef std::unordered_map<char, ItemInstance*> myMap;

#define ADD_OBJECT(a, b) a.push_back(new Object(b))

class Object {
public:
    union {
        Tile* tile;
        FireTile* firetile;
        Item* item;
        MapItem* mapitem;
        ItemInstance* iteminstance;
    };

    Object() { eType = eTYPE_NOTSET; }
    Object(Tile* t) {
        eType = eType_TILE;
        tile = t;
    }
    Object(FireTile* t) {
        eType = eType_FIRETILE;
        firetile = t;
    }
    Object(Item* i) {
        eType = eType_ITEM;
        item = i;
    }
    Object(MapItem* i) {
        eType = eType_MAPITEM;
        mapitem = i;
    }
    Object(ItemInstance* i) {
        eType = eType_ITEMINSTANCE;
        iteminstance = i;
    }

    eINSTANCEOF instanceof() { return eType; }
    eINSTANCEOF GetType() { return eType; };

private:
    eINSTANCEOF eType;
};

class Recipes {
public:
    static const int ANY_AUX_VALUE = -1;

private:
    static Recipes* instance;

    std::vector<Recipy*>* recipies;

public:
    static void staticCtor();

public:
    static Recipes* getInstance() { return instance; }

private:
    void _init();  // 4J add
    Recipes();

public:
    ShapedRecipy* addShapedRecipy(ItemInstance*, ...);
    void addShapelessRecipy(ItemInstance* result, ...);

    std::shared_ptr<ItemInstance> getItemFor(
        std::shared_ptr<CraftingContainer> craftSlots, Level* level,
        Recipy* recipesClass = nullptr);  // 4J Added recipesClass param
    std::vector<Recipy*>* getRecipies();

    // 4J-PB - Added all below for new Xbox 'crafting'
    std::shared_ptr<ItemInstance> getItemForRecipe(Recipy* r);
    Recipy::INGREDIENTS_REQUIRED* getRecipeIngredientsArray();

private:
    void buildRecipeIngredientsArray();
    Recipy::INGREDIENTS_REQUIRED* m_pRecipeIngredientsRequired;

public:
    static ToolRecipies* pToolRecipies;
    static WeaponRecipies* pWeaponRecipies;
    static StructureRecipies* pStructureRecipies;
    static OreRecipies* pOreRecipies;
    static FoodRecipies* pFoodRecipies;
    static ClothDyeRecipes* pClothDyeRecipes;
    static ArmorRecipes* pArmorRecipes;
    static FireworksRecipe* pFireworksRecipes;
};
