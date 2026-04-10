#include "UIScene_LoadOrJoinMenu.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <compare>

#include "app/common/DLC/DLCManager.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SaveList.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "java/File.h"
#include "java/InputOutputStream/FileInputStream.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "platform/network/network.h"
#include "minecraft/server/MinecraftServer.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "minecraft/world/level/LevelSettings.h"
#include "platform/NetTypes.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "strings.h"

#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
unsigned long UIScene_LoadOrJoinMenu::m_ulFileSize = 0L;
std::string UIScene_LoadOrJoinMenu::m_wstrStageText = "";
bool UIScene_LoadOrJoinMenu::m_bSaveTransferRunning = false;
#endif

#define JOIN_LOAD_ONLINE_TIMER_ID 0
#define JOIN_LOAD_ONLINE_TIMER_TIME 100

int UIScene_LoadOrJoinMenu::loadSaveDataThumbnailReturned(
    std::uint8_t* pbThumbnail, unsigned int dwThumbnailBytes) {
    app.DebugPrintf("Received data for save thumbnail\n");

    if (pbThumbnail && dwThumbnailBytes) {
        m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData =
            new std::uint8_t[dwThumbnailBytes];
        memcpy(m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData,
               pbThumbnail, dwThumbnailBytes);
        m_saveDetails[m_iRequestingThumbnailId].dwThumbnailSize =
            dwThumbnailBytes;
    } else {
        m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData = nullptr;
        m_saveDetails[m_iRequestingThumbnailId].dwThumbnailSize = 0;
        app.DebugPrintf("Save thumbnail data is nullptr, or has size 0\n");
    }
    m_bSaveThumbnailReady = true;

    return 0;
}

int UIScene_LoadOrJoinMenu::LoadSaveCallback(void* lpParam, bool bRes) {
    // UIScene_LoadOrJoinMenu *pClass= (UIScene_LoadOrJoinMenu *)lpParam;
    //  Get the save data now
    if (bRes) {
        app.DebugPrintf("Loaded save OK\n");
    }
    return 0;
}

UIScene_LoadOrJoinMenu::UIScene_LoadOrJoinMenu(int iPad, void* initData,
                                               UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();
    app.SetLiveLinkRequired(true);

    m_iRequestingThumbnailId = 0;
    m_iSaveInfoC = 0;
    m_bIgnoreInput = false;
    m_bShowingPartyGamesOnly = false;
    m_bInParty = false;
    m_currentSessions = nullptr;
    m_iState = e_SavesIdle;
    // m_bRetrievingSaveInfo=false;

    m_buttonListSaves.init(eControl_SavesList);
    m_buttonListGames.init(eControl_GamesList);

    m_labelSavesListTitle.init(IDS_START_GAME);
    m_labelJoinListTitle.init(IDS_JOIN_GAME);
    m_labelNoGames.init(IDS_NO_GAMES_FOUND);
    m_labelNoGames.setVisible(false);
    m_controlSavesTimer.setVisible(true);
    m_controlJoinTimer.setVisible(true);

    m_bUpdateSaveSize = false;

    m_bAllLoaded = false;
    m_bRetrievingSaveThumbnails = false;
    m_bSaveThumbnailReady = false;
    m_bExitScene = false;
    m_pSaveDetails = nullptr;
    m_bSavesDisplayed = false;
    m_saveDetails = nullptr;
    m_iSaveDetailsCount = 0;
    m_iTexturePacksNotInstalled = 0;
    m_bCopying = false;
    m_bCopyingCancelled = false;

    m_bSaveTransferCancelled = false;
    m_bSaveTransferInProgress = false;
    m_eAction = eAction_None;

    m_bMultiplayerAllowed = PlatformProfile.IsSignedInLive(m_iPad) &&
                            PlatformProfile.AllowedToPlayMultiplayer(m_iPad);

    int iLB = -1;

    // block input if we're waiting for DLC to install, and wipe the saves list.
    // The end of dlc mounting custom message will fill the list again
    if (app.StartInstallDLCProcess(m_iPad) == true || app.DLCInstallPending()) {
        // if we're waiting for DLC to mount, don't fill the save list. The
        // custom message on end of dlc mounting will do that
        m_bIgnoreInput = true;
    } else {
        Initialise();
    }

    UpdateGamesList();

    g_NetworkManager.SetSessionsUpdatedCallback(
        [this]() { UpdateGamesList(); });

    m_initData = new JoinMenuInitData();

    // 4J Stu - Fix for #12530 -TCR 001 BAS Game Stability: Title will crash if
    // the player disconnects while starting a new world and then opts to play
    // the tutorial once they have been returned to the Main Menu.
    MinecraftServer::resetFlags();

    // If we're not ignoring input, then we aren't still waiting for the DLC to
    // mount, and can now check for corrupt dlc. Otherwise this will happen when
    // the dlc has finished mounting.
    if (!m_bIgnoreInput) {
        app.m_dlcManager.checkForCorruptDLCAndAlert();
    }

    // 4J-PB - Only Xbox will not have trial DLC patched into the game

#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
    m_eSaveTransferState = eSaveTransfer_Idle;
#endif
}

UIScene_LoadOrJoinMenu::~UIScene_LoadOrJoinMenu() {
    g_NetworkManager.SetSessionsUpdatedCallback(nullptr);
    app.SetLiveLinkRequired(false);

    if (m_currentSessions) {
        for (auto it = m_currentSessions->begin();
             it < m_currentSessions->end(); ++it) {
            delete (*it);
        }
    }

#if TO_BE_IMPLEMENTED
    // Reset the background downloading, in case we changed it by attempting to
    // download a texture pack
    XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_AUTO);
#endif

    if (m_saveDetails) {
        for (int i = 0; i < m_iSaveDetailsCount; ++i) {
            delete m_saveDetails[i].pbThumbnailData;
        }
        delete[] m_saveDetails;
    }
}

void UIScene_LoadOrJoinMenu::updateTooltips() {
    // update the tooltips
    // if the saves list has focus, then we should show the Delete Save tooltip
    // if the games list has focus, then we should the the View Gamercard
    // tooltip
    int iRB = -1;
    int iY = -1;
    int iLB = -1;
    int iX = -1;
    if (DoesGamesListHaveFocus() && m_buttonListGames.getItemCount() > 0) {
        iY = IDS_TOOLTIPS_VIEW_GAMERCARD;
    } else if (DoesSavesListHaveFocus()) {
        if ((m_iDefaultButtonsC > 0) &&
            (m_iSaveListIndex >= m_iDefaultButtonsC)) {
            if (PlatformStorage.GetSaveDisabled()) {
                iRB = IDS_TOOLTIPS_DELETESAVE;
            } else {
                if (PlatformStorage.EnoughSpaceForAMinSaveGame()) {
                    iRB = IDS_TOOLTIPS_SAVEOPTIONS;
                } else {
                    iRB = IDS_TOOLTIPS_DELETESAVE;
                }
            }
        }
    } else if (DoesMashUpWorldHaveFocus()) {
        // If it's a mash-up pack world, give the Hide option
        iRB = IDS_TOOLTIPS_HIDE;
    }

    if (m_bInParty) {
        if (m_bShowingPartyGamesOnly)
            iLB = IDS_TOOLTIPS_ALL_GAMES;
        else
            iLB = IDS_TOOLTIPS_PARTY_GAMES;
    }

    if (PlatformStorage.GetSaveDisabled()) {
    } else {
#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
        // Is there a save from PS3 or PSVita available?
        // Sony asked that this be displayed at all times so users are aware of
        // the functionality. We'll display some text when there's no save
        // available
        // if(app.getRemoteStorage()->saveIsAvailable())
        {
            bool bSignedInLive = PlatformProfile.IsSignedInLive(m_iPad);
            if (bSignedInLive) {
                iX = IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD;
            }
        }
#else
        iX = IDS_TOOLTIPS_CHANGEDEVICE;
#endif
    }

    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT,
                   IDS_TOOLTIPS_BACK, iX, iY, -1, -1, iLB, iRB);
}

//
void UIScene_LoadOrJoinMenu::Initialise() {
    m_iSaveListIndex = 0;
    m_iGameListIndex = 0;

    m_iDefaultButtonsC = 0;
    m_iMashUpButtonsC = 0;

    if (PlatformStorage.GetSaveDisabled()) {
#if TO_BE_IMPLEMENTED
        if (PlatformStorage.GetSaveDeviceSelected(m_iPad))
#endif
        {
            // saving is disabled, but we should still be able to load from a
            // selected save device

            GetSaveInfo();
        }
#if TO_BE_IMPLEMENTED
        else {
            AddDefaultButtons();
            m_controlSavesTimer.setVisible(false);
        }
#endif
    } else {
        // 4J-PB - we need to check that there is enough space left to create a
        // copy of the save (for a rename)
        bool bCanRename = PlatformStorage.EnoughSpaceForAMinSaveGame();

        GetSaveInfo();
    }

    m_bIgnoreInput = false;
    app.m_dlcManager.checkForCorruptDLCAndAlert();
}

void UIScene_LoadOrJoinMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
}

void UIScene_LoadOrJoinMenu::handleDestroy() {
    // shut down the keyboard if it is displayed
}

