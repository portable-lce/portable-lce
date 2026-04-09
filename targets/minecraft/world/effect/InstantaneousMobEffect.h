#pragma once

#include "MobEffect.h"
#include "minecraft/GameEnums.h"

class InstantenousMobEffect : public MobEffect {
public:
    InstantenousMobEffect(int id, bool isHarmful, eMinecraftColour color);
    bool isInstantenous();
    bool isDurationEffectTick(int remainingDuration, int amplification);
};