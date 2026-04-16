#include "StdStorage.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "platform/fs/fs.h"

namespace platform_internal {
IPlatformStorage& PlatformStorage_get() {
    static StdStorage instance;
    return instance;
}
}  // namespace platform_internal

static XMARKETPLACE_CONTENTOFFER_INFO s_dummyOffer = {};
static XCONTENT_DATA s_dummyContentData = {};

StdStorage::StdStorage() {
    std::filesystem::create_directories(gameHDDDir);
    std::strncpy(this->m_szSaveUniqueName, basePath.string().c_str(),
                 sizeof(this->m_szSaveUniqueName) - 1);
}

void StdStorage::Init(unsigned int uiSaveVersion,
                      const char* pwchDefaultSaveName, char* pszSavePackName,
                      int iMinimumSaveSize,
                      std::function<int(const ESavingMessage, int)> callback,
                      const char* szGroupID) {}

// MARK: Savefile Load/Store

void StdStorage::ResetSaveData() {
    free(m_pSaveData);
    m_pSaveData = nullptr;
    m_uiSaveSize = 0;
}

void StdStorage::SetSaveTitle(const char* pwchDefaultSaveName) {
    const auto uniqueName =
        std::format("{:%Y%m%d%H%M%S}", std::chrono::system_clock::now());

    strncpy(m_szSaveUniqueName, uniqueName.c_str(),
            sizeof(m_szSaveUniqueName) - 1);
    m_szSaveUniqueName[sizeof(m_szSaveUniqueName) - 1] = '\0';

    sprintf(m_szSaveTitle, "%s", pwchDefaultSaveName);
}

bool StdStorage::GetSaveUniqueNumber(int* piVal) {
    if (m_szSaveUniqueName[0] == '\0') {
        return 0;
    }

    int year, month, day, hour, minute;
    sscanf(&m_szSaveUniqueName[4], "%02d%02d%02d%02d%02d", &year, &month, &day,
           &hour, &minute);

    *piVal = 2678400 * year + 86400 * month + 3600 * day + 60 * hour + minute;

    return true;
}

bool StdStorage::GetSaveUniqueFilename(char* pszName) {
    if (m_szSaveUniqueName[0] == '\0') {
        return false;
    }

    memset(pszName, 0, 14);
    for (int i = 0; i < 12; i++) {
        pszName[i] = m_szSaveUniqueName[i + 2];
    }

    return true;
}

void StdStorage::SetSaveUniqueFilename(char* szFilename) {
    strcpy(m_szSaveUniqueName, szFilename);
}

bool StdStorage::GetSaveDisabled(void) { return m_bIsSafeDisabled; }
void StdStorage::SetSaveDisabled(bool bDisable) {
    m_bIsSafeDisabled = bDisable;
}

unsigned int StdStorage::GetSaveSize() { return m_uiSaveSize; }

void StdStorage::GetSaveData(void* pvData, unsigned int* puiBytes) {
    if (pvData) {
        memmove(pvData, m_pSaveData, m_uiSaveSize);
        *puiBytes = m_uiSaveSize;
    } else {
        *puiBytes = 0;
    }
}

void* StdStorage::AllocateSaveData(unsigned int uiBytes) {
    free(m_pSaveData);

    m_pSaveData = malloc(uiBytes);
    if (m_pSaveData) {
        m_uiSaveSize = uiBytes;
    }

    return m_pSaveData;
}

void StdStorage::SetSaveImages(std::uint8_t* pbThumbnail,
                               unsigned int thumbnailBytes,
                               std::uint8_t* pbImage, unsigned int imageBytes,
                               std::uint8_t* pbTextData,
                               unsigned int textDataBytes) {
// PLCE: TODO, bring this over from patoke's implementation once we have
// thumbnails
#if 0
    if (this->m_pbThumbnailData) free(this->m_pbThumbnailData);

    this->m_pbImageData = pbImage;
    this->m_uiImageSize = imageBytes;

    unsigned int dwNewThumbnailBytes = thumbnailBytes;
    if (textDataBytes > 0) {
        // 4 (size) + 4 (type) + 4 (CRC) for tEXt chunk
        dwNewThumbnailBytes += textDataBytes + 12;
    }

    this->m_pbThumbnailData = static_cast<uint8_t*>(malloc(dwNewThumbnailBytes));
    this->m_uiThumbnailSize = dwNewThumbnailBytes;
    memset(this->m_pbThumbnailData, 0, dwNewThumbnailBytes);
    memcpy(this->m_pbThumbnailData, pbThumbnail, thumbnailBytes);

    if (textDataBytes > 0)
        this->AddTextFieldToPNG(this->m_pbThumbnailData, thumbnailBytes,
                                pbTextData, textDataBytes, dwNewThumbnailBytes);
#endif
}

