#include "ThornsEnchantment.h"

#include "app/common/Audio/SoundTypes.h"
#include "java/Random.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/item/ArmorItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentCategory.h"
#include "minecraft/world/item/enchantment/EnchantmentHelper.h"
#include "strings.h"

const float ThornsEnchantment::CHANCE_PER_LEVEl = 0.15f;

ThornsEnchantment::ThornsEnchantment(int id, int freq)
    : Enchantment(id, freq, EnchantmentCategory::armor_torso) {
    setDescriptionId(IDS_ENCHANTMENT_THORNS);
}

int ThornsEnchantment::getMinCost(int level) { return 10 + 20 * (level - 1); }

int ThornsEnchantment::getMaxCost(int level) {
    return Enchantment::getMinCost(level) + 50;
}

int ThornsEnchantment::getMaxLevel() { return 3; }

bool ThornsEnchantment::canEnchant(std::shared_ptr<ItemInstance> item) {
    ArmorItem* armor = dynamic_cast<ArmorItem*>(item->getItem());
    if (armor) return true;
    return Enchantment::canEnchant(item);
}

bool ThornsEnchantment::shouldHit(int level, Random* random) {
    if (level <= 0) return false;
    return random->nextFloat() < CHANCE_PER_LEVEl * level;
}

int ThornsEnchantment::getDamage(int level, Random* random) {
    if (level > 10) {
        return level - 10;
    } else {
        return 1 + random->nextInt(4);
    }
}

void ThornsEnchantment::doThornsAfterAttack(
    std::shared_ptr<Entity> source, std::shared_ptr<LivingEntity> target,
    Random* random) {
    int level = EnchantmentHelper::getArmorThorns(target);
    std::shared_ptr<ItemInstance> item =
        EnchantmentHelper::getRandomItemWith(Enchantment::thorns, target);

    if (shouldHit(level, random)) {
        source->hurt(DamageSource::thorns(target), getDamage(level, random));
        source->playSound(eSoundType_DAMAGE_THORNS, .5f, 1.0f);

        if (item != nullptr) {
            item->hurtAndBreak(3, target);
        }
    } else {
        if (item != nullptr) {
            item->hurtAndBreak(1, target);
        }
    }
}