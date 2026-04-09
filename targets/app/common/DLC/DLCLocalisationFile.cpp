#include "DLCLocalisationFile.h"

#include "DLCManager.h"
#include "app/common/DLC/DLCFile.h"
#include "app/linux/LinuxGame.h"
#include "minecraft/locale/StringTable.h"

DLCLocalisationFile::DLCLocalisationFile(const std::string& path)
    : DLCFile(DLCManager::e_DLCType_LocalisationData, path) {
    m_strings = nullptr;
}

void DLCLocalisationFile::addData(std::uint8_t* pbData,
                                  std::uint32_t dataBytes) {
    std::vector<std::string> locales;
    app.getLocale(locales);
    m_strings = new StringTable(pbData, dataBytes, locales);
}
