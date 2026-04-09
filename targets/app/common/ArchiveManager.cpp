#include "app/common/ArchiveManager.h"

#include <mutex>
#include <string>

#include "app/common/UI/All Platforms/ArchiveFile.h"
#include "app/linux/LinuxGame.h"
#include "java/File.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "platform/PlatformTypes.h"
#include "platform/fs/fs.h"

ArchiveManager::ArchiveManager()
    : m_mediaArchive(nullptr), m_dwRequiredTexturePackID(0) {}

void ArchiveManager::loadMediaArchive() {
    std::string mediapath = "";

#if _WINDOWS64
    mediapath = "Common\\Media\\MediaWindows64.arc";
#elif __linux__
    mediapath = "app/common/Media/MediaLinux.arc";
#endif

    if (!mediapath.empty()) {
#if defined(__linux__)
        std::string exeDirW = PlatformFilesystem.getBasePath().string();
        std::string candidate = exeDirW + File::pathSeparator + mediapath;
        if (File(candidate).exists()) {
            m_mediaArchive = new ArchiveFile(File(candidate));
        } else {
            m_mediaArchive = new ArchiveFile(File(mediapath));
        }
#else
        m_mediaArchive = new ArchiveFile(File(mediapath));
#endif
    }
}

int ArchiveManager::getArchiveFileSize(const std::string& filename) {
    TexturePack* tPack = nullptr;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft && pMinecraft->skins)
        tPack = pMinecraft->skins->getSelected();
    if (tPack && tPack->hasData() && tPack->getArchiveFile() &&
        tPack->getArchiveFile()->hasFile(filename)) {
        return tPack->getArchiveFile()->getFileSize(filename);
    } else
        return m_mediaArchive->getFileSize(filename);
}

bool ArchiveManager::hasArchiveFile(const std::string& filename) {
    TexturePack* tPack = nullptr;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft && pMinecraft->skins)
        tPack = pMinecraft->skins->getSelected();
    if (tPack && tPack->hasData() && tPack->getArchiveFile() &&
        tPack->getArchiveFile()->hasFile(filename))
        return true;
    else
        return m_mediaArchive->hasFile(filename);
}

std::vector<uint8_t> ArchiveManager::getArchiveFile(
    const std::string& filename) {
    TexturePack* tPack = nullptr;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft && pMinecraft->skins)
        tPack = pMinecraft->skins->getSelected();
    if (tPack && tPack->hasData() && tPack->getArchiveFile() &&
        tPack->getArchiveFile()->hasFile(filename)) {
        return tPack->getArchiveFile()->getFile(filename);
    } else
        return m_mediaArchive->getFile(filename);
}

void ArchiveManager::addMemoryTPDFile(int iConfig, std::uint8_t* pbData,
                                      unsigned int byteCount) {
    std::lock_guard<std::mutex> lock(csMemTPDLock);
    PMEMDATA pData = nullptr;
    auto it = m_MEM_TPD.find(iConfig);
    if (it == m_MEM_TPD.end()) {
        pData = new MEMDATA();
        pData->pbData = pbData;
        pData->byteCount = byteCount;
        pData->ucRefCount = 1;

        m_MEM_TPD[iConfig] = pData;
    }
}

void ArchiveManager::removeMemoryTPDFile(int iConfig) {
    std::lock_guard<std::mutex> lock(csMemTPDLock);
    PMEMDATA pData = nullptr;
    auto it = m_MEM_TPD.find(iConfig);
    if (it != m_MEM_TPD.end()) {
        pData = m_MEM_TPD[iConfig];
        delete pData;
        m_MEM_TPD.erase(iConfig);
    }
}

int ArchiveManager::getTPConfigVal(char* pwchDataFile) { return -1; }

bool ArchiveManager::isFileInTPD(int iConfig) {
    bool val = false;

    {
        std::lock_guard<std::mutex> lock(csMemTPDLock);
        auto it = m_MEM_TPD.find(iConfig);
        if (it != m_MEM_TPD.end()) val = true;
    }

    return val;
}

void ArchiveManager::getTPD(int iConfig, std::uint8_t** ppbData,
                            unsigned int* pByteCount) {
    std::lock_guard<std::mutex> lock(csMemTPDLock);
    auto it = m_MEM_TPD.find(iConfig);
    if (it != m_MEM_TPD.end()) {
        PMEMDATA pData = (*it).second;
        *ppbData = pData->pbData;
        *pByteCount = pData->byteCount;
    }
}
