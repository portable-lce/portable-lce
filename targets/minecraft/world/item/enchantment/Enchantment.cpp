#include "Enchantment.h"

#include <assert.h>
#include <wchar.h>

#include "minecraft/IGameServices.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/ArrowDamageEnchantment.h"
#include "minecraft/world/item/enchantment/ArrowFireEnchantment.h"
#include "minecraft/world/item/enchantment/ArrowInfiniteEnchantment.h"
#include "minecraft/world/item/enchantment/ArrowKnockbackEnchantment.h"
#include "minecraft/world/item/enchantment/DamageEnchantment.h"
#include "minecraft/world/item/enchantment/DigDurabilityEnchantment.h"
#include "minecraft/world/item/enchantment/DiggingEnchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentCategory.h"
#include "minecraft/world/item/enchantment/FireAspectEnchantment.h"
#include "minecraft/world/item/enchantment/KnockbackEnchantment.h"
#include "minecraft/world/item/enchantment/LootBonusEnchantment.h"
#include "minecraft/world/item/enchantment/OxygenEnchantment.h"
#include "minecraft/world/item/enchantment/ProtectionEnchantment.h"
#include "minecraft/world/item/enchantment/ThornsEnchantment.h"
#include "minecraft/world/item/enchantment/UntouchingEnchantment.h"
#include "minecraft/world/item/enchantment/WaterWorkerEnchantment.h"
#include "strings.h"

// Enchantment *Enchantment::enchantments[256];
std::vector<Enchantment*> Enchantment::enchantments =
    std::vector<Enchantment*>(256);
std::vector<Enchantment*> Enchantment::validEnchantments;

Enchantment* Enchantment::allDamageProtection = nullptr;
Enchantment* Enchantment::fireProtection = nullptr;
Enchantment* Enchantment::fallProtection = nullptr;
Enchantment* Enchantment::explosionProtection = nullptr;
Enchantment* Enchantment::projectileProtection = nullptr;
Enchantment* Enchantment::drownProtection = nullptr;
Enchantment* Enchantment::waterWorker = nullptr;
Enchantment* Enchantment::thorns = nullptr;

// weapon
Enchantment* Enchantment::damageBonus = nullptr;
Enchantment* Enchantment::damageBonusUndead = nullptr;
Enchantment* Enchantment::damageBonusArthropods = nullptr;
Enchantment* Enchantment::knockback = nullptr;
Enchantment* Enchantment::fireAspect = nullptr;
Enchantment* Enchantment::lootBonus = nullptr;

// digger
Enchantment* Enchantment::diggingBonus = nullptr;
Enchantment* Enchantment::untouching = nullptr;
Enchantment* Enchantment::digDurability = nullptr;
Enchantment* Enchantment::resourceBonus = nullptr;

// bows
Enchantment* Enchantment::arrowBonus = nullptr;
Enchantment* Enchantment::arrowKnockback = nullptr;
Enchantment* Enchantment::arrowFire = nullptr;
Enchantment* Enchantment::arrowInfinite = nullptr;

void Enchantment::staticCtor() {
    allDamageProtection =
        new ProtectionEnchantment(0, FREQ_COMMON, ProtectionEnchantment::ALL);
    fireProtection = new ProtectionEnchantment(1, FREQ_UNCOMMON,
                                               ProtectionEnchantment::FIRE);
    fallProtection = new ProtectionEnchantment(2, FREQ_UNCOMMON,
                                               ProtectionEnchantment::FALL);
    explosionProtection = new ProtectionEnchantment(
        3, FREQ_RARE, ProtectionEnchantment::EXPLOSION);
    projectileProtection = new ProtectionEnchantment(
        4, FREQ_UNCOMMON, ProtectionEnchantment::PROJECTILE);
    drownProtection = new OxygenEnchantment(5, FREQ_RARE);
    waterWorker = new WaterWorkerEnchantment(6, FREQ_RARE);
    thorns = new ThornsEnchantment(7, FREQ_VERY_RARE);

    // weapon
    damageBonus =
        new DamageEnchantment(16, FREQ_COMMON, DamageEnchantment::ALL);
    damageBonusUndead =
        new DamageEnchantment(17, FREQ_UNCOMMON, DamageEnchantment::UNDEAD);
    damageBonusArthropods =
        new DamageEnchantment(18, FREQ_UNCOMMON, DamageEnchantment::ARTHROPODS);
    knockback = new KnockbackEnchantment(19, FREQ_UNCOMMON);
    fireAspect = new FireAspectEnchantment(20, FREQ_RARE);
    lootBonus =
        new LootBonusEnchantment(21, FREQ_RARE, EnchantmentCategory::weapon);

    // digger
    diggingBonus = new DiggingEnchantment(32, FREQ_COMMON);
    untouching = new UntouchingEnchantment(33, FREQ_VERY_RARE);
    digDurability = new DigDurabilityEnchantment(34, FREQ_UNCOMMON);
    resourceBonus =
        new LootBonusEnchantment(35, FREQ_RARE, EnchantmentCategory::digger);

    // bows
    arrowBonus = new ArrowDamageEnchantment(48, FREQ_COMMON);
    arrowKnockback = new ArrowKnockbackEnchantment(49, FREQ_RARE);
    arrowFire = new ArrowFireEnchantment(50, FREQ_RARE);
    arrowInfinite = new ArrowInfiniteEnchantment(51, FREQ_VERY_RARE);

    for (unsigned int i = 0; i < 256; ++i) {
        Enchantment* enchantment = enchantments[i];
        if (enchantment != nullptr) {
            validEnchantments.push_back(enchantment);
        }
    }
}

