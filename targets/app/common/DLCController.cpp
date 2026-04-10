#include "app/common/DLCController.h"

#include <cstring>
#include <mutex>

#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/DLC/DLCSkinFile.h"
#include "app/common/Game.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "platform/XboxStubs.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"

DLCController::DLCController() {
    m_pDLCFileBuffer = nullptr;
    m_dwDLCFileSize = 0;
    m_bDefaultCapeInstallAttempted = false;
    m_bDLCInstallProcessCompleted = false;
    m_bDLCInstallPending = false;
    m_iTotalDLC = 0;
    m_iTotalDLCInstalled = 0;
    m_bNewDLCAvailable = false;
    m_bSeenNewDLCTip = false;
    m_iDLCOfferC = 0;
    m_bAllDLCContentRetrieved = true;
    m_bAllTMSContentRetrieved = true;
    m_bTickTMSDLCFiles = true;
}

std::unordered_map<PlayerUID, MOJANG_DATA*> DLCController::MojangData;
std::unordered_map<int, uint64_t> DLCController::DLCTextures_PackID;
std::unordered_map<uint64_t, DLC_INFO*> DLCController::DLCInfo_Trial;
std::unordered_map<uint64_t, DLC_INFO*> DLCController::DLCInfo_Full;
std::unordered_map<std::string, uint64_t> DLCController::DLCInfo_SkinName;

std::uint32_t DLCController::m_dwContentTypeA[e_Marketplace_MAX] = {
    XMARKETPLACE_OFFERING_TYPE_CONTENT,
    XMARKETPLACE_OFFERING_TYPE_THEME,
    XMARKETPLACE_OFFERING_TYPE_AVATARITEM,
    XMARKETPLACE_OFFERING_TYPE_TILE,
};

int DLCController::marketplaceCountsCallback(
    void* pParam, IPlatformStorage::DLC_TMS_DETAILS* pTMSDetails, int iPad) {
    app.DebugPrintf("Marketplace Counts= New - %d Total - %d\n",
                    pTMSDetails->dwNewOffers, pTMSDetails->dwTotalOffers);

    if (pTMSDetails->dwNewOffers > 0) {
        app.m_dlcController.m_bNewDLCAvailable = true;
        app.m_dlcController.m_bSeenNewDLCTip = false;
    } else {
        app.m_dlcController.m_bNewDLCAvailable = false;
        app.m_dlcController.m_bSeenNewDLCTip = true;
    }

    return 0;
}

bool DLCController::startInstallDLCProcess(int iPad) {
    app.DebugPrintf("--- DLCController::startInstallDLCProcess: pad=%i.\n",
                    iPad);

    if ((dlcInstallProcessCompleted() == false) &&
        (m_bDLCInstallPending == false)) {
        app.m_dlcManager.resetUnnamedCorruptCount();
        m_bDLCInstallPending = true;
        m_iTotalDLC = 0;
        m_iTotalDLCInstalled = 0;
        app.DebugPrintf(
            "--- DLCController::startInstallDLCProcess - "
            "PlatformStorage.GetInstalledDLC\n");

        PlatformStorage.GetInstalledDLC(iPad, [this](int iInstalledC, int pad) {
            return dlcInstalledCallback(iInstalledC, pad);
        });
        return true;
    } else {
        app.DebugPrintf(
            "--- DLCController::startInstallDLCProcess - nothing to do\n");
        return false;
    }
}

int DLCController::dlcInstalledCallback(int iInstalledC, int iPad) {
    app.DebugPrintf(
        "--- DLCController::dlcInstalledCallback: totalDLC=%i, pad=%i.\n",
        iInstalledC, iPad);
    m_iTotalDLC = iInstalledC;
    mountNextDLC(iPad);
    return 0;
}