StdStorage::ESaveGameState StdStorage::SaveSaveData(
    std::function<int(const bool)> callback) {
    std::filesystem::path saveDir = this->gameHDDDir / m_szSaveUniqueName;
    std::filesystem::create_directories(saveDir);

    // save file
    std::filesystem::path saveFile =
        saveDir / (std::string(this->m_szSaveTitle) + ".ms");
    std::ofstream file(saveFile, std::ios::binary | std::ios::trunc);
    file.write(static_cast<const char*>(m_pSaveData), m_uiSaveSize);
    assert(file && "StdStorage: failed to write save file");

    // thumbnail
    if (this->m_pbThumbnailData != nullptr && this->m_uiThumbnailSize > 0) {
        std::filesystem::path thumbDir = saveDir / "thumbnails";
        std::filesystem::create_directories(thumbDir);

        std::filesystem::path thumbFile = thumbDir / "thumbData.png";
        std::ofstream thumb(thumbFile, std::ios::binary | std::ios::trunc);
        thumb.write(reinterpret_cast<const char*>(this->m_pbThumbnailData),
                    this->m_uiThumbnailSize);
        assert(thumb && "StdStorage: failed to write thumbnail file");
    }

    callback(true);
    return StdStorage::ESaveGame_Idle;
}

StdStorage::ESaveGameState StdStorage::LoadSaveData(
    PSAVE_INFO pSaveInfo, std::function<int(const bool, const bool)> callback) {
    SetSaveUniqueFilename(pSaveInfo->UTF8SaveFilename);
    memcpy(this->m_szSaveTitle, pSaveInfo->UTF8SaveTitle,
           sizeof(this->m_szSaveTitle));

    if (m_pSaveData) free(m_pSaveData);

    m_pSaveData = malloc(pSaveInfo->metaData.dataSize);
    m_uiSaveSize = pSaveInfo->metaData.dataSize;

    std::filesystem::path saveDir = this->gameHDDDir / m_szSaveUniqueName;
    std::filesystem::create_directories(saveDir);

    // find first regular file in the save directory and assume that's the save
    // file.
    std::filesystem::path saveFile;
    for (const auto& entry : std::filesystem::directory_iterator(saveDir)) {
        if (entry.is_regular_file()) {
            saveFile = entry.path();
            break;  // @Patoke todo: add fail case?
        }
    }

    bool success = false;
    if (!saveFile.empty()) {
        std::ifstream file(saveFile, std::ios::binary);
        if (file.is_open()) {
            file.read(static_cast<char*>(m_pSaveData), m_uiSaveSize);
            assert(file &&
                   static_cast<unsigned int>(file.gcount()) == m_uiSaveSize);
            success = true;
        }
    }

    if (callback) callback(false, success);

    return StdStorage::ESaveGame_Idle;
}

