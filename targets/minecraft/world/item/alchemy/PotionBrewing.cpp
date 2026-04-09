#include "PotionBrewing.h"

#include <utility>

#include "java/JavaMath.h"
#include "minecraft/GameEnums.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "strings.h"

const int PotionBrewing::DEFAULT_APPEARANCES[] = {
    IDS_POTION_PREFIX_MUNDANE,  IDS_POTION_PREFIX_UNINTERESTING,
    IDS_POTION_PREFIX_BLAND,    IDS_POTION_PREFIX_CLEAR,
    IDS_POTION_PREFIX_MILKY,    IDS_POTION_PREFIX_DIFFUSE,
    IDS_POTION_PREFIX_ARTLESS,  IDS_POTION_PREFIX_THIN,
    IDS_POTION_PREFIX_AWKWARD,  IDS_POTION_PREFIX_FLAT,
    IDS_POTION_PREFIX_BULKY,    IDS_POTION_PREFIX_BUNGLING,
    IDS_POTION_PREFIX_BUTTERED, IDS_POTION_PREFIX_SMOOTH,
    IDS_POTION_PREFIX_SUAVE,    IDS_POTION_PREFIX_DEBONAIR,
    IDS_POTION_PREFIX_THICK,    IDS_POTION_PREFIX_ELEGANT,
    IDS_POTION_PREFIX_FANCY,    IDS_POTION_PREFIX_CHARMING,
    IDS_POTION_PREFIX_DASHING,  IDS_POTION_PREFIX_REFINED,
    IDS_POTION_PREFIX_CORDIAL,  IDS_POTION_PREFIX_SPARKLING,
    IDS_POTION_PREFIX_POTENT,   IDS_POTION_PREFIX_FOUL,
    IDS_POTION_PREFIX_ODORLESS, IDS_POTION_PREFIX_RANK,
    IDS_POTION_PREFIX_HARSH,    IDS_POTION_PREFIX_ACRID,
    IDS_POTION_PREFIX_GROSS,    IDS_POTION_PREFIX_STINKY,
};

// bit 4 is the "enabler," lit by nether seeds

// bits 0-3 are effect identifiers
// 0001 - regeneration
// 0010 - move speed
// 0011 - fire resist
// 0100 - poison
// 0101 - heal
// 0110 - night vision
// 0111 - invisibility
// 1000 - weakness
// 1001 - damage boost
// 1010 - move slow
// 1011 -
// 1100 - harm
// 1101 -
// 1110 -
// 1111 -

/* 4J-JEV: Fix for #81196,
 * Bit 13 is always set in functional potions.
 * Therefore if bit 13 is on, don't use netherwart!
 * Added "&!13" which requires that bit 13 be turned off.
 */
const std::string PotionBrewing::MOD_NETHERWART = "+4&!13";  // "+4"

#if _SIMPLIFIED_BREWING
const std::string PotionBrewing::MOD_WATER = "";
const std::string PotionBrewing::MOD_SUGAR = "-0+1-2-3&4-4+13";
const std::string PotionBrewing::MOD_GHASTTEARS = "+0-1-2-3&4-4+13";
const std::string PotionBrewing::MOD_SPIDEREYE = "-0-1+2-3&4-4+13";
const std::string PotionBrewing::MOD_FERMENTEDEYE = "-0+3-4+13";
const std::string PotionBrewing::MOD_SPECKLEDMELON = "+0-1+2-3&4-4+13";
const std::string PotionBrewing::MOD_BLAZEPOWDER = "+0-1-2+3&4-4+13";
const std::string PotionBrewing::MOD_GOLDENCARROT = "-0+1+2-3+13&4-4";
const std::string PotionBrewing::MOD_MAGMACREAM = "+0+1-2-3&4-4+13";
const std::string PotionBrewing::MOD_REDSTONE =
    "-5+6-7";  // redstone increases duration
const std::string PotionBrewing::MOD_GLOWSTONE =
    "+5-6-7";  // glowstone increases amplification
