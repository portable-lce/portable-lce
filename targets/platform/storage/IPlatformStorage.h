#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

#include "platform/PlatformTypes.h"

#define MAX_DISPLAYNAME_LENGTH 128  // CELL_SAVEDATA_SYSP_SUBTITLE_SIZE on PS3
#define MAX_DETAILS_LENGTH 128      // CELL_SAVEDATA_SYSP_SUBTITLE_SIZE on PS3
#define MAX_SAVEFILENAME_LENGTH 32  // CELL_SAVEDATA_DIRNAME_SIZE
// Current version of the dlc data creator
#define CURRENT_DLC_VERSION_NUM 3

struct CONTAINER_METADATA {
    time_t modifiedTime;
    unsigned int dataSize;
    unsigned int thumbnailSize;
};

struct SAVE_INFO {
    char UTF8SaveFilename[MAX_SAVEFILENAME_LENGTH];
    char UTF8SaveTitle[MAX_DISPLAYNAME_LENGTH];
    CONTAINER_METADATA metaData;
    std::uint8_t* thumbnailData;
};
using PSAVE_INFO = SAVE_INFO*;

struct SAVE_DETAILS {
    int iSaveC;
    PSAVE_INFO SaveInfoA;
};
using PSAVE_DETAILS = SAVE_DETAILS*;

class C4JStringTable;

class IPlatformStorage {
public:
    // Enums live here so both the interface consumer and the concrete
    // implementation share the same values without a circular include.
    enum EMessageResult {
        EMessage_Undefined = 0,
        EMessage_Busy,
        EMessage_Pending,
        EMessage_Cancelled,
        EMessage_ResultAccept,
        EMessage_ResultDecline,
        EMessage_ResultThirdOption,
        EMessage_ResultFourthOption
    };

    enum ESaveGameState {
        ESaveGame_Idle = 0,
        ESaveGame_Save,
        ESaveGame_InternalRequestingDevice,
        ESaveGame_InternalGetSaveName,
        ESaveGame_InternalSaving,
        ESaveGame_CopySave,
        ESaveGame_CopyingSave,
        ESaveGame_Load,
        ESaveGame_GetSavesInfo,
        ESaveGame_Rename,
        ESaveGame_Delete,
        ESaveGame_GetSaveThumbnail
    };

    enum ESaveGameControlState {
        ESaveGameControl_Idle = 0,
        ESaveGameControl_Save,
        ESaveGameControl_InternalRequestingDevice,
        ESaveGameControl_InternalGetSaveName,
        ESaveGameControl_InternalSaving,
        ESaveGameControl_CopySave,
        ESaveGameControl_CopyingSave,
    };

    enum ESavingMessage {
        ESavingMessage_None = 0,
        ESavingMessage_Short,
        ESavingMessage_Long
    };

    enum EDLCStatus {
        EDLC_Error = 0,
        EDLC_Idle,
        EDLC_NoOffers,
        EDLC_AlreadyEnumeratedAllOffers,
        EDLC_NoInstalledDLC,
        EDLC_Pending,
        EDLC_LoadInProgress,
        EDLC_Loaded,
        EDLC_ChangedDevice
    };

    enum ETMSStatus {
        ETMSStatus_Idle = 0,
        ETMSStatus_Fail,
        ETMSStatus_Fail_ReadInProgress,
        ETMSStatus_Fail_WriteInProgress,
        ETMSStatus_Pending,
    };

    enum eGlobalStorage {
        eGlobalStorage_Title = 0,
        eGlobalStorage_TitleUser,
        eGlobalStorage_Max
    };

    enum eTMS_FileType {
        eTMS_FileType_Normal = 0,
        eTMS_FileType_Graphic,
    };

    enum eTMS_FILETYPEVAL {
        TMS_FILETYPE_BINARY,
        TMS_FILETYPE_CONFIG,
        TMS_FILETYPE_JSON,
        TMS_FILETYPE_MAX
    };

    struct TMSPP_FILEDATA {
        unsigned int size;
        std::uint8_t* pbData;
    };
    using PTMSPP_FILEDATA = TMSPP_FILEDATA*;

