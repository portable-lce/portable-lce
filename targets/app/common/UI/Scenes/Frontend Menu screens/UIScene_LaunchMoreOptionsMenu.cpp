#include "UIScene_LaunchMoreOptionsMenu.h"

#include <wchar.h>

#include <utility>

#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_HTMLLabel.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_Slider.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameHostOptions.h"
#include "minecraft/GameTypes.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "strings.h"
#include "util/StringHelpers.h"

#define GAME_CREATE_ONLINE_TIMER_ID 0
#define GAME_CREATE_ONLINE_TIMER_TIME 100

#if defined(_LARGE_WORLDS)
int m_iWorldSizeTitleA[4] = {
    IDS_WORLD_SIZE_TITLE_CLASSIC,
    IDS_WORLD_SIZE_TITLE_SMALL,
    IDS_WORLD_SIZE_TITLE_MEDIUM,
    IDS_WORLD_SIZE_TITLE_LARGE,
};
#endif

UIScene_LaunchMoreOptionsMenu::UIScene_LaunchMoreOptionsMenu(
    int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_params = (LaunchMoreOptionsMenuInitData*)initData;

    m_labelWorldOptions.init(app.GetString(IDS_WORLD_OPTIONS));

    IggyDataValue result;

#if defined(_LARGE_WORLDS)
    IggyDataValue value[2];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = m_params->bGenerateOptions ? 0 : 1;
    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = false;
    if (m_params->currentWorldSize == e_worldSize_Classic ||
        m_params->currentWorldSize == e_worldSize_Small ||
        m_params->currentWorldSize == e_worldSize_Medium) {
        // don't show the increase world size stuff if we're already large, or
        // the size is unknown.
        value[1].boolval = true;
    }

    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetMenuType, 2, value);