void DLCController::mountNextDLC(int iPad) {
    app.DebugPrintf("--- DLCController::mountNextDLC: pad=%i.\n", iPad);
    if (m_iTotalDLCInstalled < m_iTotalDLC) {
        if (PlatformStorage.MountInstalledDLC(
                iPad, m_iTotalDLCInstalled,
                [this](int pad, std::uint32_t dwErr,
                       std::uint32_t dwLicenceMask) {
                    return dlcMountedCallback(pad, dwErr, dwLicenceMask);
                }) != 997 /* ERROR_IO_PENDING */) {
            app.DebugPrintf("Failed to mount DLC %d for pad %d\n",
                            m_iTotalDLCInstalled, iPad);
            ++m_iTotalDLCInstalled;
            mountNextDLC(iPad);
        } else {
            app.DebugPrintf("PlatformStorage.MountInstalledDLC ok\n");
        }
    } else {
        m_bDLCInstallPending = false;
        m_bDLCInstallProcessCompleted = true;
        ui.HandleDLCMountingComplete();
    }
}

#if defined(_WINDOWS64)
#define CONTENT_DATA_DISPLAY_NAME(a) (a.szDisplayName)
#else
#define CONTENT_DATA_DISPLAY_NAME(a) (a.wszDisplayName)
#endif

int DLCController::dlcMountedCallback(int iPad, std::uint32_t dwErr,
                                      std::uint32_t dwLicenceMask) {
#if defined(_WINDOWS64)
    app.DebugPrintf("--- DLCController::dlcMountedCallback\n");

    if (dwErr != 0 /* ERROR_SUCCESS */) {
        app.DebugPrintf("Failed to mount DLC for pad %d: %u\n", iPad, dwErr);
        app.m_dlcManager.incrementUnnamedCorruptCount();
    } else {
        XCONTENT_DATA ContentData =
            PlatformStorage.GetDLC(m_iTotalDLCInstalled);

        DLCPack* pack =
            app.m_dlcManager.getPack(CONTENT_DATA_DISPLAY_NAME(ContentData));

        if (pack != nullptr && pack->IsCorrupt()) {
            app.DebugPrintf(
                "Pack '%s' is corrupt, removing it from the DLC Manager.\n",
                CONTENT_DATA_DISPLAY_NAME(ContentData));
            app.m_dlcManager.removePack(pack);
            pack = nullptr;
        }

        if (pack == nullptr) {
            app.DebugPrintf("Pack \"%s\" is not installed, so adding it\n",
                            CONTENT_DATA_DISPLAY_NAME(ContentData));

#if defined(_WINDOWS64)
            pack = new DLCPack(ContentData.szDisplayName, dwLicenceMask);
#else
            pack = new DLCPack(ContentData.wszDisplayName, dwLicenceMask);
#endif
            pack->SetDLCMountIndex(m_iTotalDLCInstalled);
            pack->SetDLCDeviceID(ContentData.DeviceID);
            app.m_dlcManager.addPack(pack);
            handleDLC(pack);

            if (pack->getDLCItemsCount(DLCManager::e_DLCType_Texture) > 0) {
                Minecraft::GetInstance()->skins->addTexturePackFromDLC(
                    pack, pack->GetPackId());
            }
        } else {
            app.DebugPrintf(
                "Pack \"%s\" is already installed. Updating license to %u\n",
                CONTENT_DATA_DISPLAY_NAME(ContentData), dwLicenceMask);

            pack->SetDLCMountIndex(m_iTotalDLCInstalled);
            pack->SetDLCDeviceID(ContentData.DeviceID);
            pack->updateLicenseMask(dwLicenceMask);
        }

        PlatformStorage.UnmountInstalledDLC();
    }
    ++m_iTotalDLCInstalled;
    mountNextDLC(iPad);
#endif
    return 0;
}
#undef CONTENT_DATA_DISPLAY_NAME

void DLCController::handleDLC(DLCPack* pack) {
    unsigned int dwFilesProcessed = 0;
#if defined(_WINDOWS64) || defined(__linux__)
    std::vector<std::string> dlcFilenames;
#endif
    PlatformStorage.GetMountedDLCFileList("DLCDrive", dlcFilenames);
    for (int i = 0; i < dlcFilenames.size(); i++) {
        app.m_dlcManager.readDLCDataFile(dwFilesProcessed, dlcFilenames[i],
                                         pack);
    }
    if (dwFilesProcessed == 0) app.m_dlcManager.removePack(pack);
}

