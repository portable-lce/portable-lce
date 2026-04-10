#include "ArmorItem.h"

#include <stdio.h>

#include <format>
#include <vector>

#include "java/Class.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/core/BehaviorRegistry.h"
#include "minecraft/core/BlockSource.h"
#include "minecraft/core/DefaultDispenseItemBehavior.h"
#include "minecraft/core/FacingEnum.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/EntitySelector.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/DispenserTile.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"

class Entity;
class Icon;

const int ArmorItem::healthPerSlot[] = {11, 16, 15, 13};

const std::string ArmorItem::LEATHER_OVERLAYS[] = {
    "helmetCloth_overlay", "chestplateCloth_overlay", "leggingsCloth_overlay",
    "bootsCloth_overlay"};

const std::string ArmorItem::TEXTURE_EMPTY_SLOTS[] = {
    "slot_empty_helmet", "slot_empty_chestplate", "slot_empty_leggings",
    "slot_empty_boots"};

std::shared_ptr<ItemInstance> ArmorItem::ArmorDispenseItemBehavior::execute(
    BlockSource* source, std::shared_ptr<ItemInstance> dispensed,
    eOUTCOME& outcome) {
    FacingEnum* facing = DispenserTile::getFacing(source->getData());
    int x = source->getBlockX() + facing->getStepX();
    int y = source->getBlockY() + facing->getStepY();
    int z = source->getBlockZ() + facing->getStepZ();
    AABB bb = AABB(x, y, z, x + 1, y + 1, z + 1);
    EntitySelector* selector = new MobCanWearArmourEntitySelector(dispensed);
    std::vector<std::shared_ptr<Entity> >* entities =
        source->getWorld()->getEntitiesOfClass(typeid(LivingEntity), &bb,
                                               selector);
    delete selector;

    if (entities->size() > 0) {
        std::shared_ptr<LivingEntity> target =
            std::dynamic_pointer_cast<LivingEntity>(entities->at(0));
        int offset = target->instanceof(eTYPE_PLAYER) ? 1 : 0;
        int slot = Mob::getEquipmentSlotForItem(dispensed);
        std::shared_ptr<ItemInstance> equip = dispensed->copy();
        equip->count = 1;
        target->setEquippedSlot(slot - offset, equip);
        if (target->instanceof(eTYPE_MOB))
            std::dynamic_pointer_cast<Mob>(target)->setDropChance(slot, 2);
        dispensed->count--;

        outcome = ACTIVATED_ITEM;

        delete entities;
        return dispensed;
    } else {
        delete entities;
        return DefaultDispenseItemBehavior::execute(source, dispensed, outcome);
    }
}

typedef ArmorItem::ArmorMaterial _ArmorMaterial;

const int _ArmorMaterial::clothArray[] = {1, 3, 2, 1};
const int _ArmorMaterial::chainArray[] = {2, 5, 4, 1};
const int _ArmorMaterial::ironArray[] = {2, 6, 5, 2};
const int _ArmorMaterial::goldArray[] = {2, 5, 3, 1};
const int _ArmorMaterial::diamondArray[] = {3, 8, 6, 3};
const _ArmorMaterial* _ArmorMaterial::CLOTH =
    new _ArmorMaterial(5, _ArmorMaterial::clothArray, 15);
const _ArmorMaterial* _ArmorMaterial::CHAIN =
    new _ArmorMaterial(15, _ArmorMaterial::chainArray, 12);
const _ArmorMaterial* _ArmorMaterial::IRON =
    new _ArmorMaterial(15, _ArmorMaterial::ironArray, 9);
const _ArmorMaterial* _ArmorMaterial::GOLD =
    new _ArmorMaterial(7, _ArmorMaterial::goldArray, 25);
const _ArmorMaterial* _ArmorMaterial::DIAMOND =
    new _ArmorMaterial(33, _ArmorMaterial::diamondArray, 10);

_ArmorMaterial::ArmorMaterial(int durabilityMultiplier,
                              const int slotProtections[],
                              int enchantmentValue) {
    this->durabilityMultiplier = durabilityMultiplier;
    this->slotProtections = (int*)slotProtections;
    this->enchantmentValue = enchantmentValue;
}

_ArmorMaterial::~ArmorMaterial() { delete[] slotProtections; }

int _ArmorMaterial::getHealthForSlot(int slot) const {
    return healthPerSlot[slot] * durabilityMultiplier;
}

int _ArmorMaterial::getDefenseForSlot(int slot) const {
    return slotProtections[slot];
}

int _ArmorMaterial::getEnchantmentValue() const { return enchantmentValue; }

