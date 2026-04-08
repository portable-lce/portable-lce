
#include "UIScene_LoadMenu.h"

#include <string.h>
#include <wchar.h>

#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "minecraft/GameHostOptions.h"
#include "minecraft/GameTypes.h"
#include "minecraft/GameEnums.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_BitmapIcon.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/Scenes/Frontend Menu screens/IUIScene_StartGame.h"
#include "app/common/UI/UILayer.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "platform/NetTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/world/level/LevelSettings.h"
#include "strings.h"

#define GAME_CREATE_ONLINE_TIMER_ID 0
#define GAME_CREATE_ONLINE_TIMER_TIME 100
// 4J-PB - Only Xbox will not have trial DLC patched into the game

int UIScene_LoadMenu::m_iDifficultyTitleSettingA[4] = {
    IDS_DIFFICULTY_TITLE_PEACEFUL, IDS_DIFFICULTY_TITLE_EASY,
    IDS_DIFFICULTY_TITLE_NORMAL, IDS_DIFFICULTY_TITLE_HARD};

int UIScene_LoadMenu::loadSaveDataThumbnailReturned(
    std::uint8_t* pbThumbnail, unsigned int dwThumbnailBytes) {
    app.DebugPrintf("Received data for a thumbnail\n");

    if (pbThumbnail && dwThumbnailBytes) {
        registerSubstitutionTexture(m_thumbnailName, pbThumbnail,
                                    dwThumbnailBytes);

        m_pbThumbnailData = pbThumbnail;
        m_uiThumbnailSize = dwThumbnailBytes;
        m_bSaveThumbnailReady = true;
    } else {
        app.DebugPrintf("Thumbnail data is nullptr, or has size 0\n");
        m_bThumbnailGetFailed = true;
    }
    m_bRetrievingSaveThumbnail = false;

    return 0;
}

UIScene_LoadMenu::UIScene_LoadMenu(int iPad, void* initData,
                                   UILayer* parentLayer)
    : IUIScene_StartGame(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    LoadMenuInitData* params = (LoadMenuInitData*)initData;

    // m_labelGameName.init(app.GetString(IDS_WORLD_NAME));
    m_labelSeed.init("");
    m_labelCreatedMode.init(app.GetString(IDS_CREATED_IN_SURVIVAL));

    m_buttonGamemode.init(app.GetString(IDS_GAMEMODE_SURVIVAL),
                          eControl_GameMode);
    m_buttonMoreOptions.init(app.GetString(IDS_MORE_OPTIONS),
                             eControl_MoreOptions);
    m_buttonLoadWorld.init(app.GetString(IDS_LOAD), eControl_LoadWorld);
    m_texturePackList.init(app.GetString(IDS_DLC_MENU_TEXTUREPACKS),
                           eControl_TexturePackList);

    m_labelTexturePackName.init("");
    m_labelTexturePackDescription.init("");

    m_CurrentDifficulty = app.GetGameSettings(m_iPad, eGameSetting_Difficulty);
    char TempString[256];
    snprintf(TempString, 256, "%s: %s", app.GetString(IDS_SLIDER_DIFFICULTY),
             app.GetString(m_iDifficultyTitleSettingA[app.GetGameSettings(
                 m_iPad, eGameSetting_Difficulty)]));
    m_sliderDifficulty.init(
        TempString, eControl_Difficulty, 0, 3,
        app.GetGameSettings(m_iPad, eGameSetting_Difficulty));

    m_MoreOptionsParams.bGenerateOptions = false;
    m_MoreOptionsParams.bPVP = true;
    m_MoreOptionsParams.bTrust = true;
    m_MoreOptionsParams.bFireSpreads = true;
    m_MoreOptionsParams.bHostPrivileges = false;
    m_MoreOptionsParams.bTNT = true;
    m_MoreOptionsParams.iPad = iPad;

    m_iSaveGameInfoIndex = params->iSaveGameInfoIndex;
    m_levelGen = params->levelGen;

    m_bGameModeCreative = false;
    m_iGameModeId = GameType::SURVIVAL->getId();
    m_bHasBeenInCreative = false;
    m_bIsSaveOwner = true;

    m_bSaveThumbnailReady = false;
    m_bRetrievingSaveThumbnail = true;
    m_bShowTimer = false;
    m_pDLCPack = nullptr;
    m_bAvailableTexturePacksChecked = false;
    m_bRequestQuadrantSignin = false;
    m_iTexturePacksNotInstalled = 0;
    m_bRebuildTouchBoxes = false;
    m_bThumbnailGetFailed = false;
    m_seed = 0;
    m_bIsCorrupt = false;

    m_bMultiplayerAllowed = PlatformProfile.IsSignedInLive(m_iPad) &&
                            PlatformProfile.AllowedToPlayMultiplayer(m_iPad);
    // 4J-PB - read the settings for the online flag. We'll only save this
    // setting if the user changed it.
    bool bGameSetting_Online =
        (app.GetGameSettings(m_iPad, eGameSetting_Online) != 0);
    m_MoreOptionsParams.bOnlineSettingChangedBySystem = false;

    // Set the text for friends of friends, and default to on
    if (m_bMultiplayerAllowed) {
        m_MoreOptionsParams.bOnlineGame = bGameSetting_Online;
        if (bGameSetting_Online) {
            m_MoreOptionsParams.bInviteOnly =
                app.GetGameSettings(m_iPad, eGameSetting_InviteOnly) != 0;
            m_MoreOptionsParams.bAllowFriendsOfFriends =
                app.GetGameSettings(m_iPad, eGameSetting_FriendsOfFriends) != 0;
        } else {
            m_MoreOptionsParams.bInviteOnly = false;
            m_MoreOptionsParams.bAllowFriendsOfFriends = false;
        }
    } else {
        m_MoreOptionsParams.bOnlineGame = false;
        m_MoreOptionsParams.bInviteOnly = false;
        m_MoreOptionsParams.bAllowFriendsOfFriends = false;
        if (bGameSetting_Online) {
            // The profile settings say Online, but either the player is
            // offline, or they are not allowed to play online
            m_MoreOptionsParams.bOnlineSettingChangedBySystem = true;
        }
    }

    // Set up online game checkbox
    bool bOnlineGame = m_MoreOptionsParams.bOnlineGame;
    m_checkboxOnline.SetEnable(true);

    // 4J-PB - to stop an offline game being able to select the online flag
    if (PlatformProfile.IsSignedInLive(m_iPad) == false) {
        m_checkboxOnline.SetEnable(false);
    }

    if (m_MoreOptionsParams.bOnlineSettingChangedBySystem) {
        m_checkboxOnline.SetEnable(false);
        bOnlineGame = false;
    }

    m_checkboxOnline.init(app.GetString(IDS_ONLINE_GAME), eControl_OnlineGame,
                          bOnlineGame);

    // Level gen
    if (m_levelGen) {
        m_labelGameName.init(m_levelGen->getDisplayName());
        if (m_levelGen->requiresTexturePack()) {
            m_MoreOptionsParams.dwTexturePack =
                m_levelGen->getRequiredTexturePackId();

            m_texturePackList.setEnabled(false);

            // retrieve the save icon from the texture pack, if there is one
            TexturePack* tp =
                Minecraft::GetInstance()->skins->getTexturePackById(
                    m_MoreOptionsParams.dwTexturePack);
            std::uint32_t imageBytes = 0;
            std::uint8_t* imageData = tp->getPackIcon(imageBytes);

            if (imageBytes > 0 && imageData) {
                char textureName[64];
                snprintf(textureName, 64, "loadsave");
                registerSubstitutionTexture(textureName, imageData, imageBytes);
                m_bitmapIcon.setTextureName(textureName);
            }
        }
        // Set this level as created in creative mode, so that people can't use
        // the themed worlds as an easy way to get achievements
        m_bHasBeenInCreative = m_levelGen->getLevelHasBeenInCreative();
        if (m_bHasBeenInCreative) {
            m_labelCreatedMode.setLabel(app.GetString(IDS_CREATED_IN_CREATIVE));
        } else {
            m_labelCreatedMode.setLabel(app.GetString(IDS_CREATED_IN_SURVIVAL));
        }
    } else {
    }

    m_iTexturePacksNotInstalled = 0;

    // block input if we're waiting for DLC to install, and wipe the saves list.
    // The end of dlc mounting custom message will fill the list again
    if (app.StartInstallDLCProcess(m_iPad) == true) {
        // not doing a mount, so enable input
        m_bIgnoreInput = true;
    } else {
        m_bIgnoreInput = false;

        Minecraft* pMinecraft = Minecraft::GetInstance();
        int texturePacksCount = pMinecraft->skins->getTexturePackCount();
        for (unsigned int i = 0; i < texturePacksCount; ++i) {
            TexturePack* tp = pMinecraft->skins->getTexturePackByIndex(i);

            std::uint32_t imageBytes = 0;
            std::uint8_t* imageData = tp->getPackIcon(imageBytes);

            if (imageBytes > 0 && imageData) {
                char imageName[64];
                snprintf(imageName, 64, "tpack%08x", tp->getId());
                registerSubstitutionTexture(imageName, imageData, imageBytes);
                m_texturePackList.addPack(i, imageName);
            }
        }
        m_currentTexturePackIndex = pMinecraft->skins->getTexturePackIndex(
            m_MoreOptionsParams.dwTexturePack);
        UpdateTexturePackDescription(m_currentTexturePackIndex);
        m_texturePackList.selectSlot(m_currentTexturePackIndex);

        // 4J-PB - Only Xbox will not have trial DLC patched into the game
    }

    if (params) delete params;
    addTimer(GAME_CREATE_ONLINE_TIMER_ID, GAME_CREATE_ONLINE_TIMER_TIME);
}

