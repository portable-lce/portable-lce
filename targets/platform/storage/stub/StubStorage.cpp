#include "StubStorage.h"

#include <stdlib.h>

#include <cstring>
#include <string>
#include <vector>

namespace platform_internal {
IPlatformStorage& PlatformStorage_get() {
    static StubStorage instance;
    return instance;
}
}

static XMARKETPLACE_CONTENTOFFER_INFO s_dummyOffer = {};
static XCONTENT_DATA s_dummyContentData = {};

StubStorage::StubStorage() : m_pStringTable(nullptr) {}

void StubStorage::Tick(void) {}

StubStorage::EMessageResult StubStorage::RequestMessageBox(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int pad,
    std::function<int(int, const StubStorage::EMessageResult)> callback,
    C4JStringTable* pStringTable, char* pwchFormatString,
    unsigned int focusButton) {
    return EMessage_ResultAccept;
}

StubStorage::EMessageResult StubStorage::GetMessageBoxResult() {
    return EMessage_Undefined;
}

bool StubStorage::SetSaveDevice(std::function<int(const bool)> callback,
                               bool bForceResetOfSaveDevice) {
    return true;
}

void StubStorage::Init(unsigned int uiSaveVersion,
                      const char* pwchDefaultSaveName, char* pszSavePackName,
                      int iMinimumSaveSize,
                      std::function<int(const ESavingMessage, int)> callback,
                      const char* szGroupID) {}
void StubStorage::ResetSaveData() {}
void StubStorage::SetDefaultSaveNameForKeyboardDisplay(
    const char* pwchDefaultSaveName) {}
void StubStorage::SetSaveTitle(const char* pwchDefaultSaveName) {}
bool StubStorage::GetSaveUniqueNumber(int* piVal) {
    if (piVal) *piVal = 0;
    return true;
}
bool StubStorage::GetSaveUniqueFilename(char* pszName) {
    if (pszName) pszName[0] = '\0';
    return true;
}
void StubStorage::SetSaveUniqueFilename(char* szFilename) {}
void StubStorage::SetState(ESaveGameControlState eControlState,
                          std::function<int(const bool)> callback) {}
void StubStorage::SetSaveDisabled(bool bDisable) {}
bool StubStorage::GetSaveDisabled(void) { return false; }
unsigned int StubStorage::GetSaveSize() { return 0; }
void StubStorage::GetSaveData(void* pvData, unsigned int* puiBytes) {
    if (puiBytes) *puiBytes = 0;
}
void* StubStorage::AllocateSaveData(unsigned int uiBytes) {
    return malloc(uiBytes);
}
void StubStorage::SetSaveImages(std::uint8_t* pbThumbnail,
                               unsigned int thumbnailBytes,
                               std::uint8_t* pbImage, unsigned int imageBytes,
                               std::uint8_t* pbTextData,
                               unsigned int textDataBytes) {}
StubStorage::ESaveGameState StubStorage::SaveSaveData(
    std::function<int(const bool)> callback) {
    return ESaveGame_Idle;
}
void StubStorage::CopySaveDataToNewSave(std::uint8_t* pbThumbnail,
                                       unsigned int cbThumbnail,
                                       char* wchNewName,
                                       std::function<int(bool)> callback) {}
void StubStorage::SetSaveDeviceSelected(unsigned int uiPad, bool bSelected) {}
bool StubStorage::GetSaveDeviceSelected(unsigned int iPad) { return true; }
StubStorage::ESaveGameState StubStorage::DoesSaveExist(bool* pbExists) {
    if (pbExists) *pbExists = false;
    return ESaveGame_Idle;
}
bool StubStorage::EnoughSpaceForAMinSaveGame() { return true; }
void StubStorage::SetSaveMessageVPosition(float fY) {}
StubStorage::ESaveGameState StubStorage::GetSavesInfo(
    int iPad,
    std::function<int(SAVE_DETAILS* pSaveDetails, const bool)> callback,
    char* pszSavePackName) {
    return ESaveGame_Idle;
}
PSAVE_DETAILS StubStorage::ReturnSavesInfo() { return nullptr; }
void StubStorage::ClearSavesInfo() {}
StubStorage::ESaveGameState StubStorage::LoadSaveDataThumbnail(
    PSAVE_INFO pSaveInfo,
    std::function<int(std::uint8_t* thumbnailData,
                      unsigned int thumbnailBytes)>
        callback) {
    return ESaveGame_Idle;
}
void StubStorage::GetSaveCacheFileInfo(unsigned int fileIndex,
                                      XCONTENT_DATA& xContentData) {
    memset(&xContentData, 0, sizeof(xContentData));
}
void StubStorage::GetSaveCacheFileInfo(unsigned int fileIndex,
                                      std::uint8_t** ppbImageData,
                                      unsigned int* pImageBytes) {
    if (ppbImageData) *ppbImageData = nullptr;
    if (pImageBytes) *pImageBytes = 0;
}
StubStorage::ESaveGameState StubStorage::LoadSaveData(
    PSAVE_INFO pSaveInfo,
    std::function<int(const bool, const bool)> callback) {
    return ESaveGame_Idle;
}
StubStorage::ESaveGameState StubStorage::DeleteSaveData(
    PSAVE_INFO pSaveInfo,
    std::function<int(const bool)> callback) {
    return ESaveGame_Idle;
}
void StubStorage::RegisterMarketplaceCountsCallback(
    std::function<int(StubStorage::DLC_TMS_DETAILS*, int)> callback) {}