// 4J Stu - Don't require bit 13 to be set. We don't use it in the creative
// menu. Side effect is you can make a (virtually useless) Splash Mundane potion
// with water bottle and gunpowder
const std::string PotionBrewing::MOD_GUNPOWDER =
    "+14";  //&13-13"; // gunpowder makes them throwable! // gunpowder requires
            // 13 and sets 14
#else
const std::string PotionBrewing::MOD_WATER = "-1-3-5-7-9-11-13";
const std::string PotionBrewing::MOD_SUGAR = "+0";
const std::string PotionBrewing::MOD_GHASTTEARS = "+11";
const std::string PotionBrewing::MOD_SPIDEREYE = "+10+7+5";
const std::string PotionBrewing::MOD_FERMENTEDEYE = "+14+9";
const std::string PotionBrewing::MOD_SPECKLEDMELON = "";
const std::string PotionBrewing::MOD_BLAZEPOWDER = "+14";
const std::string PotionBrewing::MOD_MAGMACREAM = "+14+6+1";
const std::string PotionBrewing::MOD_REDSTONE =
    "";  // redstone increases duration
const std::string PotionBrewing::MOD_GLOWSTONE =
    "";  // glowstone increases amplification
const std::string PotionBrewing::MOD_GUNPOWDER =
    "";  // gunpowder makes them throwable! // gunpowder requires 13 and sets
         // 14
#endif

PotionBrewing::intStringMap PotionBrewing::potionEffectDuration;
PotionBrewing::intStringMap PotionBrewing::potionEffectAmplifier;

std::unordered_map<int, int> PotionBrewing::cachedColors;

void PotionBrewing::staticCtor() {
#if _SIMPLIFIED_BREWING
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::regeneration->getId(), "0 & !1 & !2 & !3 & 0+6"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::movementSpeed->getId(), "!0 & 1 & !2 & !3 & 1+6"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::fireResistance->getId(), "0 & 1 & !2 & !3 & 0+6"));
    potionEffectDuration.insert(
        intStringMap::value_type(MobEffect::heal->getId(), "0 & !1 & 2 & !3"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::poison->getId(), "!0 & !1 & 2 & !3 & 2+6"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::weakness->getId(), "!0 & !1 & !2 & 3 & 3+6"));
    potionEffectDuration.insert(
        intStringMap::value_type(MobEffect::harm->getId(), "!0 & !1 & 2 & 3"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::movementSlowdown->getId(), "!0 & 1 & !2 & 3 & 3+6"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::damageBoost->getId(), "0 & !1 & !2 & 3 & 3+6"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::nightVision->getId(), "!0 & 1 & 2 & !3 & 2+6"));
    potionEffectDuration.insert(intStringMap::value_type(
        MobEffect::invisibility->getId(), "!0 & 1 & 2 & 3 & 2+6"));

    // glowstone increases amplification
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::movementSpeed->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::digSpeed->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::damageBoost->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::regeneration->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::harm->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::heal->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::damageResistance->getId(), "5"));
    potionEffectAmplifier.insert(
        intStringMap::value_type(MobEffect::poison->getId(), "5"));
