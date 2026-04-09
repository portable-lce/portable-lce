#include "FolderTexturePack.h"

#include "java/File.h"
#include "java/InputOutputStream/InputStream.h"
#include "minecraft/client/skins/AbstractTexturePack.h"

class TexturePack;

FolderTexturePack::FolderTexturePack(std::uint32_t id, const std::string& name,
                                     File* folder, TexturePack* fallback)
    : AbstractTexturePack(id, folder, name, fallback) {
    // 4J Stu - These calls need to be in the most derived version of the class
    loadIcon();
    loadName();
    loadDescription();

    bUILoaded = false;
}

InputStream* FolderTexturePack::getResourceImplementation(
    const std::string& name)  // throws IOException
{
    std::string wDrive = "";
    // Make the content package point to to the UPDATE: drive is needed
    wDrive = "Common\\DummyTexturePack\\res";
    InputStream* resource = InputStream::getResourceAsStream(wDrive + name);
    // InputStream *stream =
    // DefaultTexturePack::class->getResourceAsStream(name); if (stream ==
    // nullptr)
    //{
    //	throw new FileNotFoundException(name);
    // }

    // return stream;
    return resource;
}

bool FolderTexturePack::hasFile(const std::string& name) {
    File file = File(getPath() + name);
    return file.exists() && file.isFile();
    // return true;
}

bool FolderTexturePack::isTerrainUpdateCompatible() { return true; }

std::string FolderTexturePack::getPath(bool bTitleUpdateTexture /*= false*/,
                                       const char* pchBDPatchFilename) {
    std::string wDrive;
    wDrive = "Common\\" + file->getPath() + "\\";
    return wDrive;
}

void FolderTexturePack::loadUI() {}

void FolderTexturePack::unloadUI() {}