void Enchantment::_init(int id) {
    if (enchantments[id] != nullptr) {
        Log::info("Duplicate enchantment id!");
#ifndef _CONTENT_PACKAGE
        assert(0);
#endif
        // throw new IllegalArgumentException("Duplicate enchantment id!");
    }
    enchantments[id] = this;
}

Enchantment::Enchantment(int id, int frequency,
                         const EnchantmentCategory* category)
    : id(id), frequency(frequency), category(category) {
    _init(id);
}

Enchantment::Enchantment(int id)
    : id(id), frequency(FREQ_COMMON), category(EnchantmentCategory::all) {
    _init(id);
}

int Enchantment::getFrequency() { return frequency; }

int Enchantment::getMinLevel() { return 1; }

int Enchantment::getMaxLevel() { return 1; }

int Enchantment::getMinCost(int level) { return 1 + level * 10; }

int Enchantment::getMaxCost(int level) { return getMinCost(level) + 5; }

int Enchantment::getDamageProtection(int level, DamageSource* source) {
    return 0;
}

float Enchantment::getDamageBonus(int level,
                                  std::shared_ptr<LivingEntity> target) {
    return 0.0f;
}

bool Enchantment::isCompatibleWith(Enchantment* other) const {
    return this != other;
}

Enchantment* Enchantment::setDescriptionId(int id) {
    descriptionId = id;
    return this;
}

int Enchantment::getDescriptionId() { return descriptionId; }

// 4jcraft: re-added old TU18 overload for java gui
std::string Enchantment::getFullname(int level, std::string& unformatted) {
    char formatted[256];
    snprintf(formatted, 256, "%s %s",
             gameServices().getString(getDescriptionId()),
             getLevelString(level).c_str());
    unformatted = formatted;
    snprintf(formatted, 256, "<font color=\"#%08x\">%s</font>",
             gameServices().getHTMLColour(eHTMLColor_f), unformatted.c_str());
    return formatted;
}

HtmlString Enchantment::getFullname(int level) {
    char formatted[256];
    snprintf(formatted, 256, "%s %s",
             gameServices().getString(getDescriptionId()),
             getLevelString(level).c_str());

    return HtmlString(formatted, eHTMLColor_f);
}

bool Enchantment::canEnchant(std::shared_ptr<ItemInstance> item) {
    return category->canEnchant(item->getItem());
}

// 4J Added
std::string Enchantment::getLevelString(int level) {
    int stringId = IDS_ENCHANTMENT_LEVEL_1;
    switch (level) {
        case 2:
            stringId = IDS_ENCHANTMENT_LEVEL_2;
            break;
        case 3:
            stringId = IDS_ENCHANTMENT_LEVEL_3;
            break;
        case 4:
            stringId = IDS_ENCHANTMENT_LEVEL_4;
            break;
        case 5:
            stringId = IDS_ENCHANTMENT_LEVEL_5;
            break;
        case 6:
            stringId = IDS_ENCHANTMENT_LEVEL_6;
            break;
        case 7:
            stringId = IDS_ENCHANTMENT_LEVEL_7;
            break;
        case 8:
            stringId = IDS_ENCHANTMENT_LEVEL_8;
            break;
        case 9:
            stringId = IDS_ENCHANTMENT_LEVEL_9;
            break;
        case 10:
            stringId = IDS_ENCHANTMENT_LEVEL_10;
            break;
    };
    return gameServices().getString(
        stringId);  // I18n.get("enchantment.level." + level);
}