void UIScene_LoadOrJoinMenu::handleGainFocus(bool navBack) {
    UIScene::handleGainFocus(navBack);

    updateTooltips();

    // Add load online timer
    addTimer(JOIN_LOAD_ONLINE_TIMER_ID, JOIN_LOAD_ONLINE_TIMER_TIME);

    if (navBack) {
        app.SetLiveLinkRequired(true);

        m_bMultiplayerAllowed =
            PlatformProfile.IsSignedInLive(m_iPad) &&
            PlatformProfile.AllowedToPlayMultiplayer(m_iPad);

        // re-enable button presses
        m_bIgnoreInput = false;

        // block input if we're waiting for DLC to install, and wipe the saves
        // list. The end of dlc mounting custom message will fill the list again
        if (app.StartInstallDLCProcess(m_iPad) == false) {
            // not doing a mount, so re-enable input
            m_bIgnoreInput = false;
        } else {
            m_bIgnoreInput = true;
            m_buttonListSaves.clearList();
            m_controlSavesTimer.setVisible(true);
        }

        if (m_bMultiplayerAllowed) {
#if TO_BE_IMPLEMENTED
            HXUICLASS hClassFullscreenProgress =
                XuiFindClass("CScene_FullscreenProgress");
            HXUICLASS hClassConnectingProgress =
                XuiFindClass("CScene_ConnectingProgress");

            // If we are navigating back from a full screen progress scene, then
            // that means a connection attempt failed
            if (XuiIsInstanceOf(hSceneFrom, hClassFullscreenProgress) ||
                XuiIsInstanceOf(hSceneFrom, hClassConnectingProgress)) {
                UpdateGamesList();
            }
#endif
        } else {
            m_buttonListGames.clearList();
            m_controlJoinTimer.setVisible(true);
            m_labelNoGames.setVisible(false);
#if TO_BE_IMPLEMENTED
            m_SavesList.InitFocus(m_iPad);
#endif
        }

        // are we back here because of a delete of a corrupt save?

        if (app.GetCorruptSaveDeleted()) {
            // wipe the list and repopulate it
            m_iState = e_SavesRepopulateAfterDelete;
            app.SetCorruptSaveDeleted(false);
        }
    }
}

void UIScene_LoadOrJoinMenu::handleLoseFocus() {
    // Kill load online timer
    killTimer(JOIN_LOAD_ONLINE_TIMER_ID);
}

std::string UIScene_LoadOrJoinMenu::getMoviePath() { return "LoadOrJoinMenu"; }

void UIScene_LoadOrJoinMenu::tick() {
    UIScene::tick();

#if defined(_WINDOWS64)
    if (m_bExitScene)  // navigate forward or back
    {
        if (!m_bRetrievingSaveThumbnails) {
            // need to wait for any callback retrieving thumbnail to complete
            navigateBack();
        }
    }
    // Stop loading thumbnails if we navigate forwards
    if (hasFocus(m_iPad)) {
#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
        // if the loadOrJoin menu has focus again, we can clear the saveTransfer
        // flag now. Added so we can delay the ehternet disconnect till it's
        // cleaned up
        if (m_eSaveTransferState == eSaveTransfer_Idle)
            m_bSaveTransferRunning = false;
#endif
        // Display the saves if we have them
        if (!m_bSavesDisplayed) {
            m_pSaveDetails = PlatformStorage.ReturnSavesInfo();
            if (m_pSaveDetails != nullptr) {
                // CD - Fix - Adding define for ORBIS/XBOXONE

                AddDefaultButtons();
                m_bSavesDisplayed = true;
                UpdateGamesList();

                if (m_saveDetails != nullptr) {
                    for (unsigned int i = 0; i < m_iSaveDetailsCount; ++i) {
                        if (m_saveDetails[i].pbThumbnailData != nullptr) {
                            delete m_saveDetails[i].pbThumbnailData;
                        }
                    }
                    delete m_saveDetails;
                }
                m_saveDetails = new SaveListDetails[m_pSaveDetails->iSaveC];

                m_iSaveDetailsCount = m_pSaveDetails->iSaveC;
                for (unsigned int i = 0; i < m_pSaveDetails->iSaveC; ++i) {
                    m_buttonListSaves.addItem(
                        m_pSaveDetails->SaveInfoA[i].UTF8SaveTitle, "");

                    m_saveDetails[i].saveId = i;
                    memcpy(m_saveDetails[i].UTF8SaveName,
                           m_pSaveDetails->SaveInfoA[i].UTF8SaveTitle, 128);
                    memcpy(m_saveDetails[i].UTF8SaveFilename,
                           m_pSaveDetails->SaveInfoA[i].UTF8SaveFilename,
                           MAX_SAVEFILENAME_LENGTH);
                }
                m_controlSavesTimer.setVisible(false);

                // set focus on the first button
            }
        }

        if (!m_bExitScene && m_bSavesDisplayed &&
            !m_bRetrievingSaveThumbnails && !m_bAllLoaded) {
            if (m_iRequestingThumbnailId <
                (m_buttonListSaves.getItemCount() - m_iDefaultButtonsC)) {
                m_bRetrievingSaveThumbnails = true;
                app.DebugPrintf("Requesting the first thumbnail\n");
                // set the save to load
                PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();
                IPlatformStorage::ESaveGameState eLoadStatus =
                    PlatformStorage.LoadSaveDataThumbnail(
                        &pSaveDetails->SaveInfoA[(int)m_iRequestingThumbnailId],
                        [this](std::uint8_t* data, unsigned int bytes) {
                            return loadSaveDataThumbnailReturned(data, bytes);
                        });

                if (eLoadStatus !=
                    IPlatformStorage::ESaveGame_GetSaveThumbnail) {
                    // something went wrong
                    m_bRetrievingSaveThumbnails = false;
                    m_bAllLoaded = true;
                }
            }
        } else if (m_bSavesDisplayed && m_bSaveThumbnailReady) {
            m_bSaveThumbnailReady = false;

            // check we're not waiting to exit the scene
            if (!m_bExitScene) {
                // convert to utf16
                std::uint16_t u16Message[MAX_SAVEFILENAME_LENGTH];
#if defined(_WINDOWS64)
                int result = ::MultiByteToWideChar(
                    CP_UTF8,               // convert from UTF-8
                    MB_ERR_INVALID_CHARS,  // error on invalid chars
                    m_saveDetails[m_iRequestingThumbnailId]
                        .UTF8SaveFilename,    // source UTF-8 string
                    MAX_SAVEFILENAME_LENGTH,  // total length of source UTF-8
                                              // string,
                    // in char's (= bytes), including end-of-string \0
                    (char*)u16Message,       // destination buffer
                    MAX_SAVEFILENAME_LENGTH  // size of destination buffer, in
                                             // char's
                );
#else
                std::uint32_t srcmax, dstmax;
                std::uint32_t srclen, dstlen;
                srcmax = MAX_SAVEFILENAME_LENGTH;
                dstmax = MAX_SAVEFILENAME_LENGTH;

                SceCesUcsContext context;
                sceCesUcsContextInit(&context);

                sceCesUtf8StrToUtf16Str(
                    &context,
                    (std::uint8_t*)m_saveDetails[m_iRequestingThumbnailId]
                        .UTF8SaveFilename,
                    srcmax, &srclen, u16Message, dstmax, &dstlen);
#endif
                if (m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData) {
                    registerSubstitutionTexture(
                        (char*)u16Message,
                        m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData,
                        m_saveDetails[m_iRequestingThumbnailId]
                            .dwThumbnailSize);
                }
                m_buttonListSaves.setTextureName(
                    m_iRequestingThumbnailId + m_iDefaultButtonsC,
                    (char*)u16Message);

                ++m_iRequestingThumbnailId;
                if (m_iRequestingThumbnailId <
                    (m_buttonListSaves.getItemCount() - m_iDefaultButtonsC)) {
                    app.DebugPrintf("Requesting another thumbnail\n");
                    // set the save to load
                    PSAVE_DETAILS pSaveDetails =
                        PlatformStorage.ReturnSavesInfo();
                    IPlatformStorage::ESaveGameState eLoadStatus =
                        PlatformStorage.LoadSaveDataThumbnail(
                            &pSaveDetails
                                 ->SaveInfoA[(int)m_iRequestingThumbnailId],
                            [this](std::uint8_t* data, unsigned int bytes) {
                                return loadSaveDataThumbnailReturned(data,
                                                                     bytes);
                            });
                    if (eLoadStatus !=
                        IPlatformStorage::ESaveGame_GetSaveThumbnail) {
                        // something went wrong
                        m_bRetrievingSaveThumbnails = false;
                        m_bAllLoaded = true;
                    }
                } else {
                    m_bRetrievingSaveThumbnails = false;
                    m_bAllLoaded = true;
                }
            } else {
                // stop retrieving thumbnails, and exit
                m_bRetrievingSaveThumbnails = false;
            }
        }
    }

    switch (m_iState) {
        case e_SavesIdle:
            break;
        case e_SavesRepopulate:
            m_bIgnoreInput = false;
            m_iState = e_SavesIdle;
            m_bAllLoaded = false;
            m_bRetrievingSaveThumbnails = false;
            m_iRequestingThumbnailId = 0;
            GetSaveInfo();
            break;
        case e_SavesRepopulateAfterMashupHide:
            m_bIgnoreInput = false;
            m_iRequestingThumbnailId = 0;
            m_bAllLoaded = false;
            m_bRetrievingSaveThumbnails = false;
            m_bSavesDisplayed = false;
            m_iSaveInfoC = 0;
            m_buttonListSaves.clearList();
            GetSaveInfo();
            m_iState = e_SavesIdle;
            break;
        case e_SavesRepopulateAfterDelete:
        case e_SavesRepopulateAfterTransferDownload:
            m_bIgnoreInput = false;
            m_iRequestingThumbnailId = 0;
            m_bAllLoaded = false;
            m_bRetrievingSaveThumbnails = false;
            m_bSavesDisplayed = false;
            m_iSaveInfoC = 0;
            m_buttonListSaves.clearList();
            PlatformStorage.ClearSavesInfo();
            GetSaveInfo();
            m_iState = e_SavesIdle;
            break;
    }
#else
    if (!m_bSavesDisplayed) {
        AddDefaultButtons();
        m_bSavesDisplayed = true;
        m_controlSavesTimer.setVisible(false);
    }
#endif

    // SAVE TRANSFERS
}

