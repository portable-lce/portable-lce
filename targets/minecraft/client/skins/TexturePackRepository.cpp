#include "TexturePackRepository.h"

#include <wchar.h>

#include <algorithm>
#include <utility>

#include "DLCTexturePack.h"
#include "DefaultTexturePack.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/linux/Linux_UIController.h"
#include "java/File.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Minimap.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/util/Log.h"
#include "platform/input/input.h"

TexturePack* TexturePackRepository::DEFAULT_TEXTURE_PACK = nullptr;

TexturePackRepository::TexturePackRepository(File workingDirectory,
                                             Minecraft* minecraft) {
    if (!DEFAULT_TEXTURE_PACK) DEFAULT_TEXTURE_PACK = new DefaultTexturePack();

    // 4J - added
    usingWeb = false;
    selected = nullptr;
    texturePacks = new std::vector<TexturePack*>;

    this->minecraft = minecraft;

    texturePacks->push_back(DEFAULT_TEXTURE_PACK);
    cacheById[DEFAULT_TEXTURE_PACK->getId()] = DEFAULT_TEXTURE_PACK;
    selected = DEFAULT_TEXTURE_PACK;

    DEFAULT_TEXTURE_PACK->loadColourTable();

    m_dummyTexturePack = nullptr;
    m_dummyDLCTexturePack = nullptr;
    lastSelected = nullptr;

    updateList();
}

void TexturePackRepository::addDebugPacks() {}

void TexturePackRepository::createWorkingDirecoryUnlessExists() {
    // 4J Unused
}

bool TexturePackRepository::selectSkin(TexturePack* skin) {
    if (skin == selected) return false;

    lastSelected = selected;
    usingWeb = false;
    selected = skin;
    // minecraft->options->skin = skin->getName();
    // minecraft->options->save();
    return true;
}

void TexturePackRepository::selectWebSkin(const std::string& url) {
    Log::info("TexturePackRepository::selectWebSkin is not implemented\n");
}

void TexturePackRepository::downloadWebSkin(const std::string& url, File file) {
    Log::info("TexturePackRepository::selectWebSkin is not implemented\n");
}

bool TexturePackRepository::isUsingWebSkin() { return usingWeb; }

void TexturePackRepository::resetWebSkin() {
    usingWeb = false;
    updateList();
    minecraft->delayTextureReload();
}

void TexturePackRepository::updateList() {
    // 4J Stu - We don't ever want to completely refresh the lists, we keep them
    // up-to-date as we go
}

std::string TexturePackRepository::getIdOrNull(File file) {
    Log::info("TexturePackRepository::getIdOrNull is not implemented\n");
    return "";
}

std::vector<File> TexturePackRepository::getWorkDirContents() {
    Log::info("TexturePackRepository::getWorkDirContents is not implemented\n");
    return std::vector<File>();
}

std::vector<TexturePack*>* TexturePackRepository::getAll() {
    // 4J - note that original constucted a copy of texturePacks here
    return texturePacks;
}

TexturePack* TexturePackRepository::getSelected() {
    if (selected->hasData())
        return selected;
    else
        return DEFAULT_TEXTURE_PACK;
}

bool TexturePackRepository::shouldPromptForWebSkin() {
    Log::info(
        "TexturePackRepository::shouldPromptForWebSkin is not implemented\n");
    return false;
}

bool TexturePackRepository::canUseWebSkin() {
    Log::info("TexturePackRepository::canUseWebSkin is not implemented\n");
    return false;
}

std::vector<std::pair<std::uint32_t, std::string> >*
TexturePackRepository::getTexturePackIdNames() {
    std::vector<std::pair<std::uint32_t, std::string> >* packList =
        new std::vector<std::pair<std::uint32_t, std::string> >();

    for (auto it = texturePacks->begin(); it != texturePacks->end(); ++it) {
        TexturePack* pack = *it;
        packList->push_back(std::pair<std::uint32_t, std::string>(
            pack->getId(), pack->getName()));
    }
    return packList;
}