void UIScene_LoadMenu::updateTooltips() {
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT,
                   IDS_TOOLTIPS_BACK, -1, -1);
}

void UIScene_LoadMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);

    if (PlatformRenderer.IsWidescreen()) {
        m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
    } else {
        m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
    }
}

std::string UIScene_LoadMenu::getMoviePath() { return "LoadMenu"; }

UIControl* UIScene_LoadMenu::GetMainPanel() { return &m_controlMainPanel; }

void UIScene_LoadMenu::tick() {
    if (m_bShowTimer) {
        m_bShowTimer = false;
        ui.NavigateToScene(m_iPad, eUIScene_Timer);
    }

    if (m_bThumbnailGetFailed) {
        // On Durango, this can happen if a save is still not been synchronised
        // (user cancelled, or some error). Return back to give them a choice to
        // pick another save.
        ui.NavigateBack(m_iPad, false, eUIScene_LoadOrJoinMenu);
        return;
    }

    if (m_bSaveThumbnailReady) {
        m_bSaveThumbnailReady = false;

        m_bitmapIcon.setTextureName(m_thumbnailName.c_str());

        // retrieve the seed value from the image metadata
        bool bHostOptionsRead = false;
        unsigned int uiHostOptions = 0;

        char szSeed[50];
        memset(szSeed, 0, 50);
        app.GetImageTextData(
            m_pbThumbnailData, m_uiThumbnailSize, (unsigned char*)&szSeed,
            uiHostOptions, bHostOptionsRead, m_MoreOptionsParams.dwTexturePack);

        // #ifdef _DEBUG
        // 			// dump out the thumbnail
        // 			void* hThumbnail =
        // CreateFile("GAME:\\thumbnail.png", GENERIC_WRITE, 0, nullptr,
        // OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, nullptr);
        // uint32_t dwBytes;
        // 			WriteFile(hThumbnail,pbImageData,dwImageBytes,&dwBytes,nullptr);
        // 			XCloseHandle(hThumbnail);
        // #endif

        if (szSeed[0] != 0) {
            char TempString[256];
            snprintf(TempString, 256, "%s: %hs", app.GetString(IDS_SEED),
                     szSeed);
            m_labelSeed.setLabel(TempString);
        } else {
            m_labelSeed.setLabel("");
        }

        // Setup all the text and checkboxes to match what the game was saved
        // with on
        if (bHostOptionsRead) {
            m_MoreOptionsParams.bPVP =
                app.GetGameHostOption(uiHostOptions, eGameHostOption_PvP) > 0;
            m_MoreOptionsParams.bTrust =
                app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_TrustPlayers) > 0;
            m_MoreOptionsParams.bFireSpreads =
                app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_FireSpreads) > 0;
            m_MoreOptionsParams.bTNT =
                app.GetGameHostOption(uiHostOptions, eGameHostOption_TNT) > 0;
            m_MoreOptionsParams.bHostPrivileges =
                app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_CheatsEnabled) > 0;
            m_MoreOptionsParams.bDisableSaving =
                app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_DisableSaving) > 0;
            m_MoreOptionsParams.currentWorldSize =
                (EGameHostOptionWorldSize)app.GetGameHostOption(
                    uiHostOptions, eGameHostOption_WorldSize);
            m_MoreOptionsParams.newWorldSize =
                m_MoreOptionsParams.currentWorldSize;

            m_MoreOptionsParams.bMobGriefing = app.GetGameHostOption(
                uiHostOptions, eGameHostOption_MobGriefing);
            m_MoreOptionsParams.bKeepInventory = app.GetGameHostOption(
                uiHostOptions, eGameHostOption_KeepInventory);
            m_MoreOptionsParams.bDoMobSpawning = app.GetGameHostOption(
                uiHostOptions, eGameHostOption_DoMobSpawning);
            m_MoreOptionsParams.bDoMobLoot =
                app.GetGameHostOption(uiHostOptions, eGameHostOption_DoMobLoot);
            m_MoreOptionsParams.bDoTileDrops = app.GetGameHostOption(
                uiHostOptions, eGameHostOption_DoTileDrops);
            m_MoreOptionsParams.bNaturalRegeneration = app.GetGameHostOption(
                uiHostOptions, eGameHostOption_NaturalRegeneration);
            m_MoreOptionsParams.bDoDaylightCycle = app.GetGameHostOption(
                uiHostOptions, eGameHostOption_DoDaylightCycle);

            bool cheatsOn = m_MoreOptionsParams.bHostPrivileges;
            if (!cheatsOn) {
                // Set defaults
                m_MoreOptionsParams.bMobGriefing = true;
                m_MoreOptionsParams.bKeepInventory = false;
                m_MoreOptionsParams.bDoMobSpawning = true;
                m_MoreOptionsParams.bDoDaylightCycle = true;
            }

            // turn off creative mode on the save
            // #ifdef _DEBUG
            //  			uiHostOptions&=~GAME_HOST_OPTION_BITMASK_BEENINCREATIVE;
            //  			app.SetGameHostOption(eGameHostOption_HasBeenInCreative,
            //  0);
            // #endif

            if (app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_WasntSaveOwner) > 0) {
                m_bIsSaveOwner = false;
            }

            m_bHasBeenInCreative =
                app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_HasBeenInCreative) > 0;
            if (app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_HasBeenInCreative) > 0) {
                m_labelCreatedMode.setLabel(
                    app.GetString(IDS_CREATED_IN_CREATIVE));
            } else {
                m_labelCreatedMode.setLabel(
                    app.GetString(IDS_CREATED_IN_SURVIVAL));
            }

            switch (app.GetGameHostOption(uiHostOptions,
                                          eGameHostOption_GameType)) {
                case 1:  // Creative
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_CREATIVE));
                    m_bGameModeCreative = true;
                    m_iGameModeId = GameType::CREATIVE->getId();
                    break;