#else
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = m_params->bGenerateOptions ? 0 : 1;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetMenuType, 1, value);
#endif

    m_bMultiplayerAllowed =
        PlatformProfile.IsSignedInLive(m_params->iPad) &&
        PlatformProfile.AllowedToPlayMultiplayer(m_params->iPad);

    bool bOnlineGame, bInviteOnly, bAllowFriendsOfFriends;
    bOnlineGame = m_params->bOnlineGame;
    bInviteOnly = m_params->bInviteOnly;
    bAllowFriendsOfFriends = m_params->bAllowFriendsOfFriends;

    // 4J-PB - to stop an offline game being able to select the online flag
    if (PlatformProfile.IsSignedInLive(m_params->iPad) == false) {
        m_checkboxes[eLaunchCheckbox_Online].SetEnable(false);
    }

    if (m_params->bOnlineSettingChangedBySystem && !m_bMultiplayerAllowed) {
        // 4J-JEV: Disable and uncheck these boxes if they can't play
        // multiplayer.
        m_checkboxes[eLaunchCheckbox_Online].SetEnable(false);
        m_checkboxes[eLaunchCheckbox_InviteOnly].SetEnable(false);
        m_checkboxes[eLaunchCheckbox_AllowFoF].SetEnable(false);

        bOnlineGame = bInviteOnly = bAllowFriendsOfFriends = false;
    } else if (!m_params->bOnlineGame) {
        m_checkboxes[eLaunchCheckbox_InviteOnly].SetEnable(false);
        m_checkboxes[eLaunchCheckbox_AllowFoF].SetEnable(false);
    }

    // Init cheats
    m_bUpdateCheats = false;
    // Update cheat checkboxes
    UpdateCheats();

    m_checkboxes[eLaunchCheckbox_Online].init(
        app.GetString(IDS_ONLINE_GAME), eLaunchCheckbox_Online, bOnlineGame);
    m_checkboxes[eLaunchCheckbox_InviteOnly].init(
        app.GetString(IDS_INVITE_ONLY), eLaunchCheckbox_InviteOnly,
        bInviteOnly);
    m_checkboxes[eLaunchCheckbox_AllowFoF].init(
        app.GetString(IDS_ALLOWFRIENDSOFFRIENDS), eLaunchCheckbox_AllowFoF,
        bAllowFriendsOfFriends);
    m_checkboxes[eLaunchCheckbox_PVP].init(app.GetString(IDS_PLAYER_VS_PLAYER),
                                           eLaunchCheckbox_PVP, m_params->bPVP);
    m_checkboxes[eLaunchCheckbox_TrustSystem].init(
        app.GetString(IDS_TRUST_PLAYERS), eLaunchCheckbox_TrustSystem,
        m_params->bTrust);
    m_checkboxes[eLaunchCheckbox_FireSpreads].init(
        app.GetString(IDS_FIRE_SPREADS), eLaunchCheckbox_FireSpreads,
        m_params->bFireSpreads);
    m_checkboxes[eLaunchCheckbox_TNT].init(app.GetString(IDS_TNT_EXPLODES),
                                           eLaunchCheckbox_TNT, m_params->bTNT);
    m_checkboxes[eLaunchCheckbox_HostPrivileges].init(
        app.GetString(IDS_HOST_PRIVILEGES), eLaunchCheckbox_HostPrivileges,
        m_params->bHostPrivileges);
    m_checkboxes[eLaunchCheckbox_ResetNether].init(
        app.GetString(IDS_RESET_NETHER), eLaunchCheckbox_ResetNether,
        m_params->bResetNether);
    m_checkboxes[eLaunchCheckbox_Structures].init(
        app.GetString(IDS_GENERATE_STRUCTURES), eLaunchCheckbox_Structures,
        m_params->bStructures);
    m_checkboxes[eLaunchCheckbox_FlatWorld].init(
        app.GetString(IDS_SUPERFLAT_WORLD), eLaunchCheckbox_FlatWorld,
        m_params->bFlatWorld);
    m_checkboxes[eLaunchCheckbox_BonusChest].init(
        app.GetString(IDS_BONUS_CHEST), eLaunchCheckbox_BonusChest,
        m_params->bBonusChest);

    m_checkboxes[eLaunchCheckbox_KeepInventory].init(
        app.GetString(IDS_KEEP_INVENTORY), eLaunchCheckbox_KeepInventory,
        m_params->bKeepInventory);
    m_checkboxes[eLaunchCheckbox_MobSpawning].init(
        app.GetString(IDS_MOB_SPAWNING), eLaunchCheckbox_MobSpawning,
        m_params->bDoMobSpawning);
    m_checkboxes[eLaunchCheckbox_MobLoot].init(app.GetString(IDS_MOB_LOOT),
                                               eLaunchCheckbox_MobLoot,
                                               m_params->bDoMobLoot);
    m_checkboxes[eLaunchCheckbox_MobGriefing].init(
        app.GetString(IDS_MOB_GRIEFING), eLaunchCheckbox_MobGriefing,
        m_params->bMobGriefing);
    m_checkboxes[eLaunchCheckbox_TileDrops].init(app.GetString(IDS_TILE_DROPS),
                                                 eLaunchCheckbox_TileDrops,
                                                 m_params->bDoTileDrops);
    m_checkboxes[eLaunchCheckbox_NaturalRegeneration].init(
        app.GetString(IDS_NATURAL_REGEN), eLaunchCheckbox_NaturalRegeneration,
        m_params->bNaturalRegeneration);
    m_checkboxes[eLaunchCheckbox_DayLightCycle].init(
        app.GetString(IDS_DAYLIGHT_CYCLE), eLaunchCheckbox_DayLightCycle,
        m_params->bDoDaylightCycle);

    m_labelGameOptions.init(app.GetString(IDS_GAME_OPTIONS));
    m_labelSeed.init(app.GetString(IDS_CREATE_NEW_WORLD_SEED));
    m_labelRandomSeed.init(app.GetString(IDS_CREATE_NEW_WORLD_RANDOM_SEED));
    m_editSeed.init(m_params->seed, eControl_EditSeed);

