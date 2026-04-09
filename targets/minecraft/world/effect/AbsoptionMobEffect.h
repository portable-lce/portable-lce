#pragma once

#include "MobEffect.h"
#include "minecraft/GameEnums.h"

class LivingEntity;

class AbsoptionMobEffect : public MobEffect {
public:
    AbsoptionMobEffect(int id, bool isHarmful, eMinecraftColour color);

    void removeAttributeModifiers(std::shared_ptr<LivingEntity> entity,
                                  BaseAttributeMap* attributes, int amplifier);
    void addAttributeModifiers(std::shared_ptr<LivingEntity> entity,
                               BaseAttributeMap* attributes, int amplifier);
};