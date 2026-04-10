#include "app/common/GameSettingsManager.h"

#include <cstring>

#include "app/common/Audio/SoundEngine.h"
#include "app/common/Game.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/GameEnums.h"
#include "minecraft/GameHostOptions.h"
#include "minecraft/GameTypes.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/PlayerList.h"
#include "minecraft/server/ServerAction.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/input/input.h"
#include "platform/profile/ProfileConstants.h"
#include "platform/renderer/renderer.h"
#include "platform/storage/storage.h"

GameSettingsManager::GameSettingsManager() {
    memset(GameSettingsA, 0, sizeof(GameSettingsA));
    m_uiGameHostSettings = 0;
}

void GameSettingsManager::initGameSettings() {
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        GameSettingsA[i] =
            (GAME_SETTINGS*)PlatformProfile.GetGameDefinedProfileData(i);
        // clear the flag to say the settings have changed
        GameSettingsA[i]->bSettingsChanged = false;

#if defined(_WINDOWS64)
        IPlatformProfile::PROFILESETTINGS* pProfileSettings =
            PlatformProfile.GetDashboardProfileSettings(i);
        memset(pProfileSettings, 0, sizeof(IPlatformProfile::PROFILESETTINGS));
        setDefaultOptions(pProfileSettings, i);
#else
        IPlatformProfile::PROFILESETTINGS* pProfileSettings =
            PlatformProfile.GetDashboardProfileSettings(i);
        memset(pProfileSettings, 0, sizeof(IPlatformProfile::PROFILESETTINGS));
        setDefaultOptions(pProfileSettings, i);
#endif
    }
}

int GameSettingsManager::setDefaultOptions(
    IPlatformProfile::PROFILESETTINGS* pSettings, const int iPad) {
    setGameSettings(iPad, eGameSetting_MusicVolume, DEFAULT_VOLUME_LEVEL);
    setGameSettings(iPad, eGameSetting_SoundFXVolume, DEFAULT_VOLUME_LEVEL);
    setGameSettings(iPad, eGameSetting_Gamma, 50);

    if (Minecraft::GetInstance()->level == nullptr) {
        app.DebugPrintf("SetDefaultOptions - Difficulty = 1\n");
        setGameSettings(iPad, eGameSetting_Difficulty, 1);
    }
    setGameSettings(iPad, eGameSetting_Sensitivity_InGame, 100);
    setGameSettings(iPad, eGameSetting_ViewBob, 1);
    setGameSettings(iPad, eGameSetting_ControlScheme, 0);
    setGameSettings(iPad, eGameSetting_ControlInvertLook,
                    (pSettings->iYAxisInversion != 0) ? 1 : 0);
    setGameSettings(iPad, eGameSetting_ControlSouthPaw,
                    pSettings->bSwapSticks ? 1 : 0);
    setGameSettings(iPad, eGameSetting_SplitScreenVertical, 0);
    setGameSettings(iPad, eGameSetting_GamertagsVisible, 1);

    // Interim TU 1.6.6
    setGameSettings(iPad, eGameSetting_Sensitivity_InMenu, 100);
    setGameSettings(iPad, eGameSetting_DisplaySplitscreenGamertags, 1);
    setGameSettings(iPad, eGameSetting_Hints, 1);
    setGameSettings(iPad, eGameSetting_Autosave, 2);
    setGameSettings(iPad, eGameSetting_Tooltips, 1);
    setGameSettings(iPad, eGameSetting_InterfaceOpacity, 80);

    // TU 5
    setGameSettings(iPad, eGameSetting_Clouds, 1);
    setGameSettings(iPad, eGameSetting_Online, 1);
    setGameSettings(iPad, eGameSetting_InviteOnly, 0);
    setGameSettings(iPad, eGameSetting_FriendsOfFriends, 1);

    // TU 6
    setGameSettings(iPad, eGameSetting_BedrockFog, 0);
    setGameSettings(iPad, eGameSetting_DisplayHUD, 1);
    setGameSettings(iPad, eGameSetting_DisplayHand, 1);

    // TU 7
    setGameSettings(iPad, eGameSetting_CustomSkinAnim, 1);

    // TU 9
    setGameSettings(iPad, eGameSetting_DeathMessages, 1);
    setGameSettings(iPad, eGameSetting_UISize, 1);
    setGameSettings(iPad, eGameSetting_UISizeSplitscreen, 2);
    setGameSettings(iPad, eGameSetting_AnimatedCharacter, 1);

    // TU 12
    GameSettingsA[iPad]->ucCurrentFavoriteSkinPos = 0;
    for (int i = 0; i < MAX_FAVORITE_SKINS; i++) {
        GameSettingsA[iPad]->uiFavoriteSkinA[i] = 0xFFFFFFFF;
    }

    // TU 13
    GameSettingsA[iPad]->uiMashUpPackWorldsDisplay = 0xFFFFFFFF;

    // 1.6.4
    app.SetGameHostOption(eGameHostOption_MobGriefing, 1);
    app.SetGameHostOption(eGameHostOption_KeepInventory, 0);
    app.SetGameHostOption(eGameHostOption_DoMobSpawning, 1);
    app.SetGameHostOption(eGameHostOption_DoMobLoot, 1);
    app.SetGameHostOption(eGameHostOption_DoTileDrops, 1);
    app.SetGameHostOption(eGameHostOption_NaturalRegeneration, 1);
    app.SetGameHostOption(eGameHostOption_DoDaylightCycle, 1);

    // PS3DEC13
    setGameSettings(iPad, eGameSetting_PS3_EULA_Read, 0);

    if (!app.GetGameStarted()) {
        GameSettingsA[iPad]->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
        GameSettingsA[iPad]->ucLocale = MINECRAFT_LANGUAGE_DEFAULT;
    }

    return 0;
}

