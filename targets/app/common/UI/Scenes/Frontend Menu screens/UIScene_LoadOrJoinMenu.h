#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SaveList.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "java/File.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "platform/storage/storage.h"

class LevelGenerationOptions;
class File;
class FriendSessionInfo;
class UILayer;

class UIScene_LoadOrJoinMenu : public UIScene {
private:
    enum EControls {
        eControl_SavesList,
        eControl_GamesList,
    };

    enum EState {
        e_SavesIdle,
        e_SavesRepopulate,
        e_SavesRepopulateAfterMashupHide,
        e_SavesRepopulateAfterDelete,
        e_SavesRepopulateAfterTransferDownload,
    };

    enum eActions {
        eAction_None = 0,
        eAction_ViewInvites,
        eAction_JoinGame,
    };
    eActions m_eAction;

    static const int JOIN_LOAD_CREATE_BUTTON_INDEX = 0;

    SaveListDetails* m_saveDetails;
    int m_iSaveDetailsCount;

protected:
    UIControl_SaveList m_buttonListSaves;
    UIControl_SaveList m_buttonListGames;
    UIControl_Label m_labelSavesListTitle, m_labelJoinListTitle, m_labelNoGames;
    UIControl m_controlSavesTimer, m_controlJoinTimer;

private:
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonListSaves, "SavesList")
    UI_MAP_ELEMENT(m_buttonListGames, "JoinList")

    UI_MAP_ELEMENT(m_labelSavesListTitle, "SavesListTitle")
    UI_MAP_ELEMENT(m_labelJoinListTitle, "JoinListTitle")
    UI_MAP_ELEMENT(m_labelNoGames, "NoGames")

    UI_MAP_ELEMENT(m_controlSavesTimer, "SavesTimer")
    UI_MAP_ELEMENT(m_controlJoinTimer, "JoinTimer")

    UI_END_MAP_ELEMENTS_AND_NAMES()

    int m_iDefaultButtonsC;
    int m_iMashUpButtonsC;
    int m_iState;

    std::vector<FriendSessionInfo*>* m_currentSessions;
    std::vector<LevelGenerationOptions*> m_generators;
    std::vector<File*>* m_saves;

    bool m_bIgnoreInput;
    bool m_bAllLoaded;
    bool m_bRetrievingSaveThumbnails;
    bool m_bSaveThumbnailReady;
    bool m_bShowingPartyGamesOnly;
    bool m_bInParty;
    JoinMenuInitData* m_initData;
    bool m_bMultiplayerAllowed;
    int m_iTexturePacksNotInstalled;
    int m_iRequestingThumbnailId;
    SAVE_DETAILS* m_pSaveDetails;
    bool m_bSavesDisplayed;
    bool m_bExitScene;
    bool m_bCopying;
    bool m_bCopyingCancelled;
    int m_iSaveInfoC;
    int m_iSaveListIndex;
    int m_iGameListIndex;
    // int *m_iConfigA; // track the texture packs that we don't have installed
    bool m_bSaveTransferInProgress;
    bool m_bSaveTransferCancelled;
    bool m_bUpdateSaveSize;

public:
    UIScene_LoadOrJoinMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_LoadOrJoinMenu();

    virtual void updateTooltips();
    virtual void updateComponents();

    virtual void handleDestroy();
    virtual void handleLoseFocus();
    virtual void handleGainFocus(bool navBack);
    virtual void handleTimerComplete(int id);
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    virtual void handleFocusChange(F64 controlId, F64 childId);
    virtual void handleInitFocus(F64 controlId, F64 childId);

    virtual EUIScene getSceneType() { return eUIScene_LoadOrJoinMenu; }

    static void UpdateGamesListCallback(void* pParam);
    virtual void tick();

