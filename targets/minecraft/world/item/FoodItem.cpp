#include "FoodItem.h"

#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/food/FoodConstants.h"
#include "minecraft/world/food/FoodData.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"

void FoodItem::_init() {
    // 4J Initialisers
    canAlwaysEat = false;
    effectId = 0;
    effectDurationSeconds = 0;
    effectAmplifier = 0;
    effectProbability = 0.0f;
}

FoodItem::FoodItem(int id, int nutrition, float saturationMod, bool isMeat)
    : Item(id),
      nutrition(nutrition),
      saturationModifier(saturationMod),
      m_isMeat(isMeat) {
    _init();
}

FoodItem::FoodItem(int id, int nutrition, bool isMeat)
    : Item(id),
      nutrition(nutrition),
      saturationModifier(FoodConstants::FOOD_SATURATION_NORMAL),
      m_isMeat(isMeat) {
    _init();
}

std::shared_ptr<ItemInstance> FoodItem::useTimeDepleted(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    instance->count--;
    player->getFoodData()->eat(this);
    // 4J - new sound brought forward from 1.2.3
    level->playEntitySound(player, eSoundType_RANDOM_BURP, 0.5f,
                           level->random->nextFloat() * 0.1f + 0.9f);

    addEatEffect(instance, level, player);

    return instance;
}

void FoodItem::addEatEffect(std::shared_ptr<ItemInstance> instance,
                            Level* level, std::shared_ptr<Player> player) {
    if (!level->isClientSide && effectId > 0 &&
        level->random->nextFloat() < effectProbability) {
        player->addEffect(new MobEffectInstance(
            effectId, effectDurationSeconds * SharedConstants::TICKS_PER_SECOND,
            effectAmplifier));
    }
}

int FoodItem::getUseDuration(std::shared_ptr<ItemInstance> itemInstance) {
    return EAT_DURATION;
}

UseAnim FoodItem::getUseAnimation(std::shared_ptr<ItemInstance> itemInstance) {
    return UseAnim_eat;
}

std::shared_ptr<ItemInstance> FoodItem::use(
    std::shared_ptr<ItemInstance> instance, Level* level,
    std::shared_ptr<Player> player) {
    if (player->canEat(canAlwaysEat)) {
        player->startUsingItem(instance, getUseDuration(instance));
    }

    // 4J : WESTY : Other award ... eating cooked pork chop.
    // 4J-JEV: This is just for an avatar award on the xbox.

    return instance;
}

float FoodItem::getSaturationModifier() { return saturationModifier; }

int FoodItem::getNutrition() { return nutrition; }

bool FoodItem::isMeat() { return m_isMeat; }

FoodItem* FoodItem::setEatEffect(int id, int durationInSecods, int amplifier,
                                 float effectProbability) {
    effectId = id;
    effectDurationSeconds = durationInSecods;
    effectAmplifier = amplifier;
    this->effectProbability = effectProbability;
    return this;
}

FoodItem* FoodItem::setCanAlwaysEat() {
    canAlwaysEat = true;
    return this;
}

// 4J Added
bool FoodItem::canEat(std::shared_ptr<Player> player) {
    return player->canEat(canAlwaysEat);
}