#else
    potionEffectDuration.put(movementSpeed.getId(),
                             "!10 & !4 & 5*2+0 & >1 | !7 & !4 & 5*2+0 & >1");
    potionEffectDuration.put(movementSlowdown.getId(), "10 & 7 & !4 & 7+5+1-0");
    potionEffectDuration.put(digSpeed.getId(), "2 & 12+2+6-1-7 & <8");
    potionEffectDuration.put(digSlowdown.getId(), "!2 & !1*2-9 & 14-5");
    potionEffectDuration.put(damageBoost.getId(), "9 & 3 & 9+4+5 & <11");
    potionEffectDuration.put(weakness.getId(), "=1>5>7>9+3-7-2-11 & !9 & !0");
    potionEffectDuration.put(heal.getId(), "11 & <6");
    potionEffectDuration.put(harm.getId(), "!11 & 1 & 10 & !7");
    potionEffectDuration.put(jump.getId(), "8 & 2+0 & <5");
    potionEffectDuration.put(confusion.getId(),
                             "8*2-!7+4-11 & !2 | 13 & 11 & 2*3-1-5");
    potionEffectDuration.put(regeneration.getId(), "!14 & 13*3-!0-!5-8");
    potionEffectDuration.put(damageResistance.getId(), "10 & 4 & 10+5+6 & <9");
    potionEffectDuration.put(fireResistance.getId(),
                             "14 & !5 & 6-!1 & 14+13+12");
    potionEffectDuration.put(waterBreathing.getId(),
                             "0+1+12 & !6 & 10 & !11 & !13");
    potionEffectDuration.put(invisibility.getId(), "2+5+13-0-4 & !7 & !1 & >5");
    potionEffectDuration.put(blindness.getId(), "9 & !1 & !5 & !3 & =3");
    potionEffectDuration.put(nightVision.getId(), "8*2-!7 & 5 & !0 & >3");
    potionEffectDuration.put(hunger.getId(), ">4>6>8-3-8+2");
    potionEffectDuration.put(poison.getId(), "12+9 & !13 & !0");

    potionEffectAmplifier.put(movementSpeed.getId(), "7+!3-!1");
    potionEffectAmplifier.put(digSpeed.getId(), "1+0-!11");
    potionEffectAmplifier.put(damageBoost.getId(), "2+7-!12");
    potionEffectAmplifier.put(heal.getId(), "11+!0-!1-!14");
    potionEffectAmplifier.put(harm.getId(), "!11-!14+!0-!1");
    potionEffectAmplifier.put(damageResistance.getId(), "12-!2");
    potionEffectAmplifier.put(poison.getId(), "14>5");
#endif
}

bool PotionBrewing::isWrappedLit(int brew, int position) {
    return (brew & (1 << (position % NUM_BITS))) != 0;
}

bool PotionBrewing::isLit(int brew, int position) {
    return (brew & (1 << position)) != 0;
}

int PotionBrewing::isBit(int brew, int position) {
    return isLit(brew, position) ? 1 : 0;
}

int PotionBrewing::isNotBit(int brew, int position) {
    return isLit(brew, position) ? 0 : 1;
}

int PotionBrewing::getAppearanceValue(int brew) {
    return valueOf(brew, 5, 4, 3, 2, 1);
}

int PotionBrewing::getColorValue(std::vector<MobEffectInstance*>* effects) {
    ColourTable* colourTable = Minecraft::GetInstance()->getColourTable();

    int baseColor = colourTable->getColor(eMinecraftColour_Potion_BaseColour);

    if (effects == nullptr || effects->empty()) {
        return baseColor;
    }

    float red = 0;
    float green = 0;
    float blue = 0;
    float count = 0;

    // for (MobEffectInstance effect : effects){
    for (auto it = effects->begin(); it != effects->end(); ++it) {
        MobEffectInstance* effect = *it;
        int potionColor = colourTable->getColor(
            MobEffect::effects[effect->getId()]->getColor());

        for (int potency = 0; potency <= effect->getAmplifier(); potency++) {
            red += (float)((potionColor >> 16) & 0xff) / 255.0f;
            green += (float)((potionColor >> 8) & 0xff) / 255.0f;
            blue += (float)((potionColor >> 0) & 0xff) / 255.0f;
            count++;
        }
    }

    red = (red / count) * 255.0f;
    green = (green / count) * 255.0f;
    blue = (blue / count) * 255.0f;

    return ((int)red) << 16 | ((int)green) << 8 | ((int)blue);
}

bool PotionBrewing::areAllEffectsAmbient(
    std::vector<MobEffectInstance*>* effects) {
    for (auto it = effects->begin(); it != effects->end(); ++it) {
        MobEffectInstance* effect = *it;
        if (!effect->isAmbient()) return false;
    }

    return true;
}

int PotionBrewing::getColorValue(int brew, bool includeDisabledEffects) {
    if (!includeDisabledEffects) {
        auto colIt = cachedColors.find(brew);
        if (colIt != cachedColors.end()) {
            return colIt->second;  // cachedColors.get(brew);
        }
        std::vector<MobEffectInstance*>* effects = getEffects(brew, false);
        int color = getColorValue(effects);
        if (effects != nullptr) {
            for (auto it = effects->begin(); it != effects->end(); ++it) {
                MobEffectInstance* effect = *it;
                delete effect;
            }
            delete effects;
        }
        cachedColors.insert(std::pair<int, int>(brew, color));
        return color;
    }

    return getColorValue(getEffects(brew, includeDisabledEffects));
}

