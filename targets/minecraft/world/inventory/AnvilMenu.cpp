#include "AnvilMenu.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

#include "minecraft/util/Log.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/inventory/RepairContainer.h"
#include "minecraft/world/inventory/RepairResultSlot.h"
#include "minecraft/world/inventory/ResultContainer.h"
#include "minecraft/world/inventory/Slot.h"
#include "minecraft/world/inventory/net.minecraft.world.inventory.ContainerListener.h"
#include "minecraft/world/item/EnchantedBookItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentHelper.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"
#include "nbt/ListTag.h"
#include "strings.h"
#include "util/StringHelpers.h"

AnvilMenu::AnvilMenu(std::shared_ptr<Inventory> inventory, Level* level, int xt,
                     int yt, int zt, std::shared_ptr<Player> player) {
    resultSlots = std::make_shared<ResultContainer>();
    repairSlots = std::shared_ptr<RepairContainer>(
        new RepairContainer(this, IDS_REPAIR_AND_NAME, true, 2));
    cost = 0;
    repairItemCountCost = 0;

    this->level = level;
    x = xt;
    y = yt;
    z = zt;
    this->player = player;

    addSlot(new Slot(repairSlots, INPUT_SLOT, 27, 43 + 4));
    addSlot(new Slot(repairSlots, ADDITIONAL_SLOT, 76, 43 + 4));

    // 4J Stu - Anonymous class here is now RepairResultSlot
    addSlot(new RepairResultSlot(this, xt, yt, zt, resultSlots, RESULT_SLOT,
                                 134, 43 + 4));

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 9; x++) {
            addSlot(
                new Slot(inventory, x + y * 9 + 9, 8 + x * 18, 84 + y * 18));
        }
    }
    for (int x = 0; x < 9; x++) {
        addSlot(new Slot(inventory, x, 8 + x * 18, 142));
    }
}

void AnvilMenu::slotsChanged(std::shared_ptr<Container> container) {
    AbstractContainerMenu::slotsChanged();

    if (container == repairSlots) createResult();
}

