#pragma once

#include <cstdint>
#include <string>

#include "app/common/Iggy/include/rrCore.h"

class UITTFFont {
private:
    std::uint8_t* pbData;
    const std::string m_strFontName;

    // uint32_t dwDataSize;

public:
    UITTFFont(const std::string& name, const std::string& path,
              S32 fallbackCharacter);
    ~UITTFFont();

    std::string getFontName();
};