int _ArmorMaterial::getTierItemId() const {
    if (this == CLOTH) {
        return Item::leather_Id;
    } else if (this == CHAIN) {
        return Item::ironIngot_Id;
    } else if (this == GOLD) {
        return Item::goldIngot_Id;
    } else if (this == IRON) {
        return Item::ironIngot_Id;
    } else if (this == DIAMOND) {
        return Item::diamond_Id;
    }
    return 0;
}

ArmorItem::ArmorItem(int id, const ArmorMaterial* armorType, int icon, int slot)
    : Item(id),
      armorType(armorType),
      slot(slot),
      modelIndex(icon),
      defense(armorType->getDefenseForSlot(slot)) {
    setMaxDamage(armorType->getHealthForSlot(slot));
    maxStackSize = 1;
    DispenserTile::REGISTRY.add(this, new ArmorDispenseItemBehavior());
}

int ArmorItem::getColor(std::shared_ptr<ItemInstance> item, int spriteLayer) {
    if (spriteLayer > 0) {
        return 0xFFFFFF;
    }

    int color = getColor(item);
    if (color < 0) color = 0xFFFFFF;
    return color;
}

bool ArmorItem::hasMultipleSpriteLayers() {
    return armorType == ArmorMaterial::CLOTH;
}

int ArmorItem::getEnchantmentValue() {
    return armorType->getEnchantmentValue();
}

const _ArmorMaterial* ArmorItem::getMaterial() { return armorType; }

bool ArmorItem::hasCustomColor(std::shared_ptr<ItemInstance> item) {
    if (armorType != ArmorMaterial::CLOTH) return false;
    if (!item->hasTag()) return false;
    if (!item->getTag()->contains("display")) return false;
    if (!item->getTag()->getCompound("display")->contains("color"))
        return false;

    return true;
}

int ArmorItem::getColor(std::shared_ptr<ItemInstance> item) {
    if (armorType != ArmorMaterial::CLOTH) return -1;

    CompoundTag* tag = item->getTag();
    if (tag == nullptr)
        return Minecraft::GetInstance()->getColourTable()->getColor(
            DEFAULT_LEATHER_COLOR);
    CompoundTag* display = tag->getCompound("display");
    if (display == nullptr)
        return Minecraft::GetInstance()->getColourTable()->getColor(
            DEFAULT_LEATHER_COLOR);

    if (display->contains("color")) {
        return display->getInt("color");
    } else {
        return Minecraft::GetInstance()->getColourTable()->getColor(
            DEFAULT_LEATHER_COLOR);
    }
}

Icon* ArmorItem::getLayerIcon(int auxValue, int spriteLayer) {
    if (spriteLayer == 1) {
        return overlayIcon;
    }
    return Item::getLayerIcon(auxValue, spriteLayer);
}

void ArmorItem::clearColor(std::shared_ptr<ItemInstance> item) {
    if (armorType != ArmorMaterial::CLOTH) return;
    CompoundTag* tag = item->getTag();
    if (tag == nullptr) return;
    CompoundTag* display = tag->getCompound("display");
    if (display->contains("color")) display->remove("color");
}

void ArmorItem::setColor(std::shared_ptr<ItemInstance> item, int color) {
    if (armorType != ArmorMaterial::CLOTH) {
#ifndef _CONTENT_PACKAGE
        printf("Can't dye non-leather!");
        assert(0);
#endif
        // throw new UnsupportedOperationException("Can't dye non-leather!");
    }

    CompoundTag* tag = item->getTag();

    if (tag == nullptr) {
        tag = new CompoundTag();
        item->setTag(tag);
    }

    CompoundTag* display = tag->getCompound("display");
    if (!tag->contains("display")) tag->putCompound("display", display);

    display->putInt("color", color);
}

bool ArmorItem::isValidRepairItem(std::shared_ptr<ItemInstance> source,
                                  std::shared_ptr<ItemInstance> repairItem) {
    if (armorType->getTierItemId() == repairItem->id) {
        return true;
    }
    return Item::isValidRepairItem(source, repairItem);
}

void ArmorItem::registerIcons(IconRegister* iconRegister) {
    Item::registerIcons(iconRegister);

    if (armorType == ArmorMaterial::CLOTH) {
        overlayIcon = iconRegister->registerIcon(LEATHER_OVERLAYS[slot]);
    }

    iconEmpty = iconRegister->registerIcon(TEXTURE_EMPTY_SLOTS[slot]);
}

Icon* ArmorItem::getEmptyIcon(int slot) {
    switch (slot) {
        case 0:
            return Item::helmet_diamond->iconEmpty;
        case 1:
            return Item::chestplate_diamond->iconEmpty;
        case 2:
            return Item::leggings_diamond->iconEmpty;
        case 3:
            return Item::boots_diamond->iconEmpty;
    }

    return nullptr;
}