#if defined(_ADVENTURE_MODE_ENABLED)
                case 2:  // Adventure
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_ADVENTURE));
                    m_bGameModeCreative = false;
                    m_iGameModeId = GameType::ADVENTURE->getId();
                    break;
#endif
                case 0:  // Survival
                default:
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_SURVIVAL));
                    m_bGameModeCreative = false;
                    m_iGameModeId = GameType::SURVIVAL->getId();
                    break;
            };

            bool bGameSetting_Online =
                (app.GetGameSettings(m_iPad, eGameSetting_Online) != 0);
            if (app.GetGameHostOption(uiHostOptions,
                                      eGameHostOption_FriendsOfFriends) &&
                !(m_bMultiplayerAllowed && bGameSetting_Online)) {
                m_MoreOptionsParams.bAllowFriendsOfFriends = true;
            }
        }

        Minecraft* pMinecraft = Minecraft::GetInstance();
        m_currentTexturePackIndex = pMinecraft->skins->getTexturePackIndex(
            m_MoreOptionsParams.dwTexturePack);

        UpdateTexturePackDescription(m_currentTexturePackIndex);

        m_texturePackList.selectSlot(m_currentTexturePackIndex);

        // m_labelGameName.setLabel(m_XContentData.szDisplayName);

        ui.NavigateBack(m_iPad, false, getSceneType());
    }

    if (m_iSetTexturePackDescription >= 0) {
        UpdateTexturePackDescription(m_iSetTexturePackDescription);
        m_iSetTexturePackDescription = -1;
    }
    if (m_bShowTexturePackDescription) {
        slideLeft();
        m_texturePackDescDisplayed = true;

        m_bShowTexturePackDescription = false;
    }

    if (m_bRequestQuadrantSignin) {
        m_bRequestQuadrantSignin = false;
        SignInInfo info;
        info.Func = [this](bool bContinue, int pad) {
            return StartGame_SignInReturned(this, bContinue, pad);
        };
        info.requireOnline = m_MoreOptionsParams.bOnlineGame;
        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                           eUIScene_QuadrantSignin, &info);
    }

    UIScene::tick();
}

void UIScene_LoadMenu::handleInput(int iPad, int key, bool repeat, bool pressed,
                                   bool released, bool& handled) {
    if (m_bIgnoreInput) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                app.SetCorruptSaveDeleted(false);
                navigateBack();
                handled = true;
            }
            break;
        case ACTION_MENU_OK:

            // 4J-JEV: Inform user why their game must be offline.

        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT:
        case ACTION_MENU_OTHER_STICK_UP:
        case ACTION_MENU_OTHER_STICK_DOWN:
            sendInputToMovie(key, repeat, pressed, released);

            bool bOnlineGame = m_checkboxOnline.IsChecked();
            if (m_MoreOptionsParams.bOnlineGame != bOnlineGame) {
                m_MoreOptionsParams.bOnlineGame = bOnlineGame;

                if (!m_MoreOptionsParams.bOnlineGame) {
                    m_MoreOptionsParams.bInviteOnly = false;
                    m_MoreOptionsParams.bAllowFriendsOfFriends = false;
                }
            }

            handled = true;
            break;
    }
}

