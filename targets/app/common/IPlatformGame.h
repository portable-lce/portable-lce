#pragma once

#include <cstdint>

#include "minecraft/GameEnums.h"

class IPlatformGame {
public:
    virtual ~IPlatformGame() = default;

    virtual void SetRichPresenceContext(int iPad, int contextId) = 0;

    virtual void CaptureSaveThumbnail() = 0;
    virtual void GetSaveThumbnail(std::uint8_t** thumbnailData,
                                  unsigned int* thumbnailSize) = 0;
    virtual void ReleaseSaveThumbnail() = 0;
    virtual void GetScreenshot(int iPad, std::uint8_t** screenshotData,
                               unsigned int* screenshotSize) = 0;

    virtual void ReadBannedList(int iPad, eTMSAction action = (eTMSAction)0,
                                bool bCallback = false) = 0;

    virtual int LoadLocalTMSFile(char* wchTMSFile) = 0;
    virtual int LoadLocalTMSFile(char* wchTMSFile, eFileExtensionType eExt) = 0;
    virtual void FreeLocalTMSFiles(eTMSFileType eType) = 0;
    virtual int GetLocalTMSFileIndex(char* wchTMSFile,
                                     bool bFilenameIncludesExtension,
                                     eFileExtensionType eEXT) = 0;
};