    struct DLC_TMS_DETAILS {
        std::uint32_t dwVersion;
        std::uint32_t dwNewOffers;
        std::uint32_t dwTotalOffers;
        std::uint32_t dwInstalledTotalOffers;
        std::uint8_t bPadding[1024 - sizeof(std::uint32_t) * 4];
    };

    struct DLC_FILE_DETAILS {
        unsigned int uiFileSize;
        std::uint32_t dwType;
        std::uint32_t dwWchCount;
        char wchFile[1];
    };
    using PDLC_FILE_DETAILS = DLC_FILE_DETAILS*;

    struct DLC_FILE_PARAM {
        std::uint32_t dwType;
        std::uint32_t dwWchCount;
        char wchData[1];
    };
    using PDLC_FILE_PARAM = DLC_FILE_PARAM*;

    virtual ~IPlatformStorage() = default;

    // Lifecycle
    virtual void Tick() = 0;
    virtual void Init(unsigned int uiSaveVersion,
                      const char* pwchDefaultSaveName, char* pszSavePackName,
                      int iMinimumSaveSize,
                      std::function<int(const ESavingMessage, int)> callback,
                      const char* szGroupID) = 0;
    virtual void ResetSaveData() = 0;

    // Messages
    virtual EMessageResult RequestMessageBox(
        unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
        unsigned int uiOptionC, unsigned int pad = XUSER_INDEX_ANY,
        std::function<int(int, const EMessageResult)> callback = nullptr,
        C4JStringTable* pStringTable = nullptr,
        char* pwchFormatString = nullptr, unsigned int focusButton = 0) = 0;
    virtual EMessageResult GetMessageBoxResult() = 0;

    // Save device
    virtual bool SetSaveDevice(std::function<int(const bool)> callback,
                               bool bForceResetOfSaveDevice = false) = 0;
    virtual void SetSaveDeviceSelected(unsigned int uiPad, bool bSelected) = 0;
    virtual bool GetSaveDeviceSelected(unsigned int iPad) = 0;

    // Save game
    virtual void SetDefaultSaveNameForKeyboardDisplay(
        const char* pwchDefaultSaveName) = 0;
    virtual void SetSaveTitle(const char* pwchDefaultSaveName) = 0;
    virtual bool GetSaveUniqueNumber(int* piVal) = 0;
    virtual bool GetSaveUniqueFilename(char* pszName) = 0;
    virtual void SetSaveUniqueFilename(char* szFilename) = 0;
    virtual void SetState(ESaveGameControlState eControlState,
                          std::function<int(const bool)> callback) = 0;
    virtual void SetSaveDisabled(bool bDisable) = 0;
    virtual bool GetSaveDisabled() = 0;
    virtual unsigned int GetSaveSize() = 0;
    virtual void GetSaveData(void* pvData, unsigned int* puiBytes) = 0;
    virtual void* AllocateSaveData(unsigned int uiBytes) = 0;
    virtual void SetSaveImages(std::uint8_t* pbThumbnail,
                               unsigned int thumbnailBytes,
                               std::uint8_t* pbImage, unsigned int imageBytes,
                               std::uint8_t* pbTextData,
                               unsigned int textDataBytes) = 0;
    virtual ESaveGameState SaveSaveData(
        std::function<int(const bool)> callback) = 0;
    virtual void CopySaveDataToNewSave(std::uint8_t* pbThumbnail,
                                       unsigned int cbThumbnail,
                                       char* wchNewName,
                                       std::function<int(bool)> callback) = 0;
    virtual ESaveGameState DoesSaveExist(bool* pbExists) = 0;
    virtual bool EnoughSpaceForAMinSaveGame() = 0;
    virtual void SetSaveMessageVPosition(float fY) = 0;
    virtual ESaveGameState GetSavesInfo(
        int iPad,
        std::function<int(SAVE_DETAILS* pSaveDetails, const bool)> callback,
        char* pszSavePackName) = 0;
    virtual PSAVE_DETAILS ReturnSavesInfo() = 0;
    virtual void ClearSavesInfo() = 0;
    virtual ESaveGameState LoadSaveDataThumbnail(
        PSAVE_INFO pSaveInfo, std::function<int(std::uint8_t* thumbnailData,
                                                unsigned int thumbnailBytes)>
                                  callback) = 0;
    virtual void GetSaveCacheFileInfo(unsigned int fileIndex,
                                      XCONTENT_DATA& xContentData) = 0;
    virtual void GetSaveCacheFileInfo(unsigned int fileIndex,
                                      std::uint8_t** ppbImageData,
                                      unsigned int* pImageBytes) = 0;
    virtual ESaveGameState LoadSaveData(
        PSAVE_INFO pSaveInfo,
        std::function<int(const bool, const bool)> callback) = 0;
    virtual ESaveGameState DeleteSaveData(
        PSAVE_INFO pSaveInfo, std::function<int(const bool)> callback) = 0;

