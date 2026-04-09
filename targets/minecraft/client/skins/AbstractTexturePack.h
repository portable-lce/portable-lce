#pragma once

#include <cstdint>
#include <string>

#include "TexturePack.h"

class BufferedImage;
class ColourTable;
class File;
class InputStream;

class AbstractTexturePack : public TexturePack {
private:
    const std::uint32_t id;
    const std::string name;

protected:
    File* file;
    std::string texname;
    std::string m_wsWorldName;

    std::string desc1;
    std::string desc2;

    std::uint8_t* m_iconData;
    std::uint32_t m_iconSize;

    std::uint8_t* m_comparisonData;
    std::uint32_t m_comparisonSize;

    TexturePack* fallback;

    ColourTable* m_colourTable;

protected:
    BufferedImage* iconImage;

private:
    int textureId;

protected:
    AbstractTexturePack(std::uint32_t id, File* file, const std::string& name,
                        TexturePack* fallback);

private:
    static std::string trim(std::string line);

protected:
    virtual void loadIcon();
    virtual void loadComparison();
    virtual void loadDescription();
    virtual void loadName();

public:
    virtual InputStream* getResource(const std::string& name,
                                     bool allowFallback);  // throws IOException
    // 4J Removed do to current override in TexturePack class
    // virtual InputStream *getResource(const std::string &name); //throws
    // IOException
    virtual DLCPack* getDLCPack() = 0;

protected:
    virtual InputStream* getResourceImplementation(
        const std::string& name) = 0;  // throws IOException;
public:
    virtual void unload(Textures* textures);
    virtual void load(Textures* textures);
    virtual bool hasFile(const std::string& name, bool allowFallback);
    virtual bool hasFile(const std::string& name) = 0;
    virtual std::uint32_t getId();
    virtual std::string getName();
    virtual std::string getDesc1();
    virtual std::string getDesc2();
    virtual std::string getWorldName();

    virtual std::string getAnimationString(const std::string& textureName,
                                           const std::string& path,
                                           bool allowFallback);

protected:
    virtual std::string getAnimationString(const std::string& textureName,
                                           const std::string& path);
    void loadDefaultUI();
    void loadDefaultColourTable();
    void loadDefaultHTMLColourTable();

public:
    virtual BufferedImage* getImageResource(const std::string& File,
                                            bool filenameHasExtension = false,
                                            bool bTitleUpdateTexture = false,
                                            const std::string& drive = "");
    virtual void loadColourTable();
    virtual void loadUI();
    virtual void unloadUI();
    virtual std::string getXuiRootPath();
    virtual std::uint8_t* getPackIcon(std::uint32_t& imageBytes);
    virtual std::uint8_t* getPackComparison(std::uint32_t& imageBytes);
    virtual unsigned int getDLCParentPackId();
    virtual unsigned char getDLCSubPackId();
    virtual ColourTable* getColourTable() { return m_colourTable; }
    virtual ArchiveFile* getArchiveFile() { return nullptr; }
};