void StubStorage::SetDLCPackageRoot(char* pszDLCRoot) {}
StubStorage::EDLCStatus StubStorage::GetDLCOffers(
    int iPad, std::function<int(int, std::uint32_t, int)> callback,
    std::uint32_t dwOfferTypesBitmask) {
    return EDLC_NoOffers;
}
unsigned int StubStorage::CancelGetDLCOffers() { return 0; }
void StubStorage::ClearDLCOffers() {}
XMARKETPLACE_CONTENTOFFER_INFO& StubStorage::GetOffer(unsigned int dw) {
    return s_dummyOffer;
}
int StubStorage::GetOfferCount() { return 0; }
unsigned int StubStorage::InstallOffer(int iOfferIDC, std::uint64_t* ullOfferIDA,
                                      std::function<int(int, int)> callback,
                                      bool bTrial) {
    return 0;
}
unsigned int StubStorage::GetAvailableDLCCount(int iPad) { return 0; }
StubStorage::EDLCStatus StubStorage::GetInstalledDLC(
    int iPad, std::function<int(int, int)> callback) {
    if (callback) {
        callback(0, iPad);
    }
    return EDLC_NoInstalledDLC;
}
XCONTENT_DATA& StubStorage::GetDLC(unsigned int dw) {
    return s_dummyContentData;
}
std::uint32_t StubStorage::MountInstalledDLC(
    int iPad, std::uint32_t dwDLC,
    std::function<int(int, std::uint32_t, std::uint32_t)> callback,
    const char* szMountDrive) {
    return 0;
}
unsigned int StubStorage::UnmountInstalledDLC(const char* szMountDrive) {
    return 0;
}
void StubStorage::GetMountedDLCFileList(const char* szMountDrive,
                                       std::vector<std::string>& fileList) {
    fileList.clear();
}
std::string StubStorage::GetMountedPath(std::string szMount) { return ""; }
StubStorage::ETMSStatus StubStorage::ReadTMSFile(
    int iQuadrant, eGlobalStorage eStorageFacility,
    StubStorage::eTMS_FileType eFileType, char* pwchFilename,
    std::uint8_t** ppBuffer, unsigned int* pBufferSize,
    std::function<int(char*, int, bool, int)> callback, int iAction) {
    return ETMSStatus_Fail;
}
bool StubStorage::WriteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                              char* pwchFilename, std::uint8_t* pBuffer,
                              unsigned int bufferSize) {
    return false;
}
bool StubStorage::DeleteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                               char* pwchFilename) {
    return false;
}
void StubStorage::StoreTMSPathName(char* pwchName) {}
StubStorage::ETMSStatus StubStorage::TMSPP_ReadFile(
    int iPad, StubStorage::eGlobalStorage eStorageFacility,
    StubStorage::eTMS_FILETYPEVAL eFileTypeVal, const char* szFilename,
    std::function<int(int, int, PTMSPP_FILEDATA, const char*)> callback,
    int iUserData) {
    return ETMSStatus_Fail;
}
unsigned int StubStorage::CRC(unsigned char* buf, int len) {
    unsigned int crc = 0xFFFFFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

int StubStorage::AddSubfile(int regionIndex) {
    (void)regionIndex;
    return 0;
}
unsigned int StubStorage::GetSubfileCount() { return 0; }
void StubStorage::GetSubfileDetails(unsigned int i, int* regionIndex,
                                   void** data, unsigned int* size) {
    (void)i;
    if (regionIndex) *regionIndex = 0;
    if (data) *data = 0;
    if (size) *size = 0;
}
void StubStorage::ResetSubfiles() {}
void StubStorage::UpdateSubfile(int index, void* data, unsigned int size) {
    (void)index;
    (void)data;
    (void)size;
}
void StubStorage::SaveSubfiles(std::function<int(const bool)> callback) {
    if (callback) callback(true);
}
StubStorage::ESaveGameState StubStorage::GetSaveState() { return ESaveGame_Idle; }
void StubStorage::ContinueIncompleteOperation() {}