void UIScene_LoadOrJoinMenu::GetSaveInfo() {
    unsigned int uiSaveC = 0;

    // This will return with the number retrieved in uiSaveC

    if (app.DebugSettingsOn() && app.GetLoadSavesFromFolderEnabled()) {
        uiSaveC = 0;
        File savesDir("Saves");
        if (savesDir.exists()) {
            m_saves = savesDir.listFiles();
            uiSaveC = (unsigned int)m_saves->size();
        }
        // add the New Game and Tutorial after the saves list is retrieved, if
        // there are any saves

        // Add two for New Game and Tutorial
        unsigned int listItems = uiSaveC;

        AddDefaultButtons();

        for (unsigned int i = 0; i < listItems; i++) {
            std::string wName = m_saves->at(i)->getName();
            char* name = new char[wName.size() + 1];
            for (unsigned int j = 0; j < wName.size(); ++j) {
                name[j] = wName[j];
            }
            name[wName.size()] = 0;
            m_buttonListSaves.addItem(name, "");
        }
        m_bSavesDisplayed = true;
        m_bAllLoaded = true;
        m_bIgnoreInput = false;
    } else {
        // clear the saves list
        m_bSavesDisplayed =
            false;  // we're blocking the exit from this scene until complete
        m_buttonListSaves.clearList();
        m_iSaveInfoC = 0;
        m_controlSavesTimer.setVisible(true);

        m_pSaveDetails = PlatformStorage.ReturnSavesInfo();
        if (m_pSaveDetails == nullptr) {
            IPlatformStorage::ESaveGameState eSGIStatus =
                PlatformStorage.GetSavesInfo(m_iPad, nullptr, (char*)"save");
        }

#if TO_BE_IMPLEMENTED
        if (eSGIStatus == IPlatformStorage::ESGIStatus_NoSaves) {
            uiSaveC = 0;
            m_controlSavesTimer.setVisible(false);
            m_SavesList.SetEnable(true);
        }
#endif
    }

    return;
}

void UIScene_LoadOrJoinMenu::AddDefaultButtons() {
    m_iDefaultButtonsC = 0;
    m_iMashUpButtonsC = 0;
    m_generators.clear();

    m_buttonListSaves.addItem(app.GetString(IDS_CREATE_NEW_WORLD));
    m_iDefaultButtonsC++;

    int i = 0;

    for (auto it = app.getLevelGenerators()->begin();
         it != app.getLevelGenerators()->end(); ++it) {
        LevelGenerationOptions* levelGen = *it;

        // retrieve the save icon from the texture pack, if there is one
        unsigned int uiTexturePackID = levelGen->getRequiredTexturePackId();

        if (uiTexturePackID != 0) {
            unsigned int uiMashUpWorldsBitmask =
                app.GetMashupPackWorlds(m_iPad);

            if ((uiMashUpWorldsBitmask & (1 << (uiTexturePackID - 1024))) ==
                0) {
                // this world is hidden, so skip
                continue;
            }
        }

        // 4J-JEV: For debug. Ignore worlds with no name.
        const char* wstr = levelGen->getWorldName();
        m_buttonListSaves.addItem(wstr);
        m_generators.push_back(levelGen);

        if (uiTexturePackID != 0) {
            // increment the count of the mash-up pack worlds in the save list
            m_iMashUpButtonsC++;
            TexturePack* tp =
                Minecraft::GetInstance()->skins->getTexturePackById(
                    levelGen->getRequiredTexturePackId());
            std::uint32_t imageBytes = 0;
            std::uint8_t* imageData = tp->getPackIcon(imageBytes);

            if (imageBytes > 0 && imageData) {
                char imageName[64];
                snprintf(imageName, 64, "tpack%08x", tp->getId());
                registerSubstitutionTexture(imageName, imageData, imageBytes);
                m_buttonListSaves.setTextureName(
                    m_buttonListSaves.getItemCount() - 1, imageName);
            }
        }

        ++i;
    }
    m_iDefaultButtonsC += i;
}

void UIScene_LoadOrJoinMenu::handleInput(int iPad, int key, bool repeat,
                                         bool pressed, bool released,
                                         bool& handled) {
    if (m_bIgnoreInput) return;

    // if we're retrieving save info, ignore key presses
    if (!m_bSavesDisplayed) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
                handled = true;
            }
            break;
        case ACTION_MENU_X:
#if TO_BE_IMPLEMENTED
            // Change device
            // Fix for #12531 - TCR 001: BAS Game Stability: When a player
            // selects to change a storage device, and repeatedly backs out of
            // the SD screen, disconnects from LIVE, and then selects a SD, the
            // title crashes.
            m_bIgnoreInput = true;
            PlatformStorage.SetSaveDevice(
                &CScene_MultiGameJoinLoad::DeviceSelectReturned, this, true);
            ui.PlayUISFX(eSFX_Press);
#endif
            // Save Transfer
#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
            {
                bool bSignedInLive = PlatformProfile.IsSignedInLive(iPad);
                if (bSignedInLive) {
                    LaunchSaveTransfer();
                }
            }
#endif
            break;
        case ACTION_MENU_Y:
            break;

        case ACTION_MENU_RIGHT_SCROLL:
            if (DoesSavesListHaveFocus()) {
                // 4J-PB - check we are on a valid save
                if ((m_iDefaultButtonsC != 0) &&
                    (m_iSaveListIndex >= m_iDefaultButtonsC)) {
                    m_bIgnoreInput = true;

                    // Could be delete save or Save Options
                    if (PlatformStorage.GetSaveDisabled()) {
                        // delete the save game
                        // Have to ask the player if they are sure they want to
                        // delete this game
                        unsigned int uiIDA[2];
                        uiIDA[0] = IDS_CONFIRM_CANCEL;
                        uiIDA[1] = IDS_CONFIRM_OK;
                        ui.RequestAlertMessage(
                            IDS_TOOLTIPS_DELETESAVE, IDS_TEXT_DELETE_SAVE,
                            uiIDA, 2, iPad,
                            &UIScene_LoadOrJoinMenu::DeleteSaveDialogReturned,
                            this);
                    } else {
                        if (PlatformStorage.EnoughSpaceForAMinSaveGame()) {
                            unsigned int uiIDA[4];
                            uiIDA[0] = IDS_CONFIRM_CANCEL;
                            uiIDA[1] = IDS_TITLE_RENAMESAVE;
                            uiIDA[2] = IDS_TOOLTIPS_DELETESAVE;
                            int numOptions = 3;
#if defined(SONY_REMOTE_STORAGE_UPLOAD)
                            if (PlatformProfile.IsSignedInLive(
                                    PlatformProfile.GetPrimaryPad())) {
                                numOptions = 4;
                                uiIDA[3] = IDS_TOOLTIPS_SAVETRANSFER_UPLOAD;
                            }
#endif
                            ui.RequestAlertMessage(
                                IDS_TOOLTIPS_SAVEOPTIONS, IDS_TEXT_SAVEOPTIONS,
                                uiIDA, numOptions, iPad,
                                &UIScene_LoadOrJoinMenu::
                                    SaveOptionsDialogReturned,
                                this);
                        } else {
                            // delete the save game
                            // Have to ask the player if they are sure they want
                            // to delete this game
                            unsigned int uiIDA[2];
                            uiIDA[0] = IDS_CONFIRM_CANCEL;
                            uiIDA[1] = IDS_CONFIRM_OK;
                            ui.RequestAlertMessage(IDS_TOOLTIPS_DELETESAVE,
                                                   IDS_TEXT_DELETE_SAVE, uiIDA,
                                                   2, iPad,
                                                   &UIScene_LoadOrJoinMenu::
                                                       DeleteSaveDialogReturned,
                                                   this);
                        }
                    }
                    ui.PlayUISFX(eSFX_Press);
                }
            } else if (DoesMashUpWorldHaveFocus()) {
                // hiding a mash-up world
                if ((m_iSaveListIndex != JOIN_LOAD_CREATE_BUTTON_INDEX)) {
                    LevelGenerationOptions* levelGen =
                        m_generators.at(m_iSaveListIndex - 1);

                    if (!levelGen->isTutorial()) {
                        if (levelGen->requiresTexturePack()) {
                            unsigned int uiPackID =
                                levelGen->getRequiredTexturePackId();

                            m_bIgnoreInput = true;
                            app.HideMashupPackWorld(m_iPad, uiPackID);

                            // update the saves list
                            m_iState = e_SavesRepopulateAfterMashupHide;
                        }
                    }
                }
                ui.PlayUISFX(eSFX_Press);
            }
            break;
        case ACTION_MENU_LEFT_SCROLL:
            break;
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT: {
            // if we are on the saves menu, check there are games in the games
            // list to move to
            if (DoesSavesListHaveFocus()) {
                if (m_buttonListGames.getItemCount() > 0) {
                    sendInputToMovie(key, repeat, pressed, released);
                }
            } else {
                sendInputToMovie(key, repeat, pressed, released);
            }
        } break;

        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            handled = true;
            break;
    }
}

int UIScene_LoadOrJoinMenu::handleKeyboardCompleteWorldName(bool bRes) {
    // 4J HEG - No reason to set value if keyboard was cancelled
    m_bIgnoreInput = false;
    if (bRes) {
        const char* text = PlatformInput.GetText();
        // check the name is valid
        if (text[0] != '\0') {
        } else {
            m_bIgnoreInput = false;
            updateTooltips();
        }
    } else {
        m_bIgnoreInput = false;
        updateTooltips();
    }

    return 0;
}
void UIScene_LoadOrJoinMenu::handleInitFocus(F64 controlId, F64 childId) {
    app.DebugPrintf(app.USER_SR,
                    "UIScene_LoadOrJoinMenu::handleInitFocus - %d , %d\n",
                    (int)controlId, (int)childId);
}

