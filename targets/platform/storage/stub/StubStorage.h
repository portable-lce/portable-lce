#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <string>
#include <vector>
// #include <xtms.h>

#include "../IPlatformStorage.h"
#include "PlatformTypes.h"

class C4JStringTable;

typedef std::vector<PXMARKETPLACE_CONTENTOFFER_INFO> OfferDataArray;
typedef std::vector<PXCONTENT_DATA> XContentDataArray;

class StubStorage : public IPlatformStorage {
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

    StubStorage();

    void Tick(void);

    // Messages
    StubStorage::EMessageResult RequestMessageBox(
        unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
        unsigned int uiOptionC, unsigned int pad = XUSER_INDEX_ANY,
        std::function<int(int, const StubStorage::EMessageResult)> callback =
            nullptr,
        C4JStringTable* pStringTable = nullptr,
        char* pwchFormatString = nullptr, unsigned int focusButton = 0);

    StubStorage::EMessageResult GetMessageBoxResult();

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
    void SetDefaultSaveNameForKeyboardDisplay(const char* pwchDefaultSaveName);
    void SetSaveTitle(const char* pwchDefaultSaveName);
    bool GetSaveUniqueNumber(int* piVal);
    bool GetSaveUniqueFilename(char* pszName);
    void SetSaveUniqueFilename(char* szFilename);
    void SetState(ESaveGameControlState eControlState,
                  std::function<int(const bool)> callback);
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
    StubStorage::ESaveGameState SaveSaveData(
        std::function<int(const bool)> callback);
    void CopySaveDataToNewSave(std::uint8_t* pbThumbnail,
                               unsigned int cbThumbnail, char* wchNewName,
                               std::function<int(bool)> callback);
    void SetSaveDeviceSelected(unsigned int uiPad, bool bSelected);
    bool GetSaveDeviceSelected(unsigned int iPad);
    StubStorage::ESaveGameState DoesSaveExist(bool* pbExists);
    bool EnoughSpaceForAMinSaveGame();

    void SetSaveMessageVPosition(
        float fY);  // The 'Saving' message will display at a default position
                    // unless changed
    // Get the info for the saves
    StubStorage::ESaveGameState GetSavesInfo(
        int iPad,
        std::function<int(SAVE_DETAILS* pSaveDetails, const bool)> callback,
        char* pszSavePackName);
    PSAVE_DETAILS ReturnSavesInfo();
    void ClearSavesInfo();  // Clears results
    StubStorage::ESaveGameState LoadSaveDataThumbnail(
        PSAVE_INFO pSaveInfo,
        std::function<int(std::uint8_t* thumbnailData,
                          unsigned int thumbnailBytes)>
            callback);  // Get the thumbnail for an individual save referenced
                        // by pSaveInfo

    void GetSaveCacheFileInfo(unsigned int fileIndex,
                              XCONTENT_DATA& xContentData);
    void GetSaveCacheFileInfo(unsigned int fileIndex,
                              std::uint8_t** ppbImageData,
                              unsigned int* pImageBytes);

    // Load the save. Need to call GetSaveData once the callback is called
    StubStorage::ESaveGameState LoadSaveData(
        PSAVE_INFO pSaveInfo,
        std::function<int(const bool, const bool)> callback);
    StubStorage::ESaveGameState DeleteSaveData(
        PSAVE_INFO pSaveInfo, std::function<int(const bool)> callback);

    // DLC
    void RegisterMarketplaceCountsCallback(
        std::function<int(StubStorage::DLC_TMS_DETAILS*, int)> callback);
    void SetDLCPackageRoot(char* pszDLCRoot);
    StubStorage::EDLCStatus GetDLCOffers(
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

    StubStorage::EDLCStatus GetInstalledDLC(
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

    // Global title storage
    StubStorage::ETMSStatus ReadTMSFile(
        int iQuadrant, eGlobalStorage eStorageFacility,
        StubStorage::eTMS_FileType eFileType, char* pwchFilename,
        std::uint8_t** ppBuffer, unsigned int* pBufferSize,
        std::function<int(char*, int, bool, int)> callback = nullptr,
        int iAction = 0);
    bool WriteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                      char* pwchFilename, std::uint8_t* pBuffer,
                      unsigned int bufferSize);
    bool DeleteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                       char* pwchFilename);
    void StoreTMSPathName(char* pwchName = nullptr);

    // TMS++
#ifdef _XBOX
    StubStorage::ETMSStatus WriteTMSFile(
        int iPad, StubStorage::eGlobalStorage eStorageFacility,
        StubStorage::eTMS_FileType eFileType, char* pchFilePath,
        char* pchBuffer, unsigned int bufferSize, TMSCLIENT_CALLBACK Func,
        void* lpParam);
    int GetUserQuotaInfo(int iPad, TMSCLIENT_CALLBACK Func, void* lpParam);
#endif

    // Older TMS++ write/quota entry points were kept in platform-specific
    // implementations and are intentionally not part of this shared API.
    StubStorage::ETMSStatus TMSPP_ReadFile(
        int iPad, StubStorage::eGlobalStorage eStorageFacility,
        StubStorage::eTMS_FILETYPEVAL eFileTypeVal, const char* szFilename,
        std::function<int(int, int, PTMSPP_FILEDATA, const char*)> callback =
            nullptr,
        int iUserData = 0);
    // Older TMS++ list/delete helpers stayed platform-specific. The shared
    // surface keeps the read path plus CRC/subfile helpers below.

    // 	enum eXBLWS
    // 	{
    // 		eXBLWS_GET,
    // 		eXBLWS_POST,
    // 		eXBLWS_PUT,
    // 		eXBLWS_DELETE,
    // 	};
    // bool
    // XBLWS_Command(eXBLWS eCommand);

    unsigned int CRC(unsigned char* buf, int len);

    int AddSubfile(int regionIndex);
    unsigned int GetSubfileCount();
    void GetSubfileDetails(unsigned int i, int* regionIndex, void** data,
                           unsigned int* size);
    void ResetSubfiles();
    void UpdateSubfile(int index, void* data, unsigned int size);
    void SaveSubfiles(std::function<int(const bool)> callback);
    ESaveGameState GetSaveState();

    void ContinueIncompleteOperation();

    C4JStringTable* m_pStringTable;
};
