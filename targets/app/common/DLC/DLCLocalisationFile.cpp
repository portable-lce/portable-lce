#include "DLCLocalisationFile.h"

#include "DLCManager.h"
#include "app/common/DLC/DLCFile.h"
#include "app/common/Game.h"
#include "minecraft/locale/StringTable.h"

DLCLocalisationFile::DLCLocalisationFile(const std::string& path)
    : DLCFile(DLCManager::e_DLCType_LocalisationData, path) {
    m_strings = nullptr;
}

void DLCLocalisationFile::addData(std::uint8_t* pbData,
                                  std::uint32_t dataBytes) {
    m_strings = new StringTable(
        std::span<const std::uint8_t>(pbData, dataBytes), app.getLocale());
}