StdStorage::ESaveGameState StdStorage::GetSavesInfo(
    int iPad,
    std::function<int(SAVE_DETAILS* pSaveDetails, const bool)> callback,
    char* pszSavePackName) {
    if (!m_pSaveDetails) {
        m_pSaveDetails = new SAVE_DETAILS();
        memset(m_pSaveDetails, 0, sizeof(SAVE_DETAILS));
    }

    delete[] m_pSaveDetails->SaveInfoA;
    m_pSaveDetails->SaveInfoA = nullptr;
    m_pSaveDetails->iSaveC = 0;

    if (!std::filesystem::exists(this->gameHDDDir)) {
        printf("StdStorage: saves directory does not exist\n");
        m_bHasSaveDetails = true;
        if (callback) callback(m_pSaveDetails, true);
        return StdStorage::ESaveGame_Idle;
    }

    int save_dir_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(gameHDDDir)) {
        if (entry.is_directory()) ++save_dir_count;
    }

    if (save_dir_count > 0) {
        m_pSaveDetails->SaveInfoA = new SAVE_INFO[save_dir_count];
        memset(m_pSaveDetails->SaveInfoA, 0,
               sizeof(SAVE_INFO) * save_dir_count);

        int i = 0;
        for (const auto& saveDir :
             std::filesystem::directory_iterator(gameHDDDir)) {
            if (!saveDir.is_directory()) continue;

            const std::string dirFilename = saveDir.path().filename().string();

            strncpy(m_pSaveDetails->SaveInfoA[i].UTF8SaveFilename,
                    dirFilename.c_str(),
                    sizeof(m_pSaveDetails->SaveInfoA[i].UTF8SaveFilename) - 1);

            // @Patoke add: find the first regular file to use as the save title
            // @Patoke todo: ideally check the extension too, but this is good
            // enough for now
            char szTitleName[256] = {0};
            std::filesystem::path saveFilePath;

            for (const auto& fileEntry :
                 std::filesystem::directory_iterator(saveDir.path())) {
                if (fileEntry.is_regular_file()) {
                    saveFilePath = fileEntry.path();

                    // strip extension
                    std::string stem = saveFilePath.stem().string();
                    strncpy(this->m_szSaveTitle, stem.c_str(),
                            sizeof(this->m_szSaveTitle) - 1);
                    strncpy(szTitleName, stem.c_str(), sizeof(szTitleName) - 1);
                    break;
                }
            }

            // display title from the save name
            // TODO(PLCE): this doesn't seem to match MinecraftConsoles, is
            // there an alternate metadata source for this?
            strncpy(m_pSaveDetails->SaveInfoA[i].UTF8SaveTitle, szTitleName,
                    sizeof(m_pSaveDetails->SaveInfoA[i].UTF8SaveTitle) - 1);

            // file size
            if (!saveFilePath.empty() &&
                std::filesystem::exists(saveFilePath)) {
                m_pSaveDetails->SaveInfoA[i].metaData.dataSize =
                    static_cast<uint32_t>(
                        std::filesystem::file_size(saveFilePath));
            }

            // @Patoke todo: a save can have multiple thumbnails, implement this
            // behaviour
            std::filesystem::path thumbPath =
                saveDir.path() / "thumbnails" / "thumbData.png";
            if (!std::filesystem::exists(thumbPath)) {
                if (m_pSaveDetails->SaveInfoA[i].thumbnailData) {
                    free(m_pSaveDetails->SaveInfoA[i].thumbnailData);
                    m_pSaveDetails->SaveInfoA[i].thumbnailData = nullptr;
                }
                m_pSaveDetails->SaveInfoA[i].metaData.thumbnailSize = 0;
            } else {
                m_pSaveDetails->SaveInfoA[i].metaData.thumbnailSize =
                    static_cast<uint32_t>(
                        std::filesystem::file_size(thumbPath));
            }

            ++m_pSaveDetails->iSaveC;
            ++i;
        }
    }

    m_bHasSaveDetails = true;
    if (callback) callback(m_pSaveDetails, true);

    return StdStorage::ESaveGame_Idle;
}

PSAVE_DETAILS StdStorage::ReturnSavesInfo() {
    if (m_bHasSaveDetails)
        return m_pSaveDetails;
    else
        return nullptr;
}

void StdStorage::ClearSavesInfo() {
    m_bHasSaveDetails = false;
    if (m_pSaveDetails) {
        if (m_pSaveDetails->SaveInfoA) {
            delete[] m_pSaveDetails->SaveInfoA;
            m_pSaveDetails->SaveInfoA = nullptr;
            m_pSaveDetails->iSaveC = 0;
        }
        delete m_pSaveDetails;
        m_pSaveDetails = 0;
    }
}

StdStorage::ESaveGameState StdStorage::LoadSaveDataThumbnail(
    PSAVE_INFO pSaveInfo,
    std::function<int(std::uint8_t* thumbnailData, unsigned int thumbnailBytes)>
        callback) {
    if (pSaveInfo == nullptr) return StdStorage::ESaveGame_Idle;

    unsigned int thumbSize = pSaveInfo->metaData.thumbnailSize;
    if (thumbSize > 0 && pSaveInfo->thumbnailData == nullptr) {
        std::filesystem::path thumbPath = this->gameHDDDir /
                                          pSaveInfo->UTF8SaveFilename /
                                          "thumbnails" / "thumbData.png";

        std::ifstream file(thumbPath, std::ios::binary);
        if (file.is_open()) {
            pSaveInfo->thumbnailData = new uint8_t[thumbSize];
            if (!file.read(reinterpret_cast<char*>(pSaveInfo->thumbnailData),
                           thumbSize) ||
                static_cast<unsigned int>(file.gcount()) != thumbSize) {
                delete[] pSaveInfo->thumbnailData;
                pSaveInfo->thumbnailData = nullptr;
            }
        }
    }

    callback(pSaveInfo->thumbnailData, pSaveInfo->metaData.thumbnailSize);
    return StdStorage::ESaveGame_GetSaveThumbnail;
}

