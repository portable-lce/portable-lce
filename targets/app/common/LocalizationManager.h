#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/common/App_structs.h"
#include "minecraft/GameEnums.h"
#include "platform/XboxStubs.h"

class ArchiveFile;
class Random;
class StringTable;

class LocalizationManager {
public:
    LocalizationManager();

    void localeAndLanguageInit();
    void loadStringTable(ArchiveFile* mediaArchive);
    const char* getString(int iID) const;

    std::string formatHTMLString(int iPad, const std::string& desc,
                                 int shadowColour = 0xFFFFFFFF);
    std::string getActionReplacement(int iPad, unsigned char ucAction);
    std::string getVKReplacement(unsigned int uiVKey);
    std::string getIconReplacement(unsigned int uiIcon);

    int getHTMLColour(eMinecraftColour colour);
    int getHTMLColor(eMinecraftColour colour) { return getHTMLColour(colour); }
    int getHTMLFontSize(EHTMLFontSize size);

    void initialiseTips();
    int getNextTip();

    void getLocale(std::vector<std::string>& vecWstrLocales);
    int get_eMCLang(char* pwchLocale);
    int get_xcLang(char* pwchLocale);

    StringTable* getStringTable() const { return m_stringTable; }

private:
    static int s_iHTMLFontSizesA[eHTMLSize_COUNT];

    StringTable* m_stringTable;

    std::unordered_map<int, std::string> m_localeA;
    std::unordered_map<std::string, int> m_eMCLangA;
    std::unordered_map<std::string, int> m_xcLangA;

    static const int MAX_TIPS_GAMETIP = 50;
    static const int MAX_TIPS_TRIVIATIP = 20;
    static TIPSTRUCT m_GameTipA[MAX_TIPS_GAMETIP];
    static TIPSTRUCT m_TriviaTipA[MAX_TIPS_TRIVIATIP];
    static Random* TipRandom;

    int m_TipIDA[MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP];
    unsigned int m_uiCurrentTip;
    static int TipsSortFunction(const void* a, const void* b);
};