void DLCController::addCreditText(const char* lpStr) {
    app.DebugPrintf("ADDING CREDIT - %s\n", lpStr);
    SCreditTextItemDef* pCreditStruct = new SCreditTextItemDef;
    pCreditStruct->m_eType = eSmallText;
    pCreditStruct->m_iStringID[0] = NO_TRANSLATED_STRING;
    pCreditStruct->m_iStringID[1] = NO_TRANSLATED_STRING;
    pCreditStruct->m_Text = new char[strlen(lpStr) + 1];
    strcpy((char*)pCreditStruct->m_Text, lpStr);
    vDLCCredits.push_back(pCreditStruct);
}

bool DLCController::alreadySeenCreditText(const std::string& wstemp) {
    for (unsigned int i = 0; i < m_vCreditText.size(); i++) {
        std::string temp = m_vCreditText.at(i);
        if (temp.compare(wstemp) == 0) {
            return true;
        }
    }
    m_vCreditText.push_back((char*)wstemp.c_str());
    return false;
}

unsigned int DLCController::getDLCCreditsCount() {
    return (unsigned int)vDLCCredits.size();
}

SCreditTextItemDef* DLCController::getDLCCredits(int iIndex) {
    return vDLCCredits.at(iIndex);
}

#if defined(_WINDOWS64)
int32_t DLCController::registerDLCData(char* pType, char* pBannerName,
                                       int iGender, uint64_t ullOfferID_Full,
                                       uint64_t ullOfferID_Trial,
                                       char* pFirstSkin,
                                       unsigned int uiSortIndex, int iConfig,
                                       char* pDataFile) {
    int32_t hr = 0;
    DLC_INFO* pDLCData = new DLC_INFO;
    memset(pDLCData, 0, sizeof(DLC_INFO));
    pDLCData->ullOfferID_Full = ullOfferID_Full;
    pDLCData->ullOfferID_Trial = ullOfferID_Trial;
    pDLCData->eDLCType = e_DLC_NotDefined;
    pDLCData->iGender = iGender;
    pDLCData->uiSortIndex = uiSortIndex;
    pDLCData->iConfig = iConfig;

    if (pBannerName != "") {
        wcsncpy_s(pDLCData->wchBanner, pBannerName, MAX_BANNERNAME_SIZE);
    }
    if (pDataFile[0] != 0) {
        wcsncpy_s(pDLCData->wchDataFile, pDataFile, MAX_BANNERNAME_SIZE);
    }

    if (pType != nullptr) {
        if (strcmp(pType, "Skin") == 0) {
            pDLCData->eDLCType = e_DLC_SkinPack;
        } else if (strcmp(pType, "Gamerpic") == 0) {
            pDLCData->eDLCType = e_DLC_Gamerpics;
        } else if (strcmp(pType, "Theme") == 0) {
            pDLCData->eDLCType = e_DLC_Themes;
        } else if (strcmp(pType, "Avatar") == 0) {
            pDLCData->eDLCType = e_DLC_AvatarItems;
        } else if (strcmp(pType, "MashUpPack") == 0) {
            pDLCData->eDLCType = e_DLC_MashupPacks;
            DLCTextures_PackID[pDLCData->iConfig] = ullOfferID_Full;
        } else if (strcmp(pType, "TexturePack") == 0) {
            pDLCData->eDLCType = e_DLC_TexturePacks;
            DLCTextures_PackID[pDLCData->iConfig] = ullOfferID_Full;
        }
    }

    if (ullOfferID_Trial != 0ll) DLCInfo_Trial[ullOfferID_Trial] = pDLCData;
    if (ullOfferID_Full != 0ll) DLCInfo_Full[ullOfferID_Full] = pDLCData;
    if (pFirstSkin[0] != 0) DLCInfo_SkinName[pFirstSkin] = ullOfferID_Full;

    return hr;
}
#elif defined(__linux__)
int32_t DLCController::registerDLCData(char* pType, char* pBannerName,
                                       int iGender, uint64_t ullOfferID_Full,
                                       uint64_t ullOfferID_Trial,
                                       char* pFirstSkin,
                                       unsigned int uiSortIndex, int iConfig,
                                       char* pDataFile) {
    fprintf(stderr,
            "warning: DLCController::registerDLCData unimplemented for "
            "platform `__linux__`\n");
    return 0;
}
#endif