    // DLC
    virtual void RegisterMarketplaceCountsCallback(
        std::function<int(DLC_TMS_DETAILS*, int)> callback) = 0;
    virtual void SetDLCPackageRoot(char* pszDLCRoot) = 0;
    virtual EDLCStatus GetDLCOffers(
        int iPad, std::function<int(int, std::uint32_t, int)> callback,
        std::uint32_t dwOfferTypesBitmask =
            XMARKETPLACE_OFFERING_TYPE_CONTENT) = 0;
    virtual unsigned int CancelGetDLCOffers() = 0;
    virtual void ClearDLCOffers() = 0;
    virtual XMARKETPLACE_CONTENTOFFER_INFO& GetOffer(unsigned int dw) = 0;
    virtual int GetOfferCount() = 0;
    virtual unsigned int InstallOffer(int iOfferIDC, std::uint64_t* ullOfferIDA,
                                      std::function<int(int, int)> callback,
                                      bool bTrial = false) = 0;
    virtual unsigned int GetAvailableDLCCount(int iPad) = 0;
    virtual EDLCStatus GetInstalledDLC(
        int iPad, std::function<int(int, int)> callback) = 0;
    virtual XCONTENT_DATA& GetDLC(unsigned int dw) = 0;
    virtual std::uint32_t MountInstalledDLC(
        int iPad, std::uint32_t dwDLC,
        std::function<int(int, std::uint32_t, std::uint32_t)> callback,
        const char* szMountDrive = nullptr) = 0;
    virtual unsigned int UnmountInstalledDLC(
        const char* szMountDrive = nullptr) = 0;
    virtual void GetMountedDLCFileList(const char* szMountDrive,
                                       std::vector<std::string>& fileList) = 0;
    virtual std::string GetMountedPath(std::string szMount) = 0;

    // Title storage
    virtual ETMSStatus ReadTMSFile(
        int iQuadrant, eGlobalStorage eStorageFacility, eTMS_FileType eFileType,
        char* pwchFilename, std::uint8_t** ppBuffer, unsigned int* pBufferSize,
        std::function<int(char*, int, bool, int)> callback = nullptr,
        int iAction = 0) = 0;
    virtual bool WriteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                              char* pwchFilename, std::uint8_t* pBuffer,
                              unsigned int bufferSize) = 0;
    virtual bool DeleteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                               char* pwchFilename) = 0;
    virtual void StoreTMSPathName(char* pwchName = nullptr) = 0;
    virtual ETMSStatus TMSPP_ReadFile(
        int iPad, eGlobalStorage eStorageFacility,
        eTMS_FILETYPEVAL eFileTypeVal, const char* szFilename,
        std::function<int(int, int, PTMSPP_FILEDATA, const char*)> callback =
            nullptr,
        int iUserData = 0) = 0;

    // Subfile management (save splitting)
    virtual int AddSubfile(int regionIndex) = 0;
    virtual unsigned int GetSubfileCount() = 0;
    virtual void GetSubfileDetails(unsigned int i, int* regionIndex,
                                   void** data, unsigned int* size) = 0;
    virtual void ResetSubfiles() = 0;
    virtual void UpdateSubfile(int index, void* data, unsigned int size) = 0;
    virtual void SaveSubfiles(std::function<int(const bool)> callback) = 0;
    virtual ESaveGameState GetSaveState() = 0;

    // Misc
    virtual unsigned int CRC(unsigned char* buf, int len) = 0;
    virtual void ContinueIncompleteOperation() = 0;
};