void UIScene_LoadMenu::handlePress(F64 controlId, F64 childId) {
    if (m_bIgnoreInput) return;

    // CD - Added for audio
    ui.PlayUISFX(eSFX_Press);

    switch ((int)controlId) {
        case eControl_GameMode:
            switch (m_iGameModeId) {
                case 0:  // Survival
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_CREATIVE));
                    m_iGameModeId = GameType::CREATIVE->getId();
                    m_bGameModeCreative = true;
                    break;
                case 1:  // Creative
#if defined(_ADVENTURE_MODE_ENABLED)
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_ADVENTURE));
                    m_iGameModeId = GameType::ADVENTURE->getId();
                    m_bGameModeCreative = false;
                    break;
                case 2:  // Adventure
#endif
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_SURVIVAL));
                    m_iGameModeId = GameType::SURVIVAL->getId();
                    m_bGameModeCreative = false;
                    break;
            };
            break;
        case eControl_MoreOptions:
            ui.NavigateToScene(m_iPad, eUIScene_LaunchMoreOptionsMenu,
                               &m_MoreOptionsParams);
            break;
        case eControl_TexturePackList: {
            UpdateCurrentTexturePack((int)childId);
        } break;
        case eControl_LoadWorld: {
            {
                StartSharedLaunchFlow();
            }
        } break;
    };
}

void UIScene_LoadMenu::StartSharedLaunchFlow() {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    // Check if we need to upsell the texture pack
    if (m_MoreOptionsParams.dwTexturePack != 0) {
        // texture pack hasn't been set yet, so check what it will be
        TexturePack* pTexturePack = pMinecraft->skins->getTexturePackById(
            m_MoreOptionsParams.dwTexturePack);

        if (pTexturePack == nullptr) {
#if TO_BE_IMPLEMENTED
            // They've selected a texture pack they don't have yet
            // upsell
            CXuiCtrl4JList::LIST_ITEM_INFO ListItem;
            // get the current index of the list, and then get the data
            ListItem = m_pTexturePacksList->GetData(m_currentTexturePackIndex);

            // upsell the texture pack
            // tell sentient about the upsell of the full version of the skin
            // pack
            uint64_t ullOfferID_Full;
            app.GetDLCFullOfferIDForPackID(m_MoreOptionsParams.dwTexturePack,
                                           &ullOfferID_Full);
#endif

            unsigned int uiIDA[2];

            uiIDA[0] = IDS_TEXTUREPACK_FULLVERSION;
            // uiIDA[1]=IDS_TEXTURE_PACK_TRIALVERSION;
            uiIDA[1] = IDS_CONFIRM_CANCEL;

            // Give the player a warning about the texture pack missing
            ui.RequestAlertMessage(IDS_DLC_TEXTUREPACK_NOT_PRESENT_TITLE,
                                   IDS_DLC_TEXTUREPACK_NOT_PRESENT, uiIDA, 2,
                                   PlatformProfile.GetPrimaryPad(),
                                   &TexturePackDialogReturned, this);
            return;
        }
    }
    m_bIgnoreInput = true;

    // if the profile data has been changed, then force a profile write (we save
    // the online/invite/friends of friends settings) It seems we're allowed to
    // break the 5 minute rule if it's the result of a user action check the
    // checkboxes

    // Only save the online setting if the user changed it - we may change it
    // because we're offline, but don't want that saved
    if (!m_MoreOptionsParams.bOnlineSettingChangedBySystem) {
        app.SetGameSettings(m_iPad, eGameSetting_Online,
                            m_MoreOptionsParams.bOnlineGame ? 1 : 0);
    }
    app.SetGameSettings(m_iPad, eGameSetting_InviteOnly,
                        m_MoreOptionsParams.bInviteOnly ? 1 : 0);
    app.SetGameSettings(m_iPad, eGameSetting_FriendsOfFriends,
                        m_MoreOptionsParams.bAllowFriendsOfFriends ? 1 : 0);

    app.CheckGameSettingsChanged(true, m_iPad);

    // Check that we have the rights to use a texture pack we have selected.
    if (m_MoreOptionsParams.dwTexturePack != 0) {
        // texture pack hasn't been set yet, so check what it will be
        TexturePack* pTexturePack = pMinecraft->skins->getTexturePackById(
            m_MoreOptionsParams.dwTexturePack);
        DLCTexturePack* pDLCTexPack = (DLCTexturePack*)pTexturePack;
        m_pDLCPack = pDLCTexPack->getDLCInfoParentPack();

        // do we have a license?
        if (m_pDLCPack &&
            !m_pDLCPack->hasPurchasedFile(DLCManager::e_DLCType_Texture, "")) {
            // no

            // We need to allow people to use a trial texture pack if they are
            // offline - we only need them online if they want to buy it.

            /*
            unsigned int uiIDA[1];
            uiIDA[0]=IDS_OK;

            if(!PlatformProfile.IsSignedInLive(m_iPad))
            {
                    // need to be signed in to live
                    ui.RequestMessageBox(IDS_PRO_NOTONLINE_TITLE,
            IDS_PRO_NOTONLINE_TEXT, uiIDA, 1); m_bIgnoreInput = false;
                    return;
            }
            else */
            {
                // upsell

#if defined(_WINDOWS64)
                // trial pack warning
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestAlertMessage(IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE,
                                       IDS_USING_TRIAL_TEXUREPACK_WARNING,
                                       uiIDA, 1, m_iPad,
                                       &TrialTexturePackWarningReturned, this);
#endif

                return;
            }
        }
    }
    app.SetGameHostOption(eGameHostOption_WasntSaveOwner, (!m_bIsSaveOwner));

#if TO_BE_IMPLEMENTED
    // Reset the background downloading, in case we changed it by attempting to
    // download a texture pack
    XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_AUTO);
#endif

    // Check if they have the Reset Nether flag set, and confirm they want to do
    // this
    if (m_MoreOptionsParams.bResetNether) {
        unsigned int uiIDA[2];
        uiIDA[0] = IDS_DONT_RESET_NETHER;
        uiIDA[1] = IDS_RESET_NETHER;

        ui.RequestAlertMessage(
            IDS_RESETNETHER_TITLE, IDS_RESETNETHER_TEXT, uiIDA, 2, m_iPad,
            &UIScene_LoadMenu::CheckResetNetherReturned, this);
    } else {
        LaunchGame();
    }
}

