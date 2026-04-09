#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class MobEffectInstance;

class PotionBrewing {
public:
    static inline constexpr int POTION_ID_SPLASH_DAMAGE = 32732;
    static inline constexpr int POTION_ID_SPLASH_WEAKNESS = 32696;
    static inline constexpr int POTION_ID_SPLASH_SLOWNESS = 32698;
    static inline constexpr int POTION_ID_SPLASH_POISON = 32660;
    static inline constexpr int POTION_ID_HEAL = 16341;
    static inline constexpr int POTION_ID_SWIFTNESS = 16274;
    static inline constexpr int POTION_ID_FIRE_RESISTANCE = 16307;

    static const bool SIMPLIFIED_BREWING = true;
    // 4J Stu - Made #define so we can use it to select const initialisation
#define _SIMPLIFIED_BREWING 1

    static inline constexpr int BREWING_TIME_SECONDS = 20;

    static inline constexpr int THROWABLE_BIT = 14;
    static inline constexpr int THROWABLE_MASK = (1 << THROWABLE_BIT);

    static const std::string MOD_WATER;
    static const std::string MOD_SUGAR;
    static const std::string MOD_GHASTTEARS;
    static const std::string MOD_SPIDEREYE;
    static const std::string MOD_FERMENTEDEYE;
    static const std::string MOD_SPECKLEDMELON;
    static const std::string MOD_BLAZEPOWDER;
    static const std::string MOD_MAGMACREAM;
    static const std::string MOD_REDSTONE;
    static const std::string MOD_GLOWSTONE;
    static const std::string MOD_NETHERWART;
    static const std::string MOD_GUNPOWDER;
    static const std::string MOD_GOLDENCARROT;

    static inline constexpr int BITS_FOR_MAX_NORMAL_EFFECT = 0xF;
    static inline constexpr int BITS_FOR_DURATION = (1 << 5);
    static inline constexpr int BITS_FOR_EXTENDED = (1 << 6);
    static inline constexpr int BITS_FOR_NORMAL = (1 << 13);
    static inline constexpr int BITS_FOR_SPLASH = (1 << 14);

private:
    typedef std::unordered_map<int, std::string> intStringMap;
    static intStringMap potionEffectDuration;
    static intStringMap potionEffectAmplifier;

public:
    static void staticCtor();

    static inline constexpr int NUM_BITS = 15;

    // 4J Stu - Made public
    static inline constexpr int BREW_MASK = 0x7fff;

private:
    static inline constexpr int TOP_BIT = 0x4000;

    static bool isWrappedLit(int brew, int position);

public:
    static bool isLit(int brew, int position);

private:
    static int isBit(int brew, int position);
    static int isNotBit(int brew, int position);

public:
    static int getAppearanceValue(int brew);
    static int getColorValue(std::vector<MobEffectInstance*>* effects);
    static bool areAllEffectsAmbient(std::vector<MobEffectInstance*>* effects);

private:
    static std::unordered_map<int, int> cachedColors;

public:
    static int getColorValue(int brew, bool includeDisabledEffects);
    static int getSmellValue(int brew);

private:
    static const int DEFAULT_APPEARANCES[];

public:
    static int getAppearanceName(int brew);

private:
    static inline constexpr int NO_COUNT = -1;
    static inline constexpr int EQUAL_COUNT = 0;
    static inline constexpr int GREATER_COUNT = 1;
    static inline constexpr int LESS_COUNT = 2;

    static int constructParsedValue(bool isNot, bool hasMultiplier, bool isNeg,
                                    int countCompare, int valuePart,
                                    int multiplierPart, int brew);
    static int countOnes(int brew);
    static int parseEffectFormulaValue(const std::string& definition, int start,
                                       int end, int brew);

public:
    static std::vector<MobEffectInstance*>* getEffects(
        int brew, bool includeDisabledEffects);

#if !(_SIMPLIFIED_BREWING)
    static int boil(int brew);
    static int shake(int brew);
    static int stirr(int brew);
#endif

private:
    static int applyBrewBit(int currentBrew, int bit, bool isNeg, bool isNot,
                            bool isRequired);

public:
    static int applyBrew(int currentBrew, const std::string& formula);
    static int setBit(int brew, int position, bool onOff);
    static int valueOf(int brew, int p1, int p2, int p3, int p4);
    static int valueOf(int brew, int p1, int p2, int p3, int p4, int p5);
    static std::string toString(int brew);
    // static void main(String[] args);
};