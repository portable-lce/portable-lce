#pragma once

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "../IPlatformStorage.h"
#include "platform/PlatformTypes.h"
#include "platform/fs/fs.h"

class C4JStringTable;

typedef std::vector<PXMARKETPLACE_CONTENTOFFER_INFO> OfferDataArray;
typedef std::vector<PXCONTENT_DATA> XContentDataArray;

class StdStorage : public IPlatformStorage {
public:
    struct CACHEINFOSTRUCT {
        char wchDisplayName[XCONTENT_MAX_DISPLAYNAME_LENGTH];
        char szFileName[XCONTENT_MAX_FILENAME_LENGTH];
        std::uint32_t dwImageOffset;
        std::uint32_t dwImageBytes;
    };

    enum eGTS_FileTypes { eGTS_Type_Skin = 0, eGTS_Type_Cape, eGTS_Type_MAX };

    enum ELoadGameStatus {
        ELoadGame_Idle = 0,
        ELoadGame_InProgress,
        ELoadGame_NoSaves,
        ELoadGame_ChangedDevice,
        ELoadGame_DeviceRemoved
    };

    enum EDeleteGameStatus {
        EDeleteGame_Idle = 0,
        EDeleteGame_InProgress,
    };

    enum ESGIStatus {
        ESGIStatus_Error = 0,
        ESGIStatus_Idle,
        ESGIStatus_ReadInProgress,
        ESGIStatus_NoSaves,
    };

    enum eTMS_UGCTYPE { TMS_UGCTYPE_NONE, TMS_UGCTYPE_IMAGE, TMS_UGCTYPE_MAX };

    struct TMSPP_FILE_DETAILS {
        char szFilename[256];
        int iFileSize;
        eTMS_FILETYPEVAL eFileTypeVal;
    };
    using PTMSPP_FILE_DETAILS = TMSPP_FILE_DETAILS*;

    struct TMSPP_FILE_LIST {
        int iCount;
        PTMSPP_FILE_DETAILS FileDetailsA;
    };
    using PTMSPP_FILE_LIST = TMSPP_FILE_LIST*;

    StdStorage();

    void Tick(void);

    // save device
    bool SetSaveDevice(std::function<int(const bool)> callback,
                       bool bForceResetOfSaveDevice = false);

    // savegame
    void Init(unsigned int uiSaveVersion, const char* pwchDefaultSaveName,
              char* pszSavePackName, int iMinimumSaveSize,
              std::function<int(const ESavingMessage, int)> callback,
              const char* szGroupID);
    void ResetSaveData();  // Call before a new save to clear out stored save
                           // file name
    void SetSaveTitle(const char* pwchDefaultSaveName);
    bool GetSaveUniqueNumber(int* piVal);
    bool GetSaveUniqueFilename(char* pszName);
    void SetSaveUniqueFilename(char* szFilename);
    void SetSaveDisabled(bool bDisable);
    bool GetSaveDisabled(void);
    unsigned int GetSaveSize();
    void GetSaveData(void* pvData, unsigned int* puiBytes);
    void* AllocateSaveData(unsigned int uiBytes);
    void SetSaveImages(
        std::uint8_t* pbThumbnail, unsigned int thumbnailBytes,
        std::uint8_t* pbImage, unsigned int imageBytes,
        std::uint8_t* pbTextData,
        unsigned int textDataBytes);  // Sets the thumbnail & image for the
                                      // save, optionally setting the
                                      // metadata in the png
    StdStorage::ESaveGameState SaveSaveData(
        std::function<int(const bool)> callback);
    void SetSaveDeviceSelected(unsigned int uiPad, bool bSelected);
    bool GetSaveDeviceSelected(unsigned int iPad);
    StdStorage::ESaveGameState DoesSaveExist(bool* pbExists);
    bool EnoughSpaceForAMinSaveGame();

    // Get the info for the saves
    StdStorage::ESaveGameState GetSavesInfo(
        int iPad,
        std::function<int(SAVE_DETAILS* pSaveDetails, const bool)> callback,
        char* pszSavePackName);
    PSAVE_DETAILS ReturnSavesInfo();
    void ClearSavesInfo();  // Clears results
    StdStorage::ESaveGameState LoadSaveDataThumbnail(
        PSAVE_INFO pSaveInfo,
        std::function<int(std::uint8_t* thumbnailData,
                          unsigned int thumbnailBytes)>
            callback);  // Get the thumbnail for an individual save referenced
                        // by pSaveInfo

