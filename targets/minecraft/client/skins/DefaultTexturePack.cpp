#include "DefaultTexturePack.h"

#include <cstdint>
#include <vector>

#include "java/InputOutputStream/InputStream.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/skins/AbstractTexturePack.h"

DefaultTexturePack::DefaultTexturePack()
    : AbstractTexturePack(0, nullptr, "Minecraft", nullptr) {
    // 4J Stu - These calls need to be in the most derived version of the class
    loadIcon();
    loadName();  // 4J-PB - added so the PS3 can have localised texture names'
    loadDescription();
    loadColourTable();
}

void DefaultTexturePack::loadIcon() {
    if (gameServices().hasArchiveFile("Graphics\\TexturePackIcon.png")) {
        std::vector<uint8_t> ba =
            gameServices().getArchiveFile("Graphics\\TexturePackIcon.png");
        m_iconData = ba.data();
        m_iconSize = static_cast<std::uint32_t>(ba.size());
    }
}

void DefaultTexturePack::loadDescription() {
    desc1 = "LOCALISE ME: The default look of Minecraft";
}
void DefaultTexturePack::loadName() { texname = "Minecraft"; }

bool DefaultTexturePack::hasFile(const std::string& name) {
    //	return DefaultTexturePack::class->getResourceAsStream(name) != null;
    return true;
}

bool DefaultTexturePack::isTerrainUpdateCompatible() { return true; }

InputStream* DefaultTexturePack::getResourceImplementation(
    const std::string& name)  // throws FileNotFoundException
{
    std::string wDrive = "";
    // Make the content package point to to the UPDATE: drive is needed
    wDrive = "Common\\res\\TitleUpdate\\res";

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

void DefaultTexturePack::loadUI() {
    loadDefaultUI();

    AbstractTexturePack::loadUI();
}

void DefaultTexturePack::unloadUI() { AbstractTexturePack::unloadUI(); }
