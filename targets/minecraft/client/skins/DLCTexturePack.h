#pragma once

#include <cstdint>
#include <string>

#include "platform/PlatformTypes.h"
#include "AbstractTexturePack.h"
#include "minecraft/locale/StringTable.h"

class DLCPack;
class StringTable;
class ArchiveFile;
class TexturePack;

class DLCTexturePack : public AbstractTexturePack {
private:
    DLCPack* m_dlcInfoPack;  // Description, icon etc
    DLCPack* m_dlcDataPack;  // Actual textures
    StringTable* m_stringTable;
    bool bUILoaded;
    bool m_bLoadingData, m_bHasLoadedData;
    bool m_bUsingDefaultColourTable;
    // bool m_bHasAudio;
    ArchiveFile* m_archiveFile;

public:
    using AbstractTexturePack::getResource;

    DLCTexturePack(std::uint32_t id, DLCPack* pack, TexturePack* fallback);
    ~DLCTexturePack() {};

    virtual std::string getResource(const std::string& name);
    virtual DLCPack* getDLCPack();
    virtual std::string getDesc1() {
        return m_stringTable->getString("IDS_TP_DESCRIPTION");
    }
    virtual std::string getName() {
        return m_stringTable->getString("IDS_DISPLAY_NAME");
    }
    virtual std::string getWorldName() {
        return m_stringTable->getString("IDS_WORLD_NAME");
    }

    // Added for sound banks with MashUp packs
protected:
    //@Override
    void loadIcon();
    void loadComparison();
    void loadName();
    void loadDescription();
    InputStream* getResourceImplementation(
        const std::string& name);  // throws IOException

public:
    //@Override
    bool hasFile(const std::string& name);
    bool isTerrainUpdateCompatible();

    // 4J Added
    virtual std::string getPath(bool bTitleUpdateTexture = false,
                                 const char* pchBDPatchFilename = nullptr);
    virtual std::string getAnimationString(const std::string& textureName,
                                            const std::string& path);
    virtual BufferedImage* getImageResource(const std::string& File,
                                            bool filenameHasExtension = false,
                                            bool bTitleUpdateTexture = false,
                                            const std::string& drive = "");
    virtual void loadColourTable();
    virtual bool hasData() { return m_bHasLoadedData; }
    virtual bool isLoadingData() { return m_bLoadingData; }

private:
    static std::string getRootPath(std::uint32_t packId, bool allowOverride,
                                    bool bAddDataFolder);
    static std::string getFilePath(std::uint32_t packId, std::string filename,
                                    bool bAddDataFolder = true);

public:
    int onPackMounted(int iPad, std::uint32_t dwErr,
                      std::uint32_t dwLicenceMask);
    virtual void loadData();
    virtual void loadUI();
    virtual void unloadUI();
    virtual std::string getXuiRootPath();
    virtual ArchiveFile* getArchiveFile() { return m_archiveFile; }

    virtual unsigned int getDLCParentPackId();
    virtual DLCPack* getDLCInfoParentPack();
    virtual unsigned char getDLCSubPackId();
    XCONTENTDEVICEID GetDLCDeviceID();
};
