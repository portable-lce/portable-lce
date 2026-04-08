#include "DLCPack.h"

#include <wchar.h>

#include <algorithm>
#include <sstream>
#include <utility>

#include "platform/profile/profile.h"
#include "DLCAudioFile.h"
#include "DLCCapeFile.h"
#include "DLCColourTableFile.h"
#include "DLCGameRulesFile.h"
#include "DLCGameRulesHeader.h"
#include "DLCLocalisationFile.h"
#include "DLCTextureFile.h"
#include "DLCUIDataFile.h"
#include "minecraft/Console_Debug_enum.h"
#include "app/common/DLC/DLCFile.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCSkinFile.h"
#include "minecraft/locale/StringTable.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "util/StringHelpers.h"

DLCPack::DLCPack(const std::string& name, std::uint32_t dwLicenseMask) {
    m_dataPath = "";
    m_packName = name;
    m_dwLicenseMask = dwLicenseMask;
    m_ullFullOfferId = 0LL;
    m_isCorrupt = false;
    m_packId = 0;
    m_packVersion = 0;
    m_parentPack = nullptr;
    m_dlcMountIndex = -1;

    // This pointer is for all the data used for this pack, so deleting it
    // invalidates ALL of it's children.
    m_data = nullptr;
}

DLCPack::~DLCPack() {
    for (auto it = m_childPacks.begin(); it != m_childPacks.end(); ++it) {
        delete *it;
    }

    for (unsigned int i = 0; i < DLCManager::e_DLCType_Max; ++i) {
        for (auto it = m_files[i].begin(); it != m_files[i].end(); ++it) {
            delete *it;
        }
    }

    // This pointer is for all the data used for this pack, so deleting it
    // invalidates ALL of it's children.
    if (m_data) {
#if !defined(_CONTENT_PACKAGE)
        printf("Deleting data for DLC pack %s\n", m_packName.c_str());
#endif
        // For the same reason, don't delete data pointer for any child pack as
        // it just points to a region within the parent pack that has already
        // been freed
        if (m_parentPack == nullptr) {
            delete[] m_data;
        }
    }
}

int DLCPack::GetDLCMountIndex() {
    if (m_parentPack != nullptr) {
        return m_parentPack->GetDLCMountIndex();
    }
    return m_dlcMountIndex;
}

XCONTENTDEVICEID DLCPack::GetDLCDeviceID() {
    if (m_parentPack != nullptr) {
        return m_parentPack->GetDLCDeviceID();
    }
    return m_dlcDeviceID;
}

void DLCPack::addChildPack(DLCPack* childPack) {
    const std::uint32_t packId = childPack->GetPackId();
#if !defined(_CONTENT_PACKAGE)
    if (packId < 0 || packId > 15) {
        assert(0);
    }
#endif
    childPack->SetPackId((packId << 24) | m_packId);
    m_childPacks.push_back(childPack);
    childPack->setParentPack(this);
    childPack->m_packName = m_packName + childPack->getName();
}

void DLCPack::setParentPack(DLCPack* parentPack) { m_parentPack = parentPack; }

void DLCPack::addParameter(DLCManager::EDLCParameterType type,
                           const std::string& value) {
    switch (type) {
        case DLCManager::e_DLCParamType_PackId: {
            std::uint32_t packId = 0;

            std::stringstream ss;
            // 4J Stu - numbered using decimal to make it easier for
            // artists/people to number manually
            ss << std::dec << value.c_str();
            ss >> packId;

            SetPackId(packId);
        } break;
        case DLCManager::e_DLCParamType_PackVersion: {
            std::uint32_t version = 0;

            std::stringstream ss;
            // 4J Stu - numbered using decimal to make it easier for
            // artists/people to number manually
            ss << std::dec << value.c_str();
            ss >> version;

            SetPackVersion(version);
        } break;
        case DLCManager::e_DLCParamType_DisplayName:
            m_packName = value;
            break;
        case DLCManager::e_DLCParamType_DataPath:
            m_dataPath = value;
            break;
        default:
            m_parameters[(int)type] = value;
            break;
    }
}

bool DLCPack::getParameterAsUInt(DLCManager::EDLCParameterType type,
                                 unsigned int& param) {
    auto it = m_parameters.find((int)type);
    if (it != m_parameters.end()) {
        switch (type) {
            case DLCManager::e_DLCParamType_NetherParticleColour:
            case DLCManager::e_DLCParamType_EnchantmentTextColour:
            case DLCManager::e_DLCParamType_EnchantmentTextFocusColour: {
                std::stringstream ss;
                ss << std::hex << it->second.c_str();
                ss >> param;
            } break;
            default:
                param = fromWString<unsigned int>(it->second);
        }
        return true;
    }
    return false;
}

DLCFile* DLCPack::addFile(DLCManager::EDLCType type, const std::string& path) {
    DLCFile* newFile = nullptr;

    switch (type) {
        case DLCManager::e_DLCType_Skin: {
            std::vector<std::string> splitPath = stringSplit(path, '/');
            std::string strippedPath = splitPath.back();

            newFile = new DLCSkinFile(strippedPath);

            // check to see if we can get the full offer id using this skin name
            uint64_t ullVal = 0LL;

            if (app.GetDLCFullOfferIDForSkinID(strippedPath, &ullVal)) {
                m_ullFullOfferId = ullVal;
            }
        } break;
        case DLCManager::e_DLCType_Cape: {
            std::vector<std::string> splitPath = stringSplit(path, '/');
            std::string strippedPath = splitPath.back();
            newFile = new DLCCapeFile(strippedPath);
        } break;
        case DLCManager::e_DLCType_Texture:
            newFile = new DLCTextureFile(path);
            break;
        case DLCManager::e_DLCType_UIData:
            newFile = new DLCUIDataFile(path);
            break;
        case DLCManager::e_DLCType_LocalisationData:
            newFile = new DLCLocalisationFile(path);
            break;
        case DLCManager::e_DLCType_GameRules:
            newFile = new DLCGameRulesFile(path);
            break;
        case DLCManager::e_DLCType_Audio:
            newFile = new DLCAudioFile(path);
            break;
        case DLCManager::e_DLCType_ColourTable:
            newFile = new DLCColourTableFile(path);
            break;
        case DLCManager::e_DLCType_GameRulesHeader:
            newFile = new DLCGameRulesHeader(path);
            break;
        default:
            break;
    };

    if (newFile != nullptr) {
        m_files[newFile->getType()].push_back(newFile);
    }

    return newFile;
}

