
#include "UIScene_CreateWorldMenu.h"

#include <wchar.h>

#include <cstdint>
#include <utility>

#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/Scenes/Frontend Menu screens/IUIScene_StartGame.h"
#include "app/common/UI/UILayer.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameHostOptions.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/world/level/LevelSettings.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "platform/NetTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "strings.h"
#include "util/StringHelpers.h"

#if defined(_WINDOWS64)

#include <windows.h>

#include "../../../../../Windows64/Resource.h"
#endif

#define GAME_CREATE_ONLINE_TIMER_ID 0
#define GAME_CREATE_ONLINE_TIMER_TIME 100

int UIScene_CreateWorldMenu::m_iDifficultyTitleSettingA[4] = {
    IDS_DIFFICULTY_TITLE_PEACEFUL, IDS_DIFFICULTY_TITLE_EASY,
    IDS_DIFFICULTY_TITLE_NORMAL, IDS_DIFFICULTY_TITLE_HARD};

UIScene_CreateWorldMenu::UIScene_CreateWorldMenu(int iPad, void* initData,
                                                 UILayer* parentLayer)
    : IUIScene_StartGame(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_worldName = app.GetString(IDS_DEFAULT_WORLD_NAME);
    m_seed = "";

    m_iPad = iPad;

    m_labelWorldName.init(app.GetString(IDS_WORLD_NAME));

    m_editWorldName.init(m_worldName, eControl_EditWorldName);

    m_buttonGamemode.init(app.GetString(IDS_GAMEMODE_SURVIVAL),
                          eControl_GameModeToggle);
    m_buttonMoreOptions.init(app.GetString(IDS_MORE_OPTIONS),
                             eControl_MoreOptions);
    m_buttonCreateWorld.init(app.GetString(IDS_CREATE_NEW_WORLD),
                             eControl_NewWorld);

    m_texturePackList.init(app.GetString(IDS_DLC_MENU_TEXTUREPACKS),
                           eControl_TexturePackList);

    m_labelTexturePackName.init("");
    m_labelTexturePackDescription.init("");

    char TempString[256];
    snprintf(TempString, 256, "%s: %s", app.GetString(IDS_SLIDER_DIFFICULTY),
             app.GetString(m_iDifficultyTitleSettingA[app.GetGameSettings(
                 m_iPad, eGameSetting_Difficulty)]));
    m_sliderDifficulty.init(
        TempString, eControl_Difficulty, 0, 3,
        app.GetGameSettings(m_iPad, eGameSetting_Difficulty));

    m_MoreOptionsParams.bGenerateOptions = true;
    m_MoreOptionsParams.bStructures = true;
    m_MoreOptionsParams.bFlatWorld = false;
    m_MoreOptionsParams.bBonusChest = false;
    m_MoreOptionsParams.bPVP = true;
    m_MoreOptionsParams.bTrust = true;
    m_MoreOptionsParams.bFireSpreads = true;
    m_MoreOptionsParams.bHostPrivileges = false;
    m_MoreOptionsParams.bTNT = true;
    m_MoreOptionsParams.iPad = iPad;

    m_MoreOptionsParams.bMobGriefing = true;
    m_MoreOptionsParams.bKeepInventory = false;
    m_MoreOptionsParams.bDoMobSpawning = true;
    m_MoreOptionsParams.bDoMobLoot = true;
    m_MoreOptionsParams.bDoTileDrops = true;
    m_MoreOptionsParams.bNaturalRegeneration = true;
    m_MoreOptionsParams.bDoDaylightCycle = true;

    m_bGameModeCreative = false;
    m_iGameModeId = GameType::SURVIVAL->getId();
    m_pDLCPack = nullptr;
    m_bRebuildTouchBoxes = false;

    m_bMultiplayerAllowed = PlatformProfile.IsSignedInLive(m_iPad) &&
                            PlatformProfile.AllowedToPlayMultiplayer(m_iPad);
    // 4J-PB - read the settings for the online flag. We'll only save this
    // setting if the user changed it.
    bool bGameSetting_Online =
        (app.GetGameSettings(m_iPad, eGameSetting_Online) != 0);
    m_MoreOptionsParams.bOnlineSettingChangedBySystem = false;

    // 4J-PB - Removing this so that we can attempt to create an online game on
    // PS3 when we are a restricted child account It'll fail when we choose
    // create, but this matches the behaviour of load game, and lets the player
    // know why they can't play online, instead of just greying out the online
    // setting in the More Options #ifdef 0
    // 	if(PlatformProfile.IsSignedInLive( m_iPad ))
    // 	{
    // 		PlatformProfile.GetChatAndContentRestrictions(m_iPad,true,&bChatRestricted,&bContentRestricted,nullptr);
    // 	}
    // #endif

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

    addTimer(GAME_CREATE_ONLINE_TIMER_ID, GAME_CREATE_ONLINE_TIMER_TIME);
#if TO_BE_IMPLEMENTED
    XuiSetTimer(m_hObj, CHECKFORAVAILABLETEXTUREPACKS_TIMER_ID,
                CHECKFORAVAILABLETEXTUREPACKS_TIMER_TIME);
#endif

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
                app.DebugPrintf("Adding texture pack %s at %d\n", imageName, i);
            }
        }