int PotionBrewing::getSmellValue(int brew) {
    return valueOf(brew, 12, 11, 6, 4, 0);
}

int PotionBrewing::getAppearanceName(int brew) {
    int value = getAppearanceValue(brew);
    return DEFAULT_APPEARANCES[value];
}

int PotionBrewing::constructParsedValue(bool isNot, bool hasMultiplier,
                                        bool isNeg, int countCompare,
                                        int valuePart, int multiplierPart,
                                        int brew) {
    int value = 0;
    if (isNot) {
        value = isNotBit(brew, valuePart);
    }
#if !(_SIMPLIFIED_BREWING)
    else if (countCompare != NO_COUNT)  // Never true for simplified brewing
    {
        if (countCompare == EQUAL_COUNT && countOnes(brew) == valuePart) {
            value = 1;
        } else if (countCompare == GREATER_COUNT &&
                   countOnes(brew) > valuePart) {
            value = 1;
        } else if (countCompare == LESS_COUNT && countOnes(brew) < valuePart) {
            value = 1;
        }
    }
#endif
    else {
        value = isBit(brew, valuePart);
    }
#if !(_SIMPLIFIED_BREWING)
    if (hasMultiplier)  // Always false for simplified brewing
    {
        value *= multiplierPart;
    }
#endif
    if (isNeg) {
        value *= -1;
    }
    return value;
}

int PotionBrewing::countOnes(int brew) {
    int c = 0;
    for (; brew > 0; c++) {
        brew &= brew - 1;
    }
    return c;
}

