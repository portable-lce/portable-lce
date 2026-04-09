#pragma once

#include <string>

class Team {
public:
    virtual bool isAlliedTo(Team* other);

    virtual std::string getName() = 0;
    virtual std::string getFormattedName(const std::string& teamMemberName) = 0;
    virtual bool canSeeFriendlyInvisibles() = 0;
    virtual bool isAllowFriendlyFire() = 0;
};