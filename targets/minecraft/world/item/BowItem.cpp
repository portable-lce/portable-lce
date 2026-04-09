#include "BowItem.h"

#include <memory>

#include "java/Random.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentHelper.h"
#include "minecraft/world/level/Level.h"

class Icon;

const std::string BowItem::TEXTURE_PULL[] = {"bow_pull_0", "bow_pull_1",
                                             "bow_pull_2"};

BowItem::BowItem(int id) : Item(id) {
    maxStackSize = 1;
    setMaxDamage(384);

    icons = nullptr;
}

void BowItem::releaseUsing(std::shared_ptr<ItemInstance> itemInstance,
                           Level* level, std::shared_ptr<Player> player,
                           int durationLeft) {
    bool infiniteArrows = player->abilities.instabuild ||
                          EnchantmentHelper::getEnchantmentLevel(
                              Enchantment::arrowInfinite->id, itemInstance) > 0;

    if (infiniteArrows || player->inventory->hasResource(Item::arrow_Id)) {
        int timeHeld = getUseDuration(itemInstance) - durationLeft;
        float pow = timeHeld / (float)MAX_DRAW_DURATION;
        pow = ((pow * pow) + pow * 2) / 3;
        if (pow < 0.1) return;
        if (pow > 1) pow = 1;

        std::shared_ptr<Arrow> arrow =
            std::make_shared<Arrow>(level, player, pow * 2.0f);
        if (pow == 1) arrow->setCritArrow(true);
        int damageBonus = EnchantmentHelper::getEnchantmentLevel(
            Enchantment::arrowBonus->id, itemInstance);
        if (damageBonus > 0) {
            arrow->setBaseDamage(arrow->getBaseDamage() +
                                 (double)damageBonus * .5 + .5);
        }
        int knockbackBonus = EnchantmentHelper::getEnchantmentLevel(
            Enchantment::arrowKnockback->id, itemInstance);
        if (knockbackBonus > 0) {
            arrow->setKnockback(knockbackBonus);
        }
        if (EnchantmentHelper::getEnchantmentLevel(Enchantment::arrowFire->id,
                                                   itemInstance) > 0) {
            arrow->setOnFire(100);
        }
        itemInstance->hurtAndBreak(1, player);

        level->playEntitySound(
            player, eSoundType_RANDOM_BOW, 1.0f,
            1 / (random->nextFloat() * 0.4f + 1.2f) + pow * 0.5f);

        if (infiniteArrows) {
            arrow->pickup = Arrow::PICKUP_CREATIVE_ONLY;
        } else {
            player->inventory->removeResource(Item::arrow_Id);
        }
        if (!level->isClientSide) level->addEntity(arrow);
    }
}

std::shared_ptr<ItemInstance> BowItem::useTimeDepleted(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    return instance;
}

int BowItem::getUseDuration(std::shared_ptr<ItemInstance> itemInstance) {
    return 20 * 60 * 60;
}

UseAnim BowItem::getUseAnimation(std::shared_ptr<ItemInstance> itemInstance) {
    return UseAnim_bow;
}

std::shared_ptr<ItemInstance> BowItem::use(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    if (player->abilities.instabuild ||
        player->inventory->hasResource(Item::arrow_Id)) {
        player->startUsingItem(instance, getUseDuration(instance));
    }
    return instance;
}

int BowItem::getEnchantmentValue() { return 1; }

void BowItem::registerIcons(IconRegister* iconRegister) {
    Item::registerIcons(iconRegister);

    icons = new Icon*[BOW_ICONS_COUNT];

    for (int i = 0; i < BOW_ICONS_COUNT; i++) {
        icons[i] = iconRegister->registerIcon(TEXTURE_PULL[i]);
    }
}

Icon* BowItem::getDrawnIcon(int amount) { return icons[amount]; }