#if defined(_LARGE_WORLDS)
    m_labelWorldSize.init(app.GetString(IDS_WORLD_SIZE));
    m_sliderWorldSize.init(
        app.GetString(m_iWorldSizeTitleA[m_params->worldSize]),
        eControl_WorldSize, 0, 3, m_params->worldSize);

    m_checkboxes[eLaunchCheckbox_DisableSaving].init(
        app.GetString(IDS_DISABLE_SAVING), eLaunchCheckbox_DisableSaving,
        m_params->bDisableSaving);

    if (m_params->currentWorldSize != e_worldSize_Unknown) {
        m_labelWorldResize.init(app.GetString(IDS_INCREASE_WORLD_SIZE));
        int min = int(m_params->currentWorldSize) - 1;
        int max = 3;
        int curr = int(m_params->newWorldSize) - 1;
        m_sliderWorldResize.init(app.GetString(m_iWorldSizeTitleA[curr]),
                                 eControl_WorldResize, min, max, curr);
        m_checkboxes[eLaunchCheckbox_WorldResizeType].init(
            app.GetString(IDS_INCREASE_WORLD_SIZE_OVERWRITE_EDGES),
            eLaunchCheckbox_WorldResizeType,
            m_params->newWorldSizeOverwriteEdges);
    }
#endif

    // Only the Xbox 360 needs a reset nether
    // 4J-PB - PS3 needs it now
    // #ifndef 0
    // 	if(!m_params->bGenerateOptions) removeControl(
    // &m_checkboxes[eLaunchCheckbox_ResetNether], false ); #endif

    m_tabIndex =
        m_params->bGenerateOptions ? TAB_WORLD_OPTIONS : TAB_GAME_OPTIONS;

    // set the default text
#if defined(_LARGE_WORLDS)
    std::string wsText = "";
    if (m_params->bGenerateOptions) {
        wsText = app.GetString(IDS_GAMEOPTION_SEED);
    } else {
        wsText = app.GetString(IDS_GAMEOPTION_ONLINE);
    }
#else
    std::string wsText = app.GetString(IDS_GAMEOPTION_ONLINE);
#endif
    EHTMLFontSize size = eHTMLSize_Normal;
    if (!PlatformRenderer.IsHiDef() && !PlatformRenderer.IsWidescreen()) {
        size = eHTMLSize_Splitscreen;
    }
    char startTags[64];
    snprintf(startTags, 64, "<font color=\"#%08x\">",
             app.GetHTMLColour(eHTMLColor_White));
    wsText = startTags + wsText;
    if (m_tabIndex == TAB_WORLD_OPTIONS)
        m_labelDescription_WorldOptions.setLabel(wsText);
    else
        m_labelDescription_GameOptions.setLabel(wsText);

    addTimer(GAME_CREATE_ONLINE_TIMER_ID, GAME_CREATE_ONLINE_TIMER_TIME);

    m_bIgnoreInput = false;
}

void UIScene_LaunchMoreOptionsMenu::updateTooltips() {
    int changeTabTooltip = -1;

    // Set tooltip for change tab (only two tabs)
    if (m_tabIndex == TAB_GAME_OPTIONS) {
        changeTabTooltip = IDS_WORLD_OPTIONS;
    } else {
        changeTabTooltip = IDS_GAME_OPTIONS;
    }

    // If there's a change tab tooltip, left bumper symbol should show but not
    // the text (-2)
    int lb = changeTabTooltip == -1 ? -1 : -2;

    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT,
                   IDS_TOOLTIPS_BACK, -1, -1, -1, -1, lb, changeTabTooltip);
}

void UIScene_LaunchMoreOptionsMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
    // #ifdef _LARGE_WORLDS
    //	m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
    // #else
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
    // #endif
}

std::string UIScene_LaunchMoreOptionsMenu::getMoviePath() {
    return "LaunchMoreOptionsMenu";
}

void UIScene_LaunchMoreOptionsMenu::tick() {
    UIScene::tick();

    bool bMultiplayerAllowed =
        PlatformProfile.IsSignedInLive(m_params->iPad) &&
        PlatformProfile.AllowedToPlayMultiplayer(m_params->iPad);

    if (bMultiplayerAllowed != m_bMultiplayerAllowed) {
        m_checkboxes[eLaunchCheckbox_Online].SetEnable(bMultiplayerAllowed);
        m_checkboxes[eLaunchCheckbox_InviteOnly].SetEnable(bMultiplayerAllowed);
        m_checkboxes[eLaunchCheckbox_AllowFoF].SetEnable(bMultiplayerAllowed);

        if (bMultiplayerAllowed) {
            m_checkboxes[eLaunchCheckbox_Online].setChecked(true);
            m_checkboxes[eLaunchCheckbox_AllowFoF].setChecked(true);
        }

        m_bMultiplayerAllowed = bMultiplayerAllowed;
    }

    // Check cheats
    if (m_bUpdateCheats) {
        UpdateCheats();
        m_bUpdateCheats = false;
    }
    // check online
    if (m_bUpdateOnline) {
        UpdateOnline();
        m_bUpdateOnline = false;
    }
}