void UIScene_LoadMenu::handleSliderMove(F64 sliderId, F64 currentValue) {
    char TempString[256];
    int value = (int)currentValue;
    switch ((int)sliderId) {
        case eControl_Difficulty:
            m_sliderDifficulty.handleSliderMove(value);

            app.SetGameSettings(m_iPad, eGameSetting_Difficulty, value);
            snprintf(TempString, 256, "%s: %s",
                     app.GetString(IDS_SLIDER_DIFFICULTY),
                     app.GetString(m_iDifficultyTitleSettingA[value]));
            m_sliderDifficulty.setLabel(TempString);
            break;
    }
}

void UIScene_LoadMenu::handleTouchBoxRebuild() { m_bRebuildTouchBoxes = true; }

void UIScene_LoadMenu::handleTimerComplete(int id) {
    switch (id) {
        case GAME_CREATE_ONLINE_TIMER_ID: {
            bool bMultiplayerAllowed =
                PlatformProfile.IsSignedInLive(m_iPad) &&
                PlatformProfile.AllowedToPlayMultiplayer(m_iPad);

            if (bMultiplayerAllowed != m_bMultiplayerAllowed) {
                if (bMultiplayerAllowed) {
                    bool bGameSetting_Online =
                        (app.GetGameSettings(m_iPad, eGameSetting_Online) != 0);
                    m_MoreOptionsParams.bOnlineGame =
                        bGameSetting_Online ? true : false;
                    if (bGameSetting_Online) {
                        m_MoreOptionsParams.bInviteOnly =
                            (app.GetGameSettings(m_iPad,
                                                 eGameSetting_InviteOnly) != 0)
                                ? true
                                : false;
                        m_MoreOptionsParams.bAllowFriendsOfFriends =
                            (app.GetGameSettings(
                                 m_iPad, eGameSetting_FriendsOfFriends) != 0)
                                ? true
                                : false;
                    } else {
                        m_MoreOptionsParams.bInviteOnly = false;
                        m_MoreOptionsParams.bAllowFriendsOfFriends = false;
                    }
                } else {
                    m_MoreOptionsParams.bOnlineGame = false;
                    m_MoreOptionsParams.bInviteOnly = false;
                    m_MoreOptionsParams.bAllowFriendsOfFriends = false;
                }

                m_checkboxOnline.SetEnable(bMultiplayerAllowed);
                m_checkboxOnline.setChecked(m_MoreOptionsParams.bOnlineGame);

                m_bMultiplayerAllowed = bMultiplayerAllowed;
            }
        } break;
            // 4J-PB - Only Xbox will not have trial DLC patched into the game
    }
}

void UIScene_LoadMenu::LaunchGame(void) {
    // stop the timer running that causes a check for new texture packs in TMS
    // but not installed, since this will run all through the load game, and
    // will crash if it tries to create an hbrush

    if ((m_bGameModeCreative == true || m_bHasBeenInCreative) ||
        m_MoreOptionsParams.bHostPrivileges == true) {
        unsigned int uiIDA[2];
        uiIDA[0] = IDS_CONFIRM_OK;
        uiIDA[1] = IDS_CONFIRM_CANCEL;
        if (m_bGameModeCreative == true || m_bHasBeenInCreative) {
            // 4J-PB - Need different text for Survival mode with a level that
            // has been saved in Creative
            if (!m_bGameModeCreative) {
                ui.RequestAlertMessage(
                    IDS_TITLE_START_GAME, IDS_CONFIRM_START_SAVEDINCREATIVE,
                    uiIDA, 2, m_iPad, &UIScene_LoadMenu::ConfirmLoadReturned,
                    this);
            } else  // it's creative mode
            {
                // has it previously been saved in creative?
                if (m_bHasBeenInCreative) {
                    // 4J-PB - We don't really need to tell the user this will
                    // have achievements disabled, since they already saved it
                    // in creative and they got the warning then inform them
                    // that leaderboard writes and achievements will be disabled
                    // ui.RequestMessageBox(IDS_TITLE_START_GAME,
                    // IDS_CONFIRM_START_SAVEDINCREATIVE_CONTINUE, uiIDA, 1,
                    // m_iPad,&CScene_LoadGameSettings::ConfirmLoadReturned,this,app.GetStringTable());

                    if (m_levelGen != nullptr) {
                        m_bIsCorrupt = false;
                        LoadDataComplete(this);
                    } else {
                        // set the save to load
                        PSAVE_DETAILS pSaveDetails =
                            PlatformStorage.ReturnSavesInfo();
                        app.DebugPrintf(
                            "Loading save s [%s]\n",
                            pSaveDetails->SaveInfoA[(int)m_iSaveGameInfoIndex]
                                .UTF8SaveTitle,
                            pSaveDetails->SaveInfoA[(int)m_iSaveGameInfoIndex]
                                .UTF8SaveFilename);
                        IPlatformStorage::ESaveGameState eLoadStatus =
                            PlatformStorage.LoadSaveData(
                                &pSaveDetails
                                     ->SaveInfoA[(int)m_iSaveGameInfoIndex],
                                [this](bool bCorrupt, bool bOwner) {
                                    return loadSaveDataReturned(bCorrupt, bOwner);
                                });

#if TO_BE_IMPLEMENTED
                        if (eLoadStatus ==
                            IPlatformStorage::ELoadGame_DeviceRemoved) {
                            // disable saving
                            PlatformStorage.SetSaveDisabled(true);
                            PlatformStorage.SetSaveDeviceSelected(m_iPad, false);
                            unsigned int uiIDA[1];
                            uiIDA[0] = IDS_OK;
                            ui.RequestErrorMessage(
                                IDS_STORAGEDEVICEPROBLEM_TITLE,
                                IDS_FAILED_TO_LOADSAVE_TEXT, uiIDA, 1, m_iPad,
                                &CScene_LoadGameSettings::
                                    DeviceRemovedDialogReturned,
                                this);
                        }
#endif
                    }
                } else {
                    // ask if they're sure they want to turn this into a
                    // creative map
                    ui.RequestAlertMessage(
                        IDS_TITLE_START_GAME, IDS_CONFIRM_START_CREATIVE, uiIDA,
                        2, m_iPad, &UIScene_LoadMenu::ConfirmLoadReturned,
                        this);
                }
            }
        } else {
            ui.RequestAlertMessage(
                IDS_TITLE_START_GAME, IDS_CONFIRM_START_HOST_PRIVILEGES, uiIDA,
                2, m_iPad, &UIScene_LoadMenu::ConfirmLoadReturned, this);
        }
    } else {
        if (m_levelGen != nullptr) {
            m_bIsCorrupt = false;
            LoadDataComplete(this);
        } else {
            // set the save to load
            PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();
            app.DebugPrintf("Loading save %s [%s]\n",
                            pSaveDetails->SaveInfoA[(int)m_iSaveGameInfoIndex]
                                .UTF8SaveTitle,
                            pSaveDetails->SaveInfoA[(int)m_iSaveGameInfoIndex]
                                .UTF8SaveFilename);
            IPlatformStorage::ESaveGameState eLoadStatus =
                PlatformStorage.LoadSaveData(
                    &pSaveDetails->SaveInfoA[(int)m_iSaveGameInfoIndex],
                    [this](bool bCorrupt, bool bOwner) {
                        return loadSaveDataReturned(bCorrupt, bOwner);
                    });

#if TO_BE_IMPLEMENTED
            if (eLoadStatus == IPlatformStorage::ELoadGame_DeviceRemoved) {
                // disable saving
                PlatformStorage.SetSaveDisabled(true);
                PlatformStorage.SetSaveDeviceSelected(m_iPad, false);
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_OK;
                ui.RequestErrorMessage(
                    IDS_STORAGEDEVICEPROBLEM_TITLE, IDS_FAILED_TO_LOADSAVE_TEXT,
                    uiIDA, 1, m_iPad,
                    &CScene_LoadGameSettings::DeviceRemovedDialogReturned,
                    this);
            }
#endif
        }
    }
    // return 0;
}