bool TexturePackRepository::selectTexturePackById(std::uint32_t id) {
    bool bDidSelect = false;

    // 4J-PB - add in a store of the texture pack required, so that join from
    // invite games
    //  (where they don't have the texture pack) can check this when the texture
    //  pack is installed
    gameServices().setRequiredTexturePackID(id);

    auto it = cacheById.find(id);
    if (it != cacheById.end()) {
        TexturePack* newPack = it->second;
        if (newPack != selected) {
            selectSkin(newPack);

            if (newPack->hasData()) {
                gameServices().setAction(PlatformInput.GetPrimaryPad(),
                                         eAppAction_ReloadTexturePack);
            } else {
                newPack->loadData();
            }
            // Minecraft *pMinecraft = Minecraft::GetInstance();
            // pMinecraft->textures->reloadAll();
        } else {
            Log::info("TexturePack with id %d is already selected\n", id);
        }
        bDidSelect = true;
    } else {
        Log::info("Failed to select texture pack %d as it is not in the list\n",
                  id);
        // Fail safely
        if (selectSkin(DEFAULT_TEXTURE_PACK)) {
            gameServices().setAction(PlatformInput.GetPrimaryPad(),
                                     eAppAction_ReloadTexturePack);
        }
    }
    return bDidSelect;
}

TexturePack* TexturePackRepository::getTexturePackById(std::uint32_t id) {
    auto it = cacheById.find(id);
    if (it != cacheById.end()) {
        return it->second;
    }

    return nullptr;
}

TexturePack* TexturePackRepository::addTexturePackFromDLC(DLCPack* dlcPack,
                                                          std::uint32_t id) {
    TexturePack* newPack = nullptr;
    // 4J-PB - The City texture pack went out with a child id for the texture
    // pack of 1 instead of zero we need to mask off the child id here to deal
    // with this
    const std::uint32_t parentId =
        id & 0xFFFFFFu;  // child id is <<24 and Or'd with parent

    if (dlcPack != nullptr) {
        newPack = new DLCTexturePack(parentId, dlcPack, DEFAULT_TEXTURE_PACK);
        texturePacks->push_back(newPack);
        cacheById[parentId] = newPack;

#if !defined(_CONTENT_PACKAGE)
        if (dlcPack->hasPurchasedFile(DLCManager::e_DLCType_TexturePack, "")) {
            printf("Added new FULL DLCTexturePack: %s - id=%u\n",
                   dlcPack->getName().c_str(), parentId);
        } else {
            printf("Added new TRIAL DLCTexturePack: %s - id=%u\n",
                   dlcPack->getName().c_str(), parentId);
        }
#endif
    }
    return newPack;
}

void TexturePackRepository::clearInvalidTexturePacks() {
    for (auto it = m_texturePacksToDelete.begin();
         it != m_texturePacksToDelete.end(); ++it) {
        delete *it;
    }
}

void TexturePackRepository::removeTexturePackById(std::uint32_t id) {
    auto it = cacheById.find(id);
    if (it != cacheById.end()) {
        TexturePack* oldPack = it->second;

        auto it2 = find(texturePacks->begin(), texturePacks->end(), oldPack);
        if (it2 != texturePacks->end()) {
            texturePacks->erase(it2);
            if (lastSelected == oldPack) {
                lastSelected = nullptr;
            }
        }
        m_texturePacksToDelete.push_back(oldPack);
    }
}

void TexturePackRepository::updateUI() {
    if (lastSelected != nullptr && lastSelected != selected) {
        lastSelected->unloadUI();
        selected->loadUI();
        Minimap::reloadColours();
        ui.StartReloadSkinThread();
        lastSelected = nullptr;
    }
}

bool TexturePackRepository::needsUIUpdate() {
    return lastSelected != nullptr && lastSelected != selected;
}

unsigned int TexturePackRepository::getTexturePackCount() {
    return texturePacks->size();
}

TexturePack* TexturePackRepository::getTexturePackByIndex(unsigned int index) {
    TexturePack* pack = nullptr;
    if (index < texturePacks->size()) {
        pack = texturePacks->at(index);
    }
    return pack;
}

unsigned int TexturePackRepository::getTexturePackIndex(std::uint32_t id) {
    int currentIndex = 0;
    for (auto it = texturePacks->begin(); it != texturePacks->end(); ++it) {
        TexturePack* pack = *it;
        if (pack->getId() == id) break;
        ++currentIndex;
    }
    if (currentIndex >= texturePacks->size()) currentIndex = 0;
    return currentIndex;
}
