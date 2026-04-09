#include "TexturePack.h"

std::string TexturePack::getPath(bool bTitleUpdateTexture /*= false*/,
                                 const char* pchBDPatchFileName /*= nullptr*/) {
    std::string wDrive;

    if (bTitleUpdateTexture) {
        // Make the content package point to to the UPDATE: drive is needed
        wDrive = "Common\\res\\TitleUpdate\\";
    } else {
        wDrive = "app/common/";
    }

    return wDrive;
}
