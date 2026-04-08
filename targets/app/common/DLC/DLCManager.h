#pragma once
// using namespace std;
#include <cstdint>
#include <string>
#include <vector>

#include "minecraft/world/level/dlc/DLCConstants.h"

class DLCPack;
class DLCSkinFile;

class DLCManager {
public:
    // Re-export the file-scope enums as nested type aliases so existing
    // app-side call sites that use DLCManager::EDLCType /
    // DLCManager::e_DLCType_Texture continue to compile. minecraft/-side
    // call sites should use the namespaced names from DLCConstants.h
    // directly.
    using EDLCType = ::minecraft::dlc::EDLCType;
    using EDLCParameterType = ::minecraft::dlc::EDLCParameterType;

    static constexpr EDLCType e_DLCType_Skin = ::minecraft::dlc::e_DLCType_Skin;
    static constexpr EDLCType e_DLCType_Cape = ::minecraft::dlc::e_DLCType_Cape;
    static constexpr EDLCType e_DLCType_Texture = ::minecraft::dlc::e_DLCType_Texture;
    static constexpr EDLCType e_DLCType_UIData = ::minecraft::dlc::e_DLCType_UIData;
    static constexpr EDLCType e_DLCType_PackConfig = ::minecraft::dlc::e_DLCType_PackConfig;
    static constexpr EDLCType e_DLCType_TexturePack = ::minecraft::dlc::e_DLCType_TexturePack;
    static constexpr EDLCType e_DLCType_LocalisationData = ::minecraft::dlc::e_DLCType_LocalisationData;
    static constexpr EDLCType e_DLCType_GameRules = ::minecraft::dlc::e_DLCType_GameRules;
    static constexpr EDLCType e_DLCType_Audio = ::minecraft::dlc::e_DLCType_Audio;
    static constexpr EDLCType e_DLCType_ColourTable = ::minecraft::dlc::e_DLCType_ColourTable;
    static constexpr EDLCType e_DLCType_GameRulesHeader = ::minecraft::dlc::e_DLCType_GameRulesHeader;
    static constexpr EDLCType e_DLCType_Max = ::minecraft::dlc::e_DLCType_Max;
    static constexpr EDLCType e_DLCType_All = ::minecraft::dlc::e_DLCType_All;

    static constexpr EDLCParameterType e_DLCParamType_Invalid = ::minecraft::dlc::e_DLCParamType_Invalid;
    static constexpr EDLCParameterType e_DLCParamType_DisplayName = ::minecraft::dlc::e_DLCParamType_DisplayName;
    static constexpr EDLCParameterType e_DLCParamType_ThemeName = ::minecraft::dlc::e_DLCParamType_ThemeName;
    static constexpr EDLCParameterType e_DLCParamType_Free = ::minecraft::dlc::e_DLCParamType_Free;
    static constexpr EDLCParameterType e_DLCParamType_Credit = ::minecraft::dlc::e_DLCParamType_Credit;
    static constexpr EDLCParameterType e_DLCParamType_Cape = ::minecraft::dlc::e_DLCParamType_Cape;
    static constexpr EDLCParameterType e_DLCParamType_Box = ::minecraft::dlc::e_DLCParamType_Box;
    static constexpr EDLCParameterType e_DLCParamType_Anim = ::minecraft::dlc::e_DLCParamType_Anim;
    static constexpr EDLCParameterType e_DLCParamType_PackId = ::minecraft::dlc::e_DLCParamType_PackId;
    static constexpr EDLCParameterType e_DLCParamType_NetherParticleColour = ::minecraft::dlc::e_DLCParamType_NetherParticleColour;
    static constexpr EDLCParameterType e_DLCParamType_EnchantmentTextColour = ::minecraft::dlc::e_DLCParamType_EnchantmentTextColour;
    static constexpr EDLCParameterType e_DLCParamType_EnchantmentTextFocusColour = ::minecraft::dlc::e_DLCParamType_EnchantmentTextFocusColour;
    static constexpr EDLCParameterType e_DLCParamType_DataPath = ::minecraft::dlc::e_DLCParamType_DataPath;
    static constexpr EDLCParameterType e_DLCParamType_PackVersion = ::minecraft::dlc::e_DLCParamType_PackVersion;
    static constexpr EDLCParameterType e_DLCParamType_Max = ::minecraft::dlc::e_DLCParamType_Max;

    const static char* wchTypeNamesA[e_DLCParamType_Max];

private:
    std::vector<DLCPack*> m_packs;
    // bool m_bNeedsUpdated;
    bool m_bNeedsCorruptCheck;
    unsigned int m_dwUnnamedCorruptDLCCount;

public:
    DLCManager();
    ~DLCManager();

    static EDLCParameterType getParameterType(const std::string& paramName);

    unsigned int getPackCount(EDLCType type = e_DLCType_All);

    // bool NeedsUpdated() { return m_bNeedsUpdated; }
    // void SetNeedsUpdated(bool val) { m_bNeedsUpdated = val; }

    bool NeedsCorruptCheck() { return m_bNeedsCorruptCheck; }
    void SetNeedsCorruptCheck(bool val) { m_bNeedsCorruptCheck = val; }

    void resetUnnamedCorruptCount() { m_dwUnnamedCorruptDLCCount = 0; }
    void incrementUnnamedCorruptCount() { ++m_dwUnnamedCorruptDLCCount; }

    void addPack(DLCPack* pack);
    void removePack(DLCPack* pack);
    void removeAllPacks(void);
    void LanguageChanged(void);

    DLCPack* getPack(const std::string& name);
    DLCPack* getPack(unsigned int index, EDLCType type = e_DLCType_All);
    unsigned int getPackIndex(DLCPack* pack, bool& found,
                              EDLCType type = e_DLCType_All);
    DLCSkinFile* getSkinFile(
        const std::string& path);  // Will hunt all packs of type skin to find
                                    // the right skinfile

    DLCPack* getPackContainingSkin(const std::string& path);
    unsigned int getPackIndexContainingSkin(const std::string& path,
                                            bool& found);

    unsigned int checkForCorruptDLCAndAlert(bool showMessage = true);

    // bool readDLCDataFile(unsigned int& dwFilesProcessed,
    //                      const std::wstring& path, DLCPack* pack,
    //                      bool fromArchive = false);
    bool readDLCDataFile(unsigned int& dwFilesProcessed,
                         const std::string& path, DLCPack* pack,
                         bool fromArchive = false);
    std::uint32_t retrievePackIDFromDLCDataFile(const std::string& path,
                                                DLCPack* pack);

private:
    bool processDLCDataFile(unsigned int& dwFilesProcessed,
                            std::uint8_t* pbData, unsigned int dwLength,
                            DLCPack* pack);

    std::uint32_t retrievePackID(std::uint8_t* pbData, unsigned int dwLength,
                                 DLCPack* pack);
};

std::string dlc_read_wstring(const void* data);