int GameSettingsManager::defaultOptionsCallback(
    void* pParam, IPlatformProfile::PROFILESETTINGS* pSettings,
    const int iPad) {
    Game* pApp = (Game*)pParam;

    pApp->DebugPrintf("Setting default options for player %d", iPad);
    pApp->SetAction(iPad, eAppAction_SetDefaultOptions, (void*)pSettings);

    return 0;
}

int GameSettingsManager::oldProfileVersionCallback(
    void* pParam, unsigned char* pucData, const unsigned short usVersion,
    const int iPad) {
    switch (usVersion) {
        case PROFILE_VERSION_8: {
            GAME_SETTINGS* pGameSettings = (GAME_SETTINGS*)pucData;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
            pGameSettings->uiMashUpPackWorldsDisplay = 0xFFFFFFFF;
            pGameSettings->uiBitmaskValues &= ~GAMESETTING_PS3EULAREAD;
            pGameSettings->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
        } break;
        case PROFILE_VERSION_9: {
            GAME_SETTINGS* pGameSettings = (GAME_SETTINGS*)pucData;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
            pGameSettings->uiBitmaskValues &= ~GAMESETTING_PS3EULAREAD;
            pGameSettings->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
        } break;
        case PROFILE_VERSION_10: {
            GAME_SETTINGS* pGameSettings = (GAME_SETTINGS*)pucData;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
            pGameSettings->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
        } break;
        case PROFILE_VERSION_11: {
            GAME_SETTINGS* pGameSettings = (GAME_SETTINGS*)pucData;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
        } break;
        case PROFILE_VERSION_12: {
            GAME_SETTINGS* pGameSettings = (GAME_SETTINGS*)pucData;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
        } break;
        default: {
            app.DebugPrintf(
                "Don't know what to do with this profile version!\n");

            GAME_SETTINGS* pGameSettings = (GAME_SETTINGS*)pucData;
            pGameSettings->ucMenuSensitivity = 100;
            pGameSettings->ucInterfaceOpacity = 80;
            pGameSettings->usBitmaskValues |= 0x0200;
            pGameSettings->usBitmaskValues |= 0x0400;
            pGameSettings->usBitmaskValues |= 0x1000;
            pGameSettings->usBitmaskValues |= 0x8000;

            pGameSettings->uiBitmaskValues = 0L;
            pGameSettings->uiBitmaskValues |= GAMESETTING_CLOUDS;
            pGameSettings->uiBitmaskValues |= GAMESETTING_ONLINE;
            pGameSettings->uiBitmaskValues |= GAMESETTING_FRIENDSOFFRIENDS;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYUPDATEMSG;
            pGameSettings->uiBitmaskValues &= ~GAMESETTING_BEDROCKFOG;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYHUD;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DISPLAYHAND;
            pGameSettings->uiBitmaskValues |= GAMESETTING_CUSTOMSKINANIM;
            pGameSettings->uiBitmaskValues |= GAMESETTING_DEATHMESSAGES;
            pGameSettings->uiBitmaskValues |= (GAMESETTING_UISIZE & 0x00000800);
            pGameSettings->uiBitmaskValues |=
                (GAMESETTING_UISIZE_SPLITSCREEN & 0x00004000);
            pGameSettings->uiBitmaskValues |= GAMESETTING_ANIMATEDCHARACTER;
            for (int i = 0; i < MAX_FAVORITE_SKINS; i++) {
                pGameSettings->uiFavoriteSkinA[i] = 0xFFFFFFFF;
            }
            pGameSettings->ucCurrentFavoriteSkinPos = 0;
            pGameSettings->uiMashUpPackWorldsDisplay = 0xFFFFFFFF;
            pGameSettings->uiBitmaskValues &= ~GAMESETTING_PS3EULAREAD;
            pGameSettings->ucLanguage = MINECRAFT_LANGUAGE_DEFAULT;
        } break;
    }

    return 0;
}

void GameSettingsManager::applyGameSettingsChanged(int iPad) {
    actionGameSettings(iPad, eGameSetting_MusicVolume);
    actionGameSettings(iPad, eGameSetting_SoundFXVolume);
    actionGameSettings(iPad, eGameSetting_Gamma);
    actionGameSettings(iPad, eGameSetting_Difficulty);
    actionGameSettings(iPad, eGameSetting_Sensitivity_InGame);
    actionGameSettings(iPad, eGameSetting_ViewBob);
    actionGameSettings(iPad, eGameSetting_ControlScheme);
    actionGameSettings(iPad, eGameSetting_ControlInvertLook);
    actionGameSettings(iPad, eGameSetting_ControlSouthPaw);
    actionGameSettings(iPad, eGameSetting_SplitScreenVertical);
    actionGameSettings(iPad, eGameSetting_GamertagsVisible);

    // Interim TU 1.6.6
    actionGameSettings(iPad, eGameSetting_Sensitivity_InMenu);
    actionGameSettings(iPad, eGameSetting_DisplaySplitscreenGamertags);
    actionGameSettings(iPad, eGameSetting_Hints);
    actionGameSettings(iPad, eGameSetting_InterfaceOpacity);
    actionGameSettings(iPad, eGameSetting_Tooltips);

    actionGameSettings(iPad, eGameSetting_Clouds);
    actionGameSettings(iPad, eGameSetting_BedrockFog);
    actionGameSettings(iPad, eGameSetting_DisplayHUD);
    actionGameSettings(iPad, eGameSetting_DisplayHand);
    actionGameSettings(iPad, eGameSetting_CustomSkinAnim);
    actionGameSettings(iPad, eGameSetting_DeathMessages);
    actionGameSettings(iPad, eGameSetting_UISize);
    actionGameSettings(iPad, eGameSetting_UISizeSplitscreen);
    actionGameSettings(iPad, eGameSetting_AnimatedCharacter);

    actionGameSettings(iPad, eGameSetting_PS3_EULA_Read);
}