#if _SIMPLIFIED_BREWING
// 4J Stu - Trimmed this function to remove all the unused features for
// simplified brewing
int PotionBrewing::parseEffectFormulaValue(const std::string& definition,
                                           int start, int end, int brew) {
    if (start >= definition.length() || end < 0 || start >= end) {
        return 0;
    }

    // split by and
    int andIndex = (int)definition.find_first_of('&', start);
    if (andIndex >= 0 && andIndex < end) {
        int leftSide =
            parseEffectFormulaValue(definition, start, andIndex - 1, brew);
        if (leftSide <= 0) {
            return 0;
        }

        int rightSide =
            parseEffectFormulaValue(definition, andIndex + 1, end, brew);
        if (rightSide <= 0) {
            return 0;
        }

        if (leftSide > rightSide) {
            return leftSide;
        }
        return rightSide;
    }

    bool hasMultiplier = false;
    bool hasValue = false;
    bool isNot = false;
    bool isNeg = false;
    int bitCount = NO_COUNT;
    int valuePart = 0;
    int multiplierPart = 0;
    int result = 0;
    for (int i = start; i < end; i++) {
        char current = definition.at(i);
        if (current >= '0' && current <= '9') {
            valuePart *= 10;
            valuePart += (int)(current - '0');
            hasValue = true;
        } else if (current == '!') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = isNeg = isNot = false;
                valuePart = 0;
            }

            isNot = true;
        } else if (current == '-') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = isNeg = isNot = false;
                valuePart = 0;
            }

            isNeg = true;
        } else if (current == '+') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = isNeg = isNot = false;
                valuePart = 0;
            }
        }
    }
    if (hasValue) {
        result += constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                       valuePart, multiplierPart, brew);
    }

    return result;
}
#else
int PotionBrewing::parseEffectFormulaValue(const std::string& definition,
                                           int start, int end, int brew) {
    if (start >= definition.length() || end < 0 || start >= end) {
        return 0;
    }

    // split by or
    int orIndex = definition.find_first_of('|', start);
    if (orIndex >= 0 && orIndex < end) {
        int leftSide =
            parseEffectFormulaValue(definition, start, orIndex - 1, brew);
        if (leftSide > 0) {
            return leftSide;
        }

        int rightSide =
            parseEffectFormulaValue(definition, orIndex + 1, end, brew);
        if (rightSide > 0) {
            return rightSide;
        }
        return 0;
    }
    // split by and
    int andIndex = definition.find_first_of('&', start);
    if (andIndex >= 0 && andIndex < end) {
        int leftSide =
            parseEffectFormulaValue(definition, start, andIndex - 1, brew);
        if (leftSide <= 0) {
            return 0;
        }

        int rightSide =
            parseEffectFormulaValue(definition, andIndex + 1, end, brew);
        if (rightSide <= 0) {
            return 0;
        }

        if (leftSide > rightSide) {
            return leftSide;
        }
        return rightSide;
    }

    bool isMultiplier = false;
    bool hasMultiplier = false;
    bool hasValue = false;
    bool isNot = false;
    bool isNeg = false;
    int bitCount = NO_COUNT;
    int valuePart = 0;
    int multiplierPart = 0;
    int result = 0;
    for (int i = start; i < end; i++) {
        char current = definition.at(i);
        if (current >= '0' && current <= '9') {
            if (isMultiplier) {
                multiplierPart = (int)(current - '0');
                hasMultiplier = true;
            } else {
                valuePart *= 10;
                valuePart += (int)(current - '0');
                hasValue = true;
            }
        } else if (current == '*') {
            isMultiplier = true;
        } else if (current == '!') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = hasMultiplier = isMultiplier = isNeg = isNot = false;
                valuePart = multiplierPart = 0;
                bitCount = NO_COUNT;
            }

            isNot = true;
        } else if (current == '-') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = hasMultiplier = isMultiplier = isNeg = isNot = false;
                valuePart = multiplierPart = 0;
                bitCount = NO_COUNT;
            }

            isNeg = true;
        } else if (current == '=' || current == '<' || current == '>') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = hasMultiplier = isMultiplier = isNeg = isNot = false;
                valuePart = multiplierPart = 0;
                bitCount = NO_COUNT;
            }

            if (current == '=') {
                bitCount = EQUAL_COUNT;
            } else if (current == '<') {
                bitCount = LESS_COUNT;
            } else if (current == '>') {
                bitCount = GREATER_COUNT;
            }
        } else if (current == '+') {
            if (hasValue) {
                result +=
                    constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                         valuePart, multiplierPart, brew);
                hasValue = hasMultiplier = isMultiplier = isNeg = isNot = false;
                valuePart = multiplierPart = 0;
                bitCount = NO_COUNT;
            }
        }
    }
    if (hasValue) {
        result += constructParsedValue(isNot, hasMultiplier, isNeg, bitCount,
                                       valuePart, multiplierPart, brew);
    }

    return result;
}
#endif

std::vector<MobEffectInstance*>* PotionBrewing::getEffects(
    int brew, bool includeDisabledEffects) {
    std::vector<MobEffectInstance*>* list = nullptr;

    // for (MobEffect effect : MobEffect.effects)
    for (unsigned int i = 0; i < MobEffect::NUM_EFFECTS; ++i) {
        MobEffect* effect = MobEffect::effects[i];
        if (effect == nullptr ||
            (effect->isDisabled() && !includeDisabledEffects)) {
            continue;
        }
        // string durationString = potionEffectDuration.get(effect->getId());
        auto effIt = potionEffectDuration.find(effect->getId());
        if (effIt == potionEffectDuration.end()) {
            continue;
        }
        std::string durationString = effIt->second;

        int duration = parseEffectFormulaValue(
            durationString, 0, (int)durationString.length(), brew);
        if (duration > 0) {
            int amplifier = 0;
            auto ampIt = potionEffectAmplifier.find(effect->getId());
            if (ampIt != potionEffectAmplifier.end()) {
                std::string amplifierString = ampIt->second;
                amplifier = parseEffectFormulaValue(
                    amplifierString, 0, (int)amplifierString.length(), brew);
                if (amplifier < 0) {
                    amplifier = 0;
                }
            }

            if (effect->isInstantenous()) {
                duration = 1;
            } else {
                // 3, 8, 13, 18.. minutes
                duration = (SharedConstants::TICKS_PER_SECOND * 60) *
                           (duration * 3 + (duration - 1) * 2);
                duration >>= amplifier;
                duration = (int)Math::round((double)duration *
                                            effect->getDurationModifier());

                if ((brew & THROWABLE_MASK) != 0) {
                    duration = (int)Math::round((double)duration * .75 + .5);
                }
            }

            if (list == nullptr) {
                list = new std::vector<MobEffectInstance*>();
            }
            MobEffectInstance* instance =
                new MobEffectInstance(effect->getId(), duration, amplifier);
            if ((brew & THROWABLE_MASK) != 0) instance->setSplash(true);
            list->push_back(instance);
        }
    }

    return list;
}