private:
    void Initialise();
    void GetSaveInfo();
    void UpdateGamesList();
    void AddDefaultButtons();
    bool DoesSavesListHaveFocus();
    bool DoesMashUpWorldHaveFocus();
    bool DoesGamesListHaveFocus();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    int loadSaveDataThumbnailReturned(std::uint8_t* pbThumbnail,
                                      unsigned int thumbnailBytes);
    static int LoadSaveCallback(void* lpParam, bool bRes);
    static int DeleteSaveDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int SaveOptionsDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int TexturePackDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    int deleteSaveDataReturned(bool bRes);
    int renameSaveDataReturned(bool bRes);
    int handleKeyboardCompleteWorldName(bool bRes);

protected:
    void handlePress(F64 controlId, F64 childId);
    void LoadLevelGen(LevelGenerationOptions* levelGen);
    void LoadSaveFromDisk(
        File* saveFile, ESavePlatform savePlatform = SAVE_FILE_PLATFORM_LOCAL);

public:
    virtual void HandleDLCMountingComplete();

private:
    void CheckAndJoinGame(int gameIndex);

#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
    enum eSaveTransferState {
        eSaveTransfer_Idle,
        eSaveTransfer_Busy,
        eSaveTransfer_GetRemoteSaveInfo,
        eSaveTransfer_GettingRemoteSaveInfo,
        eSaveTransfer_CreateDummyFile,
        eSaveTransfer_CreatingDummyFile,
        eSaveTransfer_GettingFileSize,
        eSaveTransfer_FileSizeRetrieved,
        eSaveTransfer_GetFileData,
        eSaveTransfer_GettingFileData,
        eSaveTransfer_FileDataRetrieved,
        eSaveTransfer_GetSavesInfo,
        eSaveTransfer_GettingSavesInfo,
        eSaveTransfer_LoadSaveFromDisc,
        eSaveTransfer_LoadingSaveFromDisc,
        eSaveTransfer_CreatingNewSave,
        eSaveTransfer_Converting,
        eSaveTransfer_Saving,
        eSaveTransfer_Succeeded,
        eSaveTransfer_Cancelled,
        eSaveTransfer_Error,
        eSaveTransfer_ErrorDeletingSave,
        eSaveTransfer_ErrorMesssage,
        eSaveTransfer_Finished,

    };
    eSaveTransferState m_eSaveTransferState;
    static unsigned long m_ulFileSize;
    static std::string m_wstrStageText;
    static bool m_bSaveTransferRunning;
    int m_iProgress;
    char
        m_downloadedUniqueFilename[64];  // SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE];
    bool m_saveTransferDownloadCancelled;
    void LaunchSaveTransfer();
    int createDummySaveDataCallback(bool bRes);
    int crossSaveGetSavesInfoCallback(SAVE_DETAILS* pSaveDetails, bool bRes);
    int loadCrossSaveDataCallback(bool bIsCorrupt, bool bIsOwner);
    static int CrossSaveFinishedCallback(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    int crossSaveDeleteOnErrorReturned(bool bRes);
    static int RemoteSaveNotFoundCallback(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int DownloadSonyCrossSaveThreadProc(void* lpParameter);
    static void SaveTransferReturned(void* lpParam, SonyRemoteStorage::Status s,
                                     int error_code);
    static ConsoleSaveFile* SonyCrossSaveConvert();

    static void CancelSaveTransferCallback(void* lpParam);

public:
    static bool isSaveTransferRunning() { return m_bSaveTransferRunning; }

private:
#endif

#if defined(SONY_REMOTE_STORAGE_UPLOAD)
    enum eSaveUploadState {
        eSaveUpload_Idle,
        eSaveUpload_UploadingFileData,
        eSaveUpload_FileDataUploaded,
        eSaveUpload_Cancelled,
        eSaveUpload_Error,
        esaveUpload_Finished
    };

    eSaveUploadState m_eSaveUploadState;
    bool m_saveTransferUploadCancelled;

    void LaunchSaveUpload();
    static int UploadSonyCrossSaveThreadProc(void* lpParameter);
    static void SaveUploadReturned(void* lpParam, SonyRemoteStorage::Status s,
                                   int error_code);
    static void CancelSaveUploadCallback(void* lpParam);
    static int SaveTransferDialogReturned(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
    static int CrossSaveUploadFinishedCallback(
        void* pParam, int iPad, IPlatformStorage::EMessageResult result);
#endif
};