void GameSettingsManager::actionGameSettings(int iPad, eGameSetting eVal) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    switch (eVal) {
        case eGameSetting_MusicVolume:
            if (iPad == PlatformProfile.GetPrimaryPad()) {
                pMinecraft->options->set(
                    Options::Option::MUSIC,
                    ((float)GameSettingsA[iPad]->ucMusicVolume) / 100.0f);
            }
            break;
        case eGameSetting_SoundFXVolume:
            if (iPad == PlatformProfile.GetPrimaryPad()) {
                pMinecraft->options->set(
                    Options::Option::SOUND,
                    ((float)GameSettingsA[iPad]->ucSoundFXVolume) / 100.0f);
            }
            break;
        case eGameSetting_Gamma:
            if (iPad == PlatformProfile.GetPrimaryPad()) {
                float fVal = ((float)GameSettingsA[iPad]->ucGamma) * 327.68f;
                PlatformRenderer.UpdateGamma((unsigned short)fVal);
            }
            break;
        case eGameSetting_Difficulty:
            if (iPad == PlatformProfile.GetPrimaryPad()) {
                pMinecraft->options->toggle(
                    Options::Option::DIFFICULTY,
                    GameSettingsA[iPad]->usBitmaskValues & 0x03);
                app.DebugPrintf("Difficulty toggle to %d\n",
                                GameSettingsA[iPad]->usBitmaskValues & 0x03);

                app.SetGameHostOption(eGameHostOption_Difficulty,
                                      pMinecraft->options->difficulty);

                bool bInGame = pMinecraft->level != nullptr;

                if (bInGame && g_NetworkManager.IsHost() &&
                    (iPad == PlatformProfile.GetPrimaryPad())) {
                    MinecraftServer::getInstance()->queueServerAction(
                        minecraft::server::BroadcastSettingChanged{
                            minecraft::server::BroadcastSettingChanged::Kind::
                                Difficulty});
                }
            } else {
                app.DebugPrintf(
                    "NOT ACTIONING DIFFICULTY - Primary pad is %d, This pad is "
                    "%d\n",
                    PlatformProfile.GetPrimaryPad(), iPad);
            }
            break;
        case eGameSetting_Sensitivity_InGame:
            pMinecraft->options->set(
                Options::Option::SENSITIVITY,
                ((float)GameSettingsA[iPad]->ucSensitivity) / 100.0f);
            break;
        case eGameSetting_ViewBob:
            break;
        case eGameSetting_ControlScheme:
            PlatformInput.SetJoypadMapVal(
                iPad, (GameSettingsA[iPad]->usBitmaskValues & 0x30) >> 4);
            break;
        case eGameSetting_ControlInvertLook:
            break;
        case eGameSetting_ControlSouthPaw:
            if (GameSettingsA[iPad]->usBitmaskValues & 0x80) {
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_LX,
                                                    AXIS_MAP_RX);
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_LY,
                                                    AXIS_MAP_RY);
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_RX,
                                                    AXIS_MAP_LX);
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_RY,
                                                    AXIS_MAP_LY);
                PlatformInput.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_0,
                                                       TRIGGER_MAP_1);
                PlatformInput.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_1,
                                                       TRIGGER_MAP_0);
            } else {
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_LX,
                                                    AXIS_MAP_LX);
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_LY,
                                                    AXIS_MAP_LY);
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_RX,
                                                    AXIS_MAP_RX);
                PlatformInput.SetJoypadStickAxisMap(iPad, AXIS_MAP_RY,
                                                    AXIS_MAP_RY);
                PlatformInput.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_0,
                                                       TRIGGER_MAP_0);
                PlatformInput.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_1,
                                                       TRIGGER_MAP_1);
            }
            break;
        case eGameSetting_SplitScreenVertical:
            if (iPad == PlatformProfile.GetPrimaryPad()) {
                pMinecraft->updatePlayerViewportAssignments();
            }
            break;
        case eGameSetting_GamertagsVisible: {
            bool bInGame = pMinecraft->level != nullptr;

            // Game Host only
            if (bInGame && g_NetworkManager.IsHost() &&
                (iPad == PlatformProfile.GetPrimaryPad())) {
                app.SetGameHostOption(
                    eGameHostOption_Gamertags,
                    ((GameSettingsA[iPad]->usBitmaskValues & 0x0008) != 0) ? 1
                                                                           : 0);
                MinecraftServer::getInstance()->queueServerAction(
                    minecraft::server::BroadcastSettingChanged{
                        minecraft::server::BroadcastSettingChanged::Kind::
                            Gamertags});

                PlayerList* players =
                    MinecraftServer::getInstance()->getPlayerList();
                for (auto it3 = players->players.begin();
                     it3 != players->players.end(); ++it3) {
                    std::shared_ptr<ServerPlayer> decorationPlayer = *it3;
                    decorationPlayer->setShowOnMaps(
                        (app.GetGameHostOption(eGameHostOption_Gamertags) != 0)
                            ? true
                            : false);
                }
            }
        } break;
        case eGameSetting_Sensitivity_InMenu:
            break;
        case eGameSetting_DisplaySplitscreenGamertags:
            for (std::uint8_t idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
                if (pMinecraft->localplayers[idx] != nullptr) {
                    if (pMinecraft->localplayers[idx]->m_iScreenSection ==
                        IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN) {
                        ui.DisplayGamertag(idx, false);
                    } else {
                        ui.DisplayGamertag(idx, true);
                    }
                }
            }
            break;
        case eGameSetting_InterfaceOpacity:
            ui.RefreshTooltips(iPad);
            break;
        case eGameSetting_Hints:
            break;
        case eGameSetting_Tooltips:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x8000) != 0) {
                ui.SetEnableTooltips(iPad, true);
            } else {
                ui.SetEnableTooltips(iPad, false);
            }
            break;
        case eGameSetting_Clouds:
            break;
        case eGameSetting_Online:
            break;
        case eGameSetting_InviteOnly:
            break;
        case eGameSetting_FriendsOfFriends:
            break;
        case eGameSetting_BedrockFog: {
            bool bInGame = pMinecraft->level != nullptr;

            if (bInGame && g_NetworkManager.IsHost() &&
                (iPad == PlatformProfile.GetPrimaryPad())) {
                app.SetGameHostOption(
                    eGameHostOption_BedrockFog,
                    getGameSettings(iPad, eGameSetting_BedrockFog) ? 1 : 0);
                MinecraftServer::getInstance()->queueServerAction(
                    minecraft::server::BroadcastSettingChanged{
                        minecraft::server::BroadcastSettingChanged::Kind::
                            BedrockFog});
            }
        } break;
        case eGameSetting_DisplayHUD:
            break;
        case eGameSetting_DisplayHand:
            break;
        case eGameSetting_CustomSkinAnim:
            break;
        case eGameSetting_DeathMessages:
            break;
        case eGameSetting_UISize:
            break;
        case eGameSetting_UISizeSplitscreen:
            break;
        case eGameSetting_AnimatedCharacter:
            break;
        case eGameSetting_PS3_EULA_Read:
            break;
        case eGameSetting_PSVita_NetworkModeAdhoc:
            break;
        default:
            break;
    }
}