int UIScene_LoadMenu::CheckResetNetherReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadMenu* pClass = (UIScene_LoadMenu*)pParam;

    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        // continue and reset the nether
        pClass->LaunchGame();
    } else if (result == IPlatformStorage::EMessage_ResultAccept) {
        // turn off the reset nether and continue
        pClass->m_MoreOptionsParams.bResetNether = false;
        pClass->LaunchGame();
    } else {
        // else they chose cancel
        pClass->m_bIgnoreInput = false;
    }
    return 0;
}

int UIScene_LoadMenu::ConfirmLoadReturned(void* pParam, int iPad,
                                          IPlatformStorage::EMessageResult result) {
    UIScene_LoadMenu* pClass = (UIScene_LoadMenu*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
        if (pClass->m_levelGen != nullptr) {
            pClass->m_bIsCorrupt = false;
            pClass->LoadDataComplete(pClass);
        } else {
            // set the save to load
            PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();
            app.DebugPrintf(
                "Loading save %s [%s]\n",
                pSaveDetails->SaveInfoA[(int)pClass->m_iSaveGameInfoIndex]
                    .UTF8SaveTitle,
                pSaveDetails->SaveInfoA[(int)pClass->m_iSaveGameInfoIndex]
                    .UTF8SaveFilename);
            IPlatformStorage::ESaveGameState eLoadStatus =
                PlatformStorage.LoadSaveData(
                    &pSaveDetails->SaveInfoA[(int)pClass->m_iSaveGameInfoIndex],
                    [pClass](const bool bCorrupt, const bool bOwner) {
                        return pClass->loadSaveDataReturned(bCorrupt, bOwner);
                    });

#if TO_BE_IMPLEMENTED
            if (eLoadStatus == IPlatformStorage::ELoadGame_DeviceRemoved) {
                // disable saving
                PlatformStorage.SetSaveDisabled(true);
                PlatformStorage.SetSaveDeviceSelected(m_iPad, false);
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_OK;
                ui.RequestErrorMessage(
                    IDS_STORAGEDEVICEPROBLEM_TITLE, IDS_FAILED_TO_LOADSAVE_TEXT,
                    uiIDA, 1, m_iPad,
                    &CScene_LoadGameSettings::DeviceRemovedDialogReturned,
                    this);
            }
#endif
        }
    } else {
        pClass->m_bIgnoreInput = false;
    }
    return 0;
}

int UIScene_LoadMenu::LoadDataComplete(void* pParam) {
    UIScene_LoadMenu* pClass = (UIScene_LoadMenu*)pParam;

    if (!pClass->m_bIsCorrupt) {
        int iPrimaryPad = PlatformProfile.GetPrimaryPad();
        bool isSignedInLive = true;
        bool isOnlineGame = pClass->m_MoreOptionsParams.bOnlineGame;
        int iPadNotSignedInLive = -1;
        bool isLocalMultiplayerAvailable = app.IsLocalMultiplayerAvailable();

        for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
            if (PlatformProfile.IsSignedIn(i) &&
                ((i == iPrimaryPad) || isLocalMultiplayerAvailable)) {
                if (isSignedInLive && !PlatformProfile.IsSignedInLive(i)) {
                    // Record the first non signed in live pad
                    iPadNotSignedInLive = i;
                }

                isSignedInLive =
                    isSignedInLive && PlatformProfile.IsSignedInLive(i);
            }
        }

        // If this is an online game but not all players are signed in to Live,
        // stop!
        if (isOnlineGame && !isSignedInLive) {
            pClass->m_bIgnoreInput = false;
            unsigned int uiIDA[1];
            uiIDA[0] = IDS_CONFIRM_OK;
            ui.RequestAlertMessage(IDS_PRO_NOTONLINE_TITLE,
                                   IDS_PRO_NOTONLINE_TEXT, uiIDA, 1,
                                   PlatformProfile.GetPrimaryPad());
            return 0;
        }

        // Check if user-created content is allowed, as we cannot play
        // multiplayer if it's not
        bool noUGC = false;
        bool pccAllowed = true;
        bool pccFriendsAllowed = true;
        bool bContentRestricted = false;
        PlatformProfile.AllowedPlayerCreatedContent(
            PlatformProfile.GetPrimaryPad(), false, &pccAllowed,
            &pccFriendsAllowed);

        noUGC = !pccAllowed && !pccFriendsAllowed;

        if (!isOnlineGame || !isLocalMultiplayerAvailable) {
            if (isOnlineGame && noUGC) {
                pClass->setVisible(true);

                ui.RequestUGCMessageBox();

                pClass->m_bIgnoreInput = false;
            } else if (isOnlineGame && bContentRestricted) {
                pClass->setVisible(true);

                ui.RequestContentRestrictedMessageBox();
                pClass->m_bIgnoreInput = false;
            } else {
                int localUsersMask = CGameNetworkManager::GetLocalPlayerMask(
                    PlatformProfile.GetPrimaryPad());

                // No guest problems so we don't need to force a sign-in of
                // players here
                StartGameFromSave(pClass, localUsersMask);
            }
        } else {
            // 4J-PB not sure why we aren't checking the content restriction for
            // the main player here when multiple controllers are connected -
            // adding now
            if (isOnlineGame && noUGC) {
                pClass->setVisible(true);
                ui.RequestUGCMessageBox();
                pClass->m_bIgnoreInput = false;
            } else if (isOnlineGame && bContentRestricted) {
                pClass->setVisible(true);
                ui.RequestContentRestrictedMessageBox();
                pClass->m_bIgnoreInput = false;
            } else {
                pClass->m_bRequestQuadrantSignin = true;
            }
        }
    } else {
        // the save is corrupt!
        pClass->m_bIgnoreInput = false;

        // give the option to delete the save
        unsigned int uiIDA[2];
        uiIDA[0] = IDS_CONFIRM_CANCEL;
        uiIDA[1] = IDS_CONFIRM_OK;
        ui.RequestAlertMessage(
            IDS_CORRUPT_OR_DAMAGED_SAVE_TITLE, IDS_CORRUPT_OR_DAMAGED_SAVE_TEXT,
            uiIDA, 2, pClass->m_iPad,
            &UIScene_LoadMenu::DeleteSaveDialogReturned, pClass);
    }

    return 0;
}

