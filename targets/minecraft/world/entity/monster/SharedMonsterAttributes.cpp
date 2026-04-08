#include "minecraft/util/Log.h"
#include "SharedMonsterAttributes.h"

#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include "minecraft/world/entity/ai/attributes/Attribute.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/attributes/AttributeModifier.h"
#include "minecraft/world/entity/ai/attributes/BaseAttribute.h"
#include "minecraft/world/entity/ai/attributes/BaseAttributeMap.h"
#include "minecraft/world/entity/ai/attributes/RangedAttribute.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"

Attribute* SharedMonsterAttributes::MAX_HEALTH =
    (new RangedAttribute(eAttributeId_GENERIC_MAXHEALTH, 20, 0,
                         std::numeric_limits<double>::max()))
        ->setSyncable(true);
Attribute* SharedMonsterAttributes::FOLLOW_RANGE =
    (new RangedAttribute(eAttributeId_GENERIC_FOLLOWRANGE, 32, 0, 2048));
Attribute* SharedMonsterAttributes::KNOCKBACK_RESISTANCE =
    (new RangedAttribute(eAttributeId_GENERIC_KNOCKBACKRESISTANCE, 0, 0, 1));
Attribute* SharedMonsterAttributes::MOVEMENT_SPEED =
    (new RangedAttribute(eAttributeId_GENERIC_MOVEMENTSPEED, 0.7f, 0,
                         std::numeric_limits<double>::max()))
        ->setSyncable(true);
Attribute* SharedMonsterAttributes::ATTACK_DAMAGE =
    new RangedAttribute(eAttributeId_GENERIC_ATTACKDAMAGE, 2, 0,
                        std::numeric_limits<double>::max());

ListTag<CompoundTag>* SharedMonsterAttributes::saveAttributes(
    BaseAttributeMap* attributes) {
    ListTag<CompoundTag>* list = new ListTag<CompoundTag>();

    std::vector<AttributeInstance*> atts;
    attributes->getAttributes(atts);
    for (auto it = atts.begin(); it != atts.end(); ++it) {
        AttributeInstance* attribute = *it;
        list->add(saveAttribute(attribute));
    }

    return list;
}

CompoundTag* SharedMonsterAttributes::saveAttribute(
    AttributeInstance* instance) {
    CompoundTag* tag = new CompoundTag();
    Attribute* attribute = instance->getAttribute();

    tag->putInt("ID", attribute->getId());
    tag->putDouble("Base", instance->getBaseValue());

    std::unordered_set<AttributeModifier*> modifiers;
    instance->getModifiers(modifiers);

    if (!modifiers.empty()) {
        ListTag<CompoundTag>* list = new ListTag<CompoundTag>();

        for (auto it = modifiers.begin(); it != modifiers.end(); ++it) {
            AttributeModifier* modifier = *it;
            if (modifier->isSerializable()) {
                list->add(saveAttributeModifier(modifier));
            }
        }

        tag->put("Modifiers", list);
    }

    return tag;
}

CompoundTag* SharedMonsterAttributes::saveAttributeModifier(
    AttributeModifier* modifier) {
    CompoundTag* tag = new CompoundTag();

    tag->putDouble("Amount", modifier->getAmount());
    tag->putInt("Operation", modifier->getOperation());
    tag->putInt("UUID", modifier->getId());

    return tag;
}

void SharedMonsterAttributes::loadAttributes(BaseAttributeMap* attributes,
                                             ListTag<CompoundTag>* list) {
    for (int i = 0; i < list->size(); i++) {
        CompoundTag* tag = list->get(i);
        AttributeInstance* instance = attributes->getInstance(
            static_cast<eATTRIBUTE_ID>(tag->getInt("ID")));

        if (instance != nullptr) {
            loadAttribute(instance, tag);
        } else {
            Log::info("Ignoring unknown attribute '%d'",
                            tag->getInt("ID"));
        }
    }
}

void SharedMonsterAttributes::loadAttribute(AttributeInstance* instance,
                                            CompoundTag* tag) {
    instance->setBaseValue(tag->getDouble("Base"));

    if (tag->contains("Modifiers")) {
        ListTag<CompoundTag>* list =
            (ListTag<CompoundTag>*)tag->getList("Modifiers");

        for (int i = 0; i < list->size(); i++) {
            AttributeModifier* modifier = loadAttributeModifier(list->get(i));
            AttributeModifier* old = instance->getModifier(modifier->getId());
            if (old != nullptr) instance->removeModifier(old);
            instance->addModifier(modifier);
        }
    }
}

AttributeModifier* SharedMonsterAttributes::loadAttributeModifier(
    CompoundTag* tag) {
    eMODIFIER_ID id = (eMODIFIER_ID)tag->getInt("UUID");
    return new AttributeModifier(id, tag->getDouble("Amount"),
                                 tag->getInt("Operation"));
}