void GameSettingsManager::hideMashupPackWorld(int iPad,
                                              unsigned int iMashupPackID) {
    unsigned int uiPackID = iMashupPackID - 1024;
    GameSettingsA[iPad]->uiMashUpPackWorldsDisplay &= ~(1 << uiPackID);
    GameSettingsA[iPad]->bSettingsChanged = true;
}

void GameSettingsManager::enableMashupPackWorlds(int iPad) {
    GameSettingsA[iPad]->uiMashUpPackWorldsDisplay = 0xFFFFFFFF;
    GameSettingsA[iPad]->bSettingsChanged = true;
}

unsigned int GameSettingsManager::getMashupPackWorlds(int iPad) {
    return GameSettingsA[iPad]->uiMashUpPackWorldsDisplay;
}

void GameSettingsManager::setMinecraftLanguage(int iPad,
                                               unsigned char ucLanguage) {
    GameSettingsA[iPad]->ucLanguage = ucLanguage;
    GameSettingsA[iPad]->bSettingsChanged = true;
}

unsigned char GameSettingsManager::getMinecraftLanguage(int iPad) {
    if (GameSettingsA[iPad] == nullptr) {
        return 0;
    } else {
        return GameSettingsA[iPad]->ucLanguage;
    }
}

void GameSettingsManager::setMinecraftLocale(int iPad, unsigned char ucLocale) {
    GameSettingsA[iPad]->ucLocale = ucLocale;
    GameSettingsA[iPad]->bSettingsChanged = true;
}

unsigned char GameSettingsManager::getMinecraftLocale(int iPad) {
    if (GameSettingsA[iPad] == nullptr) {
        return 0;
    } else {
        return GameSettingsA[iPad]->ucLocale;
    }
}