void UIScene_LaunchMoreOptionsMenu::handleDestroy() {
    // so shut down the keyboard if it is displayed
}

void UIScene_LaunchMoreOptionsMenu::handleInput(int iPad, int key, bool repeat,
                                                bool pressed, bool released,
                                                bool& handled) {
    if (m_bIgnoreInput) return;

    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");
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
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
        case ACTION_MENU_OTHER_STICK_UP:
        case ACTION_MENU_OTHER_STICK_DOWN:
            sendInputToMovie(key, repeat, pressed, released);
            handled = true;
            break;
        case ACTION_MENU_LEFT_SCROLL:
        case ACTION_MENU_RIGHT_SCROLL:
            if (pressed) {
                // Toggle tab index
                m_tabIndex = m_tabIndex == 0 ? 1 : 0;
                updateTooltips();
                IggyDataValue result;
                IggyResult out = IggyPlayerCallMethodRS(
                    getMovie(), &result, IggyPlayerRootPath(getMovie()),
                    m_funcChangeTab, 0, nullptr);
            }
            break;
    }
}

void UIScene_LaunchMoreOptionsMenu::handleCheckboxToggled(F64 controlId,
                                                          bool selected) {
    // CD - Added for audio
    ui.PlayUISFX(eSFX_Press);

    switch ((EControls)((int)controlId)) {
        case eLaunchCheckbox_Online:
            m_params->bOnlineGame = selected;
            m_bUpdateOnline = true;
            break;
        case eLaunchCheckbox_InviteOnly:
            m_params->bInviteOnly = selected;
            break;
        case eLaunchCheckbox_AllowFoF:
            m_params->bAllowFriendsOfFriends = selected;
            break;
        case eLaunchCheckbox_PVP:
            m_params->bPVP = selected;
            break;
        case eLaunchCheckbox_TrustSystem:
            m_params->bTrust = selected;
            break;
        case eLaunchCheckbox_FireSpreads:
            m_params->bFireSpreads = selected;
            break;
        case eLaunchCheckbox_TNT:
            m_params->bTNT = selected;
            break;
        case eLaunchCheckbox_HostPrivileges:
            m_params->bHostPrivileges = selected;
            m_bUpdateCheats = true;
            break;
        case eLaunchCheckbox_ResetNether:
            m_params->bResetNether = selected;
            break;
        case eLaunchCheckbox_Structures:
            m_params->bStructures = selected;
            break;
        case eLaunchCheckbox_FlatWorld:
            m_params->bFlatWorld = selected;
            break;
        case eLaunchCheckbox_BonusChest:
            m_params->bBonusChest = selected;
            break;
#if defined(_LARGE_WORLDS)
        case eLaunchCheckbox_DisableSaving:
            m_params->bDisableSaving = selected;
            break;
        case eLaunchCheckbox_WorldResizeType:
            m_params->newWorldSizeOverwriteEdges = selected;
            break;
#endif
        case eLaunchCheckbox_KeepInventory:
            m_params->bKeepInventory = selected;
            break;
        case eLaunchCheckbox_MobSpawning:
            m_params->bDoMobSpawning = selected;
            break;
        case eLaunchCheckbox_MobLoot:
            m_params->bDoMobLoot = selected;
        case eLaunchCheckbox_MobGriefing:
            m_params->bMobGriefing = selected;
            break;
        case eLaunchCheckbox_TileDrops:
            m_params->bDoTileDrops = selected;
            break;
        case eLaunchCheckbox_NaturalRegeneration:
            m_params->bNaturalRegeneration = selected;
            break;
        case eLaunchCheckbox_DayLightCycle:
            m_params->bDoDaylightCycle = selected;
            break;
        default:
            break;
    };
}

