#pragma once

#include "platform/game/IPlatformGame.h"

// True no-op platform game-services backend. Same role as
// LinuxGame's overrides today: every method is a no-op so the platform
// abstraction is satisfied without any host integration. The composition
// root can substitute a real backend (Xbox Live, Steam, GOG, etc.) at
// link time.

class StubPlatformGame : public IPlatformGame {
public:
    void SetRichPresenceContext(int /*iPad*/, int /*contextId*/) override {}

    void CaptureSaveThumbnail() override {}
    void GetSaveThumbnail(std::uint8_t** thumbnailData,
                          unsigned int* thumbnailSize) override {
        if (thumbnailData) *thumbnailData = nullptr;
        if (thumbnailSize) *thumbnailSize = 0;
    }
    void ReleaseSaveThumbnail() override {}
    void GetScreenshot(int /*iPad*/, std::uint8_t** screenshotData,
                       unsigned int* screenshotSize) override {
        if (screenshotData) *screenshotData = nullptr;
        if (screenshotSize) *screenshotSize = 0;
    }

    void ReadBannedList(int /*iPad*/, eTMSAction /*action*/,
                        bool /*bCallback*/) override {}

    int LoadLocalTMSFile(char* /*wchTMSFile*/) override { return -1; }
    int LoadLocalTMSFile(char* /*wchTMSFile*/,
                         eFileExtensionType /*eExt*/) override {
        return -1;
    }
    void FreeLocalTMSFiles(eTMSFileType /*eType*/) override {}
    int GetLocalTMSFileIndex(char* /*wchTMSFile*/,
                             bool /*bFilenameIncludesExtension*/,
                             eFileExtensionType /*eEXT*/) override {
        return -1;
    }
};
