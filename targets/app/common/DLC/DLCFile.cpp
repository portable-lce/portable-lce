#include "DLCFile.h"

#include <sstream>

#include "minecraft/Minecraft_Macros.h"
#include "app/common/DLC/DLCManager.h"

DLCFile::DLCFile(DLCManager::EDLCType type, const std::string& path) {
    m_type = type;
    m_path = path;

    // store the id
    bool dlcSkin = path.substr(0, 3).compare("dlc") == 0;

    if (dlcSkin) {
        std::string skinValue = path.substr(7, path.size());
        skinValue = skinValue.substr(0, skinValue.find_first_of('.'));
        std::stringstream ss;
        ss << std::dec << skinValue.c_str();
        ss >> m_dwSkinId;
        m_dwSkinId = MAKE_SKIN_BITMASK(true, m_dwSkinId);

    } else {
        m_dwSkinId = 0;
    }
}
