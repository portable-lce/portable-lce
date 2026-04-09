#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/common/App_structs.h"
#include "app/common/DLC/DLCManager.h"
#include "platform/XboxStubs.h"
#include "platform/storage/storage.h"

struct SCreditTextItemDef;

class DLCPack;

class DLCController {
public:
    DLCController();

    // Install process
    bool startInstallDLCProcess(int iPad);
    int dlcInstalledCallback(int iInstalledC, int iPad);
    void mountNextDLC(int iPad);
    int dlcMountedCallback(int iPad, std::uint32_t dwErr,
                           std::uint32_t dwLicenceMask);
    void handleDLC(DLCPack* pack);

    bool dlcInstallPending() { return m_bDLCInstallPending; }
    bool dlcInstallProcessCompleted() { return m_bDLCInstallProcessCompleted; }
    void clearDLCInstalled() { m_bDLCInstallProcessCompleted = false; }

    static int marketplaceCountsCallback(void* pParam,
                                         IPlatformStorage::DLC_TMS_DETAILS*,
                                         int iPad);

    // DLC info registration
    static int32_t registerDLCData(char*, char*, int, uint64_t, uint64_t, char*,
                                   unsigned int, int, char* pDataFile);
    bool getDLCFullOfferIDForSkinID(const std::string& FirstSkin,
                                    uint64_t* pullVal);
    bool getDLCFullOfferIDForPackID(const int iPackID, uint64_t* pullVal);
    DLC_INFO* getDLCInfoForTrialOfferID(uint64_t ullOfferID_Trial);
    DLC_INFO* getDLCInfoForFullOfferID(uint64_t ullOfferID_Full);
    DLC_INFO* getDLCInfoTrialOffer(int iIndex);
    DLC_INFO* getDLCInfoFullOffer(int iIndex);
    uint64_t getDLCInfoTexturesFullOffer(int iIndex);
    int getDLCInfoTrialOffersCount();
    int getDLCInfoFullOffersCount();
    int getDLCInfoTexturesOffersCount();

    // DLC content/offers
    unsigned int addDLCRequest(eDLCMarketplaceType eContentType,
                               bool bPromote = false);
    bool retrieveNextDLCContent();
    bool checkTMSDLCCanStop();
    int dlcOffersReturned(int iOfferC, std::uint32_t dwType, int iPad);
    std::uint32_t getDLCContentType(eDLCContentType eType) {
        return m_dwContentTypeA[eType];
    }
    eDLCContentType find_eDLCContentType(std::uint32_t dwType);
    int getDLCOffersCount() { return m_iDLCOfferC; }
    bool dlcContentRetrieved(eDLCMarketplaceType eType);
    void tickDLCOffersRetrieved();
    void clearAndResetDLCDownloadQueue();

    // TMS/TMSPP
    bool retrieveNextTMSPPContent();
    void tickTMSPPFilesRetrieved();
    void clearTMSPPFilesRetrieved();
    unsigned int addTMSPPFileTypeRequest(eDLCContentType eType,
                                         bool bPromote = false);
    static int tmsPPFileReturned(void* pParam, int iPad, int iUserData,
                                 IPlatformStorage::PTMSPP_FILEDATA pFileData,
                                 const char* szFilename);

    // Credit text
    void addCreditText(const char* lpStr);
    bool alreadySeenCreditText(const std::string& wstemp);
    unsigned int getDLCCreditsCount();
    SCreditTextItemDef* getDLCCredits(int iIndex);

    // New DLC available
    void clearNewDLCAvailable() {
        m_bNewDLCAvailable = false;
        m_bSeenNewDLCTip = true;
    }
    bool getNewDLCAvailable() { return m_bNewDLCAvailable; }
    void displayNewDLCTipAgain() { m_bSeenNewDLCTip = false; }
    bool displayNewDLCTip() {
        if (!m_bSeenNewDLCTip) {
            m_bSeenNewDLCTip = true;
            return true;
        } else
            return false;
    }

    void setTickTMSDLCFiles(bool bVal) { m_bTickTMSDLCFiles = bVal; }

    // Public data needed by other parts
    std::vector<std::string> m_vCreditText;
    std::uint8_t* m_pDLCFileBuffer;
    unsigned int m_dwDLCFileSize;

    // DLC install counters (accessed by dlcMountedCallback)
    int m_iTotalDLC;
    int m_iTotalDLCInstalled;

    // Static maps
    static std::unordered_map<PlayerUID, MOJANG_DATA*> MojangData;
    static std::unordered_map<int, uint64_t> DLCTextures_PackID;
    static std::unordered_map<uint64_t, DLC_INFO*> DLCInfo_Trial;
    static std::unordered_map<uint64_t, DLC_INFO*> DLCInfo_Full;
    static std::unordered_map<std::string, uint64_t> DLCInfo_SkinName;
    static std::uint32_t m_dwContentTypeA[e_Marketplace_MAX];

private:
    std::vector<SCreditTextItemDef*> vDLCCredits;
    std::vector<DLCRequest*> m_DLCDownloadQueue;
    std::vector<TMSPPRequest*> m_TMSPPDownloadQueue;

    int m_iDLCOfferC;
    bool m_bAllDLCContentRetrieved;
    bool m_bAllTMSContentRetrieved;
    bool m_bTickTMSDLCFiles;
    std::mutex csDLCDownloadQueue;
    std::mutex csTMSPPDownloadQueue;

    bool m_bDLCInstallProcessCompleted;
    bool m_bDLCInstallPending;
    bool m_bDefaultCapeInstallAttempted;

    bool m_bNewDLCAvailable;
    bool m_bSeenNewDLCTip;
};
