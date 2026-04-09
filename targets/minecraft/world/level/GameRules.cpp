#include "GameRules.h"

#include <assert.h>

#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"

// 4J: GameRules isn't in use anymore, just routes any requests to app game host
// options, kept things commented out for context

const int GameRules::RULE_DOFIRETICK = 0;
const int GameRules::RULE_MOBGRIEFING = 1;
const int GameRules::RULE_KEEPINVENTORY = 2;
const int GameRules::RULE_DOMOBSPAWNING = 3;
const int GameRules::RULE_DOMOBLOOT = 4;
const int GameRules::RULE_DOTILEDROPS = 5;
// const int GameRules::RULE_COMMANDBLOCKOUTPUT = 6;
const int GameRules::RULE_NATURAL_REGENERATION = 7;
const int GameRules::RULE_DAYLIGHT = 8;

GameRules::GameRules() {
    /*registerRule(RULE_DOFIRETICK, "1");
    registerRule(RULE_MOBGRIEFING, "1");
    registerRule(RULE_KEEPINVENTORY, "0");
    registerRule(RULE_DOMOBSPAWNING, "1");
    registerRule(RULE_DOMOBLOOT, "1");
    registerRule(RULE_DOTILEDROPS, "1");
    registerRule(RULE_COMMANDBLOCKOUTPUT, "1");
    registerRule(RULE_NATURAL_REGENERATION, "1");
    registerRule(RULE_DAYLIGHT, "1");*/
}

GameRules::~GameRules() {
    /*for(auto it = rules.begin(); it != rules.end(); ++it)
    {
            delete it->second;
    }*/
}

bool GameRules::getBoolean(const int rule) {
    switch (rule) {
        case GameRules::RULE_DOFIRETICK:
            return gameServices().getGameHostOption(
                eGameHostOption_FireSpreads);
        case GameRules::RULE_MOBGRIEFING:
            return gameServices().getGameHostOption(
                eGameHostOption_MobGriefing);
        case GameRules::RULE_KEEPINVENTORY:
            return gameServices().getGameHostOption(
                eGameHostOption_KeepInventory);
        case GameRules::RULE_DOMOBSPAWNING:
            return gameServices().getGameHostOption(
                eGameHostOption_DoMobSpawning);
        case GameRules::RULE_DOMOBLOOT:
            return gameServices().getGameHostOption(eGameHostOption_DoMobLoot);
        case GameRules::RULE_DOTILEDROPS:
            return gameServices().getGameHostOption(
                eGameHostOption_DoTileDrops);
        case GameRules::RULE_NATURAL_REGENERATION:
            return gameServices().getGameHostOption(
                eGameHostOption_NaturalRegeneration);
        case GameRules::RULE_DAYLIGHT:
            return gameServices().getGameHostOption(
                eGameHostOption_DoDaylightCycle);
        default:
            assert(0);
            return false;
    }
}

/*
void GameRules::registerRule(const std::string &name, const std::string
&startValue)
{
        rules[name] = new GameRule(startValue);
}

void GameRules::set(const std::string &ruleName, const std::string &newValue)
{
        auto it = rules.find(ruleName);
        if(it != rules.end() )
        {
                GameRule *gameRule = it->second;
                gameRule->set(newValue);
        }
        else
        {
                registerRule(ruleName, newValue);
        }
}

std::string GameRules::get(const std::string &ruleName)
{
        auto it = rules.find(ruleName);
        if(it != rules.end() )
        {
                GameRule *gameRule = it->second;
                return gameRule->get();
        }
        return "";
}

int GameRules::getInt(const std::string &ruleName)
{
        auto it = rules.find(ruleName);
        if(it != rules.end() )
        {
                GameRule *gameRule = it->second;
                return gameRule->getInt();
        }
        return 0;
}

double GameRules::getDouble(const std::string &ruleName)
{
        auto it = rules.find(ruleName);
        if(it != rules.end() )
        {
                GameRule *gameRule = it->second;
                return gameRule->getDouble();
        }
        return 0;
}

CompoundTag *GameRules::createTag()
{
        CompoundTag *result = new CompoundTag("GameRules");

        for(auto it = rules.begin(); it != rules.end(); ++it)
        {
                GameRule *gameRule = it->second;
                result->putString(it->first, gameRule->get());
        }

        return result;
}

void GameRules::loadFromTag(CompoundTag *tag)
{
        vector<Tag *> allTags = tag->getAllTags();
        for (auto it = allTags.begin(); it != allTags.end(); ++it)
        {
                Tag *ruleTag = *it;
                std::string ruleName = ruleTag->getName();
                std::string value = tag->getString(ruleTag->getName());

                set(ruleName, value);
        }
}

// Need to delete returned vector.
vector<std::string> *GameRules::getRuleNames()
{
        vector<std::string> *out = new vector<std::string>();
        for (auto it = rules.begin(); it != rules.end(); it++)
out->push_back(it->first); return out;
}

bool GameRules::contains(const std::string &rule)
{
        auto it = rules.find(rule);
        return it != rules.end();
}

GameRules::GameRule::GameRule(const std::string &startValue)
{
        value = "";
        booleanValue = false;
        intValue = 0;
        doubleValue = 0.0;
        set(startValue);
}

void GameRules::GameRule::set(const std::string &newValue)
{
        value = newValue;
        booleanValue = fromWString<bool>(newValue);
        intValue = fromWString<int>(newValue);
        doubleValue = fromWString<double>(newValue);
}

std::string GameRules::GameRule::get()
{
        return value;
}

bool GameRules::GameRule::getBoolean()
{
        return booleanValue;
}

int GameRules::GameRule::getInt()
{
        return intValue;
}

double GameRules::GameRule::getDouble()
{
        return doubleValue;
}*/