void UIScene_LaunchMoreOptionsMenu::handleFocusChange(F64 controlId,
                                                      F64 childId) {
    int stringId = 0;
    switch ((int)controlId) {
        case eLaunchCheckbox_Online:
            stringId = IDS_GAMEOPTION_ONLINE;
            break;
        case eLaunchCheckbox_InviteOnly:
            stringId = IDS_GAMEOPTION_INVITEONLY;
            break;
        case eLaunchCheckbox_AllowFoF:
            stringId = IDS_GAMEOPTION_ALLOWFOF;
            break;
        case eLaunchCheckbox_PVP:
            stringId = IDS_GAMEOPTION_PVP;
            break;
        case eLaunchCheckbox_TrustSystem:
            stringId = IDS_GAMEOPTION_TRUST;
            break;
        case eLaunchCheckbox_FireSpreads:
            stringId = IDS_GAMEOPTION_FIRE_SPREADS;
            break;
        case eLaunchCheckbox_TNT:
            stringId = IDS_GAMEOPTION_TNT_EXPLODES;
            break;
        case eLaunchCheckbox_HostPrivileges:
            stringId = IDS_GAMEOPTION_HOST_PRIVILEGES;
            break;
        case eLaunchCheckbox_ResetNether:
            stringId = IDS_GAMEOPTION_RESET_NETHER;
            break;
        case eLaunchCheckbox_Structures:
            stringId = IDS_GAMEOPTION_STRUCTURES;
            break;
        case eLaunchCheckbox_FlatWorld:
            stringId = IDS_GAMEOPTION_SUPERFLAT;
            break;
        case eLaunchCheckbox_BonusChest:
            stringId = IDS_GAMEOPTION_BONUS_CHEST;
            break;
        case eLaunchCheckbox_KeepInventory:
            stringId = IDS_GAMEOPTION_KEEP_INVENTORY;
            break;
        case eLaunchCheckbox_MobSpawning:
            stringId = IDS_GAMEOPTION_MOB_SPAWNING;
            break;
        case eLaunchCheckbox_MobLoot:
            stringId = IDS_GAMEOPTION_MOB_LOOT;  // PLACEHOLDER
            break;
        case eLaunchCheckbox_MobGriefing:
            stringId = IDS_GAMEOPTION_MOB_GRIEFING;  // PLACEHOLDER
            break;
        case eLaunchCheckbox_TileDrops:
            stringId = IDS_GAMEOPTION_TILE_DROPS;
            break;
        case eLaunchCheckbox_NaturalRegeneration:
            stringId = IDS_GAMEOPTION_NATURAL_REGEN;
            break;
        case eLaunchCheckbox_DayLightCycle:
            stringId = IDS_GAMEOPTION_DAYLIGHT_CYCLE;
            break;
        case eControl_EditSeed:
            stringId = IDS_GAMEOPTION_SEED;
            break;
#if defined(_LARGE_WORLDS)
        case eControl_WorldSize:
            stringId = IDS_GAMEOPTION_WORLD_SIZE;
            break;
        case eControl_WorldResize:
            stringId = IDS_GAMEOPTION_INCREASE_WORLD_SIZE;
            break;
        case eLaunchCheckbox_DisableSaving:
            stringId = IDS_GAMEOPTION_DISABLE_SAVING;
            break;
        case eLaunchCheckbox_WorldResizeType:
            stringId = IDS_GAMEOPTION_INCREASE_WORLD_SIZE_OVERWRITE_EDGES;
            break;
#endif
    };

    std::string wsText = app.GetString(stringId);
    EHTMLFontSize size = eHTMLSize_Normal;
    if (!PlatformRenderer.IsHiDef() && !PlatformRenderer.IsWidescreen()) {
        size = eHTMLSize_Splitscreen;
    }
    char startTags[64];
    snprintf(startTags, 64, "<font color=\"#%08x\">",
             app.GetHTMLColour(eHTMLColor_White));
    wsText = startTags + wsText;

    if (m_tabIndex == TAB_WORLD_OPTIONS)
        m_labelDescription_WorldOptions.setLabel(wsText);
    else
        m_labelDescription_GameOptions.setLabel(wsText);
}