#if !(_SIMPLIFIED_BREWING)
int PotionBrewing::boil(int brew) {
    if ((brew & 1) == 0) {
        return brew;
    }

    // save highest bit
    int savedBit = NUM_BITS - 1;
    while ((brew & (1 << savedBit)) == 0 && savedBit >= 0) {
        savedBit--;
    }
    // it's not possible to boil if there are no "empty slots" in front of
    // the last bit
    if (savedBit < 2 || (brew & (1 << (savedBit - 1))) != 0) {
        return brew;
    }
    if (savedBit >= 0) {
        brew &= ~(1 << savedBit);
    }

    brew <<= 1;

    if (savedBit >= 0) {
        brew |= (1 << savedBit);
        brew |= (1 << (savedBit - 1));
    }

    return brew & BREW_MASK;
}

int PotionBrewing::shake(int brew) {
    // save highest bit
    int savedBit = NUM_BITS - 1;
    while ((brew & (1 << savedBit)) == 0 && savedBit >= 0) {
        savedBit--;
    }
    if (savedBit >= 0) {
        brew &= ~(1 << savedBit);
    }

    int currentResult = 0;
    int nextResult = brew;

    while (nextResult != currentResult) {
        nextResult = brew;
        currentResult = 0;
        // evaluate each bit
        for (int bit = 0; bit < NUM_BITS; bit++) {
            bool on = isWrappedLit(brew, bit);
            if (on) {
                if (!isWrappedLit(brew, bit + 1) &&
                    isWrappedLit(brew, bit + 2)) {
                    on = false;
                } else if (!isWrappedLit(brew, bit - 1) &&
                           isWrappedLit(brew, bit - 2)) {
                    on = false;
                }
            } else {
                // turn on if both neighbors are on
                on = isWrappedLit(brew, bit - 1) && isWrappedLit(brew, bit + 1);
            }
            if (on) {
                currentResult |= (1 << bit);
            }
        }
        brew = currentResult;
    }

    if (savedBit >= 0) {
        currentResult |= (1 << savedBit);
    }

    return currentResult & BREW_MASK;
}

int PotionBrewing::stirr(int brew) {
    if ((brew & 1) != 0) {
        brew = boil(brew);
    }
    return shake(brew);
}
#endif

int PotionBrewing::applyBrewBit(int currentBrew, int bit, bool isNeg,
                                bool isNot, bool isRequired) {
    if (isRequired) {
        // 4J-JEV: I wanted to be able to specify that a
        // bit is required to be false.
        if (isLit(currentBrew, bit) == isNot) {
            return 0;
        }
    } else if (isNeg) {
        currentBrew &= ~(1 << bit);
    } else if (isNot) {
        if ((currentBrew & (1 << bit)) == 0) {
            currentBrew |= (1 << bit);
        } else {
            currentBrew &= ~(1 << bit);
        }
    } else {
        currentBrew |= (1 << bit);
    }
    return currentBrew;
}