void UIScene_LoadOrJoinMenu::handleFocusChange(F64 controlId, F64 childId) {
    app.DebugPrintf(app.USER_SR,
                    "UIScene_LoadOrJoinMenu::handleFocusChange - %d , %d\n",
                    (int)controlId, (int)childId);

    switch ((int)controlId) {
        case eControl_GamesList:
            m_iGameListIndex = childId;
            m_buttonListGames.updateChildFocus((int)childId);
            break;
        case eControl_SavesList:
            m_iSaveListIndex = childId;
            m_bUpdateSaveSize = true;
            break;
    };
    updateTooltips();
}

#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
void UIScene_LoadOrJoinMenu::remoteStorageGetSaveCallback(
    void* lpParam, SonyRemoteStorage::Status s, int error_code) {
    app.DebugPrintf("remoteStorageGetCallback err : 0x%08x\n", error_code);
    assert(error_code == 0);
    ((UIScene_LoadOrJoinMenu*)lpParam)->LoadSaveFromCloud();
}
#endif

void UIScene_LoadOrJoinMenu::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_SavesList: {
            m_bIgnoreInput = true;

            int lGenID = (int)childId - 1;

            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            if ((int)childId == JOIN_LOAD_CREATE_BUTTON_INDEX) {
                app.SetTutorialMode(false);

                m_controlJoinTimer.setVisible(false);

                app.SetCorruptSaveDeleted(false);

                CreateWorldMenuInitData* params = new CreateWorldMenuInitData();
                params->iPad = m_iPad;
                ui.NavigateToScene(m_iPad, eUIScene_CreateWorldMenu,
                                   (void*)params);
            } else if (lGenID < m_generators.size()) {
                LevelGenerationOptions* levelGen = m_generators.at(lGenID);
                app.SetTutorialMode(levelGen->isTutorial());
                // Reset the autosave time
                app.SetAutosaveTimerTime();

                if (levelGen->isTutorial()) {
                    LoadLevelGen(levelGen);
                } else {
                    LoadMenuInitData* params = new LoadMenuInitData();
                    params->iPad = m_iPad;
                    // need to get the iIndex from the list item, since the
                    // position in the list doesn't correspond to the
                    // GetSaveGameInfo list because of sorting
                    params->iSaveGameInfoIndex = -1;
                    // params->pbSaveRenamed=&m_bSaveRenamed;
                    params->levelGen = levelGen;
                    params->saveDetails = nullptr;

                    // navigate to the settings scene
                    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                       eUIScene_LoadMenu, params);
                }
            } else {
                {
                    app.SetTutorialMode(false);

                    if (app.DebugSettingsOn() &&
                        app.GetLoadSavesFromFolderEnabled()) {
                        LoadSaveFromDisk(
                            m_saves->at((int)childId - m_iDefaultButtonsC));
                    } else {
                        LoadMenuInitData* params = new LoadMenuInitData();
                        params->iPad = m_iPad;
                        // need to get the iIndex from the list item, since the
                        // position in the list doesn't correspond to the
                        // GetSaveGameInfo list because of sorting
                        params->iSaveGameInfoIndex =
                            ((int)childId) - m_iDefaultButtonsC;
                        // params->pbSaveRenamed=&m_bSaveRenamed;
                        params->levelGen = nullptr;
                        params->saveDetails =
                            &m_saveDetails[((int)childId) - m_iDefaultButtonsC];

                        {
                            // navigate to the settings scene
                            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                               eUIScene_LoadMenu, params);
                        }
                    }
                }
            }
        } break;
        case eControl_GamesList: {
            m_bIgnoreInput = true;

            m_eAction = eAction_JoinGame;

            // CD - Added for audio
            ui.PlayUISFX(eSFX_Press);

            {
                int nIndex = (int)childId;
                m_iGameListIndex = nIndex;
                CheckAndJoinGame(nIndex);
            }

            break;
        }
    }
}

void UIScene_LoadOrJoinMenu::CheckAndJoinGame(int gameIndex) {
    if (m_buttonListGames.getItemCount() > 0 &&
        gameIndex < m_currentSessions->size()) {
        // CScene_MultiGameInfo::JoinMenuInitData *initData = new
        // CScene_MultiGameInfo::JoinMenuInitData();
        m_initData->iPad = 0;
        ;
        m_initData->selectedSession = m_currentSessions->at(gameIndex);

        // check that we have the texture pack available
        // If it's not the default texture pack
        if (m_initData->selectedSession->data.texturePackParentId != 0) {
            int texturePacksCount =
                Minecraft::GetInstance()->skins->getTexturePackCount();
            bool bHasTexturePackInstalled = false;

            for (int i = 0; i < texturePacksCount; i++) {
                TexturePack* tp =
                    Minecraft::GetInstance()->skins->getTexturePackByIndex(i);
                if (tp->getDLCParentPackId() ==
                    m_initData->selectedSession->data.texturePackParentId) {
                    bHasTexturePackInstalled = true;
                    break;
                }
            }

            if (bHasTexturePackInstalled == false) {
                // upsell the texture pack
                // tell sentient about the upsell of the full version of the
                // skin pack
                unsigned int uiIDA[2];

                uiIDA[0] = IDS_TEXTUREPACK_FULLVERSION;
                // uiIDA[1]=IDS_TEXTURE_PACK_TRIALVERSION;
                uiIDA[1] = IDS_CONFIRM_CANCEL;

                // Give the player a warning about the texture pack missing
                ui.RequestAlertMessage(
                    IDS_DLC_TEXTUREPACK_NOT_PRESENT_TITLE,
                    IDS_DLC_TEXTUREPACK_NOT_PRESENT, uiIDA, 2, m_iPad,
                    &UIScene_LoadOrJoinMenu::TexturePackDialogReturned, this);

                return;
            }
        }
        m_controlJoinTimer.setVisible(false);

        m_bIgnoreInput = true;
        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(), eUIScene_JoinMenu,
                           m_initData);
    }
}

void UIScene_LoadOrJoinMenu::LoadLevelGen(LevelGenerationOptions* levelGen) {
    // Load data from disc
    // File saveFile( "Tutorial\\Tutorial" );
    // LoadSaveFromDisk(&saveFile);

    // clear out the app's terrain features list
    app.ClearTerrainFeaturePosition();

    PlatformStorage.ResetSaveData();
    // Make our next save default to the name of the level
    PlatformStorage.SetSaveTitle(levelGen->getDefaultSaveName().c_str());

    bool isClientSide = false;
    bool isPrivate = false;
    // TODO int maxPlayers = MINECRAFT_NET_MAX_PLAYERS;
    int maxPlayers = 8;

    if (app.GetTutorialMode()) {
        isClientSide = false;
        maxPlayers = 4;
    }

    g_NetworkManager.HostGame(0, isClientSide, isPrivate, maxPlayers, 0);

    NetworkGameInitData* param = new NetworkGameInitData();
    param->seed = 0;
    param->saveData = nullptr;
    param->settings = app.GetGameHostOption(eGameHostOption_Tutorial);
    param->levelGen = levelGen;

    if (levelGen->requiresTexturePack()) {
        param->texturePackId = levelGen->getRequiredTexturePackId();

        Minecraft* pMinecraft = Minecraft::GetInstance();
        pMinecraft->skins->selectTexturePackById(param->texturePackId);
        // pMinecraft->skins->updateUI();
    }

    g_NetworkManager.FakeLocalPlayerJoined();

    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
    loadingParams->lpParam = param;

    UIFullscreenProgressCompletionData* completionData =
        new UIFullscreenProgressCompletionData();
    completionData->bShowBackground = true;
    completionData->bShowLogo = true;
    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
    completionData->iPad = DEFAULT_XUI_MENU_USER;
    loadingParams->completionData = completionData;

    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                       eUIScene_FullscreenProgress, loadingParams);
}

void UIScene_LoadOrJoinMenu::UpdateGamesListCallback(void* pParam) {
    if (pParam != nullptr) {
        UIScene_LoadOrJoinMenu* pScene = (UIScene_LoadOrJoinMenu*)pParam;
        pScene->UpdateGamesList();
    }
}

