#include "minecraft/util/Log.h"
#include "platform/stubs.h"
#include "AbstractTexturePack.h"


#include <cinttypes>
#include <wchar.h>

#include <vector>

#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/IGameServices.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/client/BufferedImage.h"
#include "util/StringHelpers.h"
#include "java/File.h"
#include "java/InputOutputStream/BufferedReader.h"
#include "java/InputOutputStream/FileInputStream.h"
#include "java/InputOutputStream/InputStream.h"
#include "java/InputOutputStream/InputStreamReader.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/skins/TexturePack.h"

AbstractTexturePack::AbstractTexturePack(std::uint32_t id, File* file,
                                         const std::string& name,
                                         TexturePack* fallback)
    : id(id), name(name) {
    // 4J init
    textureId = -1;
    m_colourTable = nullptr;

    this->file = file;
    this->fallback = fallback;

    m_iconData = nullptr;
    m_iconSize = 0;

    m_comparisonData = nullptr;
    m_comparisonSize = 0;

    // 4J Stu - These calls need to be in the most derived version of the class
    // loadIcon();
    // loadDescription();
}

std::string AbstractTexturePack::trim(std::string line) {
    if (!line.empty() && line.length() > 34) {
        line = line.substr(0, 34);
    }
    return line;
}

void AbstractTexturePack::loadIcon() {}

void AbstractTexturePack::loadComparison() {}

void AbstractTexturePack::loadDescription() {
    // 4J Unused currently
}

void AbstractTexturePack::loadName() {}

InputStream* AbstractTexturePack::getResource(
    const std::string& name, bool allowFallback)  // throws IOException
{
    Log::info("texture - %s\n", name.c_str());
    InputStream* is = getResourceImplementation(name);
    if (is == nullptr && fallback != nullptr && allowFallback) {
        is = fallback->getResource(name, true);
    }

    return is;
}

// 4J Currently removed due to override in TexturePack class
// InputStream *AbstractTexturePack::getResource(const std::string &name)
// //throws IOException
//{
//	return getResource(name, true);
//}

void AbstractTexturePack::unload(Textures* textures) {
    if (iconImage != nullptr && textureId != -1) {
        textures->releaseTexture(textureId);
    }
}

void AbstractTexturePack::load(Textures* textures) {
    if (iconImage != nullptr) {
        if (textureId == -1) {
            textureId = textures->getTexture(iconImage);
        }
        glBindTexture(GL_TEXTURE_2D, textureId);
        textures->clearLastBoundId();
    } else {
        // 4J Stu - Don't do this
        // textures->bindTexture("/gui/unknown_pack.png");
    }
}

bool AbstractTexturePack::hasFile(const std::string& name,
                                  bool allowFallback) {
    bool hasFile = this->hasFile(name);

    return !hasFile && (allowFallback && fallback != nullptr)
               ? fallback->hasFile(name, allowFallback)
               : hasFile;
}

std::uint32_t AbstractTexturePack::getId() { return id; }

std::string AbstractTexturePack::getName() { return texname; }

std::string AbstractTexturePack::getWorldName() { return m_wsWorldName; }

std::string AbstractTexturePack::getDesc1() { return desc1; }

std::string AbstractTexturePack::getDesc2() { return desc2; }

std::string AbstractTexturePack::getAnimationString(
    const std::string& textureName, const std::string& path,
    bool allowFallback) {
    return getAnimationString(textureName, path);
}

std::string AbstractTexturePack::getAnimationString(
    const std::string& textureName, const std::string& path) {
    std::string animationDefinitionFile = textureName + ".txt";

    bool requiresFallback = !hasFile("\\" + textureName + ".png", false);

    std::string result = "";

    InputStream* fileStream =
        getResource("\\" + path + animationDefinitionFile, requiresFallback);

    if (fileStream) {
        // Minecraft::getInstance()->getLogger().info("Found animation info for:
        // " + animationDefinitionFile);
#if !defined(_CONTENT_PACKAGE)
        Log::info("Found animation info for: %s\n",
                        animationDefinitionFile.c_str());
#endif
        InputStreamReader isr(fileStream);
        BufferedReader br(&isr);

        std::string line = br.readLine();
        while (!line.empty()) {
            line = trimString(line);
            if (line.length() > 0) {
                result.append(",");
                result.append(line);
            }
            line = br.readLine();
        }
        delete fileStream;
    }

    return result;
}

BufferedImage* AbstractTexturePack::getImageResource(
    const std::string& File, bool filenameHasExtension /*= false*/,
    bool bTitleUpdateTexture /*=false*/, const std::string& drive /*=""*/) {
    std::string pchTexture = File;
    std::string pchDrive = drive;
    Log::info("AbstractTexturePack::getImageResource - %s, drive is %s\n",
                    pchTexture.c_str(), pchDrive.c_str());

    return new BufferedImage(TexturePack::getResource("/" + File),
                             filenameHasExtension, bTitleUpdateTexture, drive);
}

void AbstractTexturePack::loadDefaultUI() { ui.ReloadSkin(); }

void AbstractTexturePack::loadColourTable() {
    loadDefaultColourTable();
    loadDefaultHTMLColourTable();
}

void AbstractTexturePack::loadDefaultColourTable() {
    // Load the file
    File coloursFile(
        AbstractTexturePack::getPath(true).append("res/colours.col"));

    if (coloursFile.exists()) {
        uint32_t dataLength = coloursFile.length();
        std::vector<uint8_t> data(dataLength);

        FileInputStream fis(coloursFile);
        fis.read(data, 0, dataLength);
        fis.close();
        if (m_colourTable != nullptr) delete m_colourTable;
        m_colourTable = new ColourTable(data.data(), dataLength);

    } else {
        Log::info("Failed to load the default colours table\n");
        gameServices().fatalLoadError();
    }
}

void AbstractTexturePack::loadDefaultHTMLColourTable() {
    if (gameServices().hasArchiveFile("HTMLColours.col")) {
        std::vector<uint8_t> textColours =
            gameServices().getArchiveFile("HTMLColours.col");
        m_colourTable->loadColoursFromData(textColours.data(),
                                           textColours.size());
    }
}

void AbstractTexturePack::loadUI() { loadColourTable(); }

void AbstractTexturePack::unloadUI() {
    // Do nothing
}

std::string AbstractTexturePack::getXuiRootPath() {
    // const uintptr_t c_ModuleHandle = (uintptr_t)GetModuleHandle(nullptr);
    const uintptr_t c_ModuleHandle = 0; // 4jcraft changed

    // Load new skin
    constexpr int LOCATOR_SIZE =
        256;  // Use this to allocate space to hold a ResourceLocator string
    char szResourceLocator[LOCATOR_SIZE];

    snprintf(szResourceLocator, LOCATOR_SIZE, "section://%" PRIxPTR ",%s#%s",
             c_ModuleHandle, "media", "media/");
    return szResourceLocator;
}

std::uint8_t* AbstractTexturePack::getPackIcon(std::uint32_t& imageBytes) {
    if (m_iconSize == 0 || m_iconData == nullptr) loadIcon();
    imageBytes = m_iconSize;
    return m_iconData;
}

std::uint8_t* AbstractTexturePack::getPackComparison(
    std::uint32_t& imageBytes) {
    if (m_comparisonSize == 0 || m_comparisonData == nullptr) loadComparison();

    imageBytes = m_comparisonSize;
    return m_comparisonData;
}

unsigned int AbstractTexturePack::getDLCParentPackId() { return 0; }

unsigned char AbstractTexturePack::getDLCSubPackId() { return 0; }
