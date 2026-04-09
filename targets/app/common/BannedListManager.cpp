#include "app/common/BannedListManager.h"

#include <cstring>

#include "platform/XboxStubs.h"

BannedListManager::BannedListManager() {
    m_pBannedListFileBuffer = nullptr;
    m_dwBannedListFileSize = 0;
    std::memset(m_pszUniqueMapName, 0, 14);

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        m_bRead_BannedListA[i] = false;
        m_BanListCheck[i] = false;
        m_vBannedListA[i] = new std::vector<PBANNEDLISTDATA>;
    }
}

void BannedListManager::invalidate(int iPad) {
    if (m_bRead_BannedListA[iPad] == true) {
        m_bRead_BannedListA[iPad] = false;
        setBanListCheck(iPad, false);
        m_vBannedListA[iPad]->clear();

        if (BannedListA[iPad].pBannedList) {
            delete[] BannedListA[iPad].pBannedList;
            BannedListA[iPad].pBannedList = nullptr;
        }
    }
}

void BannedListManager::addLevel(int iPad, PlayerUID xuid, char* pszLevelName,
                                 bool bWriteToTMS) {
    // we will have retrieved the banned level list from TMS, so add this one to
    // it and write it back to TMS

    BANNEDLISTDATA* pBannedListData = new BANNEDLISTDATA;
    memset(pBannedListData, 0, sizeof(BANNEDLISTDATA));

    memcpy(&pBannedListData->xuid, &xuid, sizeof(PlayerUID));
    strcpy(pBannedListData->pszLevelName, pszLevelName);
    m_vBannedListA[iPad]->push_back(pBannedListData);

    if (bWriteToTMS) {
        const std::size_t bannedListCount = m_vBannedListA[iPad]->size();
        const unsigned int dataBytes =
            static_cast<unsigned int>(sizeof(BANNEDLISTDATA) * bannedListCount);
        PBANNEDLISTDATA pBannedList = new BANNEDLISTDATA[bannedListCount];
        int iCount = 0;
        for (auto it = m_vBannedListA[iPad]->begin();
             it != m_vBannedListA[iPad]->end(); ++it) {
            PBANNEDLISTDATA pData = *it;
            memcpy(&pBannedList[iCount++], pData, sizeof(BANNEDLISTDATA));
        }

        // 4J-PB - write to TMS++ now

        // bool
        // bRes=PlatformStorage.WriteTMSFile(iPad,IPlatformStorage::eGlobalStorage_TitleUser,"BannedList",(std::uint8_t*)pBannedList,
        // dwDataBytes);

        delete[] pBannedList;
    }
    // update telemetry too
}

bool BannedListManager::isInList(int iPad, PlayerUID xuid, char* pszLevelName) {
    for (auto it = m_vBannedListA[iPad]->begin();
         it != m_vBannedListA[iPad]->end(); ++it) {
        PBANNEDLISTDATA pData = *it;
        if (IsEqualXUID(pData->xuid, xuid) &&
            (strcmp(pData->pszLevelName, pszLevelName) == 0)) {
            return true;
        }
    }

    return false;
}

void BannedListManager::removeLevel(int iPad, PlayerUID xuid,
                                    char* pszLevelName) {
    // bool bFound=false;
    // bool bRes;

    // we will have retrieved the banned level list from TMS, so remove this one
    // from it and write it back to TMS
    for (auto it = m_vBannedListA[iPad]->begin();
         it != m_vBannedListA[iPad]->end();) {
        PBANNEDLISTDATA pBannedListData = *it;

        if (pBannedListData != nullptr) {
            if (IsEqualXUID(pBannedListData->xuid, xuid) &&
                (strcmp(pBannedListData->pszLevelName, pszLevelName) == 0)) {
                // match found, so remove this entry
                it = m_vBannedListA[iPad]->erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    const std::size_t bannedListCount = m_vBannedListA[iPad]->size();
    const unsigned int dataBytes =
        static_cast<unsigned int>(sizeof(BANNEDLISTDATA) * bannedListCount);
    if (dataBytes == 0) {
        // wipe the file
    } else {
        PBANNEDLISTDATA pBannedList =
            (BANNEDLISTDATA*)(new std::uint8_t[dataBytes]);

        for (std::size_t i = 0; i < bannedListCount; ++i) {
            PBANNEDLISTDATA pBannedListData = m_vBannedListA[iPad]->at(i);

            memcpy(&pBannedList[i], pBannedListData, sizeof(BANNEDLISTDATA));
        }
        delete[] pBannedList;
    }

    // update telemetry too
}

void BannedListManager::setUniqueMapName(char* pszUniqueMapName) {
    memcpy(m_pszUniqueMapName, pszUniqueMapName, 14);
}

char* BannedListManager::getUniqueMapName() { return m_pszUniqueMapName; }