void GameSettingsManager::setGameSettings(int iPad, eGameSetting eVal,
                                          unsigned char ucVal) {
    switch (eVal) {
        case eGameSetting_MusicVolume:
            if (GameSettingsA[iPad]->ucMusicVolume != ucVal) {
                GameSettingsA[iPad]->ucMusicVolume = ucVal;
                if (iPad == PlatformProfile.GetPrimaryPad()) {
                    actionGameSettings(iPad, eVal);
                }
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_SoundFXVolume:
            if (GameSettingsA[iPad]->ucSoundFXVolume != ucVal) {
                GameSettingsA[iPad]->ucSoundFXVolume = ucVal;
                if (iPad == PlatformProfile.GetPrimaryPad()) {
                    actionGameSettings(iPad, eVal);
                }
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Gamma:
            if (GameSettingsA[iPad]->ucGamma != ucVal) {
                GameSettingsA[iPad]->ucGamma = ucVal;
                if (iPad == PlatformProfile.GetPrimaryPad()) {
                    actionGameSettings(iPad, eVal);
                }
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Difficulty:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x03) !=
                (ucVal & 0x03)) {
                GameSettingsA[iPad]->usBitmaskValues &= ~0x03;
                GameSettingsA[iPad]->usBitmaskValues |= ucVal & 0x03;
                if (iPad == PlatformProfile.GetPrimaryPad()) {
                    actionGameSettings(iPad, eVal);
                }
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Sensitivity_InGame:
            if (GameSettingsA[iPad]->ucSensitivity != ucVal) {
                GameSettingsA[iPad]->ucSensitivity = ucVal;
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_ViewBob:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0004) !=
                ((ucVal & 0x01) << 2)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0004;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0004;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_ControlScheme:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x30) !=
                ((ucVal & 0x03) << 4)) {
                GameSettingsA[iPad]->usBitmaskValues &= ~0x0030;
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= (ucVal & 0x03) << 4;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_ControlInvertLook:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0040) !=
                ((ucVal & 0x01) << 6)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0040;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0040;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_ControlSouthPaw:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0080) !=
                ((ucVal & 0x01) << 7)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0080;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0080;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_SplitScreenVertical:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0100) !=
                ((ucVal & 0x01) << 8)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0100;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0100;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_GamertagsVisible:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0008) !=
                ((ucVal & 0x01) << 3)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0008;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0008;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Sensitivity_InMenu:
            if (GameSettingsA[iPad]->ucMenuSensitivity != ucVal) {
                GameSettingsA[iPad]->ucMenuSensitivity = ucVal;
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_DisplaySplitscreenGamertags:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0200) !=
                ((ucVal & 0x01) << 9)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0200;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0200;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Hints:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x0400) !=
                ((ucVal & 0x01) << 10)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x0400;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x0400;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Autosave:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x7800) !=
                ((ucVal & 0x0F) << 11)) {
                GameSettingsA[iPad]->usBitmaskValues &= ~0x7800;
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= (ucVal & 0x0F)
                                                            << 11;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Tooltips:
            if ((GameSettingsA[iPad]->usBitmaskValues & 0x8000) !=
                ((ucVal & 0x01) << 15)) {
                if (ucVal != 0) {
                    GameSettingsA[iPad]->usBitmaskValues |= 0x8000;
                } else {
                    GameSettingsA[iPad]->usBitmaskValues &= ~0x8000;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_InterfaceOpacity:
            if (GameSettingsA[iPad]->ucInterfaceOpacity != ucVal) {
                GameSettingsA[iPad]->ucInterfaceOpacity = ucVal;
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Clouds:
            if ((GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_CLOUDS) !=
                (ucVal & 0x01)) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |= GAMESETTING_CLOUDS;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &= ~GAMESETTING_CLOUDS;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_Online:
            if ((GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_ONLINE) !=
                (ucVal & 0x01) << 1) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |= GAMESETTING_ONLINE;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &= ~GAMESETTING_ONLINE;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_InviteOnly:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_INVITEONLY) != (ucVal & 0x01) << 2) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_INVITEONLY;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_INVITEONLY;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_FriendsOfFriends:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_FRIENDSOFFRIENDS) != (ucVal & 0x01) << 3) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_FRIENDSOFFRIENDS;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_FRIENDSOFFRIENDS;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_DisplayUpdateMessage:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_DISPLAYUPDATEMSG) != (ucVal & 0x03) << 4) {
                GameSettingsA[iPad]->uiBitmaskValues &=
                    ~GAMESETTING_DISPLAYUPDATEMSG;
                if (ucVal > 0) {
                    GameSettingsA[iPad]->uiBitmaskValues |= (ucVal & 0x03) << 4;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_BedrockFog:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_BEDROCKFOG) != (ucVal & 0x01) << 6) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_BEDROCKFOG;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_BEDROCKFOG;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_DisplayHUD:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_DISPLAYHUD) != (ucVal & 0x01) << 7) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_DISPLAYHUD;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_DISPLAYHUD;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_DisplayHand:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_DISPLAYHAND) != (ucVal & 0x01) << 8) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_DISPLAYHAND;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_DISPLAYHAND;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_CustomSkinAnim:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_CUSTOMSKINANIM) != (ucVal & 0x01) << 9) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_CUSTOMSKINANIM;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_CUSTOMSKINANIM;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_DeathMessages:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_DEATHMESSAGES) != (ucVal & 0x01) << 10) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_DEATHMESSAGES;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_DEATHMESSAGES;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_UISize:
            if ((GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_UISIZE) !=
                ((ucVal & 0x03) << 11)) {
                GameSettingsA[iPad]->uiBitmaskValues &= ~GAMESETTING_UISIZE;
                if (ucVal != 0) {
                    GameSettingsA[iPad]->uiBitmaskValues |= (ucVal & 0x03)
                                                            << 11;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_UISizeSplitscreen:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_UISIZE_SPLITSCREEN) != ((ucVal & 0x03) << 13)) {
                GameSettingsA[iPad]->uiBitmaskValues &=
                    ~GAMESETTING_UISIZE_SPLITSCREEN;
                if (ucVal != 0) {
                    GameSettingsA[iPad]->uiBitmaskValues |= (ucVal & 0x03)
                                                            << 13;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_AnimatedCharacter:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_ANIMATEDCHARACTER) != (ucVal & 0x01) << 15) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_ANIMATEDCHARACTER;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_ANIMATEDCHARACTER;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_PS3_EULA_Read:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_PS3EULAREAD) != (ucVal & 0x01) << 16) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_PS3EULAREAD;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_PS3EULAREAD;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
        case eGameSetting_PSVita_NetworkModeAdhoc:
            if ((GameSettingsA[iPad]->uiBitmaskValues &
                 GAMESETTING_PSVITANETWORKMODEADHOC) != (ucVal & 0x01) << 17) {
                if (ucVal == 1) {
                    GameSettingsA[iPad]->uiBitmaskValues |=
                        GAMESETTING_PSVITANETWORKMODEADHOC;
                } else {
                    GameSettingsA[iPad]->uiBitmaskValues &=
                        ~GAMESETTING_PSVITANETWORKMODEADHOC;
                }
                actionGameSettings(iPad, eVal);
                GameSettingsA[iPad]->bSettingsChanged = true;
            }
            break;
    }
}

unsigned char GameSettingsManager::getGameSettings(eGameSetting eVal) {
    int iPad = PlatformProfile.GetPrimaryPad();
    return getGameSettings(iPad, eVal);
}