bool DLCController::getDLCFullOfferIDForSkinID(const std::string& FirstSkin,
                                               uint64_t* pullVal) {
    auto it = DLCInfo_SkinName.find(FirstSkin);
    if (it == DLCInfo_SkinName.end()) {
        return false;
    } else {
        *pullVal = (uint64_t)it->second;
        return true;
    }
}

bool DLCController::getDLCFullOfferIDForPackID(const int iPackID,
                                               uint64_t* pullVal) {
    auto it = DLCTextures_PackID.find(iPackID);
    if (it == DLCTextures_PackID.end()) {
        *pullVal = (uint64_t)0;
        return false;
    } else {
        *pullVal = (uint64_t)it->second;
        return true;
    }
}

DLC_INFO* DLCController::getDLCInfoForTrialOfferID(uint64_t ullOfferID_Trial) {
    if (DLCInfo_Trial.size() > 0) {
        auto it = DLCInfo_Trial.find(ullOfferID_Trial);
        if (it == DLCInfo_Trial.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    } else
        return nullptr;
}

DLC_INFO* DLCController::getDLCInfoForFullOfferID(uint64_t ullOfferID_Full) {
    if (DLCInfo_Full.size() > 0) {
        auto it = DLCInfo_Full.find(ullOfferID_Full);
        if (it == DLCInfo_Full.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    } else
        return nullptr;
}

DLC_INFO* DLCController::getDLCInfoTrialOffer(int iIndex) {
    std::unordered_map<uint64_t, DLC_INFO*>::iterator it =
        DLCInfo_Trial.begin();
    for (int i = 0; i < iIndex; i++) {
        ++it;
    }
    return it->second;
}

DLC_INFO* DLCController::getDLCInfoFullOffer(int iIndex) {
    std::unordered_map<uint64_t, DLC_INFO*>::iterator it = DLCInfo_Full.begin();
    for (int i = 0; i < iIndex; i++) {
        ++it;
    }
    return it->second;
}

uint64_t DLCController::getDLCInfoTexturesFullOffer(int iIndex) {
    std::unordered_map<int, uint64_t>::iterator it = DLCTextures_PackID.begin();
    for (int i = 0; i < iIndex; i++) {
        ++it;
    }
    return it->second;
}

int DLCController::getDLCInfoTrialOffersCount() {
    return (int)DLCInfo_Trial.size();
}

int DLCController::getDLCInfoFullOffersCount() {
    return (int)DLCInfo_Full.size();
}

int DLCController::getDLCInfoTexturesOffersCount() {
    return (int)DLCTextures_PackID.size();
}

unsigned int DLCController::addDLCRequest(eDLCMarketplaceType eType,
                                          bool bPromote) {
    {
        std::lock_guard<std::mutex> lock(csDLCDownloadQueue);

        int iPosition = 0;
        for (auto it = m_DLCDownloadQueue.begin();
             it != m_DLCDownloadQueue.end(); ++it) {
            DLCRequest* pCurrent = *it;
            if (pCurrent->dwType == m_dwContentTypeA[eType]) {
                if (pCurrent->eState == e_DLC_ContentState_Retrieving ||
                    pCurrent->eState == e_DLC_ContentState_Retrieved) {
                    return 0;
                } else {
                    if (bPromote) {
                        m_DLCDownloadQueue.erase(m_DLCDownloadQueue.begin() +
                                                 iPosition);
                        m_DLCDownloadQueue.insert(m_DLCDownloadQueue.begin(),
                                                  pCurrent);
                    }
                    return 0;
                }
            }
            iPosition++;
        }

        DLCRequest* pDLCreq = new DLCRequest;
        pDLCreq->dwType = m_dwContentTypeA[eType];
        pDLCreq->eState = e_DLC_ContentState_Idle;
        m_DLCDownloadQueue.push_back(pDLCreq);
        m_bAllDLCContentRetrieved = false;
    }

    app.DebugPrintf("[Consoles_App] Added DLC request.\n");
    return 1;
}

bool DLCController::retrieveNextDLCContent() {
    int primPad = PlatformProfile.GetPrimaryPad();
    if (primPad == -1 || !PlatformProfile.IsSignedInLive(primPad)) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(csDLCDownloadQueue);
        for (auto it = m_DLCDownloadQueue.begin();
             it != m_DLCDownloadQueue.end(); ++it) {
            DLCRequest* pCurrent = *it;
            if (pCurrent->eState == e_DLC_ContentState_Retrieving) {
                return true;
            }
        }

        for (auto it = m_DLCDownloadQueue.begin();
             it != m_DLCDownloadQueue.end(); ++it) {
            DLCRequest* pCurrent = *it;
            if (pCurrent->eState == e_DLC_ContentState_Idle) {
#if defined(_DEBUG)
                app.DebugPrintf("RetrieveNextDLCContent - type = %d\n",
                                pCurrent->dwType);
#endif
                IPlatformStorage::EDLCStatus status =
                    PlatformStorage.GetDLCOffers(
                        PlatformProfile.GetPrimaryPad(),
                        [this](int iOfferC, std::uint32_t dwType, int pad) {
                            return dlcOffersReturned(iOfferC, dwType, pad);
                        },
                        pCurrent->dwType);
                if (status == IPlatformStorage::EDLC_Pending) {
                    pCurrent->eState = e_DLC_ContentState_Retrieving;
                } else {
                    app.DebugPrintf("RetrieveNextDLCContent - PROBLEM\n");
                    pCurrent->eState = e_DLC_ContentState_Retrieved;
                }
                return true;
            }
        }
    }

    app.DebugPrintf("[Consoles_App] Finished downloading dlc content.\n");
    return false;
}

bool DLCController::checkTMSDLCCanStop() {
    std::lock_guard<std::mutex> lock(csTMSPPDownloadQueue);
    for (auto it = m_TMSPPDownloadQueue.begin();
         it != m_TMSPPDownloadQueue.end(); ++it) {
        TMSPPRequest* pCurrent = *it;
        if (pCurrent->eState == e_TMS_ContentState_Retrieving) {
            return false;
        }
    }
    return true;
}

int DLCController::dlcOffersReturned(int iOfferC, std::uint32_t dwType,
                                     int iPad) {
    {
        std::lock_guard<std::mutex> lock(csTMSPPDownloadQueue);
        for (auto it = m_DLCDownloadQueue.begin();
             it != m_DLCDownloadQueue.end(); ++it) {
            DLCRequest* pCurrent = *it;
            if (pCurrent->dwType == static_cast<std::uint32_t>(dwType)) {
                m_iDLCOfferC = iOfferC;
                app.DebugPrintf(
                    "DLCOffersReturned - type %u, count %d - setting to "
                    "retrieved\n",
                    dwType, iOfferC);
                pCurrent->eState = e_DLC_ContentState_Retrieved;
                break;
            }
        }
    }
    return 0;
}

eDLCContentType DLCController::find_eDLCContentType(std::uint32_t dwType) {
    for (int i = 0; i < e_DLC_MAX; i++) {
        if (m_dwContentTypeA[i] == dwType) {
            return (eDLCContentType)i;
        }
    }
    return (eDLCContentType)0;
}

bool DLCController::dlcContentRetrieved(eDLCMarketplaceType eType) {
    std::lock_guard<std::mutex> lock(csDLCDownloadQueue);
    for (auto it = m_DLCDownloadQueue.begin(); it != m_DLCDownloadQueue.end();
         ++it) {
        DLCRequest* pCurrent = *it;
        if ((pCurrent->dwType == m_dwContentTypeA[eType]) &&
            (pCurrent->eState == e_DLC_ContentState_Retrieved)) {
            return true;
        }
    }
    return false;
}

void DLCController::tickDLCOffersRetrieved() {
    if (!m_bAllDLCContentRetrieved) {
        if (!retrieveNextDLCContent()) {
            app.DebugPrintf("[Consoles_App] All content retrieved.\n");
            m_bAllDLCContentRetrieved = true;
        }
    }
}

void DLCController::clearAndResetDLCDownloadQueue() {
    app.DebugPrintf("[Consoles_App] Clear and reset download queue.\n");

    int iPosition = 0;
    {
        std::lock_guard<std::mutex> lock(csTMSPPDownloadQueue);
        for (auto it = m_DLCDownloadQueue.begin();
             it != m_DLCDownloadQueue.end(); ++it) {
            DLCRequest* pCurrent = *it;
            delete pCurrent;
            iPosition++;
        }
        m_DLCDownloadQueue.clear();
        m_bAllDLCContentRetrieved = true;
    }
}

bool DLCController::retrieveNextTMSPPContent() { return false; }

void DLCController::tickTMSPPFilesRetrieved() {
    if (m_bTickTMSDLCFiles && !m_bAllTMSContentRetrieved) {
        if (retrieveNextTMSPPContent() == false) {
            m_bAllTMSContentRetrieved = true;
        }
    }
}

void DLCController::clearTMSPPFilesRetrieved() {
    int iPosition = 0;
    {
        std::lock_guard<std::mutex> lock(csTMSPPDownloadQueue);
        for (auto it = m_TMSPPDownloadQueue.begin();
             it != m_TMSPPDownloadQueue.end(); ++it) {
            TMSPPRequest* pCurrent = *it;
            delete pCurrent;
            iPosition++;
        }
        m_TMSPPDownloadQueue.clear();
        m_bAllTMSContentRetrieved = true;
    }
}

unsigned int DLCController::addTMSPPFileTypeRequest(eDLCContentType eType,
                                                    bool bPromote) {
    std::lock_guard<std::mutex> lock(csTMSPPDownloadQueue);

    if (eType == e_DLC_TexturePackData) {
        int iCount = getDLCInfoFullOffersCount();

        for (int i = 0; i < iCount; i++) {
            DLC_INFO* pDLC = getDLCInfoFullOffer(i);

            if ((pDLC->eDLCType == e_DLC_TexturePacks) ||
                (pDLC->eDLCType == e_DLC_MashupPacks)) {
                if (pDLC->wchDataFile[0] != 0) {
                    {
                        bool bPresent = app.IsFileInTPD(pDLC->iConfig);

                        if (!bPresent) {
                            bool bAlreadyInQueue = false;
                            for (auto it = m_TMSPPDownloadQueue.begin();
                                 it != m_TMSPPDownloadQueue.end(); ++it) {
                                TMSPPRequest* pCurrent = *it;
                                if (strcmp(pDLC->wchDataFile,
                                           pCurrent->wchFilename) == 0) {
                                    bAlreadyInQueue = true;
                                    break;
                                }
                            }

                            if (!bAlreadyInQueue) {
                                TMSPPRequest* pTMSPPreq = new TMSPPRequest;
                                pTMSPPreq->CallbackFunc =
                                    &DLCController::tmsPPFileReturned;
                                pTMSPPreq->lpCallbackParam = this;
                                pTMSPPreq->eStorageFacility =
                                    IPlatformStorage::eGlobalStorage_Title;
                                pTMSPPreq->eFileTypeVal =
                                    IPlatformStorage::TMS_FILETYPE_BINARY;
                                memcpy(pTMSPPreq->wchFilename,
                                       pDLC->wchDataFile,
                                       sizeof(char) * MAX_BANNERNAME_SIZE);
                                pTMSPPreq->eType = e_DLC_TexturePackData;
                                pTMSPPreq->eState = e_TMS_ContentState_Queued;
                                m_bAllTMSContentRetrieved = false;
                                m_TMSPPDownloadQueue.push_back(pTMSPPreq);
                            }
                        } else {
                            app.DebugPrintf(
                                "Texture data already present in the TPD\n");
                        }
                    }
                }
            }
        }
    } else {
        int iCount;
        iCount = getDLCInfoFullOffersCount();
        for (int i = 0; i < iCount; i++) {
            DLC_INFO* pDLC = getDLCInfoFullOffer(i);
            if (pDLC->eDLCType == eType) {
                char* cString = pDLC->wchBanner;
                {
                    bool bPresent = app.IsFileInMemoryTextures(cString);

                    if (!bPresent) {
                        bool bAlreadyInQueue = false;
                        for (auto it = m_TMSPPDownloadQueue.begin();
                             it != m_TMSPPDownloadQueue.end(); ++it) {
                            TMSPPRequest* pCurrent = *it;
                            if (strcmp(pDLC->wchBanner,
                                       pCurrent->wchFilename) == 0) {
                                bAlreadyInQueue = true;
                                break;
                            }
                        }

                        if (!bAlreadyInQueue) {
                            TMSPPRequest* pTMSPPreq = new TMSPPRequest;
                            memset(pTMSPPreq, 0, sizeof(TMSPPRequest));
                            pTMSPPreq->CallbackFunc =
                                &DLCController::tmsPPFileReturned;
                            pTMSPPreq->lpCallbackParam = this;
                            pTMSPPreq->eStorageFacility =
                                IPlatformStorage::eGlobalStorage_Title;
                            pTMSPPreq->eFileTypeVal =
                                IPlatformStorage::TMS_FILETYPE_BINARY;
                            memcpy(pTMSPPreq->wchFilename, pDLC->wchBanner,
                                   sizeof(char) * MAX_BANNERNAME_SIZE);
                            pTMSPPreq->eType = eType;
                            pTMSPPreq->eState = e_TMS_ContentState_Queued;
                            m_bAllTMSContentRetrieved = false;
                            m_TMSPPDownloadQueue.push_back(pTMSPPreq);
                            app.DebugPrintf(
                                "===m_TMSPPDownloadQueue Adding %s, q size is "
                                "%d\n",
                                pTMSPPreq->wchFilename,
                                m_TMSPPDownloadQueue.size());
                        }
                    }
                }
            }
        }
    }

    return 1;
}

int DLCController::tmsPPFileReturned(
    void* pParam, int iPad, int iUserData,
    IPlatformStorage::PTMSPP_FILEDATA pFileData, const char* szFilename) {
    DLCController* pClass = (DLCController*)pParam;

    {
        std::lock_guard<std::mutex> lock(pClass->csTMSPPDownloadQueue);
        for (auto it = pClass->m_TMSPPDownloadQueue.begin();
             it != pClass->m_TMSPPDownloadQueue.end(); ++it) {
            TMSPPRequest* pCurrent = *it;
#if defined(_WINDOWS64)
            char szFile[MAX_TMSFILENAME_SIZE];
            strncpy(szFile, pCurrent->wchFilename, MAX_TMSFILENAME_SIZE);

            if (strcmp(szFilename, szFile) == 0)
#endif
            {
                pCurrent->eState = e_TMS_ContentState_Retrieved;

                if (pFileData != nullptr) {
                    switch (pCurrent->eType) {
                        case e_DLC_TexturePackData: {
                            app.DebugPrintf("--- Got texturepack data %s\n",
                                            pCurrent->wchFilename);
                            int iConfig =
                                app.GetTPConfigVal(pCurrent->wchFilename);
                            app.AddMemoryTPDFile(iConfig, pFileData->pbData,
                                                 pFileData->size);
                        } break;
                        default:
                            app.DebugPrintf("--- Got image data - %s\n",
                                            pCurrent->wchFilename);
                            app.AddMemoryTextureFile(pCurrent->wchFilename,
                                                     pFileData->pbData,
                                                     pFileData->size);
                            break;
                    }
                } else {
                    app.DebugPrintf("TMSImageReturned failed (%s)...\n",
                                    szFilename);
                }
                break;
            }
        }
    }

    return 0;
}