int PotionBrewing::applyBrew(int currentBrew, const std::string& formula) {
    int start = 0;
    int end = (int)formula.length();

    bool hasValue = false;
    bool isNot = false;
    bool isNeg = false;
    bool isRequired = false;
    int valuePart = 0;
    for (int i = start; i < end; i++) {
        char current = formula.at(i);
        if (current >= '0' && current <= '9') {
            valuePart *= 10;
            valuePart += (int)(current - '0');
            hasValue = true;
        } else if (current == '!') {
            if (hasValue) {
                currentBrew = applyBrewBit(currentBrew, valuePart, isNeg, isNot,
                                           isRequired);
                hasValue = isNeg = isNot = isRequired = false;
                valuePart = 0;
            }

            isNot = true;
        } else if (current == '-') {
            if (hasValue) {
                currentBrew = applyBrewBit(currentBrew, valuePart, isNeg, isNot,
                                           isRequired);
                hasValue = isNeg = isNot = isRequired = false;
                valuePart = 0;
            }

            isNeg = true;
        } else if (current == '+') {
            if (hasValue) {
                currentBrew = applyBrewBit(currentBrew, valuePart, isNeg, isNot,
                                           isRequired);
                hasValue = isNeg = isNot = isRequired = false;
                valuePart = 0;
            }
        } else if (current == '&') {
            if (hasValue) {
                currentBrew = applyBrewBit(currentBrew, valuePart, isNeg, isNot,
                                           isRequired);
                hasValue = isNeg = isNot = isRequired = false;
                valuePart = 0;
            }
            isRequired = true;
        }
    }
    if (hasValue) {
        currentBrew =
            applyBrewBit(currentBrew, valuePart, isNeg, isNot, isRequired);
    }

    return currentBrew & BREW_MASK;
}

int PotionBrewing::setBit(int brew, int position, bool onOff) {
    if (onOff) {
        return brew | (1 << position);
    }
    return brew & ~(1 << position);
}

int PotionBrewing::valueOf(int brew, int p1, int p2, int p3, int p4) {
    return ((isLit(brew, p1) ? 0x08 : 0) | (isLit(brew, p2) ? 0x04 : 0) |
            (isLit(brew, p3) ? 0x02 : 0) | (isLit(brew, p4) ? 0x01 : 0));
}

int PotionBrewing::valueOf(int brew, int p1, int p2, int p3, int p4, int p5) {
    return (isLit(brew, p1) ? 0x10 : 0) | (isLit(brew, p2) ? 0x08 : 0) |
           (isLit(brew, p3) ? 0x04 : 0) | (isLit(brew, p4) ? 0x02 : 0) |
           (isLit(brew, p5) ? 0x01 : 0);
}

std::string PotionBrewing::toString(int brew) {
    std::string string;

    int bit = NUM_BITS - 1;
    while (bit >= 0) {
        if ((brew & (1 << bit)) != 0) {
            string.append("O");
        } else {
            string.append("x");
        }
        bit--;
    }

    return string;
}

// void main(String[] args)
//{

//	HashMap<String, Integer> existingCombinations = new HashMap<String,
// Integer>(); 	HashMap<String, Integer> distinctCombinations = new
// HashMap<String, Integer>(); 	int noEffects = 0; 	for (int brew = 0; brew
// <= BREW_MASK; brew++) { 		List<MobEffectInstance> effects =
// PotionBrewing.getEffects(brew, true); 		if (effects != null) {

//			{
//				StringBuilder builder = new StringBuilder();
//				for (MobEffectInstance effect : effects) {
//					builder.append(effect.toString());
//					builder.append(" ");
//				}
//				String string = builder.toString();
//				Integer count =
// existingCombinations.get(string); 				if (count !=
// null) { 					count++;
// } else { 					count = 1;
//				}
//				existingCombinations.put(string, count);
//			}
//			{
//				StringBuilder builder = new StringBuilder();
//				for (MobEffectInstance effect : effects) {
//					builder.append(effect.getDescriptionId());
//					builder.append(" ");
//				}
//				String string = builder.toString();
//				Integer count =
// distinctCombinations.get(string); 				if (count !=
// null) { 					count++;
// } else { 					count = 1;
//				}
//				distinctCombinations.put(string, count);
//			}
//		} else {
//			noEffects++;
//		}
//	}

//	for (String combination : existingCombinations.keySet()) {
//		Integer count = existingCombinations.get(combination);
//		if (count > 20) {
//			System.out.println(combination + ": " + count);
//		}
//	}

//	System.out.println("Combination with no effects: " + noEffects + " (" +
//((double) noEffects / BREW_MASK * 100.0) + " %)");
// System.out.println("Unique combinations: " + existingCombinations.size());
// System.out.println("Distinct combinations: " + distinctCombinations.size());
//}