int UIScene_LoadMenu::loadSaveDataReturned(bool bIsCorrupt, bool bIsOwner) {
    m_bIsCorrupt = bIsCorrupt;

    if (bIsOwner) {
        LoadDataComplete(this);
    } else {
        // messagebox
        m_bIgnoreInput = false;
    }

    return 0;
}

int UIScene_LoadMenu::TrophyDialogReturned(void* pParam, int iPad,
                                           IPlatformStorage::EMessageResult result) {
    UIScene_LoadMenu* pClass = (UIScene_LoadMenu*)pParam;
    return LoadDataComplete(pClass);
}

int UIScene_LoadMenu::DeleteSaveDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LoadMenu* pClass = (UIScene_LoadMenu*)pParam;

    // results switched for this dialog
    if (result == IPlatformStorage::EMessage_ResultDecline) {
        PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();
        PlatformStorage.DeleteSaveData(
            &pSaveDetails->SaveInfoA[(int)pClass->m_iSaveGameInfoIndex],
            [pClass](const bool bSuccess) {
                return pClass->deleteSaveDataReturned(bSuccess);
            });
    } else {
        pClass->m_bIgnoreInput = false;
    }
    return 0;
}

int UIScene_LoadMenu::deleteSaveDataReturned(bool bSuccess) {
    app.SetCorruptSaveDeleted(true);
    navigateBack();

    return 0;
}