unsigned char GameSettingsManager::getGameSettings(int iPad,
                                                   eGameSetting eVal) {
    switch (eVal) {
        case eGameSetting_MusicVolume:
            return GameSettingsA[iPad]->ucMusicVolume;
        case eGameSetting_SoundFXVolume:
            return GameSettingsA[iPad]->ucSoundFXVolume;
        case eGameSetting_Gamma:
            return GameSettingsA[iPad]->ucGamma;
        case eGameSetting_Difficulty:
            return GameSettingsA[iPad]->usBitmaskValues & 0x0003;
        case eGameSetting_Sensitivity_InGame:
            return GameSettingsA[iPad]->ucSensitivity;
        case eGameSetting_ViewBob:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0004) >> 2);
        case eGameSetting_GamertagsVisible:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0008) >> 3);
        case eGameSetting_ControlScheme:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0030) >> 4);
        case eGameSetting_ControlInvertLook:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0040) >> 6);
        case eGameSetting_ControlSouthPaw:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0080) >> 7);
        case eGameSetting_SplitScreenVertical:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0100) >> 8);
        case eGameSetting_Sensitivity_InMenu:
            return GameSettingsA[iPad]->ucMenuSensitivity;
        case eGameSetting_DisplaySplitscreenGamertags:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0200) >> 9);
        case eGameSetting_Hints:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x0400) >> 10);
        case eGameSetting_Autosave: {
            unsigned char ucVal =
                (GameSettingsA[iPad]->usBitmaskValues & 0x7800) >> 11;
            return ucVal;
        }
        case eGameSetting_Tooltips:
            return ((GameSettingsA[iPad]->usBitmaskValues & 0x8000) >> 15);
        case eGameSetting_InterfaceOpacity:
            return GameSettingsA[iPad]->ucInterfaceOpacity;
        case eGameSetting_Clouds:
            return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_CLOUDS);
        case eGameSetting_Online:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_ONLINE) >>
                   1;
        case eGameSetting_InviteOnly:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_INVITEONLY) >>
                   2;
        case eGameSetting_FriendsOfFriends:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_FRIENDSOFFRIENDS) >>
                   3;
        case eGameSetting_DisplayUpdateMessage:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_DISPLAYUPDATEMSG) >>
                   4;
        case eGameSetting_BedrockFog:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_BEDROCKFOG) >>
                   6;
        case eGameSetting_DisplayHUD:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_DISPLAYHUD) >>
                   7;
        case eGameSetting_DisplayHand:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_DISPLAYHAND) >>
                   8;
        case eGameSetting_CustomSkinAnim:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_CUSTOMSKINANIM) >>
                   9;
        case eGameSetting_DeathMessages:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_DEATHMESSAGES) >>
                   10;
        case eGameSetting_UISize: {
            unsigned char ucVal =
                (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_UISIZE) >>
                11;
            return ucVal;
        }
        case eGameSetting_UISizeSplitscreen: {
            unsigned char ucVal = (GameSettingsA[iPad]->uiBitmaskValues &
                                   GAMESETTING_UISIZE_SPLITSCREEN) >>
                                  13;
            return ucVal;
        }
        case eGameSetting_AnimatedCharacter:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_ANIMATEDCHARACTER) >>
                   15;
        case eGameSetting_PS3_EULA_Read:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_PS3EULAREAD) >>
                   16;
        case eGameSetting_PSVita_NetworkModeAdhoc:
            return (GameSettingsA[iPad]->uiBitmaskValues &
                    GAMESETTING_PSVITANETWORKMODEADHOC) >>
                   17;
    }
    return 0;
}

void GameSettingsManager::checkGameSettingsChanged(bool bOverride5MinuteTimer,
                                                   int iPad) {
    if (iPad == XUSER_INDEX_ANY) {
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (GameSettingsA[i]->bSettingsChanged) {
                PlatformProfile.WriteToProfile(i, true, bOverride5MinuteTimer);
                GameSettingsA[i]->bSettingsChanged = false;
            }
        }
    } else {
        if (GameSettingsA[iPad]->bSettingsChanged) {
            PlatformProfile.WriteToProfile(iPad, true, bOverride5MinuteTimer);
            GameSettingsA[iPad]->bSettingsChanged = false;
        }
    }
}

void GameSettingsManager::clearGameSettingsChangedFlag(int iPad) {
    GameSettingsA[iPad]->bSettingsChanged = false;
}

#if !defined(_DEBUG_MENUS_ENABLED)
unsigned int GameSettingsManager::getGameSettingsDebugMask(
    int iPad, bool bOverridePlayer) {
    return 0;
}

void GameSettingsManager::setGameSettingsDebugMask(int iPad,
                                                   unsigned int uiVal) {}

void GameSettingsManager::actionDebugMask(int iPad, bool bSetAllClear) {}

#else

unsigned int GameSettingsManager::getGameSettingsDebugMask(
    int iPad, bool bOverridePlayer) {
    if (iPad == -1) {
        iPad = PlatformProfile.GetPrimaryPad();
    }
    if (iPad < 0) iPad = 0;

    std::shared_ptr<Player> player =
        Minecraft::GetInstance()->localplayers[iPad];

    if (bOverridePlayer || player == nullptr) {
        return GameSettingsA[iPad]->uiDebugBitmask;
    } else {
        return player->GetDebugOptions();
    }
}

void GameSettingsManager::setGameSettingsDebugMask(int iPad,
                                                   unsigned int uiVal) {
#if !defined(_CONTENT_PACKAGE)
    GameSettingsA[iPad]->bSettingsChanged = true;
    GameSettingsA[iPad]->uiDebugBitmask = uiVal;

    std::shared_ptr<Player> player =
        Minecraft::GetInstance()->localplayers[iPad];

    if (player) {
        Minecraft::GetInstance()->localgameModes[iPad]->handleDebugOptions(
            uiVal, player);
    }
#endif
}

