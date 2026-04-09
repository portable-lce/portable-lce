#pragma once

#include "app/common/DLC/DLCPack.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"

class DLCPack;

class IUIScene_PauseMenu {
protected:
    DLCPack* m_pDLCPack;

public:
    static int ExitGameDialogReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result);
    static int ExitGameSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int ExitGameAndSaveReturned(void* pParam, int iPad,
                                       IPlatformStorage::EMessageResult result);
    static int ExitGameDeclineSaveReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int WarningTrialTexturePackReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int SaveGameDialogReturned(void* pParam, int iPad,
                                      IPlatformStorage::EMessageResult result);
    static int EnableAutosaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int DisableAutosaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);

    static int SaveWorldThreadProc(void* lpParameter);
    static int ExitWorldThreadProc(void* lpParameter);
    static void _ExitWorld(void* lpParameter);  // Call only from a thread

protected:
    virtual void ShowScene(bool show) = 0;
    virtual void SetIgnoreInput(bool ignoreInput) = 0;
};