#if TO_BE_IMPLEMENTED
        // 4J-PB - there may be texture packs we don't have, so use the info
        // from TMS for this

        DLC_INFO* pDLCInfo = nullptr;

        // first pass - look to see if there are any that are not in the list
        bool bTexturePackAlreadyListed;
        bool bNeedToGetTPD = false;

        for (unsigned int i = 0; i < app.GetDLCInfoTexturesOffersCount(); ++i) {
            bTexturePackAlreadyListed = false;
            uint64_t ull = app.GetDLCInfoTexturesFullOffer(i);
            pDLCInfo = app.GetDLCInfoForFullOfferID(ull);
            for (unsigned int i = 0; i < texturePacksCount; ++i) {
                TexturePack* tp = pMinecraft->skins->getTexturePackByIndex(i);
                if (pDLCInfo->iConfig == tp->getDLCParentPackId()) {
                    bTexturePackAlreadyListed = true;
                }
            }
            if (bTexturePackAlreadyListed == false) {
                // some missing
                bNeedToGetTPD = true;

                m_iTexturePacksNotInstalled++;
            }
        }

        if (bNeedToGetTPD == true) {
            // add a TMS request for them
            app.DebugPrintf("+++ Adding TMSPP request for texture pack data\n");
            app.AddTMSPPFileTypeRequest(e_DLC_TexturePackData);
            m_iConfigA = new int[m_iTexturePacksNotInstalled];
            m_iTexturePacksNotInstalled = 0;

            for (unsigned int i = 0; i < app.GetDLCInfoTexturesOffersCount();
                 ++i) {
                bTexturePackAlreadyListed = false;
                uint64_t ull = app.GetDLCInfoTexturesFullOffer(i);
                pDLCInfo = app.GetDLCInfoForFullOfferID(ull);
                for (unsigned int i = 0; i < texturePacksCount; ++i) {
                    TexturePack* tp =
                        pMinecraft->skins->getTexturePackByIndex(i);
                    if (pDLCInfo->iConfig == tp->getDLCParentPackId()) {
                        bTexturePackAlreadyListed = true;
                    }
                }
                if (bTexturePackAlreadyListed == false) {
                    m_iConfigA[m_iTexturePacksNotInstalled++] =
                        pDLCInfo->iConfig;
                }
            }
        }
#endif

        UpdateTexturePackDescription(m_currentTexturePackIndex);

        m_texturePackList.selectSlot(m_currentTexturePackIndex);
    }
}

UIScene_CreateWorldMenu::~UIScene_CreateWorldMenu() {}

void UIScene_CreateWorldMenu::updateTooltips() {
    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT,
                   IDS_TOOLTIPS_BACK);
}

void UIScene_CreateWorldMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

std::string UIScene_CreateWorldMenu::getMoviePath() {
    return "CreateWorldMenu";
}

UIControl* UIScene_CreateWorldMenu::GetMainPanel() {
    return &m_controlMainPanel;
}

void UIScene_CreateWorldMenu::handleDestroy() {
    // shut down the keyboard if it is displayed
}

void UIScene_CreateWorldMenu::tick() {
    UIScene::tick();

    if (m_iSetTexturePackDescription >= 0) {
        UpdateTexturePackDescription(m_iSetTexturePackDescription);
        m_iSetTexturePackDescription = -1;
    }
    if (m_bShowTexturePackDescription) {
        slideLeft();
        m_texturePackDescDisplayed = true;

        m_bShowTexturePackDescription = false;
    }
}