void GameSettingsManager::actionDebugMask(int iPad, bool bSetAllClear) {
    unsigned int ulBitmask = app.GetGameSettingsDebugMask(iPad);

    if (bSetAllClear) ulBitmask = 0L;

    if (PlatformProfile.GetPrimaryPad() != iPad) return;

    for (int i = 0; i < eDebugSetting_Max; i++) {
        switch (i) {
            case eDebugSetting_LoadSavesFromDisk:
                if (ulBitmask & (1 << i)) {
                    app.SetLoadSavesFromFolderEnabled(true);
                } else {
                    app.SetLoadSavesFromFolderEnabled(false);
                }
                break;
            case eDebugSetting_WriteSavesToDisk:
                if (ulBitmask & (1 << i)) {
                    app.SetWriteSavesToFolderEnabled(true);
                } else {
                    app.SetWriteSavesToFolderEnabled(false);
                }
                break;
            case eDebugSetting_FreezePlayers:
                if (ulBitmask & (1 << i)) {
                    app.SetFreezePlayers(true);
                } else {
                    app.SetFreezePlayers(false);
                }
                break;
            case eDebugSetting_Safearea:
                if (ulBitmask & (1 << i)) {
                    app.ShowSafeArea(true);
                } else {
                    app.ShowSafeArea(false);
                }
                break;
            case eDebugSetting_ShowUIConsole:
                if (ulBitmask & (1 << i)) {
                    ui.ShowUIDebugConsole(true);
                } else {
                    ui.ShowUIDebugConsole(false);
                }
                break;
            case eDebugSetting_ShowUIMarketingGuide:
                if (ulBitmask & (1 << i)) {
                    ui.ShowUIDebugMarketingGuide(true);
                } else {
                    ui.ShowUIDebugMarketingGuide(false);
                }
                break;
            case eDebugSetting_MobsDontAttack:
                if (ulBitmask & (1 << i)) {
                    app.SetMobsDontAttackEnabled(true);
                } else {
                    app.SetMobsDontAttackEnabled(false);
                }
                break;
            case eDebugSetting_UseDpadForDebug:
                if (ulBitmask & (1 << i)) {
                    app.SetUseDPadForDebug(true);
                } else {
                    app.SetUseDPadForDebug(false);
                }
                break;
            case eDebugSetting_MobsDontTick:
                if (ulBitmask & (1 << i)) {
                    app.SetMobsDontTickEnabled(true);
                } else {
                    app.SetMobsDontTickEnabled(false);
                }
                break;
        }
    }
}
#endif

void GameSettingsManager::setSpecialTutorialCompletionFlag(int iPad,
                                                           int index) {
    if (index >= 0 && index < 32 && GameSettingsA[iPad] != nullptr) {
        GameSettingsA[iPad]->uiSpecialTutorialBitmask |= (1 << index);
    }
}

int GameSettingsManager::displaySavingMessage(
    IPlatformStorage::ESavingMessage eVal, int iPad) {
    ui.ShowSavingMessage(iPad, eVal);
    return 0;
}

void GameSettingsManager::setActionConfirmed(void* param) {
    XuiActionParam* actionInfo = (XuiActionParam*)param;
    app.SetAction(actionInfo->iPad, actionInfo->action);
}

void GameSettingsManager::handleButtonPresses() {
    for (int i = 0; i < 4; i++) {
        handleButtonPresses(i);
    }
}

void GameSettingsManager::handleButtonPresses(int iPad) {
    // Stub - button presses are handled elsewhere now
}

void GameSettingsManager::setGameHostOption(unsigned int& uiHostSettings,
                                            eGameHostOption eVal,
                                            unsigned int uiVal) {
    switch (eVal) {
        case eGameHostOption_FriendsOfFriends:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS;
            }
            break;
        case eGameHostOption_Difficulty:
            uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DIFFICULTY;
            uiHostSettings |= (GAME_HOST_OPTION_BITMASK_DIFFICULTY & uiVal);
            break;
        case eGameHostOption_Gamertags:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_GAMERTAGS;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_GAMERTAGS;
            }
            break;
        case eGameHostOption_GameType:
            uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_GAMETYPE;
            uiHostSettings |=
                (GAME_HOST_OPTION_BITMASK_GAMETYPE & (uiVal << 4));
            break;
        case eGameHostOption_LevelType:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_LEVELTYPE;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_LEVELTYPE;
            }
            break;
        case eGameHostOption_Structures:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_STRUCTURES;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_STRUCTURES;
            }
            break;
        case eGameHostOption_BonusChest:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_BONUSCHEST;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_BONUSCHEST;
            }
            break;
        case eGameHostOption_HasBeenInCreative:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_BEENINCREATIVE;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_BEENINCREATIVE;
            }
            break;
        case eGameHostOption_PvP:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_PVP;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_PVP;
            }
            break;
        case eGameHostOption_TrustPlayers:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS;
            }
            break;
        case eGameHostOption_TNT:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_TNT;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_TNT;
            }
            break;
        case eGameHostOption_FireSpreads:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_FIRESPREADS;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_FIRESPREADS;
            }
            break;
        case eGameHostOption_CheatsEnabled:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTFLY;
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTFLY;
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
            }
            break;
        case eGameHostOption_HostCanFly:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTFLY;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTFLY;
            }
            break;
        case eGameHostOption_HostCanChangeHunger:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
            }
            break;
        case eGameHostOption_HostCanBeInvisible:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
            }
            break;
        case eGameHostOption_BedrockFog:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_BEDROCKFOG;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_BEDROCKFOG;
            }
            break;
        case eGameHostOption_DisableSaving:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_DISABLESAVE;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DISABLESAVE;
            }
            break;
        case eGameHostOption_WasntSaveOwner:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_NOTOWNER;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_NOTOWNER;
            }
            break;
        case eGameHostOption_MobGriefing:
            if (uiVal != 1) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_MOBGRIEFING;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_MOBGRIEFING;
            }
            break;
        case eGameHostOption_KeepInventory:
            if (uiVal != 0) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_KEEPINVENTORY;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_KEEPINVENTORY;
            }
            break;
        case eGameHostOption_DoMobSpawning:
            if (uiVal != 1) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING;
            }
            break;
        case eGameHostOption_DoMobLoot:
            if (uiVal != 1) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_DOMOBLOOT;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DOMOBLOOT;
            }
            break;
        case eGameHostOption_DoTileDrops:
            if (uiVal != 1) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_DOTILEDROPS;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DOTILEDROPS;
            }
            break;
        case eGameHostOption_NaturalRegeneration:
            if (uiVal != 1) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_NATURALREGEN;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_NATURALREGEN;
            }
            break;
        case eGameHostOption_DoDaylightCycle:
            if (uiVal != 1) {
                uiHostSettings |= GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE;
            } else {
                uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE;
            }
            break;
        case eGameHostOption_WorldSize:
            uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_WORLDSIZE;
            uiHostSettings |=
                (GAME_HOST_OPTION_BITMASK_WORLDSIZE &
                 (uiVal << GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT));
            break;
        case eGameHostOption_All:
            uiHostSettings = uiVal;
            break;
        default:
            break;
    }
}