void UIScene_LoadOrJoinMenu::UpdateGamesList() {
    // If we're ignoring input scene isn't active so do nothing
    if (m_bIgnoreInput) return;

    // If a texture pack is loading, or will be loading, then ignore this ( we
    // are going to be destroyed anyway)
    if (Minecraft::GetInstance()->skins->getSelected()->isLoadingData() ||
        (Minecraft::GetInstance()->skins->needsUIUpdate() ||
         ui.IsReloadingSkin()))
        return;

    // if we're retrieving save info, don't show the list yet as we will be
    // ignoring press events
    if (!m_bSavesDisplayed) {
        return;
    }

    FriendSessionInfo* pSelectedSession = nullptr;
    if (DoesGamesListHaveFocus() && m_buttonListGames.getItemCount() > 0) {
        const int nIndex = m_buttonListGames.getCurrentSelection();
        pSelectedSession = m_currentSessions->at(nIndex);
    }

    SessionID selectedSessionId;
    memset(&selectedSessionId, 0, sizeof(SessionID));
    if (pSelectedSession != nullptr)
        selectedSessionId = pSelectedSession->sessionId;
    pSelectedSession = nullptr;

    m_controlJoinTimer.setVisible(false);

    // if the saves list has focus, then we should show the Delete Save tooltip
    // if the games list has focus, then we should show the View Gamercard
    // tooltip
    int iRB = -1;
    int iY = -1;
    int iX = -1;

    delete m_currentSessions;
    m_currentSessions =
        g_NetworkManager.GetSessionList(m_iPad, 1, m_bShowingPartyGamesOnly);

    // Update the xui list displayed
    unsigned int xuiListSize = m_buttonListGames.getItemCount();
    unsigned int filteredListSize = (unsigned int)m_currentSessions->size();

    const bool gamesListHasFocus = DoesGamesListHaveFocus();

    if (filteredListSize > 0) {
#if TO_BE_IMPLEMENTED
        if (!m_pGamesList->IsEnabled()) {
            m_pGamesList->SetEnable(true);
            m_pGamesList->SetCurSel(0);
        }
#endif
        m_labelNoGames.setVisible(false);
        m_controlJoinTimer.setVisible(false);
    } else {
#if TO_BE_IMPLEMENTED
        m_pGamesList->SetEnable(false);
#endif
        m_controlJoinTimer.setVisible(false);
        m_labelNoGames.setVisible(true);

#if TO_BE_IMPLEMENTED
        if (gamesListHasFocus) m_pGamesList->InitFocus(m_iPad);
#endif
    }

    // clear out the games list and re-fill
    m_buttonListGames.clearList();

    if (filteredListSize > 0) {
        // Reset the focus to the selected session if it still exists
        unsigned int sessionIndex = 0;
        m_buttonListGames.setCurrentSelection(0);

        for (auto it = m_currentSessions->begin();
             it < m_currentSessions->end(); ++it) {
            FriendSessionInfo* sessionInfo = *it;

            char textureName[64] = "\0";

            // Is this a default game or a texture pack game?
            if (sessionInfo->data.texturePackParentId != 0) {
                // Do we have the texture pack
                Minecraft* pMinecraft = Minecraft::GetInstance();
                TexturePack* tp = pMinecraft->skins->getTexturePackById(
                    sessionInfo->data.texturePackParentId);
                int32_t hr;

                std::uint32_t imageBytes = 0;
                std::uint8_t* imageData = nullptr;

                if (tp == nullptr) {
                    unsigned int dwBytes = 0;
                    std::uint8_t* pbData = nullptr;
                    app.GetTPD(sessionInfo->data.texturePackParentId, &pbData,
                               &dwBytes);

                    // is it in the tpd data ?
                    unsigned int tpdImageBytes = 0;
                    app.GetFileFromTPD(eTPDFileType_Icon, pbData, dwBytes,
                                       &imageData, &tpdImageBytes);
                    imageBytes = static_cast<std::uint32_t>(tpdImageBytes);
                    if (imageBytes > 0 && imageData) {
                        snprintf(textureName, 64, "%s",
                                 sessionInfo->displayLabel);
                        registerSubstitutionTexture(textureName, imageData,
                                                    imageBytes);
                    }
                } else {
                    imageData = tp->getPackIcon(imageBytes);
                    if (imageBytes > 0 && imageData) {
                        snprintf(textureName, 64, "%s",
                                 sessionInfo->displayLabel);
                        registerSubstitutionTexture(textureName, imageData,
                                                    imageBytes);
                    }
                }
            } else {
                // default texture pack
                Minecraft* pMinecraft = Minecraft::GetInstance();
                TexturePack* tp = pMinecraft->skins->getTexturePackByIndex(0);

                std::uint32_t imageBytes = 0;
                std::uint8_t* imageData = tp->getPackIcon(imageBytes);

                if (imageBytes > 0 && imageData) {
                    snprintf(textureName, 64, "%s", sessionInfo->displayLabel);
                    registerSubstitutionTexture(textureName, imageData,
                                                imageBytes);
                }
            }

            m_buttonListGames.addItem(sessionInfo->displayLabel, textureName);

            if (memcmp(&selectedSessionId, &sessionInfo->sessionId,
                       sizeof(SessionID)) == 0) {
                m_buttonListGames.setCurrentSelection(sessionIndex);
                break;
            }
            ++sessionIndex;
        }
    }

    updateTooltips();
}

void UIScene_LoadOrJoinMenu::HandleDLCMountingComplete() { Initialise(); }

bool UIScene_LoadOrJoinMenu::DoesSavesListHaveFocus() {
    if (m_buttonListSaves.hasFocus()) {
        // check it's not the first or second element (new world or tutorial)
        if (m_iSaveListIndex > (m_iDefaultButtonsC - 1)) {
            return true;
        }
    }
    return false;
}

bool UIScene_LoadOrJoinMenu::DoesMashUpWorldHaveFocus() {
    if (m_buttonListSaves.hasFocus()) {
        // check it's not the first or second element (new world or tutorial)
        if (m_iSaveListIndex > (m_iDefaultButtonsC - 1)) {
            return false;
        }

        if (m_iSaveListIndex > (m_iDefaultButtonsC - 1 - m_iMashUpButtonsC)) {
            return true;
        } else
            return false;
    } else
        return false;
}

bool UIScene_LoadOrJoinMenu::DoesGamesListHaveFocus() {
    return m_buttonListGames.hasFocus();
}

void UIScene_LoadOrJoinMenu::handleTimerComplete(int id) {
    switch (id) {
        case JOIN_LOAD_ONLINE_TIMER_ID: {
            bool bMultiplayerAllowed =
                PlatformProfile.IsSignedInLive(m_iPad) &&
                PlatformProfile.AllowedToPlayMultiplayer(m_iPad);
            if (bMultiplayerAllowed != m_bMultiplayerAllowed) {
                if (bMultiplayerAllowed) {
                    // 					m_CheckboxOnline.SetEnable(true);
                    // 					m_CheckboxPrivate.SetEnable(true);
                } else {
                    m_bInParty = false;
                    m_buttonListGames.clearList();
                    m_controlJoinTimer.setVisible(true);
                    m_labelNoGames.setVisible(false);
                }

                m_bMultiplayerAllowed = bMultiplayerAllowed;
            }
        } break;
            // 4J-PB - Only Xbox will not have trial DLC patched into the game
    }
}

void UIScene_LoadOrJoinMenu::LoadSaveFromDisk(
    File* saveFile, ESavePlatform savePlatform /*= SAVE_FILE_PLATFORM_LOCAL*/) {
    // we'll only be coming in here when the tutorial is loaded now

    PlatformStorage.ResetSaveData();

    // Make our next save default to the name of the level
    PlatformStorage.SetSaveTitle(saveFile->getName().c_str());

    int64_t fileSize = saveFile->length();
    FileInputStream fis(*saveFile);
    std::vector<uint8_t> ba(fileSize);
    fis.read(ba);
    fis.close();

    bool isClientSide = false;
    bool isPrivate = false;
    int maxPlayers = MINECRAFT_NET_MAX_PLAYERS;

    if (app.GetTutorialMode()) {
        isClientSide = false;
        maxPlayers = 4;
    }

    app.SetGameHostOption(eGameHostOption_GameType,
                          GameType::CREATIVE->getId());

    g_NetworkManager.HostGame(0, isClientSide, isPrivate, maxPlayers, 0);

    LoadSaveDataThreadParam* saveData =
        new LoadSaveDataThreadParam(ba.data(), ba.size(), saveFile->getName());

    NetworkGameInitData* param = new NetworkGameInitData();
    param->seed = 0;
    param->saveData = saveData;
    param->settings = app.GetGameHostOption(eGameHostOption_All);
    param->savePlatform = savePlatform;

    g_NetworkManager.FakeLocalPlayerJoined();

    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
    loadingParams->lpParam = param;

    UIFullscreenProgressCompletionData* completionData =
        new UIFullscreenProgressCompletionData();
    completionData->bShowBackground = true;
    completionData->bShowLogo = true;
    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
    completionData->iPad = DEFAULT_XUI_MENU_USER;
    loadingParams->completionData = completionData;

    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                       eUIScene_FullscreenProgress, loadingParams);
}

#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)
void UIScene_LoadOrJoinMenu::LoadSaveFromCloud() {
    char wFileName[128];
    strncpy(
        wFileName, app.getRemoteStorage()->getLocalFilename(),
        strlen(app.getRemoteStorage()->getLocalFilename()) + 1);  // plus null
    File cloudFile(wFileName);

    PlatformStorage.ResetSaveData();

    // Make our next save default to the name of the level
    char wSaveName[128];
    strncpy(
        wSaveName, app.getRemoteStorage()->getSaveNameUTF8(),
        strlen(app.getRemoteStorage()->getSaveNameUTF8()) + 1);  // plus null
    PlatformStorage.SetSaveTitle(wSaveName);

    int64_t fileSize = cloudFile.length();
    FileInputStream fis(cloudFile);
    std::vector<uint8_t> ba(fileSize);
    fis.read(ba);
    fis.close();

    bool isClientSide = false;
    bool isPrivate = false;
    int maxPlayers = MINECRAFT_NET_MAX_PLAYERS;

    if (app.GetTutorialMode()) {
        isClientSide = false;
        maxPlayers = 4;
    }

    app.SetGameHostOption(eGameHostOption_All,
                          app.getRemoteStorage()->getSaveHostOptions());

    g_NetworkManager.HostGame(0, isClientSide, isPrivate, maxPlayers, 0);

    LoadSaveDataThreadParam* saveData =
        new LoadSaveDataThreadParam(ba.data(), ba.size(), cloudFile.getName());

    NetworkGameInitData* param = new NetworkGameInitData();
    param->seed = app.getRemoteStorage()->getSaveSeed();
    param->saveData = saveData;
    param->settings = app.GetGameHostOption(eGameHostOption_All);
    param->savePlatform = app.getRemoteStorage()->getSavePlatform();
    param->texturePackId = app.getRemoteStorage()->getSaveTexturePack();

    g_NetworkManager.FakeLocalPlayerJoined();

    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
    loadingParams->lpParam = param;

    UIFullscreenProgressCompletionData* completionData =
        new UIFullscreenProgressCompletionData();
    completionData->bShowBackground = true;
    completionData->bShowLogo = true;
    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
    completionData->iPad = DEFAULT_XUI_MENU_USER;
    loadingParams->completionData = completionData;

    ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                       eUIScene_FullscreenProgress, loadingParams);
}

