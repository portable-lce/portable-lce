#pragma once

#include <cstdint>
#include <string>

#include "minecraft/GameEnums.h"

class InputStream;
class Minecraft;
class ArchiveFile;
class BufferedImage;
class ColourTable;
class DLCPack;
class Textures;

class TexturePack {
public:
    TexturePack() { m_bHasAudio = false; }
    virtual ~TexturePack() {}
    virtual bool hasData() = 0;
    virtual bool hasAudio() { return m_bHasAudio; }
    virtual void setHasAudio(bool bVal) { m_bHasAudio = bVal; }
    virtual bool isLoadingData() = 0;
    virtual void loadData() {}
    virtual void unload(Textures* textures) = 0;
    virtual void load(Textures* textures) = 0;
    virtual InputStream* getResource(
        const std::string& name,
        bool allowFallback) = 0;  // throws IOException;
    // virtual InputStream *getResource(const std::string &name) = 0;// throws
    // IOException;
    virtual std::uint32_t getId() = 0;
    virtual std::string getName() = 0;
    virtual std::string getDesc1() = 0;
    virtual std::string getDesc2() = 0;
    virtual bool hasFile(const std::string& name, bool allowFallback) = 0;
    virtual bool isTerrainUpdateCompatible() = 0;

    virtual std::string getResource(
        const std::string& name)  // 4J - changed to just return a name rather
                                  // than an input stream
    {
        /* 4J - TODO
return TexturePack.class.getResourceAsStream(name);
        */
        return name;
    }
    virtual DLCPack* getDLCPack() { return nullptr; }

    // 4J Added
    virtual std::string getPath(bool bTitleUpdateTexture = false,
                                const char* pchBDPatchFilename = nullptr);
    virtual std::string getAnimationString(const std::string& textureName,
                                           const std::string& path,
                                           bool allowFallback) = 0;
    virtual BufferedImage* getImageResource(const std::string& File,
                                            bool filenameHasExtension = false,
                                            bool bTitleUpdateTexture = false,
                                            const std::string& drive = "") = 0;
    virtual void loadColourTable() = 0;
    virtual void loadUI() = 0;
    virtual void unloadUI() = 0;
    virtual std::string getXuiRootPath() = 0;
    virtual std::uint8_t* getPackIcon(std::uint32_t& imageBytes) = 0;
    virtual std::uint8_t* getPackComparison(std::uint32_t& imageBytes) = 0;
    virtual unsigned int getDLCParentPackId() = 0;
    virtual unsigned char getDLCSubPackId() = 0;
    virtual ColourTable* getColourTable() = 0;
    virtual ArchiveFile* getArchiveFile() = 0;

private:
    bool m_bHasAudio;
};
