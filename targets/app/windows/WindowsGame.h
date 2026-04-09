#pragma once

class WindowsGame : public Game {
public:
    WindowsGame();

    virtual void SetRichPresenceContext(int iPad, int contextId);

    virtual void StoreLaunchData();
    virtual void ExitGame();
    virtual void FatalLoadError();

    virtual void CaptureSaveThumbnail();
    virtual void GetSaveThumbnail(std::uint8_t** thumbnailData,
                                  unsigned int* thumbnailSize);
    virtual void ReleaseSaveThumbnail();
    virtual void GetScreenshot(int iPad, std::uint8_t** screenshotData,
                               unsigned int* screenshotSize);

    virtual int LoadLocalTMSFile(char* wchTMSFile);
    virtual int LoadLocalTMSFile(char* wchTMSFile, eFileExtensionType eExt);

    virtual void FreeLocalTMSFiles(eTMSFileType eType);
    virtual int GetLocalTMSFileIndex(
        char* wchTMSFile, bool bFilenameIncludesExtension,
        eFileExtensionType eEXT = eFileExtensionType_PNG);

    // BANNED LEVEL LIST
    virtual void ReadBannedList(int iPad, eTMSAction action = (eTMSAction)0,
                                bool bCallback = false) {}

    C4JStringTable* GetStringTable() { return nullptr; }

    // original code
    virtual void TemporaryCreateGameStart();
};

extern WindowsGame app;