void AnvilMenu::createResult() {
    std::shared_ptr<ItemInstance> input = repairSlots->getItem(INPUT_SLOT);
    cost = 0;
    int price = 0;
    int tax = 0;
    int namingCost = 0;

    if (DEBUG_COST) Log::info("----");

    if (input == nullptr) {
        resultSlots->setItem(0, nullptr);
        cost = 0;
        return;
    } else {
        std::shared_ptr<ItemInstance> result = input->copy();
        std::shared_ptr<ItemInstance> addition =
            repairSlots->getItem(ADDITIONAL_SLOT);
        std::unordered_map<int, int>* enchantments =
            EnchantmentHelper::getEnchantments(result);
        bool usingBook = false;

        tax += input->getBaseRepairCost() +
               (addition == nullptr ? 0 : addition->getBaseRepairCost());
        if (DEBUG_COST) {
            Log::info(
                "Starting with base repair tax of %d (%d + %d)\n", tax,
                input->getBaseRepairCost(),
                (addition == nullptr ? 0 : addition->getBaseRepairCost()));
        }

        repairItemCountCost = 0;

        if (addition != nullptr) {
            usingBook =
                addition->id == Item::enchantedBook_Id &&
                Item::enchantedBook->getEnchantments(addition)->size() > 0;

            if (result->isDamageableItem() &&
                Item::items[result->id]->isValidRepairItem(input, addition)) {
                int repairAmount = std::min(result->getDamageValue(),
                                            result->getMaxDamage() / 4);
                if (repairAmount <= 0) {
                    resultSlots->setItem(0, nullptr);
                    cost = 0;
                    return;
                } else {
                    int count = 0;
                    while (repairAmount > 0 && count < addition->count) {
                        int resultDamage =
                            result->getDamageValue() - repairAmount;
                        result->setAuxValue(resultDamage);
                        price += std::max(1, repairAmount / 100) +
                                 enchantments->size();

                        repairAmount = std::min(result->getDamageValue(),
                                                result->getMaxDamage() / 4);
                        count++;
                    }
                    repairItemCountCost = count;
                }
            } else if (!usingBook && (result->id != addition->id ||
                                      !result->isDamageableItem())) {
                resultSlots->setItem(0, nullptr);
                cost = 0;
                return;
            } else {
                if (result->isDamageableItem() && !usingBook) {
                    int remaining1 =
                        input->getMaxDamage() - input->getDamageValue();
                    int remaining2 =
                        addition->getMaxDamage() - addition->getDamageValue();
                    int additional =
                        remaining2 + result->getMaxDamage() * 12 / 100;
                    int remaining = remaining1 + additional;
                    int resultDamage = result->getMaxDamage() - remaining;
                    if (resultDamage < 0) resultDamage = 0;

                    if (resultDamage < result->getAuxValue()) {
                        result->setAuxValue(resultDamage);
                        price += std::max(1, additional / 100);
                        if (DEBUG_COST) {
                            Log::info(
                                "Repairing; price is now %d (went up by %d)\n",
                                price, std::max(1, additional / 100));
                        }
                    }
                }

                std::unordered_map<int, int>* additionalEnchantments =
                    EnchantmentHelper::getEnchantments(addition);

                for (auto it = additionalEnchantments->begin();
                     it != additionalEnchantments->end(); ++it) {
                    int id = it->first;
                    Enchantment* enchantment = Enchantment::enchantments[id];
                    auto localIt = enchantments->find(id);
                    int current =
                        localIt != enchantments->end() ? localIt->second : 0;
                    int level = it->second;
                    level = (current == level) ? level += 1
                                               : std::max(level, current);
                    int extra = level - current;
                    bool compatible = enchantment->canEnchant(input);

                    if (player->abilities.instabuild ||
                        input->id == EnchantedBookItem::enchantedBook_Id)
                        compatible = true;

                    for (auto it2 = enchantments->begin();
                         it2 != enchantments->end(); ++it2) {
                        int other = it2->first;
                        if (other != id &&
                            !enchantment->isCompatibleWith(
                                Enchantment::enchantments[other])) {
                            compatible = false;

                            price += extra;
                            if (DEBUG_COST) {
                                Log::info(
                                    "Enchantment incompatibility fee; price is "
                                    "now %d (went up by %d)\n",
                                    price, extra);
                            }
                        }
                    }

                    if (!compatible) continue;
                    if (level > enchantment->getMaxLevel())
                        level = enchantment->getMaxLevel();
                    (*enchantments)[id] = level;
                    int fee = 0;

                    switch (enchantment->getFrequency()) {
                        case Enchantment::FREQ_COMMON:
                            fee = 1;
                            break;
                        case Enchantment::FREQ_UNCOMMON:
                            fee = 2;
                            break;
                        case Enchantment::FREQ_RARE:
                            fee = 4;
                            break;
                        case Enchantment::FREQ_VERY_RARE:
                            fee = 8;
                            break;
                    }

                    if (usingBook) fee = std::max(1, fee / 2);

                    price += fee * extra;
                    if (DEBUG_COST) {
                        Log::info(
                            "Enchantment increase fee; price is now %d (went "
                            "up by %d)\n",
                            price, fee * extra);
                    }
                }
                delete additionalEnchantments;
            }
        }

        if (itemName.empty()) {
            if (input->hasCustomHoverName()) {
                namingCost = input->isDamageableItem() ? 7 : input->count * 5;

                price += namingCost;
                if (DEBUG_COST) {
                    Log::info("Un-naming cost; price is now %d (went up by %d)",
                              price, namingCost);
                }
                result->resetHoverName();
            }
        } else if (itemName.length() > 0 &&
                   !equalsIgnoreCase(itemName, input->getHoverName()) &&
                   itemName.length() > 0) {
            namingCost = input->isDamageableItem() ? 7 : input->count * 5;

            price += namingCost;
            if (DEBUG_COST) {
                Log::info("Naming cost; price is now %d (went up by %d)", price,
                          namingCost);
            }

            if (input->hasCustomHoverName()) {
                tax += namingCost / 2;

                if (DEBUG_COST) {
                    Log::info(
                        "Already-named tax; tax is now %d (went up by %d)", tax,
                        (namingCost / 2));
                }
            }

            result->setHoverName(itemName);
        }

        int count = 0;
        for (auto it = enchantments->begin(); it != enchantments->end(); ++it) {
            int id = it->first;
            Enchantment* enchantment = Enchantment::enchantments[id];
            int level = it->second;
            int fee = 0;

            count++;

            switch (enchantment->getFrequency()) {
                case Enchantment::FREQ_COMMON:
                    fee = 1;
                    break;
                case Enchantment::FREQ_UNCOMMON:
                    fee = 2;
                    break;
                case Enchantment::FREQ_RARE:
                    fee = 4;
                    break;
                case Enchantment::FREQ_VERY_RARE:
                    fee = 8;
                    break;
            }

            if (usingBook) fee = std::max(1, fee / 2);

            tax += count + level * fee;
            if (DEBUG_COST) {
                Log::info("Enchantment tax; tax is now %d (went up by %d)", tax,
                          (count + level * fee));
            }
        }

        if (usingBook) tax = std::max(1, tax / 2);

        cost = tax + price;
        if (price <= 0) {
            if (DEBUG_COST) Log::info("No purchase, only tax; aborting");
            result = nullptr;
        }
        if (namingCost == price && namingCost > 0 && cost >= 40) {
            if (DEBUG_COST) Log::info("Cost is too high; aborting");
            Log::info(
                "Naming an item only, cost too high; giving discount to cap "
                "cost to 39 levels");
            cost = 39;
        }
        if (cost >= 40 && !player->abilities.instabuild) {
            if (DEBUG_COST) Log::info("Cost is too high; aborting");
            result = nullptr;
        }

        if (result != nullptr) {
            int baseCost = result->getBaseRepairCost();
            if (addition != nullptr && baseCost < addition->getBaseRepairCost())
                baseCost = addition->getBaseRepairCost();
            if (result->hasCustomHoverName()) baseCost -= 9;
            if (baseCost < 0) baseCost = 0;
            baseCost += 2;

            result->setRepairCost(baseCost);
            EnchantmentHelper::setEnchantments(enchantments, result);
        }

        resultSlots->setItem(0, result);
    }

    broadcastChanges();

    if (DEBUG_COST) {
        if (level->isClientSide) {
            Log::info("CLIENT Cost is %d (%d price, %d tax)\n", cost, price,
                      tax);
        } else {
            Log::info("SERVER Cost is %d (%d price, %d tax)\n", cost, price,
                      tax);
        }
    }
}