StdStorage::ESaveGameState StdStorage::DeleteSaveData(
    PSAVE_INFO pSaveInfo, std::function<int(const bool)> callback) {
    std::filesystem::path baseDir =
        this->gameHDDDir / pSaveInfo->UTF8SaveFilename;
    std::filesystem::path thumbDir = baseDir / "thumbnails";

    // nuke all non-directory files directly under the save directory
    if (std::filesystem::exists(baseDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(baseDir)) {
            if (entry.is_regular_file()) {
                std::filesystem::remove(entry.path());
                break;
            }
        }
    }

    std::filesystem::remove_all(thumbDir);
    std::filesystem::remove(baseDir);

    PSAVE_INFO m_pDeleteInfo = pSaveInfo;
    assert(
        (m_pDeleteInfo >= &m_pSaveDetails->SaveInfoA[0]) &&
        (m_pDeleteInfo < &m_pSaveDetails->SaveInfoA[m_pSaveDetails->iSaveC]));

    uint64_t index = pSaveInfo - this->m_pSaveDetails->SaveInfoA;

    for (int j = index; j < this->m_pSaveDetails->iSaveC - 1; ++j) {
        this->m_pSaveDetails->SaveInfoA[j] =
            this->m_pSaveDetails->SaveInfoA[j + 1];
    }
    --this->m_pSaveDetails->iSaveC;

    callback(true);

    return StdStorage::ESaveGame_Idle;
}

// these are all more or less stubs, but that's alright for now
bool StdStorage::SetSaveDevice(std::function<int(const bool)> callback,
                               bool bForceResetOfSaveDevice) {
    return true;
}
void StdStorage::SetSaveDeviceSelected(unsigned int uiPad, bool bSelected) {
    // XUI only
}
bool StdStorage::GetSaveDeviceSelected(unsigned int iPad) {
    // XUI only
    return true;
}
StdStorage::ESaveGameState StdStorage::DoesSaveExist(bool* pbExists) {
    *pbExists = true;
    return ESaveGame_Idle;
}
bool StdStorage::EnoughSpaceForAMinSaveGame() {
    // other platforms did this too
    return true;
}

// MARK: CDLC

// Offers - these are stubbed since we don't care about the marketplace
XMARKETPLACE_CONTENTOFFER_INFO InternalContentOfferInfo;

void StdStorage::RegisterMarketplaceCountsCallback(
    std::function<int(StdStorage::DLC_TMS_DETAILS*, int)> callback) {
    // unused
}
StdStorage::EDLCStatus StdStorage::GetDLCOffers(
    int iPad, std::function<int(int, std::uint32_t, int)> callback,
    std::uint32_t dwOfferTypesBitmask) {
    return EDLC_Idle;
}
unsigned int StdStorage::CancelGetDLCOffers() { return 0; }

void StdStorage::ClearDLCOffers() {}
XMARKETPLACE_CONTENTOFFER_INFO& StdStorage::GetOffer(unsigned int dw) {
    return InternalContentOfferInfo;
}
int StdStorage::GetOfferCount() { return 0; }
unsigned int StdStorage::InstallOffer(int iOfferIDC, std::uint64_t* ullOfferIDA,
                                      std::function<int(int, int)> callback,
                                      bool bTrial) {
    return 0;
}
unsigned int StdStorage::GetAvailableDLCCount(int iPad) { return 0; }

// the good stuff
void StdStorage::Tick(void) {
    if (m_iHasNewInstalledDLCs) {
        m_iHasNewInstalledDLCs = false;
        m_pInstalledDLCFunc(static_cast<int>(m_vInstalledDLCs.size()), 0);
    }
    if (m_iHasNewMountedDLCs) {
        m_iHasNewMountedDLCs = false;
        m_pMountedDLCFunc(0, 0, m_dwLicenseMask);
    }
}

