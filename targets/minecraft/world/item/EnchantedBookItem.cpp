#include "EnchantedBookItem.h"

#include <vector>

#include "java/Random.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/util/WeighedTreasure.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/Rarity.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentInstance.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"

const std::string EnchantedBookItem::TAG_STORED_ENCHANTMENTS =
    "StoredEnchantments";

EnchantedBookItem::EnchantedBookItem(int id) : Item(id) {}

bool EnchantedBookItem::isFoil(std::shared_ptr<ItemInstance> itemInstance) {
    return true;
}

bool EnchantedBookItem::isEnchantable(
    std::shared_ptr<ItemInstance> itemInstance) {
    return false;
}

const Rarity* EnchantedBookItem::getRarity(
    std::shared_ptr<ItemInstance> itemInstance) {
    ListTag<CompoundTag>* enchantments = getEnchantments(itemInstance);
    if (enchantments && enchantments->size() > 0) {
        return Rarity::uncommon;
    } else {
        return Item::getRarity(itemInstance);
    }
}

ListTag<CompoundTag>* EnchantedBookItem::getEnchantments(
    std::shared_ptr<ItemInstance> item) {
    if (item->tag == nullptr ||
        !item->tag->contains((char*)TAG_STORED_ENCHANTMENTS.c_str())) {
        return new ListTag<CompoundTag>();
    }

    return (ListTag<CompoundTag>*)item->tag->get(
        (char*)TAG_STORED_ENCHANTMENTS.c_str());
}

void EnchantedBookItem::appendHoverText(
    std::shared_ptr<ItemInstance> itemInstance, std::shared_ptr<Player> player,
    std::vector<HtmlString>* lines, bool advanced) {
    Item::appendHoverText(itemInstance, player, lines, advanced);

    ListTag<CompoundTag>* list = getEnchantments(itemInstance);

    if (list != nullptr) {
        std::string unformatted = "";
        for (int i = 0; i < list->size(); i++) {
            int type = list->get(i)->getShort((char*)ItemInstance::TAG_ENCH_ID);
            int level =
                list->get(i)->getShort((char*)ItemInstance::TAG_ENCH_LEVEL);

            if (Enchantment::enchantments[type] != nullptr) {
                lines->push_back(
                    Enchantment::enchantments[type]->getFullname(level));
            }
        }
    }
}

void EnchantedBookItem::addEnchantment(std::shared_ptr<ItemInstance> item,
                                       EnchantmentInstance* enchantment) {
    ListTag<CompoundTag>* enchantments = getEnchantments(item);
    bool add = true;

    for (int i = 0; i < enchantments->size(); i++) {
        CompoundTag* tag = enchantments->get(i);

        if (tag->getShort((char*)ItemInstance::TAG_ENCH_ID) ==
            enchantment->enchantment->id) {
            if (tag->getShort((char*)ItemInstance::TAG_ENCH_LEVEL) <
                enchantment->level) {
                tag->putShort((char*)ItemInstance::TAG_ENCH_LEVEL,
                              (short)enchantment->level);
            }

            add = false;
            break;
        }
    }

    if (add) {
        CompoundTag* tag = new CompoundTag();

        tag->putShort((char*)ItemInstance::TAG_ENCH_ID,
                      (short)enchantment->enchantment->id);
        tag->putShort((char*)ItemInstance::TAG_ENCH_LEVEL,
                      (short)enchantment->level);

        enchantments->add(tag);
    }

    if (!item->hasTag()) item->setTag(new CompoundTag());
    item->getTag()->put((char*)TAG_STORED_ENCHANTMENTS.c_str(), enchantments);
}

std::shared_ptr<ItemInstance> EnchantedBookItem::createForEnchantment(
    EnchantmentInstance* enchant) {
    std::shared_ptr<ItemInstance> item = std::make_shared<ItemInstance>(this);
    addEnchantment(item, enchant);
    return item;
}

void EnchantedBookItem::createForEnchantment(
    Enchantment* enchant, std::vector<std::shared_ptr<ItemInstance> >* items) {
    for (int i = enchant->getMinLevel(); i <= enchant->getMaxLevel(); i++) {
        items->push_back(
            createForEnchantment(new EnchantmentInstance(enchant, i)));
    }
}

std::shared_ptr<ItemInstance> EnchantedBookItem::createForRandomLoot(
    Random* random) {
    Enchantment* enchantment = Enchantment::validEnchantments[random->nextInt(
        Enchantment::validEnchantments.size())];
    std::shared_ptr<ItemInstance> book =
        std::make_shared<ItemInstance>(id, 1, 0);
    int level =
        random->nextInt(enchantment->getMinLevel(), enchantment->getMaxLevel());

    addEnchantment(book, new EnchantmentInstance(enchantment, level));

    return book;
}

WeighedTreasure* EnchantedBookItem::createForRandomTreasure(Random* random) {
    return createForRandomTreasure(random, 1, 1, 1);
}

WeighedTreasure* EnchantedBookItem::createForRandomTreasure(Random* random,
                                                            int minCount,
                                                            int maxCount,
                                                            int weight) {
    Enchantment* enchantment = Enchantment::validEnchantments[random->nextInt(
        Enchantment::validEnchantments.size())];
    std::shared_ptr<ItemInstance> book =
        std::make_shared<ItemInstance>(id, 1, 0);
    int level =
        random->nextInt(enchantment->getMinLevel(), enchantment->getMaxLevel());

    addEnchantment(book, new EnchantmentInstance(enchantment, level));

    return new WeighedTreasure(book, minCount, maxCount, weight);
}