#endif

int UIScene_LoadOrJoinMenu::DeleteSaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)pParam;
    // results switched for this dialog

    // Check that we have a valid save selected (can get a bad index if the save
    // list has been refreshed)
    bool validSelection =
        pClass->m_iDefaultButtonsC != 0 &&
        pClass->m_iSaveListIndex >= pClass->m_iDefaultButtonsC;

    if (result == IPlatformStorage::EMessage_ResultDecline && validSelection) {
        if (app.DebugSettingsOn() && app.GetLoadSavesFromFolderEnabled()) {
            pClass->m_bIgnoreInput = false;
        } else {
            {
                size_t cbId = pClass->GetCallbackUniqueId();
                PlatformStorage.DeleteSaveData(
                    &pClass->m_pSaveDetails
                         ->SaveInfoA[pClass->m_iSaveListIndex -
                                     pClass->m_iDefaultButtonsC],
                    [cbId](const bool bRes) {
                        ui.lockCallbackScenes();
                        auto* p =
                            (UIScene_LoadOrJoinMenu*)ui.GetSceneFromCallbackId(
                                cbId);
                        if (p) {
                            p->deleteSaveDataReturned(bRes);
                        }
                        ui.unlockCallbackScenes();
                        return 0;
                    });
            }
            pClass->m_controlSavesTimer.setVisible(true);
        }
    } else {
        pClass->m_bIgnoreInput = false;
    }

    return 0;
}

int UIScene_LoadOrJoinMenu::deleteSaveDataReturned(bool bRes) {
    if (bRes) {
        // wipe the list and repopulate it
        m_iState = e_SavesRepopulateAfterDelete;
    } else
        m_bIgnoreInput = false;

    updateTooltips();
    return 0;
}

int UIScene_LoadOrJoinMenu::renameSaveDataReturned(bool bRes) {
    if (bRes) {
        m_iState = e_SavesRepopulate;
    } else
        m_bIgnoreInput = false;

    updateTooltips();

    return 0;
}

int UIScene_LoadOrJoinMenu::SaveOptionsDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)pParam;

    // results switched for this dialog
    // EMessage_ResultAccept means cancel
    switch (result) {
        case IPlatformStorage::EMessage_ResultDecline:  // rename
        {
            pClass->m_bIgnoreInput = true;
            // bring up a keyboard
            char wSaveName[128];
            // CD - Fix - We must memset the SaveName
            memset(wSaveName, 0, 128 * sizeof(char));
            strncpy(
                wSaveName,
                pClass
                    ->m_saveDetails[pClass->m_iSaveListIndex -
                                    pClass->m_iDefaultButtonsC]
                    .UTF8SaveName,
                strlen(pClass->m_saveDetails->UTF8SaveName) + 1);  // plus null
            char* ptr = wSaveName;
            PlatformInput.RequestKeyboard(
                app.GetString(IDS_RENAME_WORLD_TITLE), wSaveName, 0, 25,
                [pClass](bool bRes) -> int {
                    return pClass->handleKeyboardCompleteWorldName(bRes);
                },
                IPlatformInput::EKeyboardMode_Default);
        } break;

        case IPlatformStorage::EMessage_ResultThirdOption:  // delete -
        {
            // delete the save game
            // Have to ask the player if they are sure they want to delete this
            // game
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_CANCEL;
            uiIDA[1] = IDS_CONFIRM_OK;
            ui.RequestAlertMessage(
                IDS_TOOLTIPS_DELETESAVE, IDS_TEXT_DELETE_SAVE, uiIDA, 2, iPad,
                &UIScene_LoadOrJoinMenu::DeleteSaveDialogReturned, pClass);
        } break;

#if defined(SONY_REMOTE_STORAGE_UPLOAD)
        case IPlatformStorage::EMessage_ResultFourthOption:  // upload to cloud
        {
            unsigned int uiIDA[2];
            uiIDA[0] = IDS_CONFIRM_OK;
            uiIDA[1] = IDS_CONFIRM_CANCEL;

            ui.RequestAlertMessage(
                IDS_TOOLTIPS_SAVETRANSFER_UPLOAD, IDS_SAVE_TRANSFER_TEXT, uiIDA,
                2, iPad, &UIScene_LoadOrJoinMenu::SaveTransferDialogReturned,
                pClass);
        } break;
#endif

        case IPlatformStorage::EMessage_Cancelled:
        default: {
            // reset the tooltips
            pClass->updateTooltips();
            pClass->m_bIgnoreInput = false;
        } break;
    }
    return 0;
}

int UIScene_LoadOrJoinMenu::TexturePackDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)pParam;

    // Exit with or without saving
    if (result == IPlatformStorage::EMessage_ResultAccept) {
        // we need to enable background downloading for the DLC
        // XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
    }
    pClass->m_bIgnoreInput = false;
    return 0;
}

#if defined(SONY_REMOTE_STORAGE_DOWNLOAD)

void UIScene_LoadOrJoinMenu::LaunchSaveTransfer() {
    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func =
        &UIScene_LoadOrJoinMenu::DownloadSonyCrossSaveThreadProc;
    loadingParams->lpParam = this;

    UIFullscreenProgressCompletionData* completionData =
        new UIFullscreenProgressCompletionData();
    completionData->bShowBackground = true;
    completionData->bShowLogo = true;
    completionData->type = e_ProgressCompletion_NavigateBackToScene;
    completionData->iPad = DEFAULT_XUI_MENU_USER;
    loadingParams->completionData = completionData;

    loadingParams->cancelFunc =
        &UIScene_LoadOrJoinMenu::CancelSaveTransferCallback;
    loadingParams->m_cancelFuncParam = this;
    loadingParams->cancelText = IDS_TOOLTIPS_CANCEL;

    ui.NavigateToScene(m_iPad, eUIScene_FullscreenProgress, loadingParams);
}

int UIScene_LoadOrJoinMenu::createDummySaveDataCallback(bool bRes) {
    if (bRes) {
        m_eSaveTransferState = eSaveTransfer_GetSavesInfo;
    } else {
        m_eSaveTransferState = eSaveTransfer_Error;
        app.DebugPrintf("createDummySaveDataCallback failed\n");
    }
    return 0;
}

int UIScene_LoadOrJoinMenu::crossSaveGetSavesInfoCallback(
    SAVE_DETAILS* pSaveDetails, bool bRes) {
    if (bRes) {
        m_eSaveTransferState = eSaveTransfer_GetFileData;
    } else {
        m_eSaveTransferState = eSaveTransfer_Error;
        app.DebugPrintf("crossSaveGetSavesInfoCallback failed\n");
    }
    return 0;
}

int UIScene_LoadOrJoinMenu::loadCrossSaveDataCallback(bool bIsCorrupt,
                                                      bool bIsOwner) {
    if (bIsCorrupt == false && bIsOwner) {
        m_eSaveTransferState = eSaveTransfer_CreatingNewSave;
    } else {
        m_eSaveTransferState = eSaveTransfer_Error;
        app.DebugPrintf("loadCrossSaveDataCallback failed \n");
    }
    return 0;
}

int UIScene_LoadOrJoinMenu::CrossSaveFinishedCallback(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)pParam;
    pClass->m_eSaveTransferState = eSaveTransfer_Idle;
    return 0;
}

int UIScene_LoadOrJoinMenu::crossSaveDeleteOnErrorReturned(bool bRes) {
    m_eSaveTransferState = eSaveTransfer_ErrorMesssage;
    return 0;
}

int UIScene_LoadOrJoinMenu::RemoteSaveNotFoundCallback(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)pParam;
    pClass->m_eSaveTransferState = eSaveTransfer_Idle;
    return 0;
}

// MGH -  added this global to force the delete of the previous data, for the
// remote storage saves
//	need to speak to Chris why this is necessary
bool g_bForceVitaSaveWipe = false;