// MGH - added this comp func, as the embedded func in find_if was confusing the
// PS3 compiler
static const std::string* g_pathCmpString = nullptr;
static bool pathCmp(DLCFile* val) {
    return (g_pathCmpString->compare(val->getPath()) == 0);
}

bool DLCPack::doesPackContainFile(DLCManager::EDLCType type,
                                  const std::string& path) {
    bool hasFile = false;
    if (type == DLCManager::e_DLCType_All) {
        for (DLCManager::EDLCType currentType = (DLCManager::EDLCType)0;
             currentType < DLCManager::e_DLCType_Max;
             currentType = (DLCManager::EDLCType)(currentType + 1)) {
            hasFile = doesPackContainFile(currentType, path);
            if (hasFile) break;
        }
    } else {
        g_pathCmpString = &path;
        auto it =
            std::find_if(m_files[type].begin(), m_files[type].end(), pathCmp);
        hasFile = it != m_files[type].end();
        if (!hasFile && m_parentPack) {
            hasFile = m_parentPack->doesPackContainFile(type, path);
        }
    }
    return hasFile;
}

DLCFile* DLCPack::getFile(DLCManager::EDLCType type, unsigned int index) {
    DLCFile* file = nullptr;
    if (type == DLCManager::e_DLCType_All) {
        for (DLCManager::EDLCType currentType = (DLCManager::EDLCType)0;
             currentType < DLCManager::e_DLCType_Max;
             currentType = (DLCManager::EDLCType)(currentType + 1)) {
            file = getFile(currentType, index);
            if (file != nullptr) break;
        }
    } else {
        if (m_files[type].size() > index) file = m_files[type][index];
        if (!file && m_parentPack) {
            file = m_parentPack->getFile(type, index);
        }
    }
    return file;
}

DLCFile* DLCPack::getFile(DLCManager::EDLCType type, const std::string& path) {
    DLCFile* file = nullptr;
    if (type == DLCManager::e_DLCType_All) {
        for (DLCManager::EDLCType currentType = (DLCManager::EDLCType)0;
             currentType < DLCManager::e_DLCType_Max;
             currentType = (DLCManager::EDLCType)(currentType + 1)) {
            file = getFile(currentType, path);
            if (file != nullptr) break;
        }
    } else {
        g_pathCmpString = &path;
        auto it =
            std::find_if(m_files[type].begin(), m_files[type].end(), pathCmp);

        if (it == m_files[type].end()) {
            // Not found
            file = nullptr;
        } else {
            file = *it;
        }
        if (!file && m_parentPack) {
            file = m_parentPack->getFile(type, path);
        }
    }
    return file;
}

unsigned int DLCPack::getDLCItemsCount(
    DLCManager::EDLCType type /*= DLCManager::e_DLCType_All*/) {
    unsigned int count = 0;

    switch (type) {
        case DLCManager::e_DLCType_All:
            for (int i = 0; i < DLCManager::e_DLCType_Max; ++i) {
                count += getDLCItemsCount((DLCManager::EDLCType)i);
            }
            break;
        default:
            count = static_cast<unsigned int>(m_files[(int)type].size());
            break;
    };
    return count;
};

unsigned int DLCPack::getFileIndexAt(DLCManager::EDLCType type,
                                     const std::string& path, bool& found) {
    if (type == DLCManager::e_DLCType_All) {
        app.DebugPrintf("Unimplemented\n");
#if !defined(__CONTENT_PACKAGE)
        assert(0);
#endif
        return 0;
    }

    unsigned int foundIndex = 0;
    found = false;
    unsigned int index = 0;
    for (auto it = m_files[type].begin(); it != m_files[type].end(); ++it) {
        if (path.compare((*it)->getPath()) == 0) {
            foundIndex = index;
            found = true;
            break;
        }
        ++index;
    }

    return foundIndex;
}

bool DLCPack::hasPurchasedFile(DLCManager::EDLCType type,
                               const std::string& path) {
    if (type == DLCManager::e_DLCType_All) {
        app.DebugPrintf("Unimplemented\n");
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
        return false;
    }
#if !defined(_CONTENT_PACKAGE)
    if (app.GetGameSettingsDebugMask(PlatformProfile.GetPrimaryPad()) &
        (1L << eDebugSetting_UnlockAllDLC)) {
        return true;
    } else
#endif
        if (m_dwLicenseMask == 0) {
        // not purchased.
        return false;
    } else {
        // purchased
        return true;
    }
}

void DLCPack::UpdateLanguage() {
    // find the language file
    if (m_files[DLCManager::e_DLCType_LocalisationData].size() > 0) {
        DLCLocalisationFile* localisationFile = (DLCLocalisationFile*)getFile(
            DLCManager::e_DLCType_LocalisationData, "languages.loc");
        StringTable* strTable = localisationFile->getStringTable();
        strTable->ReloadStringTable();
    }
}
