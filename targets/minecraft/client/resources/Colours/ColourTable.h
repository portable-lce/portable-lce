#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

#include "minecraft/GameEnums.h"

class ColourTable {
private:
    unsigned int m_colourValues[eMinecraftColour_COUNT];

    static const char* ColourTableElements[eMinecraftColour_COUNT];
    static std::unordered_map<std::string, eMinecraftColour> s_colourNamesMap;

public:
    static void staticCtor();

    ColourTable(std::uint8_t* pbData, std::uint32_t dataLength);
    ColourTable(ColourTable* defaultColours, std::uint8_t* pbData,
                std::uint32_t dataLength);

    unsigned int getColour(eMinecraftColour id);
    unsigned int getColor(eMinecraftColour id) { return getColour(id); }

    void loadColoursFromData(std::uint8_t* pbData, std::uint32_t dataLength);
    void setColour(const std::string& colourName, int value);
    void setColour(const std::string& colourName, const std::string& value);
};