void UIScene_CreateWorldMenu::handleInput(int iPad, int key, bool repeat,
                                          bool pressed, bool released,
                                          bool& handled) {
    if (m_bIgnoreInput) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
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

void UIScene_CreateWorldMenu::handlePress(F64 controlId, F64 childId) {
    if (m_bIgnoreInput) return;

    // CD - Added for audio
    ui.PlayUISFX(eSFX_Press);

    switch ((int)controlId) {
        case eControl_EditWorldName: {
            m_bIgnoreInput = true;
            PlatformInput.RequestKeyboard(
                app.GetString(IDS_CREATE_NEW_WORLD), m_editWorldName.getLabel(),
                0, 25,
                [this](bool bRes) -> int {
                    m_bIgnoreInput = false;
                    // 4J HEG - No reason to set value if keyboard was cancelled
                    if (bRes) {
                        std::string str = PlatformInput.GetText();
                        if (!str.empty()) {
                            m_editWorldName.setLabel(str);
                            m_worldName = std::move(str);
                        }
                        m_buttonCreateWorld.setEnable(!m_worldName.empty());
                    }
                    return 0;
                },
                IPlatformInput::EKeyboardMode_Default);
        } break;
        case eControl_GameModeToggle:
            switch (m_iGameModeId) {
                case 0:  // Survival
                    m_buttonGamemode.setLabel(
                        app.GetString(IDS_GAMEMODE_CREATIVE));
                    m_iGameModeId = GameType::CREATIVE->getId();
                    m_bGameModeCreative = true;
                    break;
                case 1:  // Creative
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
        case eControl_NewWorld: {
            {
                StartSharedLaunchFlow();
            }
            break;
        }
    }
}

void UIScene_CreateWorldMenu::StartSharedLaunchFlow() {
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
    checkStateAndStartGame();
}

void UIScene_CreateWorldMenu::handleSliderMove(F64 sliderId, F64 currentValue) {
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

void UIScene_CreateWorldMenu::handleTimerComplete(int id) {
    switch (id) {
        case GAME_CREATE_ONLINE_TIMER_ID: {
            bool bMultiplayerAllowed =
                PlatformProfile.IsSignedInLive(m_iPad) &&
                PlatformProfile.AllowedToPlayMultiplayer(m_iPad);

            if (bMultiplayerAllowed != m_bMultiplayerAllowed) {
                if (bMultiplayerAllowed) {
                    bool bGameSetting_Online =
                        (app.GetGameSettings(m_iPad, eGameSetting_Online) != 0);
                    m_MoreOptionsParams.bOnlineGame = bGameSetting_Online;
                    if (bGameSetting_Online) {
                        m_MoreOptionsParams.bInviteOnly =
                            app.GetGameSettings(m_iPad,
                                                eGameSetting_InviteOnly) != 0;
                        m_MoreOptionsParams.bAllowFriendsOfFriends =
                            app.GetGameSettings(
                                m_iPad, eGameSetting_FriendsOfFriends) != 0;
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
    };
}

void UIScene_CreateWorldMenu::handleGainFocus(bool navBack) {
    if (navBack) {
        m_checkboxOnline.setChecked(m_MoreOptionsParams.bOnlineGame);
    }
}

void UIScene_CreateWorldMenu::checkStateAndStartGame() {
    int primaryPad = PlatformProfile.GetPrimaryPad();
    bool isSignedInLive = true;
    bool isOnlineGame = m_MoreOptionsParams.bOnlineGame;
    int iPadNotSignedInLive = -1;
    bool isLocalMultiplayerAvailable = app.IsLocalMultiplayerAvailable();

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; i++) {
        if (PlatformProfile.IsSignedIn(i) &&
            (i == primaryPad || isLocalMultiplayerAvailable)) {
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
        m_bIgnoreInput = false;
        unsigned int uiIDA[1];
        uiIDA[0] = IDS_CONFIRM_OK;
        ui.RequestAlertMessage(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_NOTONLINE_TEXT,
                               uiIDA, 1, PlatformProfile.GetPrimaryPad());
        return;
    }

    unsigned int uiIDA[2];
    if (m_bGameModeCreative == true ||
        m_MoreOptionsParams.bHostPrivileges == true) {
        uiIDA[0] = IDS_CONFIRM_OK;
        uiIDA[1] = IDS_CONFIRM_CANCEL;
        if (m_bGameModeCreative == true) {
            ui.RequestAlertMessage(
                IDS_TITLE_START_GAME, IDS_CONFIRM_START_CREATIVE, uiIDA, 2,
                m_iPad, &UIScene_CreateWorldMenu::ConfirmCreateReturned, this);
        } else {
            ui.RequestAlertMessage(
                IDS_TITLE_START_GAME, IDS_CONFIRM_START_HOST_PRIVILEGES, uiIDA,
                2, m_iPad, &UIScene_CreateWorldMenu::ConfirmCreateReturned,
                this);
        }
    } else {
        // 4J Stu - If we only have one controller connected, then don't show
        // the sign-in UI again
        int connectedControllers = 0;
        for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
            if (PlatformInput.IsPadConnected(i) ||
                PlatformProfile.IsSignedIn(i))
                ++connectedControllers;
        }

        // Check if user-created content is allowed, as we cannot play
        // multiplayer if it's not
        // bool isClientSide =
        // PlatformProfile.IsSignedInLive(PlatformProfile.GetPrimaryPad()) &&
        // m_MoreOptionsParams.bOnlineGame;
        bool noUGC = false;
        bool pccAllowed = true;
        bool pccFriendsAllowed = true;
        bool bContentRestricted = false;

        PlatformProfile.AllowedPlayerCreatedContent(
            PlatformProfile.GetPrimaryPad(), false, &pccAllowed,
            &pccFriendsAllowed);

        noUGC = !pccAllowed && !pccFriendsAllowed;

        if (isOnlineGame && isSignedInLive &&
            app.IsLocalMultiplayerAvailable()) {
            // 4J-PB not sure why we aren't checking the content restriction for
            // the main player here when multiple controllers are connected -
            // adding now
            if (noUGC) {
                m_bIgnoreInput = false;
                ui.RequestUGCMessageBox();
            } else if (bContentRestricted) {
                m_bIgnoreInput = false;
                ui.RequestContentRestrictedMessageBox();
            } else {
                // PlatformProfile.RequestSignInUI(false, false, false, true,
                // false,&CScene_MultiGameCreate::StartGame_SignInReturned,
                // this,PlatformProfile.GetPrimaryPad());
                SignInInfo info;
                info.Func = [this](bool bContinue, int pad) {
                    return StartGame_SignInReturned(this, bContinue, pad);
                };
                info.requireOnline = m_MoreOptionsParams.bOnlineGame;
                ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                                   eUIScene_QuadrantSignin, &info);
            }
        } else {
            if (!pccAllowed && !pccFriendsAllowed) noUGC = true;

            if (isOnlineGame && isSignedInLive && noUGC) {
                m_bIgnoreInput = false;
                ui.RequestUGCMessageBox();
            } else if (isOnlineGame && isSignedInLive && bContentRestricted) {
                m_bIgnoreInput = false;
                ui.RequestContentRestrictedMessageBox();
            } else {
                CreateGame(this, 0);
            }
        }
    }
}

// 4J Stu - Shared functionality that is the same whether we needed a quadrant
// sign-in or not
void UIScene_CreateWorldMenu::CreateGame(UIScene_CreateWorldMenu* pClass,
                                         int localUsersMask) {
#if TO_BE_IMPLEMENTED
    // stop the timer running that causes a check for new texture packs in TMS
    // but not installed, since this will run all through the create game, and
    // will crash if it tries to create an hbrush
    XuiKillTimer(pClass->m_hObj, CHECKFORAVAILABLETEXTUREPACKS_TIMER_ID);
#endif

    bool isClientSide =
        PlatformProfile.IsSignedInLive(PlatformProfile.GetPrimaryPad()) &&
        pClass->m_MoreOptionsParams.bOnlineGame;

    bool isPrivate = pClass->m_MoreOptionsParams.bInviteOnly ? true : false;

    // clear out the app's terrain features list
    app.ClearTerrainFeaturePosition();

    // create the world and launch
    std::string wWorldName = pClass->m_worldName;

    PlatformStorage.ResetSaveData();
    // Make our next save default to the name of the level
    PlatformStorage.SetSaveTitle((char*)wWorldName.c_str());

    std::string wSeed;
    if (!pClass->m_MoreOptionsParams.seed.empty()) {
        wSeed = pClass->m_MoreOptionsParams.seed;
    } else {
        // random
        wSeed = "";
    }

    // start the game
    bool isFlat = pClass->m_MoreOptionsParams.bFlatWorld;
    int64_t seedValue = 0;

    NetworkGameInitData* param = new NetworkGameInitData();

    if (wSeed.length() != 0) {
        int64_t value = 0;
        unsigned int len = (unsigned int)wSeed.length();

        // Check if the input string contains a numerical value
        bool isNumber = true;
        for (unsigned int i = 0; i < len; ++i) {
            if (wSeed.at(i) < '0' || wSeed.at(i) > '9') {
                if (!(i == 0 && wSeed.at(i) == '-')) {
                    isNumber = false;
                    break;
                }
            }
        }

        // If the input string is a numerical value, convert it to a number
        if (isNumber) value = fromWString<int64_t>(wSeed);

        // If the value is not 0 use it, otherwise use the algorithm from the
        // java String.hashCode() function to hash it
        if (value != 0)
            seedValue = value;
        else {
            int hashValue = 0;
            for (unsigned int i = 0; i < len; ++i)
                hashValue = 31 * hashValue + wSeed.at(i);
            seedValue = hashValue;
        }
    } else {
        param->findSeed =
            true;  // 4J - java code sets the seed to was (new
                   // Random())->nextLong() here - we used to at this point find
                   // a suitable seed, but now just set a flag so this is
                   // performed in Minecraft::Server::initServer.
    }

    param->seed = seedValue;
    param->saveData = nullptr;
    param->texturePackId = pClass->m_MoreOptionsParams.dwTexturePack;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    pMinecraft->skins->selectTexturePackById(
        pClass->m_MoreOptionsParams.dwTexturePack);

    app.SetGameHostOption(eGameHostOption_Difficulty,
                          Minecraft::GetInstance()->options->difficulty);
    app.SetGameHostOption(eGameHostOption_FriendsOfFriends,
                          pClass->m_MoreOptionsParams.bAllowFriendsOfFriends);
    app.SetGameHostOption(
        eGameHostOption_Gamertags,
        app.GetGameSettings(pClass->m_iPad, eGameSetting_GamertagsVisible) ? 1
                                                                           : 0);

    app.SetGameHostOption(
        eGameHostOption_BedrockFog,
        app.GetGameSettings(pClass->m_iPad, eGameSetting_BedrockFog) ? 1 : 0);

    app.SetGameHostOption(eGameHostOption_GameType, pClass->m_iGameModeId);
    app.SetGameHostOption(eGameHostOption_LevelType,
                          pClass->m_MoreOptionsParams.bFlatWorld);
    app.SetGameHostOption(eGameHostOption_Structures,
                          pClass->m_MoreOptionsParams.bStructures);
    app.SetGameHostOption(eGameHostOption_BonusChest,
                          pClass->m_MoreOptionsParams.bBonusChest);

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

    app.SetGameHostOption(eGameHostOption_WasntSaveOwner, false);
#if defined(_LARGE_WORLDS)
    app.SetGameHostOption(eGameHostOption_WorldSize,
                          pClass->m_MoreOptionsParams.worldSize +
                              1);  // 0 is GAME_HOST_OPTION_WORLDSIZE_UNKNOWN
    pClass->m_MoreOptionsParams.currentWorldSize =
        (EGameHostOptionWorldSize)(pClass->m_MoreOptionsParams.worldSize + 1);
    pClass->m_MoreOptionsParams.newWorldSize =
        (EGameHostOptionWorldSize)(pClass->m_MoreOptionsParams.worldSize + 1);
#endif

    g_NetworkManager.HostGame(localUsersMask, isClientSide, isPrivate,
                              MINECRAFT_NET_MAX_PLAYERS, 0);

    param->settings = app.GetGameHostOption(eGameHostOption_All);

#if defined(_LARGE_WORLDS)
    switch (pClass->m_MoreOptionsParams.worldSize) {
        case 0:
            // Classic
            param->xzSize = LEVEL_WIDTH_CLASSIC;
            param->hellScale =
                HELL_LEVEL_SCALE_CLASSIC;  // hellsize = 54/3 = 18
            break;
        case 1:
            // Small
            param->xzSize = LEVEL_WIDTH_SMALL;
            param->hellScale =
                HELL_LEVEL_SCALE_SMALL;  // hellsize = ceil(64/3) = 22
            break;
        case 2:
            // Medium
            param->xzSize = LEVEL_WIDTH_MEDIUM;
            param->hellScale =
                HELL_LEVEL_SCALE_MEDIUM;  // hellsize= ceil(3*64/6) = 32
            break;
        case 3:
            // Large
            param->xzSize = LEVEL_WIDTH_LARGE;
            param->hellScale =
                HELL_LEVEL_SCALE_LARGE;  // hellsize = ceil(5*64/8) = 40
            break;
    };
#else
    param->xzSize = LEVEL_MAX_WIDTH;
    param->hellScale = HELL_LEVEL_MAX_SCALE;
#endif

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

    ui.NavigateToScene(pClass->m_iPad, eUIScene_FullscreenProgress,
                       loadingParams);
}

int UIScene_CreateWorldMenu::StartGame_SignInReturned(void* pParam,
                                                      bool bContinue,
                                                      int iPad) {
    UIScene_CreateWorldMenu* pClass = (UIScene_CreateWorldMenu*)pParam;

    if (bContinue == true) {
        // It's possible that the player has not signed in - they can back out
        if (PlatformProfile.IsSignedIn(pClass->m_iPad)) {
            bool isOnlineGame = PlatformProfile.IsSignedInLive(
                                    PlatformProfile.GetPrimaryPad()) &&
                                pClass->m_MoreOptionsParams.bOnlineGame;
            // bool isOnlineGame = pClass->m_MoreOptionsParams.bOnlineGame;
            int primaryPad = PlatformProfile.GetPrimaryPad();
            bool noPrivileges = false;
            int localUsersMask = 0;
            bool isSignedInLive = PlatformProfile.IsSignedInLive(primaryPad);
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

            if (isOnlineGame && (noPrivileges || noUGC)) {
                if (noUGC) {
                    pClass->m_bIgnoreInput = false;
                    unsigned int uiIDA[1];
                    uiIDA[0] = IDS_CONFIRM_OK;
                    ui.RequestAlertMessage(
                        IDS_FAILED_TO_CREATE_GAME_TITLE,
                        IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE, uiIDA, 1,
                        PlatformProfile.GetPrimaryPad());
                } else {
                    pClass->m_bIgnoreInput = false;
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
                CreateGame(pClass, localUsersMask);
            }
        }
    } else {
        pClass->m_bIgnoreInput = false;
    }
    return 0;
}

int UIScene_CreateWorldMenu::ConfirmCreateReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_CreateWorldMenu* pClass = (UIScene_CreateWorldMenu*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
        bool isClientSide =
            PlatformProfile.IsSignedInLive(PlatformProfile.GetPrimaryPad()) &&
            pClass->m_MoreOptionsParams.bOnlineGame;

        // 4J Stu - If we only have one controller connected, then don't show
        // the sign-in UI again
        int connectedControllers = 0;
        for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
            if (PlatformInput.IsPadConnected(i) ||
                PlatformProfile.IsSignedIn(i))
                ++connectedControllers;
        }

        if (isClientSide && app.IsLocalMultiplayerAvailable()) {
            // PlatformProfile.RequestSignInUI(false, false, false, true,
            // false,&UIScene_CreateWorldMenu::StartGame_SignInReturned,
            // pClass,PlatformProfile.GetPrimaryPad());
            SignInInfo info;
            info.Func = [pClass](bool bContinue, int pad) {
                return StartGame_SignInReturned(pClass, bContinue, pad);
            };
            info.requireOnline = pClass->m_MoreOptionsParams.bOnlineGame;
            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_QuadrantSignin, &info);
        } else {
            // Check if user-created content is allowed, as we cannot play
            // multiplayer if it's not
            bool isClientSide = PlatformProfile.IsSignedInLive(
                                    PlatformProfile.GetPrimaryPad()) &&
                                pClass->m_MoreOptionsParams.bOnlineGame;
            bool noUGC = false;
            bool pccAllowed = true;
            bool pccFriendsAllowed = true;

            PlatformProfile.AllowedPlayerCreatedContent(
                PlatformProfile.GetPrimaryPad(), false, &pccAllowed,
                &pccFriendsAllowed);
            if (!pccAllowed && !pccFriendsAllowed) noUGC = true;

            if (isClientSide && noUGC) {
                pClass->m_bIgnoreInput = false;
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestAlertMessage(
                    IDS_FAILED_TO_CREATE_GAME_TITLE,
                    IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE, uiIDA, 1,
                    PlatformProfile.GetPrimaryPad());
            } else {
                CreateGame(pClass, 0);
            }
        }
    } else {
        pClass->m_bIgnoreInput = false;
    }
    return 0;
}

void UIScene_CreateWorldMenu::handleTouchBoxRebuild() {
    m_bRebuildTouchBoxes = true;
}
