#include "Achievement.h"

#include <vector>

#include "Achievements.h"
#include "DescFormatter.h"
#include "minecraft/locale/I18n.h"
#include "minecraft/stats/Stat.h"
#include "minecraft/world/item/ItemInstance.h"

class Item;
class Tile;

/**
 * @class Achievement
 * @brief Represents a Minecraft achievement.
 *
 * Achievements are stat objects that can be unlocked by the player.
 * Each achievement has a position in the achievement tree
 * a description and an optional icon and prerequisite.
 *
 * Use postConstruct() to register the achievement globally.
 */

/**
 * @brief Performs internal initialization for the achievement.
 *
 * Updates the global achievement grid bounds.
 * These bounds are used for rendering the
 * achievement UI.
 */
void Achievement::_init() {
    isGoldenVar = false;

    if (x < Achievements::xMin) Achievements::xMin = x;
    if (y < Achievements::yMin) Achievements::yMin = y;
    if (x > Achievements::xMax) Achievements::xMax = x;
    if (y > Achievements::yMax) Achievements::yMax = y;
}

/**
 * @brief Creates an achievement with an item icon.
 *
 * @param id Local achievement ID
 * @param name Internal achievement name used for localization
 * @param x X position in the achievement tree
 * @param y Y position in the achievement tree
 * @param icon Item used as the achievement icon
 * @param prerequisite Achievement object that is required to unlock this one
 */
Achievement::Achievement(int id, const std::string& name, int x, int y,
                         Item* icon, Achievement* prerequisite)
    : Stat(Achievements::ACHIEVEMENT_OFFSET + id,
           I18n::get(std::string("achievement.").append(name))),
      desc(I18n::get(std::string("achievement.").append(name).append(".desc"))),
      icon(new ItemInstance(icon)),
      x(x),
      y(y),
      prerequisite(prerequisite) {}

Achievement::Achievement(int id, const std::string& name, int x, int y,
                         Tile* icon, Achievement* prerequisite)
    : Stat(Achievements::ACHIEVEMENT_OFFSET + id,
           I18n::get(std::string("achievement.").append(name))),
      desc(I18n::get(std::string("achievement.").append(name).append(".desc"))),
      icon(new ItemInstance(icon)),
      x(x),
      y(y),
      prerequisite(prerequisite) {}

Achievement::Achievement(int id, const std::string& name, int x, int y,
                         std::shared_ptr<ItemInstance> icon,
                         Achievement* prerequisite)
    : Stat(Achievements::ACHIEVEMENT_OFFSET + id,
           I18n::get(std::string("achievement.").append(name))),
      desc(I18n::get(std::string("achievement.").append(name).append(".desc"))),
      icon(icon),
      x(x),
      y(y),
      prerequisite(prerequisite) {}

/**
 * @brief Marks the achievement as locally awarded only.
 * @return self
 */
Achievement* Achievement::setAwardLocallyOnly() {
    awardLocallyOnly = true;
    return this;
}

/**
 * @brief Marks the achievement as a golden achievement.
 *
 * Golden achievements are rendered differently
 * in the achievement UI.
 *
 * @return self
 */
Achievement* Achievement::setGolden() {
    isGoldenVar = true;
    return this;
}
/**
 * @brief Adds the achievement to the global achievement registry.
 * @return self
 */

Achievement* Achievement::postConstruct() {
    Stat::postConstruct();

    Achievements::achievements->push_back(this);

    return this;
}

/**
 * @brief Indicates that this stat represents an achievement.
 *
 * @return Always true
 */
bool Achievement::isAchievement() { return true; }

/**
 * @brief Gets the description of an Achivement according to it's DescFormatter'
 * @return string
 **/
std::string Achievement::getDescription() {
    if (descFormatter != nullptr) {
        return descFormatter->format(desc);
    }
    return desc;
}

Achievement* Achievement::setDescFormatter(DescFormatter* descFormatter) {
    this->descFormatter = descFormatter;
    return this;
}

bool Achievement::isGolden() { return isGoldenVar; }

int Achievement::getAchievementID() {
    return id - Achievements::ACHIEVEMENT_OFFSET;
}