void StdStorage::SetDLCPackageRoot(char* pszDLCRoot) {
    strcpy(this->m_szPackageRoot, pszDLCRoot);
}
StdStorage::EDLCStatus StdStorage::GetInstalledDLC(
    int iPad, std::function<int(int, int)> callback) {
    if (callback) {
        callback(0, iPad);
    }
    if (m_iHasNewInstalledDLCs) return StdStorage::EDLC_Pending;

    m_pInstalledDLCFunc = callback;
    m_iHasNewInstalledDLCs = true;

    // MinecrafftConsoles falls back to Windows64/DLC if nothing was found
    std::filesystem::path dlcDir;
    if (std::filesystem::is_directory(basePath / "Windows64Media/DLC")) {
        dlcDir = basePath / "Windows64Media/DLC";
    } else if (std::filesystem::is_directory(basePath / "Windows64/DLC")) {
        dlcDir = basePath / "Windows64/DLC";
    } else {
        fprintf(stderr, "No DLC directory, can't have any DLC installed\n");
        return StdStorage::EDLC_Error;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dlcDir)) {
        if (!entry.is_directory()) continue;

        const std::string name = entry.path().filename().string();
        if (name.empty() || name[0] == '.') continue;

        XCONTENT_DATA data{};
        std::snprintf(data.szFileName, sizeof(data.szFileName), "%s/%s",
                      dlcDir.string().c_str(), name.c_str());
        std::snprintf(data.szDisplayName, 256, "%s", name.c_str());

        data.DeviceID = 0;
        data.dwContentType = 0;

        m_vInstalledDLCs.push_back(data);
    }

    return StdStorage::EDLC_Idle;
}
XCONTENT_DATA& StdStorage::GetDLC(unsigned int dw) {
    return m_vInstalledDLCs[dw];
}
std::uint32_t StdStorage::MountInstalledDLC(
    int iPad, std::uint32_t dwDLC,
    std::function<int(int, std::uint32_t, std::uint32_t)> callback,
    const char* szMountDrive) {
    m_pMountedDLCFunc = callback;
    m_szMountPath = szMountDrive ? szMountDrive : m_szPackageRoot;
    m_uiCurrentMappedDLC = dwDLC;

    const char* dlcdirPath = m_vInstalledDLCs[m_uiCurrentMappedDLC].szFileName;
    m_vDLCDriveMappings.emplace_back(dlcdirPath, m_szMountPath);

    m_iHasNewMountedDLCs = true;

    return 997;
}
unsigned int StdStorage::UnmountInstalledDLC(const char* szMountDrive) {
    const std::string szDrive = szMountDrive ? szMountDrive : m_szPackageRoot;

    for (auto it = m_vDLCDriveMappings.begin(); it != m_vDLCDriveMappings.end();
         ++it) {
        if (it->m_szDirectoryPath == szDrive) {
            m_vDLCDriveMappings.erase(it);
            return 0;
        }
    }
    return 0;
}
void StdStorage::GetMountedDLCFileList(const char* szMountDrive,
                                       std::vector<std::string>& fileList) {
    const std::filesystem::path dlcDir =
        m_vInstalledDLCs[m_uiCurrentMappedDLC].szFileName;

    for (const auto& entry : std::filesystem::directory_iterator(dlcDir)) {
        if (entry.is_regular_file()) fileList.push_back(entry.path().string());
    }
}

// this is used by java/File.h to enable acccessing mounted TPACK:/ directories
// extracted from .pck files while the game is running in the DLC folder
std::string StdStorage::GetMountedPath(std::string szMount) {
    for (size_t ch = 0; ch < szMount.size(); ++ch) {
        if (szMount[ch] == '/' || szMount[ch] == '\\') return "";

        if (szMount[ch] == ':') {
            const std::string driveName = szMount.substr(0, ch);
            for (const auto& mapping : m_vDLCDriveMappings) {
                if (mapping.m_szMountPath == driveName) {
                    std::string newPath = mapping.m_szDirectoryPath;
                    newPath += szMount.substr(ch + 1);
                    return newPath;
                }
            }
            break;
        }
    }
    return "";
}

// MARK: Split Save Stubs

int StdStorage::AddSubfile(int regionIndex) {
    (void)regionIndex;
    return 0;
}
unsigned int StdStorage::GetSubfileCount() { return 0; }
void StdStorage::GetSubfileDetails(unsigned int i, int* regionIndex,
                                   void** data, unsigned int* size) {
    (void)i;
    if (regionIndex) *regionIndex = 0;
    if (data) *data = 0;
    if (size) *size = 0;
}
void StdStorage::ResetSubfiles() {}
void StdStorage::UpdateSubfile(int index, void* data, unsigned int size) {
    (void)index;
    (void)data;
    (void)size;
}
void StdStorage::SaveSubfiles(std::function<int(const bool)> callback) {
    if (callback) callback(true);
}
StdStorage::ESaveGameState StdStorage::GetSaveState() { return ESaveGame_Idle; }
void StdStorage::ContinueIncompleteOperation() {}