// 4J Stu - Shared functionality that is the same whether we needed a quadrant
// sign-in or not
void UIScene_LoadMenu::StartGameFromSave(UIScene_LoadMenu* pClass,
                                         int localUsersMask) {
    if (pClass->m_levelGen == nullptr) {
        int32_t saveOrCheckpointId = 0;
        bool validSave =
            PlatformStorage.GetSaveUniqueNumber(&saveOrCheckpointId);
    } else {
        PlatformStorage.ResetSaveData();
        // Make our next save default to the name of the level
        PlatformStorage.SetSaveTitle(
            pClass->m_levelGen->getDefaultSaveName().c_str());
    }

    bool isClientSide =
        PlatformProfile.IsSignedInLive(PlatformProfile.GetPrimaryPad()) &&
        pClass->m_MoreOptionsParams.bOnlineGame;

    bool isPrivate =
        (app.GetGameSettings(pClass->m_iPad, eGameSetting_InviteOnly) > 0)
            ? true
            : false;

    PSAVE_DETAILS pSaveDetails = PlatformStorage.ReturnSavesInfo();

    NetworkGameInitData* param = new NetworkGameInitData();
    param->seed = pClass->m_seed;
    param->saveData = nullptr;
    param->levelGen = pClass->m_levelGen;
    param->texturePackId = pClass->m_MoreOptionsParams.dwTexturePack;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    pMinecraft->skins->selectTexturePackById(
        pClass->m_MoreOptionsParams.dwTexturePack);
    // pMinecraft->skins->updateUI();

    app.SetGameHostOption(eGameHostOption_Difficulty,
                          Minecraft::GetInstance()->options->difficulty);
    app.SetGameHostOption(
        eGameHostOption_FriendsOfFriends,
        app.GetGameSettings(pClass->m_iPad, eGameSetting_FriendsOfFriends));
    app.SetGameHostOption(
        eGameHostOption_Gamertags,
        app.GetGameSettings(pClass->m_iPad, eGameSetting_GamertagsVisible));

    app.SetGameHostOption(
        eGameHostOption_BedrockFog,
        app.GetGameSettings(pClass->m_iPad, eGameSetting_BedrockFog) ? 1 : 0);

    app.SetGameHostOption(eGameHostOption_PvP,
                          pClass->m_MoreOptionsParams.bPVP);
    app.SetGameHostOption(eGameHostOption_TrustPlayers,
                          pClass->m_MoreOptionsParams.bTrust);
    app.SetGameHostOption(eGameHostOption_FireSpreads,
                          pClass->m_MoreOptionsParams.bFireSpreads);
    app.SetGameHostOption(eGameHostOption_TNT,
                          pClass->m_MoreOptionsParams.bTNT);
    app.SetGameHostOption(eGameHostOption_HostCanFly,
                          pClass->m_MoreOptionsParams.bHostPrivileges);
    app.SetGameHostOption(eGameHostOption_HostCanChangeHunger,
                          pClass->m_MoreOptionsParams.bHostPrivileges);
    app.SetGameHostOption(eGameHostOption_HostCanBeInvisible,
                          pClass->m_MoreOptionsParams.bHostPrivileges);

    app.SetGameHostOption(eGameHostOption_MobGriefing,
                          pClass->m_MoreOptionsParams.bMobGriefing);
    app.SetGameHostOption(eGameHostOption_KeepInventory,
                          pClass->m_MoreOptionsParams.bKeepInventory);
    app.SetGameHostOption(eGameHostOption_DoMobSpawning,
                          pClass->m_MoreOptionsParams.bDoMobSpawning);
    app.SetGameHostOption(eGameHostOption_DoMobLoot,
                          pClass->m_MoreOptionsParams.bDoMobLoot);
    app.SetGameHostOption(eGameHostOption_DoTileDrops,
                          pClass->m_MoreOptionsParams.bDoTileDrops);
    app.SetGameHostOption(eGameHostOption_NaturalRegeneration,
                          pClass->m_MoreOptionsParams.bNaturalRegeneration);
    app.SetGameHostOption(eGameHostOption_DoDaylightCycle,
                          pClass->m_MoreOptionsParams.bDoDaylightCycle);

#if defined(_LARGE_WORLDS)
    app.SetGameHostOption(eGameHostOption_WorldSize,
                          pClass->m_MoreOptionsParams.worldSize +
                              1);  // 0 is GAME_HOST_OPTION_WORLDSIZE_UNKNOWN
#endif
    // 	app.SetGameNewWorldSize(64, true );
    //	app.SetGameNewWorldSize(0, false );

    // flag if the user wants to reset the Nether to force a Fortress with
    // netherwart etc.
    app.SetResetNether(
        (pClass->m_MoreOptionsParams.bResetNether == true) ? true : false);
    // clear out the app's terrain features list
    app.ClearTerrainFeaturePosition();

    app.SetGameHostOption(eGameHostOption_GameType, pClass->m_iGameModeId);

    g_NetworkManager.HostGame(localUsersMask, isClientSide, isPrivate,
                              MINECRAFT_NET_MAX_PLAYERS, 0);

    param->settings = app.GetGameHostOption(eGameHostOption_All);

    g_NetworkManager.FakeLocalPlayerJoined();

    LoadingInputParams* loadingParams = new LoadingInputParams();
    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
    loadingParams->lpParam = param;

    // Reset the autosave time
    app.SetAutosaveTimerTime();

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

void UIScene_LoadMenu::checkStateAndStartGame() {
    // Check if they have the Reset Nether flag set, and confirm they want to do
    // this
    if (m_MoreOptionsParams.bResetNether) {
        unsigned int uiIDA[2];
        uiIDA[0] = IDS_DONT_RESET_NETHER;
        uiIDA[1] = IDS_RESET_NETHER;

        ui.RequestAlertMessage(
            IDS_RESETNETHER_TITLE, IDS_RESETNETHER_TEXT, uiIDA, 2, m_iPad,
            &UIScene_LoadMenu::CheckResetNetherReturned, this);
    } else {
        LaunchGame();
    }
}

int UIScene_LoadMenu::StartGame_SignInReturned(void* pParam, bool bContinue,
                                               int iPad) {
    UIScene_LoadMenu* pClass = (UIScene_LoadMenu*)pParam;

    if (bContinue == true) {
        // It's possible that the player has not signed in - they can back out
        if (PlatformProfile.IsSignedIn(pClass->m_iPad)) {
            int primaryPad = PlatformProfile.GetPrimaryPad();
            bool noPrivileges = false;
            int localUsersMask = 0;
            bool isSignedInLive = PlatformProfile.IsSignedInLive(primaryPad);
            bool isOnlineGame = pClass->m_MoreOptionsParams.bOnlineGame;
            int iPadNotSignedInLive = -1;
            bool isLocalMultiplayerAvailable =
                app.IsLocalMultiplayerAvailable();

            for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                if (PlatformProfile.IsSignedIn(i) &&
                    ((i == primaryPad) || isLocalMultiplayerAvailable)) {
                    if (isSignedInLive && !PlatformProfile.IsSignedInLive(i)) {
                        // Record the first non signed in live pad
                        iPadNotSignedInLive = i;
                    }

                    if (!PlatformProfile.AllowedToPlayMultiplayer(i))
                        noPrivileges = true;
                    localUsersMask |=
                        CGameNetworkManager::GetLocalPlayerMask(i);
                    isSignedInLive =
                        isSignedInLive && PlatformProfile.IsSignedInLive(i);
                }
            }

            // If this is an online game but not all players are signed in to
            // Live, stop!
            if (isOnlineGame && !isSignedInLive) {
                pClass->m_bIgnoreInput = false;
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestAlertMessage(IDS_PRO_NOTONLINE_TITLE,
                                       IDS_PRO_NOTONLINE_TEXT, uiIDA, 1,
                                       PlatformProfile.GetPrimaryPad());
                return 0;
            }

            // Check if user-created content is allowed, as we cannot play
            // multiplayer if it's not
            bool noUGC = false;
            bool pccAllowed = true;
            bool pccFriendsAllowed = true;

            PlatformProfile.AllowedPlayerCreatedContent(
                PlatformProfile.GetPrimaryPad(), false, &pccAllowed,
                &pccFriendsAllowed);
            if (!pccAllowed && !pccFriendsAllowed) noUGC = true;

            if (isSignedInLive && isOnlineGame && (noPrivileges || noUGC)) {
                if (noUGC) {
                    pClass->m_bIgnoreInput = false;
                    pClass->setVisible(true);
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    ui.RequestAlertMessage(
                        IDS_FAILED_TO_CREATE_GAME_TITLE,
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE, uiIDA, 1,
                        PlatformProfile.GetPrimaryPad());
                } else {
                    pClass->m_bIgnoreInput = false;
                    pClass->setVisible(true);
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    ui.RequestAlertMessage(
                        IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                        IDS_NO_MULTIPLAYER_PRIVILEGE_HOST_TEXT, uiIDA, 1,
                        PlatformProfile.GetPrimaryPad());
                }
            } else {
                // This is NOT called from a storage manager thread, and is in
                // fact called from the main thread in the Profile library tick.
                StartGameFromSave(pClass, localUsersMask);
            }
        }
    } else {
        pClass->m_bIgnoreInput = false;
    }

    return 0;
}

void UIScene_LoadMenu::handleGainFocus(bool navBack) {
    if (navBack) {
        m_checkboxOnline.setChecked(m_MoreOptionsParams.bOnlineGame == true);
    }
}
