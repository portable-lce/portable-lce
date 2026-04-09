#pragma once

#include <stdint.h>

#include <format>
#include <memory>
#include <string>
#include <vector>

#include "GenericStats.h"
#include "StatFormatter.h"
#include "minecraft/IGameServices.h"

class DecimalFormat;
class LocalPlayer;

class Stat {
public:
    const int id;
    const std::string name;
    bool awardLocallyOnly;

private:
    const StatFormatter* formatter;
    void _init();

public:
    Stat(int id, const std::string& name, StatFormatter* formatter);
    Stat(int id, const std::string& name);
    Stat* setAwardLocallyOnly();

    virtual Stat* postConstruct();
    virtual bool isAchievement();
    std::string format(int value);

private:
    // static NumberFormat *numberFormat;

public:
    class DefaultFormat : public StatFormatter {
    public:
        std::string format(int value);
    } static* defaultFormatter;

private:
    static DecimalFormat* decimalFormat;

public:
    class TimeFormatter : public StatFormatter {
    public:
        std::string format(int value);
    } static* timeFormatter;

    class DistanceFormatter : public StatFormatter {
    public:
        std::string format(int cm);
    } static* distanceFormatter;

    std::string toString();

public:
    // 4J-JEV, for Durango stats
    virtual void handleParamBlob(std::shared_ptr<LocalPlayer> plr,
                                 std::vector<uint8_t>& param) {
        gameServices().debugPrintf("'Stat.h', Unhandled AwardStat blob.\n");
        return;
    }
};
