#include "PlayerTeam.h"

#include "minecraft/world/scores/Scoreboard.h"
#include "minecraft/world/scores/Team.h"

PlayerTeam::PlayerTeam(Scoreboard* scoreboard, const std::string& name) {
    this->scoreboard = scoreboard;
    this->name = name;
    displayName = name;

    prefix = "";
    suffix = "";
    allowFriendlyFire = true;
    seeFriendlyInvisibles = true;
}

Scoreboard* PlayerTeam::getScoreboard() { return scoreboard; }

std::string PlayerTeam::getName() { return name; }

std::string PlayerTeam::getDisplayName() { return displayName; }

void PlayerTeam::setDisplayName(const std::string& displayName) {
    // if (displayName == null) throw new IllegalArgumentException("Name cannot
    // be null");
    this->displayName = displayName;
    scoreboard->onTeamChanged(this);
}

std::unordered_set<std::string>* PlayerTeam::getPlayers() { return &players; }

std::string PlayerTeam::getPrefix() { return prefix; }

void PlayerTeam::setPrefix(const std::string& prefix) {
    // if (prefix == null) throw new IllegalArgumentException("Prefix cannot be
    // null");
    this->prefix = prefix;
    scoreboard->onTeamChanged(this);
}

std::string PlayerTeam::getSuffix() { return suffix; }

void PlayerTeam::setSuffix(const std::string& suffix) {
    // if (suffix == null) throw new IllegalArgumentException("Suffix cannot be
    // null");
    this->suffix = suffix;
    scoreboard->onTeamChanged(this);
}

std::string PlayerTeam::getFormattedName(const std::string& teamMemberName) {
    return getPrefix() + teamMemberName + getSuffix();
}

std::string PlayerTeam::formatNameForTeam(PlayerTeam* team) {
    return formatNameForTeam(team, team->getDisplayName());
}

std::string PlayerTeam::formatNameForTeam(Team* team, const std::string& name) {
    if (team == nullptr) return name;
    return team->getFormattedName(name);
}

bool PlayerTeam::isAllowFriendlyFire() { return allowFriendlyFire; }

void PlayerTeam::setAllowFriendlyFire(bool allowFriendlyFire) {
    this->allowFriendlyFire = allowFriendlyFire;
    scoreboard->onTeamChanged(this);
}

bool PlayerTeam::canSeeFriendlyInvisibles() { return seeFriendlyInvisibles; }

void PlayerTeam::setSeeFriendlyInvisibles(bool seeFriendlyInvisibles) {
    this->seeFriendlyInvisibles = seeFriendlyInvisibles;
    scoreboard->onTeamChanged(this);
}

int PlayerTeam::packOptions() {
    int result = 0;

    if (isAllowFriendlyFire()) result |= 1 << BIT_FRIENDLY_FIRE;
    if (canSeeFriendlyInvisibles()) result |= 1 << BIT_SEE_INVISIBLES;

    return result;
}

void PlayerTeam::unpackOptions(int options) {
    setAllowFriendlyFire((options & (1 << BIT_FRIENDLY_FIRE)) > 0);
    setSeeFriendlyInvisibles((options & (1 << BIT_SEE_INVISIBLES)) > 0);
}