void AnvilMenu::sendData(int id, int value) {
    AbstractContainerMenu::sendData(id, value);
}

void AnvilMenu::addSlotListener(ContainerListener* listener) {
    AbstractContainerMenu::addSlotListener(listener);
    listener->setContainerData(this, DATA_TOTAL_COST, cost);
}

void AnvilMenu::setData(int id, int value) {
    if (id == DATA_TOTAL_COST) cost = value;
}

void AnvilMenu::removed(std::shared_ptr<Player> player) {
    AbstractContainerMenu::removed(player);
    if (level->isClientSide) return;

    for (int i = 0; i < repairSlots->getContainerSize(); i++) {
        std::shared_ptr<ItemInstance> item = repairSlots->removeItemNoUpdate(i);
        if (item != nullptr) {
            player->drop(item);
        }
    }
}

bool AnvilMenu::stillValid(std::shared_ptr<Player> player) {
    if (level->getTile(x, y, z) != Tile::anvil_Id) return false;
    if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
    return true;
}

std::shared_ptr<ItemInstance> AnvilMenu::quickMoveStack(
    std::shared_ptr<Player> player, int slotIndex) {
    std::shared_ptr<ItemInstance> clicked = nullptr;
    Slot* slot = slots.at(slotIndex);
    if (slot != nullptr && slot->hasItem()) {
        std::shared_ptr<ItemInstance> stack = slot->getItem();
        clicked = stack->copy();

        if (slotIndex == RESULT_SLOT) {
            if (!moveItemStackTo(stack, INV_SLOT_START, USE_ROW_SLOT_END,
                                 true)) {
                return nullptr;
            }
            slot->onQuickCraft(stack, clicked);
        } else if (slotIndex == INPUT_SLOT || slotIndex == ADDITIONAL_SLOT) {
            if (!moveItemStackTo(stack, INV_SLOT_START, USE_ROW_SLOT_END,
                                 false)) {
                return nullptr;
            }
        } else if (slotIndex >= INV_SLOT_START &&
                   slotIndex < USE_ROW_SLOT_END) {
            if (!moveItemStackTo(stack, INPUT_SLOT, RESULT_SLOT, false)) {
                return nullptr;
            }
        }
        if (stack->count == 0) {
            slot->set(nullptr);
        } else {
            slot->setChanged();
        }
        if (stack->count == clicked->count) {
            return nullptr;
        } else {
            slot->onTake(player, stack);
        }
    }
    return clicked;
}

void AnvilMenu::setItemName(const std::string& name) {
    itemName = name;
    if (getSlot(RESULT_SLOT)->hasItem()) {
        std::shared_ptr<ItemInstance> item = getSlot(RESULT_SLOT)->getItem();

        if (name.empty()) {
            item->resetHoverName();
        } else {
            item->setHoverName(itemName);
        }
    }
    createResult();
}