    // Load the save. Need to call GetSaveData once the callback is called
    StdStorage::ESaveGameState LoadSaveData(
        PSAVE_INFO pSaveInfo,
        std::function<int(const bool, const bool)> callback);
    StdStorage::ESaveGameState DeleteSaveData(
        PSAVE_INFO pSaveInfo, std::function<int(const bool)> callback);

    // DLC
    void RegisterMarketplaceCountsCallback(
        std::function<int(StdStorage::DLC_TMS_DETAILS*, int)> callback);
    void SetDLCPackageRoot(char* pszDLCRoot);
    StdStorage::EDLCStatus GetDLCOffers(
        int iPad, std::function<int(int, std::uint32_t, int)> callback,
        std::uint32_t dwOfferTypesBitmask = XMARKETPLACE_OFFERING_TYPE_CONTENT);
    unsigned int CancelGetDLCOffers();
    void ClearDLCOffers();
    XMARKETPLACE_CONTENTOFFER_INFO& GetOffer(unsigned int dw);
    int GetOfferCount();
    unsigned int InstallOffer(int iOfferIDC, std::uint64_t* ullOfferIDA,
                              std::function<int(int, int)> callback,
                              bool bTrial = false);
    unsigned int GetAvailableDLCCount(int iPad);

    StdStorage::EDLCStatus GetInstalledDLC(
        int iPad, std::function<int(int, int)> callback);
    XCONTENT_DATA& GetDLC(unsigned int dw);
    std::uint32_t MountInstalledDLC(
        int iPad, std::uint32_t dwDLC,
        std::function<int(int, std::uint32_t, std::uint32_t)> callback,
        const char* szMountDrive = nullptr);
    unsigned int UnmountInstalledDLC(const char* szMountDrive = nullptr);
    void GetMountedDLCFileList(const char* szMountDrive,
                               std::vector<std::string>& fileList);
    std::string GetMountedPath(std::string szMount);

    int AddSubfile(int regionIndex);
    unsigned int GetSubfileCount();
    void GetSubfileDetails(unsigned int i, int* regionIndex, void** data,
                           unsigned int* size);
    void ResetSubfiles();
    void UpdateSubfile(int index, void* data, unsigned int size);
    void SaveSubfiles(std::function<int(const bool)> callback);
    ESaveGameState GetSaveState();

    void ContinueIncompleteOperation();

    std::filesystem::path basePath = PlatformFilesystem.getBasePath();
    std::filesystem::path gameHDDDir = basePath / "Windows64" / "GameHDD";

    void* m_pSaveData = nullptr;
    unsigned int m_uiSaveSize = 0;
    char m_szSaveUniqueName[32] = {};
    char m_szSaveTitle[256] = {};
    bool m_bIsSafeDisabled;
    bool m_bHasSaveDetails;
    SAVE_DETAILS* m_pSaveDetails = nullptr;

    uint8_t* m_pbThumbnailData = nullptr;
    unsigned int m_uiThumbnailSize = 0;
    uint8_t* m_pbImageData = nullptr;
    unsigned int m_uiImageSize = 0;

    // DLC-specific
    struct DriveMapping
    {
        DriveMapping(std::string szDirectoryPath, std::string szMountPath) : m_szDirectoryPath(szDirectoryPath), m_szMountPath(szMountPath)
        {
            ;
        }

        std::string m_szMountPath;
        std::string m_szDirectoryPath;
    };

    std::function<int(int, int)> m_pInstalledDLCFunc;
    int m_iHasNewInstalledDLCs;
    std::vector<XCONTENT_DATA> m_vInstalledDLCs;
    uint32_t m_iHasNewMountedDLCs;
    std::function<int(int, uint32_t, uint32_t)> m_pMountedDLCFunc;
    std::string m_szMountPath;
    uint32_t m_uiCurrentMappedDLC;
    uint32_t m_dwLicenseMask;
    char m_szPackageRoot[40];
    std::vector<DriveMapping> m_vDLCDriveMappings;
    char m_szDLCProductCode[16];
    char m_szProductUpgradeKey[60];
};