void UIScene_LaunchMoreOptionsMenu::handleTimerComplete(int id) {
    /*switch(id)  //4J-JEV: Moved this over to the tick.
    {
    case GAME_CREATE_ONLINE_TIMER_ID:
            {
                    bool bMultiplayerAllowed
                            =	PlatformProfile.IsSignedInLive(m_params->iPad)
                            &&
    PlatformProfile.AllowedToPlayMultiplayer(m_params->iPad);

                    if (bMultiplayerAllowed != m_bMultiplayerAllowed)
                    {
                            m_checkboxes[
    eLaunchCheckbox_Online].SetEnable(bMultiplayerAllowed);
                            m_checkboxes[eLaunchCheckbox_InviteOnly].SetEnable(bMultiplayerAllowed);
                            m_checkboxes[
    eLaunchCheckbox_AllowFoF].SetEnable(bMultiplayerAllowed);

                            m_checkboxes[eLaunchCheckbox_Online].setChecked(bMultiplayerAllowed);

                            m_bMultiplayerAllowed = bMultiplayerAllowed;
                    }
            }
            break;
    };*/
}

void UIScene_LaunchMoreOptionsMenu::handlePress(F64 controlId, F64 childId) {
    if (m_bIgnoreInput) return;

    switch ((int)controlId) {
        case eControl_EditSeed: {
            m_bIgnoreInput = true;
            PlatformInput.RequestKeyboard(
                app.GetString(IDS_CREATE_NEW_WORLD_SEED), m_editSeed.getLabel(),
                0, 60,
                [this](bool bRes) -> int {
                    // 4J HEG - No reason to set value if keyboard was cancelled
                    if (bRes) {
                        std::string str = PlatformInput.GetText();
                        m_editSeed.setLabel(str);
                        m_params->seed = std::move(str);
                    }
                    m_bIgnoreInput = false;
                    return 0;
                },
                IPlatformInput::EKeyboardMode_Default);
        } break;
    }
}

void UIScene_LaunchMoreOptionsMenu::handleSliderMove(F64 sliderId,
                                                     F64 currentValue) {
    int value = (int)currentValue;
    switch ((int)sliderId) {
        case eControl_WorldSize:
#if defined(_LARGE_WORLDS)
            m_sliderWorldSize.handleSliderMove(value);
            m_params->worldSize = value;
            m_sliderWorldSize.setLabel(
                app.GetString(m_iWorldSizeTitleA[value]));
#endif
            break;
        case eControl_WorldResize:
#if defined(_LARGE_WORLDS)
            EGameHostOptionWorldSize changedSize =
                EGameHostOptionWorldSize(value + 1);
            if (changedSize >= m_params->currentWorldSize) {
                m_sliderWorldResize.handleSliderMove(value);
                m_params->newWorldSize = EGameHostOptionWorldSize(value + 1);
                m_sliderWorldResize.setLabel(
                    app.GetString(m_iWorldSizeTitleA[value]));
            }
#endif
            break;
    }
}

void UIScene_LaunchMoreOptionsMenu::UpdateCheats() {
    bool cheatsOn = m_params->bHostPrivileges;

    m_checkboxes[eLaunchCheckbox_KeepInventory].SetEnable(cheatsOn);
    m_checkboxes[eLaunchCheckbox_MobSpawning].SetEnable(cheatsOn);
    m_checkboxes[eLaunchCheckbox_MobGriefing].SetEnable(cheatsOn);
    m_checkboxes[eLaunchCheckbox_DayLightCycle].SetEnable(cheatsOn);

    if (!cheatsOn) {
        // Set defaults
        m_params->bMobGriefing = true;
        m_params->bKeepInventory = false;
        m_params->bDoMobSpawning = true;
        m_params->bDoDaylightCycle = true;

        m_checkboxes[eLaunchCheckbox_KeepInventory].setChecked(
            m_params->bKeepInventory);
        m_checkboxes[eLaunchCheckbox_MobSpawning].setChecked(
            m_params->bDoMobSpawning);
        m_checkboxes[eLaunchCheckbox_MobGriefing].setChecked(
            m_params->bMobGriefing);
        m_checkboxes[eLaunchCheckbox_DayLightCycle].setChecked(
            m_params->bDoDaylightCycle);
    }
}

void UIScene_LaunchMoreOptionsMenu::UpdateOnline() {
    bool bOnline = m_params->bOnlineGame;

    m_checkboxes[eLaunchCheckbox_InviteOnly].SetEnable(bOnline);
    m_checkboxes[eLaunchCheckbox_AllowFoF].SetEnable(bOnline);
}