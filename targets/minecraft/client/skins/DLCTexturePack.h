#pragma once

#include <cstdint>
#include <string>

#include "AbstractTexturePack.h"
#include "minecraft/locale/StringTable.h"
#include "platform/PlatformTypes.h"

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
    ~DLCTexturePack() override = default;

    std::string getResource(const std::string& name) override;
    DLCPack* getDLCPack() override;
    std::string getDesc1() override {
        return m_stringTable->getString("IDS_TP_DESCRIPTION");
    }
    std::string getName() override {
        return m_stringTable->getString("IDS_DISPLAY_NAME");
    }
    std::string getWorldName() override {
        return m_stringTable->getString("IDS_WORLD_NAME");
    }

    // Added for sound banks with MashUp packs
protected:
    //@Override
    void loadIcon() override;
    void loadComparison() override;
    void loadName() override;
    void loadDescription() override;
    InputStream* getResourceImplementation(
        const std::string& name) override;  // throws IOException

public:
    //@Override
    bool hasFile(const std::string& name) override;
    bool isTerrainUpdateCompatible() override;

    // 4J Added
    std::string getPath(bool bTitleUpdateTexture = false,
                        const char* pchBDPatchFilename = nullptr) override;
    std::string getAnimationString(const std::string& textureName,
                                   const std::string& path) override;
    BufferedImage* getImageResource(const std::string& File,
                                    bool filenameHasExtension = false,
                                    bool bTitleUpdateTexture = false,
                                    const std::string& drive = "") override;
    void loadColourTable() override;
    bool hasData() override { return m_bHasLoadedData; }
    bool isLoadingData() override { return m_bLoadingData; }

private:
    static std::string getRootPath(std::uint32_t packId, bool allowOverride,
                                   bool bAddDataFolder);
    static std::string getFilePath(std::uint32_t packId, std::string filename,
                                   bool bAddDataFolder = true);

public:
    int onPackMounted(int iPad, std::uint32_t dwErr,
                      std::uint32_t dwLicenceMask);
    void loadData() override;
    void loadUI() override;
    void unloadUI() override;
    std::string getXuiRootPath() override;
    ArchiveFile* getArchiveFile() override { return m_archiveFile; }

    unsigned int getDLCParentPackId() override;
    virtual DLCPack* getDLCInfoParentPack();
    unsigned char getDLCSubPackId() override;
    XCONTENTDEVICEID GetDLCDeviceID();

    [[nodiscard]] bool needsPurchase() override;
};
