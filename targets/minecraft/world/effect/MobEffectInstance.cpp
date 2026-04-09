#include "minecraft/world/effect/MobEffectInstance.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "minecraft/util/Log.h"
#include "minecraft/world/effect/MobEffect.h"
#include "nbt/CompoundTag.h"

class LivingEntity;

void MobEffectInstance::_init(int id, int duration, int amplifier) {
    this->id = id;
    this->duration = duration;
    this->amplifier = amplifier;

    splash = false;
    ambient = false;
    noCounter = false;
}

MobEffectInstance::MobEffectInstance(int id) { _init(id, 0, 0); }

MobEffectInstance::MobEffectInstance(int id, int duration) {
    _init(id, duration, 0);
}

MobEffectInstance::MobEffectInstance(int id, int duration, int amplifier) {
    _init(id, duration, amplifier);
}

MobEffectInstance::MobEffectInstance(int id, int duration, int amplifier,
                                     bool ambient) {
    _init(id, duration, amplifier);
    this->ambient = ambient;
}

MobEffectInstance::MobEffectInstance(MobEffectInstance* copy) {
    this->id = copy->id;
    this->duration = copy->duration;
    this->amplifier = copy->amplifier;
    this->splash = copy->splash;
    this->ambient = copy->ambient;
    this->noCounter = copy->noCounter;
}

void MobEffectInstance::update(MobEffectInstance* takeOver) {
    if (id != takeOver->id) {
        Log::info("This method should only be called for matching effects!");
    }
    if (takeOver->amplifier > amplifier) {
        amplifier = takeOver->amplifier;
        duration = takeOver->duration;
    } else if (takeOver->amplifier == amplifier &&
               duration < takeOver->duration) {
        duration = takeOver->duration;
    } else if (!takeOver->ambient && ambient) {
        ambient = takeOver->ambient;
    }
}

int MobEffectInstance::getId() { return id; }

int MobEffectInstance::getDuration() { return duration; }

int MobEffectInstance::getAmplifier() { return amplifier; }

bool MobEffectInstance::isSplash() { return splash; }

void MobEffectInstance::setSplash(bool splash) { this->splash = splash; }

bool MobEffectInstance::isAmbient() { return ambient; }

/**
 * Runs the effect on a Mob target.
 *
 * @param target
 * @return True if the effect is still active.
 */
bool MobEffectInstance::tick(std::shared_ptr<LivingEntity> target) {
    if (duration > 0) {
        if (MobEffect::effects[id]->isDurationEffectTick(duration, amplifier)) {
            applyEffect(target);
        }
        tickDownDuration();
    }
    return duration > 0;
}

int MobEffectInstance::tickDownDuration() { return --duration; }

void MobEffectInstance::applyEffect(std::shared_ptr<LivingEntity> mob) {
    if (duration > 0) {
        MobEffect::effects[id]->applyEffectTick(mob, amplifier);
    }
}

int MobEffectInstance::getDescriptionId() {
    return MobEffect::effects[id]->getDescriptionId();
}

// 4J Added
int MobEffectInstance::getPostfixDescriptionId() {
    return MobEffect::effects[id]->getPostfixDescriptionId();
}

int MobEffectInstance::hashCode() {
    // return id;

    // 4J Stu - Changed this to return a value that represents id, amp and
    // duration
    return (id & 0xff) | ((amplifier & 0xff) << 8) |
           ((duration & 0xffff) << 16);
}

std::string MobEffectInstance::toString() {
    std::string result =
        "MobEffectInstance::toString - NON IMPLEMENTED OR LOCALISED FUNCTION";
    // string result = "";
    // if (getAmplifier() > 0)
    //{
    //	result = getDescriptionId() + " x " + (getAmplifier() + 1) + ",
    // Duration: " + getDuration();
    // }
    // else
    //{
    //	result = getDescriptionId() + ", Duration: " + getDuration();
    // }
    // if (MobEffect.effects[id].isDisabled())
    //{
    //	return "(" + result + ")";
    // }
    return result;
}

// Was bool equals(Object obj)
bool MobEffectInstance::equals(MobEffectInstance* instance) {
    return id == instance->id && amplifier == instance->amplifier &&
           duration == instance->duration && splash == instance->splash &&
           ambient == instance->ambient;
}

CompoundTag* MobEffectInstance::save(CompoundTag* tag) {
    tag->putByte("Id", (uint8_t)getId());
    tag->putByte("Amplifier", (uint8_t)getAmplifier());
    tag->putInt("Duration", getDuration());
    tag->putBoolean("Ambient", isAmbient());
    return tag;
}

MobEffectInstance* MobEffectInstance::load(CompoundTag* tag) {
    int id = tag->getByte("Id");
    int amplifier = tag->getByte("Amplifier");
    int duration = tag->getInt("Duration");
    bool ambient = tag->getBoolean("Ambient");
    return new MobEffectInstance(id, duration, amplifier, ambient);
}

void MobEffectInstance::setNoCounter(bool noCounter) {
    this->noCounter = noCounter;
}

bool MobEffectInstance::isNoCounter() { return noCounter; }