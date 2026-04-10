#include "DLCColourTableFile.h"

#include "DLCManager.h"
#include "app/common/DLC/DLCFile.h"
#include "app/common/Game.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"

DLCColourTableFile::DLCColourTableFile(const std::string& path)
    : DLCFile(DLCManager::e_DLCType_ColourTable, path) {
    m_colourTable = nullptr;
}

DLCColourTableFile::~DLCColourTableFile() {
    if (m_colourTable != nullptr) {
        app.DebugPrintf("Deleting DLCColourTableFile data\n");
        delete m_colourTable;
    }
}

void DLCColourTableFile::addData(std::uint8_t* pbData,
                                 std::uint32_t dataBytes) {
    ColourTable* defaultColourTable =
        Minecraft::GetInstance()->skins->getDefault()->getColourTable();
    m_colourTable = new ColourTable(defaultColourTable, pbData, dataBytes);
}