int UIScene_LoadOrJoinMenu::DownloadSonyCrossSaveThreadProc(void* lpParameter) {
    m_bSaveTransferRunning = true;
    Compression::UseDefaultThreadStorage();
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)lpParameter;
    pClass->m_saveTransferDownloadCancelled = false;
    m_bSaveTransferRunning = true;
    bool bAbortCalled = false;
    Minecraft* pMinecraft = Minecraft::GetInstance();
    bool bSaveFileCreated = false;
    char wSaveName[128];

    // get the save file size
    pMinecraft->progressRenderer->progressStagePercentage(0);
    pMinecraft->progressRenderer->progressStart(
        IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD);
    pMinecraft->progressRenderer->progressStage(
        IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD);

    ConsoleSaveFile* pSave = nullptr;

    pClass->m_eSaveTransferState = eSaveTransfer_GetRemoteSaveInfo;

    while (pClass->m_eSaveTransferState != eSaveTransfer_Idle) {
        switch (pClass->m_eSaveTransferState) {
            case eSaveTransfer_Idle:
                break;
            case eSaveTransfer_GetRemoteSaveInfo:
                app.DebugPrintf("UIScene_LoadOrJoinMenu getSaveInfo\n");
                app.getRemoteStorage()->getSaveInfo();
                pClass->m_eSaveTransferState =
                    eSaveTransfer_GettingRemoteSaveInfo;
                break;
            case eSaveTransfer_GettingRemoteSaveInfo:
                if (pClass->m_saveTransferDownloadCancelled) {
                    pClass->m_eSaveTransferState = eSaveTransfer_Error;
                    break;
                }
                if (app.getRemoteStorage()->waitingForSaveInfo() == false) {
                    if (app.getRemoteStorage()->saveIsAvailable()) {
                        if (app.getRemoteStorage()->saveVersionSupported()) {
                            pClass->m_eSaveTransferState =
                                eSaveTransfer_CreateDummyFile;
                        } else {
                            // must be a newer version of the save in the cloud
                            // that we don't support yet
                            unsigned int uiIDA[1];
                            uiIDA[0] = IDS_CONFIRM_OK;
                            ui.RequestAlertMessage(
                                IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD,
                                IDS_SAVE_TRANSFER_WRONG_VERSION, uiIDA, 1,
                                PlatformProfile.GetPrimaryPad(),
                                RemoteSaveNotFoundCallback, pClass);
                        }
                    } else {
                        // no save available, inform the user about the
                        // functionality
                        unsigned int uiIDA[1];
                        uiIDA[0] = IDS_CONFIRM_OK;
                        ui.RequestAlertMessage(
                            IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD,
                            IDS_SAVE_TRANSFER_NOT_AVAILABLE_TEXT, uiIDA, 1,
                            PlatformProfile.GetPrimaryPad(),
                            RemoteSaveNotFoundCallback, pClass);
                    }
                }
                break;
            case eSaveTransfer_CreateDummyFile: {
                PlatformStorage.ResetSaveData();
                byte* compData = (byte*)PlatformStorage.AllocateSaveData(
                    app.getRemoteStorage()->getSaveFilesize());
                // Make our next save default to the name of the level
                const char* pNameUTF8 =
                    app.getRemoteStorage()->getSaveNameUTF8();
                strncpy(wSaveName, pNameUTF8,
                        strlen(pNameUTF8) + 1);  // plus null
                PlatformStorage.SetSaveTitle(wSaveName);
                std::uint8_t* pbThumbnailData = nullptr;
                unsigned int dwThumbnailDataSize = 0;

                std::uint8_t* pbDataSaveImage = nullptr;
                unsigned int dwDataSizeSaveImage = 0;

                PlatformStorage.GetDefaultSaveImage(
                    &pbDataSaveImage,
                    &dwDataSizeSaveImage);  // Get the default save thumbnail
                                            // (as set by SetDefaultImages) for
                                            // use on saving games t
                PlatformStorage.GetDefaultSaveThumbnail(
                    &pbThumbnailData,
                    &dwThumbnailDataSize);  // Get the default save image (as
                                            // set by SetDefaultImages) for use
                                            // on saving games that

                std::uint8_t bTextMetadata[88];
                memset(bTextMetadata, 0, 88);
                unsigned int hostOptions =
                    app.getRemoteStorage()->getSaveHostOptions();
                int iTextMetadataBytes = app.CreateImageTextData(
                    bTextMetadata, app.getRemoteStorage()->getSaveSeed(), true,
                    hostOptions, app.getRemoteStorage()->getSaveTexturePack());

                // set the icon and save image
                PlatformStorage.SetSaveImages(
                    pbThumbnailData, dwThumbnailDataSize, pbDataSaveImage,
                    dwDataSizeSaveImage, bTextMetadata, iTextMetadataBytes);

                app.getRemoteStorage()->waitForPlatformStorageIdle();
                IPlatformStorage::ESaveGameState saveState =
                    PlatformStorage.SaveSaveData([pClass](const bool bRes) {
                        return pClass->createDummySaveDataCallback(bRes);
                    });
                if (saveState == IPlatformStorage::ESaveGame_Save) {
                    pClass->m_eSaveTransferState =
                        eSaveTransfer_CreatingDummyFile;
                } else {
                    app.DebugPrintf("Failed to create dummy save file\n");
                    pClass->m_eSaveTransferState = eSaveTransfer_Error;
                }
            } break;
            case eSaveTransfer_CreatingDummyFile:
                break;
            case eSaveTransfer_GetSavesInfo: {
                // we can't cancel here, we need the saves info so we can delete
                // the file
                if (pClass->m_saveTransferDownloadCancelled) {
                    char wcTemp[256];
                    snprintf(
                        wcTemp, 256,
                        app.GetString(
                            IDS_CANCEL));  // MGH - should change this string to
                                           // "cancelling download"
                    m_wstrStageText = wcTemp;
                    pMinecraft->progressRenderer->progressStage(
                        m_wstrStageText);
                }

                app.getRemoteStorage()->waitForPlatformStorageIdle();
                app.DebugPrintf("CALL GetSavesInfo B\n");
                IPlatformStorage::ESaveGameState eSGIStatus =
                    PlatformStorage.GetSavesInfo(
                        pClass->m_iPad,
                        [pClass](SAVE_DETAILS* pSaveDetails, const bool bRes) {
                            return pClass->crossSaveGetSavesInfoCallback(
                                pSaveDetails, bRes);
                        },
                        "save");
                pClass->m_eSaveTransferState = eSaveTransfer_GettingSavesInfo;
            } break;
            case eSaveTransfer_GettingSavesInfo:
                if (pClass->m_saveTransferDownloadCancelled) {
                    char wcTemp[256];
                    snprintf(
                        wcTemp, 256,
                        app.GetString(
                            IDS_CANCEL));  // MGH - should change this string to
                                           // "cancelling download"
                    m_wstrStageText = wcTemp;
                    pMinecraft->progressRenderer->progressStage(
                        m_wstrStageText);
                }
                break;

            case eSaveTransfer_GetFileData: {
                bSaveFileCreated = true;
                PlatformStorage.GetSaveUniqueFileDir(
                    pClass->m_downloadedUniqueFilename);

                if (pClass->m_saveTransferDownloadCancelled) {
                    pClass->m_eSaveTransferState = eSaveTransfer_Error;
                    break;
                }
                PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();
                int idx = pClass->m_iSaveListIndex - pClass->m_iDefaultButtonsC;
                app.getRemoteStorage()->waitForPlatformStorageIdle();
                bool bGettingOK = app.getRemoteStorage()->getSaveData(
                    pClass->m_downloadedUniqueFilename, SaveTransferReturned,
                    pClass);
                if (bGettingOK) {
                    pClass->m_eSaveTransferState =
                        eSaveTransfer_GettingFileData;
                } else {
                    pClass->m_eSaveTransferState = eSaveTransfer_Error;
                    app.DebugPrintf(
                        "app.getRemoteStorage()->getSaveData failed\n");
                }
            }

            case eSaveTransfer_GettingFileData: {
                char wcTemp[256];

                int dataProgress = app.getRemoteStorage()->getDataProgress();
                pMinecraft->progressRenderer->progressStagePercentage(
                    dataProgress);

                // snprintf(wcTemp, 256, "Downloading data : %d",
                // dataProgress);//app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),0,pClass->m_ulFileSize);
                snprintf(wcTemp, 256,
                         app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),
                         dataProgress);
                m_wstrStageText = wcTemp;
                pMinecraft->progressRenderer->progressStage(m_wstrStageText);
                if (pClass->m_saveTransferDownloadCancelled &&
                    bAbortCalled == false) {
                    app.getRemoteStorage()->abort();
                    bAbortCalled = true;
                }
            } break;
            case eSaveTransfer_FileDataRetrieved:
                pClass->m_eSaveTransferState = eSaveTransfer_LoadSaveFromDisc;
                break;
            case eSaveTransfer_LoadSaveFromDisc: {
                if (pClass->m_saveTransferDownloadCancelled) {
                    pClass->m_eSaveTransferState = eSaveTransfer_Error;
                    break;
                }

                PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();
                int saveInfoIndex = -1;
                for (int i = 0; i < pSaveDetails->iSaveC; i++) {
                    if (strcmp(pSaveDetails->SaveInfoA[i].UTF8SaveFilename,
                               pClass->m_downloadedUniqueFilename) == 0) {
                        // found it
                        saveInfoIndex = i;
                    }
                }
                if (saveInfoIndex == -1) {
                    pClass->m_eSaveTransferState = eSaveTransfer_Error;
                    app.DebugPrintf(
                        "CrossSaveGetSavesInfoCallback failed - couldn't find "
                        "save\n");
                } else {
                    IPlatformStorage::ESaveGameState eLoadStatus =
                        PlatformStorage.LoadSaveData(
                            &pSaveDetails->SaveInfoA[saveInfoIndex],
                            [pClass](const bool bIsCorrupt,
                                     const bool bIsOwner) {
                                return pClass->loadCrossSaveDataCallback(
                                    bIsCorrupt, bIsOwner);
                            });
                    if (eLoadStatus == IPlatformStorage::ESaveGame_Load) {
                        pClass->m_eSaveTransferState =
                            eSaveTransfer_LoadingSaveFromDisc;
                    } else {
                        pClass->m_eSaveTransferState = eSaveTransfer_Error;
                    }
                }
            } break;
            case eSaveTransfer_LoadingSaveFromDisc:

                break;
            case eSaveTransfer_CreatingNewSave: {
                unsigned int fileSize = PlatformStorage.GetSaveSize();
                std::vector<uint8_t> ba(fileSize);
                PlatformStorage.GetSaveData(ba.data(), &fileSize);
                assert(ba.size() == fileSize);

                PlatformStorage.ResetSaveData();
                {
                    std::uint8_t* pbThumbnailData = nullptr;
                    unsigned int dwThumbnailDataSize = 0;

                    std::uint8_t* pbDataSaveImage = nullptr;
                    unsigned int dwDataSizeSaveImage = 0;

                    PlatformStorage.GetDefaultSaveImage(
                        &pbDataSaveImage,
                        &dwDataSizeSaveImage);  // Get the default save
                                                // thumbnail (as set by
                                                // SetDefaultImages) for use on
                                                // saving games t
                    PlatformStorage.GetDefaultSaveThumbnail(
                        &pbThumbnailData,
                        &dwThumbnailDataSize);  // Get the default save image
                                                // (as set by SetDefaultImages)
                                                // for use on saving games that

                    std::uint8_t bTextMetadata[88];
                    memset(bTextMetadata, 0, 88);
                    unsigned int remoteHostOptions =
                        app.getRemoteStorage()->getSaveHostOptions();
                    app.SetGameHostOption(eGameHostOption_All,
                                          remoteHostOptions);
                    int iTextMetadataBytes = app.CreateImageTextData(
                        bTextMetadata, app.getRemoteStorage()->getSaveSeed(),
                        true, remoteHostOptions,
                        app.getRemoteStorage()->getSaveTexturePack());

                    // set the icon and save image
                    PlatformStorage.SetSaveImages(
                        pbThumbnailData, dwThumbnailDataSize, pbDataSaveImage,
                        dwDataSizeSaveImage, bTextMetadata, iTextMetadataBytes);
                }

#if defined(SPLIT_SAVES)
                ConsoleSaveFileOriginal oldFormatSave(
                    wSaveName, ba.data(), ba.size(), false,
                    app.getRemoteStorage()->getSavePlatform());
                pSave = new ConsoleSaveFileSplit(&oldFormatSave, false,
                                                 pMinecraft->progressRenderer);

                pMinecraft->progressRenderer->progressStage(
                    IDS_SAVETRANSFER_STAGE_SAVING);
                pSave->Flush(false, false);
                pClass->m_eSaveTransferState = eSaveTransfer_Saving;
#else
                pSave = new ConsoleSaveFileOriginal(
                    wSaveName, ba.data(), ba.size(), false,
                    app.getRemoteStorage()->getSavePlatform());
                pClass->m_eSaveTransferState = eSaveTransfer_Converting;
                pMinecraft->progressRenderer->progressStage(
                    IDS_SAVETRANSFER_STAGE_CONVERTING);
#endif
            } break;
            case eSaveTransfer_Converting: {
                pSave->ConvertToLocalPlatform();  // check if we need to convert
                                                  // this file from PS3->PS4
                pClass->m_eSaveTransferState = eSaveTransfer_Saving;
                pMinecraft->progressRenderer->progressStage(
                    IDS_SAVETRANSFER_STAGE_SAVING);
                PlatformStorage.SetSaveTitle(wSaveName);
                PlatformStorage.SetSaveUniqueFilename(
                    pClass->m_downloadedUniqueFilename);

                app.getRemoteStorage()
                    ->waitForPlatformStorageIdle();  // we need to wait for the
                                                     // save system to be idle
                                                     // here, as Flush doesn't
                                                     // check for it.
                pSave->Flush(false, false);
            } break;
            case eSaveTransfer_Saving: {
                // On Durango/Orbis, we need to wait for all the asynchronous
                // saving processes to complete before destroying the levels, as
                // that will ultimately delete the directory level storage &
                // therefore the ConsoleSaveSplit instance, which needs to be
                // around until all the sub files have completed saving.

                delete pSave;

                pMinecraft->progressRenderer->progressStage(
                    IDS_PROGRESS_SAVING_TO_DISC);
                pClass->m_eSaveTransferState = eSaveTransfer_Succeeded;
            } break;

            case eSaveTransfer_Succeeded: {
                // if we've arrived here, the save has been created successfully
                pClass->m_iState = e_SavesRepopulate;
                pClass->updateTooltips();
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                app.getRemoteStorage()
                    ->waitForPlatformStorageIdle();  // wait for everything to
                                                     // complete before we hand
                                                     // control back to the
                                                     // player
                ui.RequestErrorMessage(IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD,
                                       IDS_SAVE_TRANSFER_DOWNLOADCOMPLETE,
                                       uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad(),
                                       CrossSaveFinishedCallback, pClass);
                pClass->m_eSaveTransferState = eSaveTransfer_Finished;
            } break;

            case eSaveTransfer_Cancelled:  // this is no longer used
            {
                assert(0);  // pClass->m_eSaveTransferState =
                            // eSaveTransfer_Idle;
            } break;
            case eSaveTransfer_Error: {
                if (bSaveFileCreated) {
                    if (pClass->m_saveTransferDownloadCancelled) {
                        char wcTemp[256];
                        snprintf(wcTemp, 256,
                                 app.GetString(
                                     IDS_CANCEL));  // MGH - should change this
                                                    // string to "cancelling
                                                    // download"
                        m_wstrStageText = wcTemp;
                        pMinecraft->progressRenderer->progressStage(
                            m_wstrStageText);
                        pMinecraft->progressRenderer->progressStage(
                            m_wstrStageText);
                    }
                    // if the save file has already been created we have to
                    // delete it again if there's been an error
                    PSAVE_DETAILS pSaveDetails =
                        PlatformStorage.ReturnSavesInfo();
                    int saveInfoIndex = -1;
                    for (int i = 0; i < pSaveDetails->iSaveC; i++) {
                        if (strcmp(pSaveDetails->SaveInfoA[i].UTF8SaveFilename,
                                   pClass->m_downloadedUniqueFilename) == 0) {
                            // found it
                            saveInfoIndex = i;
                        }
                    }
                    if (saveInfoIndex == -1) {
                        app.DebugPrintf(
                            "eSaveTransfer_Error failed - couldn't find "
                            "save\n");
                        assert(0);
                        pClass->m_eSaveTransferState =
                            eSaveTransfer_ErrorMesssage;
                    } else {
                        // delete the save file
                        app.getRemoteStorage()->waitForPlatformStorageIdle();
                        IPlatformStorage::ESaveGameState eDeleteStatus =
                            PlatformStorage.DeleteSaveData(
                                &pSaveDetails->SaveInfoA[saveInfoIndex],
                                [pClass](const bool bRes) {
                                    return pClass
                                        ->crossSaveDeleteOnErrorReturned(bRes);
                                });
                        if (eDeleteStatus ==
                            IPlatformStorage::ESaveGame_Delete) {
                            pClass->m_eSaveTransferState =
                                eSaveTransfer_ErrorDeletingSave;
                        } else {
                            app.DebugPrintf(
                                "PlatformStorage.DeleteSaveData failed!!\n");
                            pClass->m_eSaveTransferState =
                                eSaveTransfer_ErrorMesssage;
                        }
                    }
                } else {
                    pClass->m_eSaveTransferState = eSaveTransfer_ErrorMesssage;
                }
            } break;

            case eSaveTransfer_ErrorDeletingSave:
                break;
            case eSaveTransfer_ErrorMesssage: {
                app.getRemoteStorage()
                    ->waitForPlatformStorageIdle();  // wait for everything to
                                                     // complete before we hand
                                                     // control back to the
                                                     // player
                if (pClass->m_saveTransferDownloadCancelled) {
                    pClass->m_eSaveTransferState = eSaveTransfer_Idle;
                } else {
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    uint32_t errorMessage = IDS_SAVE_TRANSFER_DOWNLOADFAILED;
                    if (!PlatformProfile.IsSignedInLive(
                            PlatformProfile.GetPrimaryPad())) {
                        errorMessage =
                            IDS_ERROR_NETWORK;  // show "A network error has
                                                // occurred."
#if defined(__VITA__)
                        if (!PlatformProfile.IsSignedInPSN(
                                PlatformProfile.GetPrimaryPad())) {
                            errorMessage =
                                IDS_PRO_NOTONLINE_TEXT;  // show "not signed
                                                         // into PSN"
                        }
#endif
                    }
                    ui.RequestErrorMessage(IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD,
                                           errorMessage, uiIDA, 1,
                                           PlatformProfile.GetPrimaryPad(),
                                           CrossSaveFinishedCallback, pClass);
                    pClass->m_eSaveTransferState = eSaveTransfer_Finished;
                }
                if (bSaveFileCreated)  // save file has been created, then
                                       // deleted.
                    pClass->m_iState = e_SavesRepopulateAfterDelete;
                else
                    pClass->m_iState = e_SavesRepopulate;
                pClass->updateTooltips();
            } break;
            case eSaveTransfer_Finished: {
            }
            // waiting to dismiss the dialog
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    m_bSaveTransferRunning = false;
    return 0;
}

void UIScene_LoadOrJoinMenu::SaveTransferReturned(void* lpParam,
                                                  SonyRemoteStorage::Status s,
                                                  int error_code) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)lpParam;

    if (s == SonyRemoteStorage::e_getDataSucceeded) {
        pClass->m_eSaveTransferState = eSaveTransfer_FileDataRetrieved;
    } else {
        pClass->m_eSaveTransferState = eSaveTransfer_Error;
        app.DebugPrintf(
            "SaveTransferReturned failed with error code : 0x%08x\n",
            error_code);
    }
}
ConsoleSaveFile* UIScene_LoadOrJoinMenu::SonyCrossSaveConvert() {
    return nullptr;
}

void UIScene_LoadOrJoinMenu::CancelSaveTransferCallback(void* lpParam) {
    UIScene_LoadOrJoinMenu* pClass = (UIScene_LoadOrJoinMenu*)lpParam;
    pClass->m_saveTransferDownloadCancelled = true;
    ui.SetTooltips(
        DEFAULT_XUI_MENU_USER, -1, -1, -1, -1, -1, -1, -1,
        -1);  // MGH -  added - remove the "cancel" tooltip, so the player knows
              // it's underway (really needs a "cancelling" message)
}

#endif
