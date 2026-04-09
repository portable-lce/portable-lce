#pragma once

#include <cstdint>
#include <string>

#include "AbstractTexturePack.h"

class File;
class TexturePack;

class FolderTexturePack : public AbstractTexturePack {
private:
    bool bUILoaded;

public:
    FolderTexturePack(std::uint32_t id, const std::string& name, File* folder,
                      TexturePack* fallback);

protected:
    //@Override
    InputStream* getResourceImplementation(
        const std::string& name);  // throws IOException

public:
    //@Override
    bool hasFile(const std::string& name);
    bool isTerrainUpdateCompatible();

    // 4J Added
    virtual std::string getPath(bool bTitleUpdateTexture = false,
                                const char* pchBDPatchFilename = nullptr);
    virtual void loadUI();
    virtual void unloadUI();
};