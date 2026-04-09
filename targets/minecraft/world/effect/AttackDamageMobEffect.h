#pragma once

#include "MobEffect.h"
#include "minecraft/GameEnums.h"

class AttributeModifier;

class AttackDamageMobEffect : public MobEffect {
public:
    AttackDamageMobEffect(int id, bool isHarmful, eMinecraftColour color);

    double getAttributeModifierValue(int amplifier,
                                     AttributeModifier* original);
};