unsigned int GameSettingsManager::getGameHostOption(unsigned int uiHostSettings,
                                                    eGameHostOption eVal) {
    switch (eVal) {
        case eGameHostOption_FriendsOfFriends:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS);
        case eGameHostOption_Difficulty:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_DIFFICULTY);
        case eGameHostOption_Gamertags:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_GAMERTAGS);
        case eGameHostOption_GameType:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_GAMETYPE) >> 4;
        case eGameHostOption_All:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_ALL);
        case eGameHostOption_Tutorial:
            return ((uiHostSettings & GAME_HOST_OPTION_BITMASK_GAMERTAGS) |
                    GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS |
                    GAME_HOST_OPTION_BITMASK_FIRESPREADS |
                    GAME_HOST_OPTION_BITMASK_TNT |
                    GAME_HOST_OPTION_BITMASK_PVP |
                    GAME_HOST_OPTION_BITMASK_STRUCTURES | 1);
        case eGameHostOption_LevelType:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_LEVELTYPE);
        case eGameHostOption_Structures:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_STRUCTURES);
        case eGameHostOption_BonusChest:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_BONUSCHEST);
        case eGameHostOption_HasBeenInCreative:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_BEENINCREATIVE);
        case eGameHostOption_PvP:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_PVP);
        case eGameHostOption_TrustPlayers:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS);
        case eGameHostOption_TNT:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_TNT);
        case eGameHostOption_FireSpreads:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_FIRESPREADS);
        case eGameHostOption_CheatsEnabled:
            return (uiHostSettings & (GAME_HOST_OPTION_BITMASK_HOSTFLY |
                                      GAME_HOST_OPTION_BITMASK_HOSTHUNGER |
                                      GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE));
        case eGameHostOption_HostCanFly:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_HOSTFLY);
        case eGameHostOption_HostCanChangeHunger:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_HOSTHUNGER);
        case eGameHostOption_HostCanBeInvisible:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE);
        case eGameHostOption_BedrockFog:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_BEDROCKFOG);
        case eGameHostOption_DisableSaving:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_DISABLESAVE);
        case eGameHostOption_WasntSaveOwner:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_NOTOWNER);
        case eGameHostOption_WorldSize:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_WORLDSIZE) >>
                   GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT;
        case eGameHostOption_MobGriefing:
            return !(uiHostSettings & GAME_HOST_OPTION_BITMASK_MOBGRIEFING);
        case eGameHostOption_KeepInventory:
            return (uiHostSettings & GAME_HOST_OPTION_BITMASK_KEEPINVENTORY);
        case eGameHostOption_DoMobSpawning:
            return !(uiHostSettings & GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING);
        case eGameHostOption_DoMobLoot:
            return !(uiHostSettings & GAME_HOST_OPTION_BITMASK_DOMOBLOOT);
        case eGameHostOption_DoTileDrops:
            return !(uiHostSettings & GAME_HOST_OPTION_BITMASK_DOTILEDROPS);
        case eGameHostOption_NaturalRegeneration:
            return !(uiHostSettings & GAME_HOST_OPTION_BITMASK_NATURALREGEN);
        case eGameHostOption_DoDaylightCycle:
            return !(uiHostSettings & GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE);
        default:
            return 0;
    }
    return false;
}

bool GameSettingsManager::canRecordStatsAndAchievements() {
    bool isTutorial = Minecraft::GetInstance() != nullptr &&
                      Minecraft::GetInstance()->isTutorial();
    return !(app.GetGameHostOption(eGameHostOption_HasBeenInCreative) ||
             app.GetGameHostOption(eGameHostOption_HostCanBeInvisible) ||
             app.GetGameHostOption(eGameHostOption_HostCanChangeHunger) ||
             app.GetGameHostOption(eGameHostOption_HostCanFly) ||
             app.GetGameHostOption(eGameHostOption_WasntSaveOwner) ||
             !app.GetGameHostOption(eGameHostOption_MobGriefing) ||
             app.GetGameHostOption(eGameHostOption_KeepInventory) ||
             !app.GetGameHostOption(eGameHostOption_DoMobSpawning) ||
             (!app.GetGameHostOption(eGameHostOption_DoDaylightCycle) &&
              !isTutorial));
}
