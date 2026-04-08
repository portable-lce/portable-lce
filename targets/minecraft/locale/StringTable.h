#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#define LOCALE_COUNT 11

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
    StringTable(std::uint8_t* pbData, unsigned int dataSize);
    ~StringTable(void);
    void ReloadStringTable();

    void getData(std::uint8_t** ppData, unsigned int* pSize);

    const char* getString(const std::string& id);
    const char* getString(int id);

    // static const char* m_wchLocaleCode[LOCALE_COUNT];

private:
    // std::string getLangId(uint32_t dwLanguage=0);
    void ProcessStringTableData(void);
};
