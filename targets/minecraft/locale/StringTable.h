#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

inline constexpr int kLocaleCount = 11;

class StringTable {
private:
    bool isStatic;

    std::unordered_map<std::string, std::string> m_stringsMap;
    std::vector<std::string> m_stringsVec;

    std::vector<uint8_t> src;

public:
    // 	enum eLocale
    // 	{
    // 		eLocale_Default=0,
    // 		eLocale_American,
    // 		eLocale_Japanese,
    // 		eLocale_German,
    // 		eLocale_French,
    // 		eLocale_Spanish,
    // 		eLocale_Italian,
    // 		eLocale_Korean,
    // 		eLocale_TradChinese,
    // 		eLocale_Portuguese,
    // 		eLocale_Brazilian,
    // #if 0 || 0 || 0
    // 		eLocale_Russian,
    // 		eLocale_Dutch,
    // 		eLocale_Finish,
    // 		eLocale_Swedish,
    // 		eLocale_Danish,
    // 		eLocale_Norwegian,
    // 		eLocale_Polish,
    // 		eLocale_Turkish,
    // 		eLocale_LatinAmericanSpanish,
    // 		eLocale_Greek,
    // #elif 0 || 0
    // 		eLocale_British,
    // 		eLocale_Irish,
    // 		eLocale_Australian,
    // 		eLocale_NewZealand,
    // 		eLocale_Canadian,
    // 		eLocale_Mexican,
    // 		eLocale_FrenchCanadian,
    // 		eLocale_Austrian,
    // #endif
    // 	};

    StringTable(void);
    StringTable(std::span<const std::uint8_t> data,
                std::span<const std::string> locales);
    ~StringTable(void);
    void ReloadStringTable(std::span<const std::string> locales);

    void getData(std::uint8_t** ppData, unsigned int* pSize);

    const char* getString(const std::string& id);
    const char* getString(int id);

private:
    // std::string getLangId(uint32_t dwLanguage=0);
    void ProcessStringTableData(std::span<const std::string> locales);
};
