#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

class ObjectiveCriteria {
public:
    static std::unordered_map<std::string, ObjectiveCriteria*> CRITERIA_BY_NAME;

    static ObjectiveCriteria* DUMMY;
    static ObjectiveCriteria* DEATH_COUNT;
    static ObjectiveCriteria* KILL_COUNT_PLAYERS;
    static ObjectiveCriteria* KILL_COUNT_ALL;
    static ObjectiveCriteria* HEALTH;

    virtual std::string getName() = 0;
    virtual int getScoreModifier(
        std::vector<std::shared_ptr<Player> >* players) = 0;
    virtual bool isReadOnly() = 0;
};