#pragma once

#include <format>
#include <string>
#include <vector>

#include "minecraft/GameEnums.h"

// 4J: Simple std::string wrapper that includes basic formatting information
class HtmlString {
public:
    std::string text;        // Text content of std::string
    eMinecraftColour color;  // Hex color
    bool italics;            // Show text in italics
    bool indent;             // Indent text

    HtmlString(std::string text,
               eMinecraftColour color = eMinecraftColour_NOT_SET,
               bool italics = false, bool indent = false);
    std::string ToString();

    static std::string Compose(std::vector<HtmlString>* strings);
};