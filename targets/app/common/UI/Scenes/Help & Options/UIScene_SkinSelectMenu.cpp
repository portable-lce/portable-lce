
#include "UIScene_SkinSelectMenu.h"

#include <wchar.h>

#include <vector>

#include "app/common/DLC/DLCManager.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/DLC/DLCSkinFile.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_PlayerSkinPreview.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/Minecraft_Macros.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/model/SkinBox.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/profile/ProfileConstants.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "strings.h"
#include "util/StringHelpers.h"

class ModelPart;

#define SKIN_SELECT_PACK_DEFAULT 0
#define SKIN_SELECT_PACK_FAVORITES 1
// #define SKIN_SELECT_PACK_PLAYER_CUSTOM 1
#define SKIN_SELECT_MAX_DEFAULTS 2

const char* UIScene_SkinSelectMenu::wchDefaultNamesA[] = {
    "USE LOCALISED VERSION",  // Server selected
    "Steve",
    "Tennis Steve",
    "Tuxedo Steve",
    "Athlete Steve",
    "Scottish Steve",
    "Prisoner Steve",
    "Cyclist Steve",
    "Boxer Steve",
};

UIScene_SkinSelectMenu::UIScene_SkinSelectMenu(int iPad, void* initData,
                                               UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_labelSelected.init(app.GetString(IDS_SELECTED));

    m_bIgnoreInput = false;
    m_bNoSkinsToShow = false;

    m_currentPack = nullptr;
    m_packIndex = SKIN_SELECT_PACK_DEFAULT;
    m_skinIndex = 0;

    m_originalSkinId = app.GetPlayerSkinId(iPad);
    m_currentSkinPath = app.GetPlayerSkinName(iPad);
    m_selectedSkinPath = "";
    m_selectedCapePath = "";
    m_vAdditionalSkinBoxes = nullptr;

    m_bSlidingSkins = false;
    m_bAnimatingMove = false;
    m_bSkinIndexChanged = false;

    m_currentNavigation = eSkinNavigation_Skin;

    m_currentPackCount = 0;

    m_characters[eCharacter_Current].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Forward);

    m_characters[eCharacter_Next1].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Left);
    m_characters[eCharacter_Next2].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Left);
    m_characters[eCharacter_Next3].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Left);
    m_characters[eCharacter_Next4].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Left);

    m_characters[eCharacter_Previous1].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Right);
    m_characters[eCharacter_Previous2].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Right);
    m_characters[eCharacter_Previous3].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Right);
    m_characters[eCharacter_Previous4].SetFacing(
        UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Right);

    m_labelSkinName.init("");
    m_labelSkinOrigin.init("");

    m_leftLabel = "";
    m_centreLabel = "";
    m_rightLabel = "";

    // block input if we're waiting for DLC to install. The end of dlc mounting
    // custom message will fill the save list
    if (app.StartInstallDLCProcess(m_iPad)) {
        // DLC mounting in progress, so disable input
        m_bIgnoreInput = true;

        m_controlTimer.setVisible(true);
        m_controlIggyCharacters.setVisible(false);
        m_controlSkinNamePlate.setVisible(false);

        setCharacterLocked(false);
        setCharacterSelected(false);
    } else {
        m_controlTimer.setVisible(false);

        if (app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin) > 0) {
            // Change to display the favorites if there are any. The current
            // skin will be in there (probably) - need to check for it
            m_currentPack =
                app.m_dlcManager.getPackContainingSkin(m_currentSkinPath);
            bool bFound;
            if (m_currentPack != nullptr) {
                m_packIndex =
                    app.m_dlcManager.getPackIndex(m_currentPack, bFound,
                                                  DLCManager::e_DLCType_Skin) +
                    SKIN_SELECT_MAX_DEFAULTS;
            }
        }

        // If we have any favourites, set this to the favourites
        // first validate the favorite skins - we might have uninstalled the DLC
        // needed for them
        app.ValidateFavoriteSkins(m_iPad);

        if (app.GetPlayerFavoriteSkinsCount(m_iPad) > 0) {
            m_packIndex = SKIN_SELECT_PACK_FAVORITES;
        }

        handlePackIndexChanged();
    }

    // Display the tooltips
}

void UIScene_SkinSelectMenu::updateTooltips() {
    ui.SetTooltips(m_iPad, m_bNoSkinsToShow ? -1 : IDS_TOOLTIPS_SELECT_SKIN,
                   IDS_TOOLTIPS_CANCEL, -1, -1, -1, -1, -1, -1,
                   IDS_TOOLTIPS_NAVIGATE);
}

void UIScene_SkinSelectMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

std::string UIScene_SkinSelectMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "SkinSelectMenuSplit";
    } else {
        return "SkinSelectMenu";
    }
}

void UIScene_SkinSelectMenu::tick() {
    UIScene::tick();

    if (m_bSkinIndexChanged) {
        m_bSkinIndexChanged = false;
        handleSkinIndexChanged();
    }

    // check for new DLC installed

    // check for the patch error dialog
}

void UIScene_SkinSelectMenu::handleAnimationEnd() {
    if (m_bSlidingSkins) {
        m_bSlidingSkins = false;

        m_characters[eCharacter_Current].SetFacing(
            UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Forward, false);
        m_characters[eCharacter_Next1].SetFacing(
            UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Left, false);
        m_characters[eCharacter_Previous1].SetFacing(
            UIControl_PlayerSkinPreview::e_SkinPreviewFacing_Right, false);

        m_bSkinIndexChanged = true;
        // handleSkinIndexChanged();

        m_bAnimatingMove = false;
    }
}

void UIScene_SkinSelectMenu::handleInput(int iPad, int key, bool repeat,
                                         bool pressed, bool released,
                                         bool& handled) {
    if (m_bIgnoreInput) return;
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
                app.CheckGameSettingsChanged(true, iPad);
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
            if (pressed) {
                InputActionOK(iPad);
            }
            break;
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
            if (pressed) {
                if (m_packIndex == SKIN_SELECT_PACK_FAVORITES) {
                    if (app.GetPlayerFavoriteSkinsCount(iPad) == 0) {
                        // ignore this, since there are no skins being displayed
                        break;
                    }
                }

                ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
                ui.PlayUISFX(eSFX_Scroll);
                switch (m_currentNavigation) {
                    case eSkinNavigation_Pack:
                        m_currentNavigation = eSkinNavigation_Skin;
                        break;
                    case eSkinNavigation_Skin:
                        m_currentNavigation = eSkinNavigation_Pack;
                        break;
                    default:
                        break;
                };
                sendInputToMovie(key, repeat, pressed, released);
            }
            break;
        case ACTION_MENU_LEFT:
            if (pressed) {
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    if (!m_bAnimatingMove) {
                        ui.AnimateKeyPress(iPad, key, repeat, pressed,
                                           released);
                        ui.PlayUISFX(eSFX_Scroll);

                        m_skinIndex = getPreviousSkinIndex(m_skinIndex);
                        // handleSkinIndexChanged();

                        m_bSlidingSkins = true;
                        m_bAnimatingMove = true;

                        m_characters[eCharacter_Current].SetFacing(
                            UIControl_PlayerSkinPreview::
                                e_SkinPreviewFacing_Left,
                            true);
                        m_characters[eCharacter_Previous1].SetFacing(
                            UIControl_PlayerSkinPreview::
                                e_SkinPreviewFacing_Forward,
                            true);

                        // 4J Stu - Swapped nav buttons
                        sendInputToMovie(ACTION_MENU_RIGHT, repeat, pressed,
                                         released);
                    }
                } else if (m_currentNavigation == eSkinNavigation_Pack) {
                    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
                    ui.PlayUISFX(eSFX_Scroll);
                    int startingIndex = m_packIndex;
                    m_packIndex = getPreviousPackIndex(m_packIndex);
                    if (startingIndex != m_packIndex) {
                        handlePackIndexChanged();
                    }
                }
            }
            break;
        case ACTION_MENU_RIGHT:
            if (pressed) {
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    if (!m_bAnimatingMove) {
                        ui.AnimateKeyPress(iPad, key, repeat, pressed,
                                           released);
                        ui.PlayUISFX(eSFX_Scroll);
                        m_skinIndex = getNextSkinIndex(m_skinIndex);
                        // handleSkinIndexChanged();

                        m_bSlidingSkins = true;
                        m_bAnimatingMove = true;

                        m_characters[eCharacter_Current].SetFacing(
                            UIControl_PlayerSkinPreview::
                                e_SkinPreviewFacing_Right,
                            true);
                        m_characters[eCharacter_Next1].SetFacing(
                            UIControl_PlayerSkinPreview::
                                e_SkinPreviewFacing_Forward,
                            true);

                        // 4J Stu - Swapped nav buttons
                        sendInputToMovie(ACTION_MENU_LEFT, repeat, pressed,
                                         released);
                    }
                } else if (m_currentNavigation == eSkinNavigation_Pack) {
                    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
                    ui.PlayUISFX(eSFX_Scroll);
                    int startingIndex = m_packIndex;
                    m_packIndex = getNextPackIndex(m_packIndex);
                    if (startingIndex != m_packIndex) {
                        handlePackIndexChanged();
                    }
                }
            }
            break;
        case ACTION_MENU_OTHER_STICK_PRESS:
            if (pressed) {
                ui.PlayUISFX(eSFX_Press);
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    m_characters[eCharacter_Current].ResetRotation();
                }
            }
            break;
        case ACTION_MENU_OTHER_STICK_LEFT:
            if (pressed) {
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    m_characters[eCharacter_Current].m_incYRot = true;
                } else {
                    ui.PlayUISFX(eSFX_Scroll);
                }
            } else if (released) {
                m_characters[eCharacter_Current].m_incYRot = false;
            }
            break;
        case ACTION_MENU_OTHER_STICK_RIGHT:
            if (pressed) {
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    m_characters[eCharacter_Current].m_decYRot = true;
                } else {
                    ui.PlayUISFX(eSFX_Scroll);
                }
            } else if (released) {
                m_characters[eCharacter_Current].m_decYRot = false;
            }
            break;
        case ACTION_MENU_OTHER_STICK_UP:
            if (pressed) {
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    // m_previewControl->m_incXRot = true;
                    m_characters[eCharacter_Current].CyclePreviousAnimation();
                } else {
                    ui.PlayUISFX(eSFX_Scroll);
                }
            }
            break;
        case ACTION_MENU_OTHER_STICK_DOWN:
            if (pressed) {
                if (m_currentNavigation == eSkinNavigation_Skin) {
                    // m_previewControl->m_decXRot = true;
                    m_characters[eCharacter_Current].CycleNextAnimation();
                } else {
                    ui.PlayUISFX(eSFX_Scroll);
                }
            }
            break;
    }
}

void UIScene_SkinSelectMenu::InputActionOK(unsigned int iPad) {
    ui.AnimateKeyPress(iPad, ACTION_MENU_OK, false, true, false);

    // if the profile data has been changed, then force a profile write
    // It seems we're allowed to break the 5 minute rule if it's the result of a
    // user action
    switch (m_packIndex) {
        case SKIN_SELECT_PACK_DEFAULT:
            app.SetPlayerSkin(iPad, m_skinIndex);
            app.SetPlayerCape(iPad, 0);
            m_currentSkinPath = app.GetPlayerSkinName(iPad);
            m_originalSkinId = app.GetPlayerSkinId(iPad);
            setCharacterSelected(true);
            ui.PlayUISFX(eSFX_Press);
            break;
        case SKIN_SELECT_PACK_FAVORITES:
            if (app.GetPlayerFavoriteSkinsCount(iPad) > 0) {
                // get the pack number from the skin id
                char chars[256];
                snprintf(chars, 256, "dlcskin%08d.png",
                         app.GetPlayerFavoriteSkin(iPad, m_skinIndex));

                DLCPack* Pack = app.m_dlcManager.getPackContainingSkin(chars);

                if (Pack) {
                    DLCSkinFile* skinFile = Pack->getSkinFile(chars);
                    app.SetPlayerSkin(iPad, skinFile->getPath());
                    app.SetPlayerCape(iPad,
                                      skinFile->getParameterAsString(
                                          DLCManager::e_DLCParamType_Cape));
                    setCharacterSelected(true);
                    m_currentSkinPath = app.GetPlayerSkinName(iPad);
                    m_originalSkinId = app.GetPlayerSkinId(iPad);
                    app.SetPlayerFavoriteSkinsPos(iPad, m_skinIndex);
                }
            }
            break;
        default:
            if (m_currentPack != nullptr) {
                bool renableInputAfterOperation = true;
                m_bIgnoreInput = true;

                DLCSkinFile* skinFile = m_currentPack->getSkinFile(m_skinIndex);

                // Is this a free skin?

                if (!skinFile->getParameterAsBool(
                        DLCManager::e_DLCParamType_Free)) {
                    // do we have a license?
                    // if(true)
                    if (!m_currentPack->hasPurchasedFile(
                            DLCManager::e_DLCType_Skin, skinFile->getPath())) {
                        // no
                        unsigned int uiIDA[1];
                        uiIDA[0] = IDS_OK;

                        // We need to upsell the full version
                        if (PlatformProfile.IsGuest(iPad)) {
                            // can't buy
                            ui.RequestAlertMessage(IDS_PRO_GUESTPROFILE_TITLE,
                                                   IDS_PRO_GUESTPROFILE_TEXT,
                                                   uiIDA, 1, iPad);
                        } else {
                            // upsell
                            bool bContentRestricted = false;
                            if (bContentRestricted) {
#if !defined(_WIN64)
                                // check this for other platforms you can't see
                                // the store
                                unsigned int uiIDA[1];
                                uiIDA[0] = IDS_CONFIRM_OK;
                                ui.RequestAlertMessage(IDS_ONLINE_SERVICE_TITLE,
                                                       IDS_CONTENT_RESTRICTION,
                                                       uiIDA, 1, iPad);
#endif
                            } else {
                                // 4J-PB - need to check for an empty store
                                {
                                    m_bIgnoreInput = true;
                                    renableInputAfterOperation = false;

                                    unsigned int uiIDA[2] = {
                                        IDS_CONFIRM_OK, IDS_CONFIRM_CANCEL};
                                    ui.RequestAlertMessage(
                                        IDS_UNLOCK_DLC_TITLE,
                                        IDS_UNLOCK_DLC_SKIN, uiIDA, 2, iPad,
                                        &UIScene_SkinSelectMenu::
                                            UnlockSkinReturned,
                                        this);
                                }
                            }
                        }
                    } else {
                        app.SetPlayerSkin(iPad, skinFile->getPath());
                        app.SetPlayerCape(iPad,
                                          skinFile->getParameterAsString(
                                              DLCManager::e_DLCParamType_Cape));
                        setCharacterSelected(true);
                        m_currentSkinPath = app.GetPlayerSkinName(iPad);
                        m_originalSkinId = app.GetPlayerSkinId(iPad);

                        // push this onto the favorite list
                        AddFavoriteSkin(m_iPad, GET_DLC_SKIN_ID_FROM_BITMASK(
                                                    m_originalSkinId));
                    }
                } else {
                    app.SetPlayerSkin(iPad, skinFile->getPath());
                    app.SetPlayerCape(iPad,
                                      skinFile->getParameterAsString(
                                          DLCManager::e_DLCParamType_Cape));
                    setCharacterSelected(true);
                    m_currentSkinPath = app.GetPlayerSkinName(iPad);
                    m_originalSkinId = app.GetPlayerSkinId(iPad);

                    // push this onto the favorite list
                    AddFavoriteSkin(
                        iPad, GET_DLC_SKIN_ID_FROM_BITMASK(m_originalSkinId));
                }

                if (renableInputAfterOperation) {
                    m_bIgnoreInput = false;
                }
            }

            ui.PlayUISFX(eSFX_Press);
            break;
    }
}

void UIScene_SkinSelectMenu::customDraw(IggyCustomDrawCallbackRegion* region) {
    // 4jcraft: fuck char
    int characterId = -1;
    if (region->name != nullptr &&
        std::char_traits<char16_t>::length(region->name) > 9 &&
        std::char_traits<char16_t>::compare(region->name, u"Character", 9) ==
            0) {
        int i = 9;
        characterId = 0;

        while (region->name[i] >= u'0' && region->name[i] <= u'9') {
            characterId = characterId * 10 + (region->name[i] - u'0');
            i++;
        }
    }

    if (characterId == -1) {
        app.DebugPrintf("Invalid character to render found\n");
    } else {
        // Setup GDraw, normal game render states and matrices
        CustomDrawData* customDrawRegion = ui.setupCustomDraw(this, region);
        delete customDrawRegion;

        // app.DebugPrintf("Scissor x0= %d, y0= %d, x1= %d, y1= %d\n",
        // region->scissor_x0, region->scissor_y0, region->scissor_x1,
        // region->scissor_y1); app.DebugPrintf("Stencil mask= %d, stencil ref=
        // %d, stencil write= %d\n", region->stencil_func_mask,
        // region->stencil_func_ref, region->stencil_write_mask);
        if (region->stencil_func_ref != 0)
            PlatformRenderer.StateSetStencil(GL_EQUAL, region->stencil_func_ref,
                                             region->stencil_func_mask,
                                             region->stencil_write_mask);
        m_characters[characterId].render(region);

        // Finish GDraw and anything else that needs to be finalised
        ui.endCustomDraw(region);
    }
}

void UIScene_SkinSelectMenu::handleSkinIndexChanged() {
    bool showPrevious = false, showNext = false;
    int previousIndex = 0, nextIndex = 0;
    std::string skinName = "";
    std::string skinOrigin = "";
    bool bSkinIsFree = false;
    bool bLicensed = false;
    DLCSkinFile* skinFile = nullptr;
    DLCPack* Pack = nullptr;
    int sidePreviewControlsL, sidePreviewControlsR;
    m_bNoSkinsToShow = false;

    TEXTURE_NAME backupTexture = TN_MOB_CHAR;

    setCharacterSelected(false);

    m_controlSkinNamePlate.setVisible(false);

    if (m_currentPack != nullptr) {
        skinFile = m_currentPack->getSkinFile(m_skinIndex);
        m_selectedSkinPath = skinFile->getPath();
        m_selectedCapePath =
            skinFile->getParameterAsString(DLCManager::e_DLCParamType_Cape);
        m_vAdditionalSkinBoxes = skinFile->getAdditionalBoxes();

        skinName = skinFile->getParameterAsString(
            DLCManager::e_DLCParamType_DisplayName);
        skinOrigin = skinFile->getParameterAsString(
            DLCManager::e_DLCParamType_ThemeName);

        if (m_selectedSkinPath.compare(m_currentSkinPath) == 0) {
            setCharacterSelected(true);
        }

        bSkinIsFree =
            skinFile->getParameterAsBool(DLCManager::e_DLCParamType_Free);
        bLicensed = m_currentPack->hasPurchasedFile(DLCManager::e_DLCType_Skin,
                                                    m_selectedSkinPath);

        setCharacterLocked(!(bSkinIsFree || bLicensed));

        m_characters[eCharacter_Current].setVisible(true);
        m_controlSkinNamePlate.setVisible(true);
    } else {
        m_selectedSkinPath = "";
        m_selectedCapePath = "";
        m_vAdditionalSkinBoxes = nullptr;

        switch (m_packIndex) {
            case SKIN_SELECT_PACK_DEFAULT:
                backupTexture = getTextureId(m_skinIndex);

                if (m_skinIndex ==
                    std::to_underlying(EDefaultSkins::ServerSelected)) {
                    skinName = app.GetString(IDS_DEFAULT_SKINS);
                } else {
                    skinName = wchDefaultNamesA[m_skinIndex];
                }

                if (m_originalSkinId == m_skinIndex) {
                    setCharacterSelected(true);
                }
                setCharacterLocked(false);
                setCharacterLocked(false);

                m_characters[eCharacter_Current].setVisible(true);
                m_controlSkinNamePlate.setVisible(true);

                break;
            case SKIN_SELECT_PACK_FAVORITES:

                if (app.GetPlayerFavoriteSkinsCount(m_iPad) > 0) {
                    // get the pack number from the skin id
                    char chars[256];
                    snprintf(chars, 256, "dlcskin%08d.png",
                             app.GetPlayerFavoriteSkin(m_iPad, m_skinIndex));

                    Pack = app.m_dlcManager.getPackContainingSkin(chars);
                    if (Pack) {
                        skinFile = Pack->getSkinFile(chars);

                        m_selectedSkinPath = skinFile->getPath();
                        m_selectedCapePath = skinFile->getParameterAsString(
                            DLCManager::e_DLCParamType_Cape);
                        m_vAdditionalSkinBoxes = skinFile->getAdditionalBoxes();

                        skinName = skinFile->getParameterAsString(
                            DLCManager::e_DLCParamType_DisplayName);
                        skinOrigin = skinFile->getParameterAsString(
                            DLCManager::e_DLCParamType_ThemeName);

                        if (m_selectedSkinPath.compare(m_currentSkinPath) ==
                            0) {
                            setCharacterSelected(true);
                        }

                        bSkinIsFree = skinFile->getParameterAsBool(
                            DLCManager::e_DLCParamType_Free);
                        bLicensed = Pack->hasPurchasedFile(
                            DLCManager::e_DLCType_Skin, m_selectedSkinPath);

                        setCharacterLocked(!(bSkinIsFree || bLicensed));
                        m_controlSkinNamePlate.setVisible(true);
                    } else {
                        setCharacterSelected(false);
                        setCharacterLocked(false);
                    }
                } else {
                    // disable the display
                    m_characters[eCharacter_Current].setVisible(false);

                    // change the tooltips
                    m_bNoSkinsToShow = true;
                }
                break;
        }
    }

    m_labelSkinName.setLabel(skinName);
    m_labelSkinOrigin.setLabel(skinOrigin);

    if (m_vAdditionalSkinBoxes && m_vAdditionalSkinBoxes->size() != 0) {
        // add the boxes to the humanoid model, but only if we've not done this
        // already

        std::vector<ModelPart*>* pAdditionalModelParts =
            app.GetAdditionalModelParts(skinFile->getSkinID());
        if (pAdditionalModelParts == nullptr) {
            pAdditionalModelParts = app.SetAdditionalSkinBoxes(
                skinFile->getSkinID(), m_vAdditionalSkinBoxes);
        }
    }

    if (skinFile != nullptr) {
        app.SetAnimOverrideBitmask(skinFile->getSkinID(),
                                   skinFile->getAnimOverrideBitmask());
    }

    m_characters[eCharacter_Current].SetTexture(m_selectedSkinPath,
                                                backupTexture);
    m_characters[eCharacter_Current].SetCapeTexture(m_selectedCapePath);

    showNext = true;
    showPrevious = true;
    nextIndex = getNextSkinIndex(m_skinIndex);
    previousIndex = getPreviousSkinIndex(m_skinIndex);

    std::string otherSkinPath = "";
    std::string otherCapePath = "";
    std::vector<SKIN_BOX*>* othervAdditionalSkinBoxes = nullptr;
    char chars[256];

    // turn off all displays
    for (unsigned int i = eCharacter_Current + 1; i < eCharacter_COUNT; ++i) {
        m_characters[i].setVisible(false);
    }

    unsigned int uiCurrentFavoriteC = app.GetPlayerFavoriteSkinsCount(m_iPad);

    if (m_packIndex == SKIN_SELECT_PACK_FAVORITES) {
        // might not be enough to cycle through
        if (uiCurrentFavoriteC < ((sidePreviewControls * 2) + 1)) {
            if (uiCurrentFavoriteC == 0) {
                sidePreviewControlsL = sidePreviewControlsR = 0;
            }
            // might be an odd number
            else if ((uiCurrentFavoriteC - 1) % 2 == 1) {
                sidePreviewControlsL = 1 + (uiCurrentFavoriteC - 1) / 2;
                sidePreviewControlsR = (uiCurrentFavoriteC - 1) / 2;
            } else {
                sidePreviewControlsL = sidePreviewControlsR =
                    (uiCurrentFavoriteC - 1) / 2;
            }
        } else {
            sidePreviewControlsL = sidePreviewControlsR = sidePreviewControls;
        }
    } else {
        sidePreviewControlsL = sidePreviewControlsR = sidePreviewControls;
    }

    for (int i = 0; i < sidePreviewControlsR; ++i) {
        if (showNext) {
            skinFile = nullptr;

            m_characters[eCharacter_Next1 + i].setVisible(true);

            if (m_currentPack != nullptr) {
                skinFile = m_currentPack->getSkinFile(nextIndex);
                otherSkinPath = skinFile->getPath();
                otherCapePath = skinFile->getParameterAsString(
                    DLCManager::e_DLCParamType_Cape);
                othervAdditionalSkinBoxes = skinFile->getAdditionalBoxes();
                backupTexture = TN_MOB_CHAR;
            } else {
                otherSkinPath = "";
                otherCapePath = "";
                othervAdditionalSkinBoxes = nullptr;
                switch (m_packIndex) {
                    case SKIN_SELECT_PACK_DEFAULT:
                        backupTexture = getTextureId(nextIndex);
                        break;
                    case SKIN_SELECT_PACK_FAVORITES:
                        if (uiCurrentFavoriteC > 0) {
                            // get the pack number from the skin id
                            snprintf(
                                chars, 256, "dlcskin%08d.png",
                                app.GetPlayerFavoriteSkin(m_iPad, nextIndex));

                            Pack =
                                app.m_dlcManager.getPackContainingSkin(chars);
                            if (Pack) {
                                skinFile = Pack->getSkinFile(chars);

                                otherSkinPath = skinFile->getPath();
                                otherCapePath = skinFile->getParameterAsString(
                                    DLCManager::e_DLCParamType_Cape);
                                othervAdditionalSkinBoxes =
                                    skinFile->getAdditionalBoxes();
                                backupTexture = TN_MOB_CHAR;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            if (othervAdditionalSkinBoxes &&
                othervAdditionalSkinBoxes->size() != 0) {
                std::vector<ModelPart*>* pAdditionalModelParts =
                    app.GetAdditionalModelParts(skinFile->getSkinID());
                if (pAdditionalModelParts == nullptr) {
                    pAdditionalModelParts = app.SetAdditionalSkinBoxes(
                        skinFile->getSkinID(), othervAdditionalSkinBoxes);
                }
            }
            // 4J-PB - anim override needs set before SetTexture
            if (skinFile != nullptr) {
                app.SetAnimOverrideBitmask(skinFile->getSkinID(),
                                           skinFile->getAnimOverrideBitmask());
            }
            m_characters[eCharacter_Next1 + i].SetTexture(otherSkinPath,
                                                          backupTexture);
            m_characters[eCharacter_Next1 + i].SetCapeTexture(otherCapePath);
        }

        nextIndex = getNextSkinIndex(nextIndex);
    }

    for (int i = 0; i < sidePreviewControlsL; ++i) {
        if (showPrevious) {
            skinFile = nullptr;

            m_characters[eCharacter_Previous1 + i].setVisible(true);

            if (m_currentPack != nullptr) {
                skinFile = m_currentPack->getSkinFile(previousIndex);
                otherSkinPath = skinFile->getPath();
                otherCapePath = skinFile->getParameterAsString(
                    DLCManager::e_DLCParamType_Cape);
                othervAdditionalSkinBoxes = skinFile->getAdditionalBoxes();
                backupTexture = TN_MOB_CHAR;
            } else {
                otherSkinPath = "";
                otherCapePath = "";
                othervAdditionalSkinBoxes = nullptr;
                switch (m_packIndex) {
                    case SKIN_SELECT_PACK_DEFAULT:
                        backupTexture = getTextureId(previousIndex);
                        break;
                    case SKIN_SELECT_PACK_FAVORITES:
                        if (uiCurrentFavoriteC > 0) {
                            // get the pack number from the skin id
                            snprintf(chars, 256, "dlcskin%08d.png",
                                     app.GetPlayerFavoriteSkin(m_iPad,
                                                               previousIndex));

                            Pack =
                                app.m_dlcManager.getPackContainingSkin(chars);
                            if (Pack) {
                                skinFile = Pack->getSkinFile(chars);

                                otherSkinPath = skinFile->getPath();
                                otherCapePath = skinFile->getParameterAsString(
                                    DLCManager::e_DLCParamType_Cape);
                                othervAdditionalSkinBoxes =
                                    skinFile->getAdditionalBoxes();
                                backupTexture = TN_MOB_CHAR;
                            }
                        }

                        break;
                    default:
                        break;
                }
            }
            if (othervAdditionalSkinBoxes &&
                othervAdditionalSkinBoxes->size() != 0) {
                std::vector<ModelPart*>* pAdditionalModelParts =
                    app.GetAdditionalModelParts(skinFile->getSkinID());
                if (pAdditionalModelParts == nullptr) {
                    pAdditionalModelParts = app.SetAdditionalSkinBoxes(
                        skinFile->getSkinID(), othervAdditionalSkinBoxes);
                }
            }
            // 4J-PB - anim override needs set before SetTexture
            if (skinFile) {
                app.SetAnimOverrideBitmask(skinFile->getSkinID(),
                                           skinFile->getAnimOverrideBitmask());
            }
            m_characters[eCharacter_Previous1 + i].SetTexture(otherSkinPath,
                                                              backupTexture);
            m_characters[eCharacter_Previous1 + i].SetCapeTexture(
                otherCapePath);
        }

        previousIndex = getPreviousSkinIndex(previousIndex);
    }

    updateTooltips();
}

TEXTURE_NAME UIScene_SkinSelectMenu::getTextureId(int skinIndex) {
    TEXTURE_NAME texture = TN_MOB_CHAR;
    switch (static_cast<EDefaultSkins>(skinIndex)) {
        case EDefaultSkins::ServerSelected:
        case EDefaultSkins::Skin0:
            texture = TN_MOB_CHAR;
            break;
        case EDefaultSkins::Skin1:
            texture = TN_MOB_CHAR1;
            break;
        case EDefaultSkins::Skin2:
            texture = TN_MOB_CHAR2;
            break;
        case EDefaultSkins::Skin3:
            texture = TN_MOB_CHAR3;
            break;
        case EDefaultSkins::Skin4:
            texture = TN_MOB_CHAR4;
            break;
        case EDefaultSkins::Skin5:
            texture = TN_MOB_CHAR5;
            break;
        case EDefaultSkins::Skin6:
            texture = TN_MOB_CHAR6;
            break;
        case EDefaultSkins::Skin7:
            texture = TN_MOB_CHAR7;
            break;
        case EDefaultSkins::Count:
            break;
    };

    return texture;
}

int UIScene_SkinSelectMenu::getNextSkinIndex(int sourceIndex) {
    int nextSkin = sourceIndex;

    // special case for favourites
    switch (m_packIndex) {
        case SKIN_SELECT_PACK_FAVORITES:
            ++nextSkin;
            if (nextSkin >= app.GetPlayerFavoriteSkinsCount(m_iPad)) {
                nextSkin = 0;
            }

            break;
        default:
            ++nextSkin;

            if (m_packIndex == SKIN_SELECT_PACK_DEFAULT &&
                nextSkin >= std::to_underlying(EDefaultSkins::Count)) {
                nextSkin = std::to_underlying(EDefaultSkins::ServerSelected);
            } else if (m_currentPack != nullptr &&
                       nextSkin >= m_currentPack->getSkinCount()) {
                nextSkin = 0;
            }
            break;
    }

    return nextSkin;
}

int UIScene_SkinSelectMenu::getPreviousSkinIndex(int sourceIndex) {
    int previousSkin = sourceIndex;
    switch (m_packIndex) {
        case SKIN_SELECT_PACK_FAVORITES:
            if (previousSkin == 0) {
                previousSkin = app.GetPlayerFavoriteSkinsCount(m_iPad) - 1;
            } else {
                --previousSkin;
            }
            break;
        default:
            if (previousSkin == 0) {
                if (m_packIndex == SKIN_SELECT_PACK_DEFAULT) {
                    previousSkin = std::to_underlying(EDefaultSkins::Count) - 1;
                } else if (m_currentPack != nullptr) {
                    previousSkin = m_currentPack->getSkinCount() - 1;
                }
            } else {
                --previousSkin;
            }
            break;
    }

    return previousSkin;
}

void UIScene_SkinSelectMenu::handlePackIndexChanged() {
    if (m_packIndex >= SKIN_SELECT_MAX_DEFAULTS) {
        m_currentPack = app.m_dlcManager.getPack(
            m_packIndex - SKIN_SELECT_MAX_DEFAULTS, DLCManager::e_DLCType_Skin);
    } else {
        m_currentPack = nullptr;
    }
    m_skinIndex = 0;
    if (m_currentPack != nullptr) {
        bool found;
        int currentSkinIndex =
            m_currentPack->getSkinIndexAt(m_currentSkinPath, found);
        if (found) m_skinIndex = currentSkinIndex;
    } else {
        switch (m_packIndex) {
            case SKIN_SELECT_PACK_DEFAULT:
                if (!GET_IS_DLC_SKIN_FROM_BITMASK(m_originalSkinId)) {
                    std::uint32_t ugcSkinIndex =
                        GET_UGC_SKIN_ID_FROM_BITMASK(m_originalSkinId);
                    std::uint32_t defaultSkinIndex =
                        GET_DEFAULT_SKIN_ID_FROM_BITMASK(m_originalSkinId);
                    if (ugcSkinIndex == 0) {
                        m_skinIndex = static_cast<int>(defaultSkinIndex);
                    }
                }
                break;
            case SKIN_SELECT_PACK_FAVORITES:
                if (app.GetPlayerFavoriteSkinsCount(m_iPad) > 0) {
                    bool found;
                    char chars[256];
                    // get the pack number from the skin id
                    snprintf(
                        chars, 256, "dlcskin%08d.png",
                        app.GetPlayerFavoriteSkin(
                            m_iPad, app.GetPlayerFavoriteSkinsPos(m_iPad)));

                    DLCPack* Pack =
                        app.m_dlcManager.getPackContainingSkin(chars);
                    if (Pack) {
                        int currentSkinIndex =
                            Pack->getSkinIndexAt(m_currentSkinPath, found);
                        if (found)
                            m_skinIndex = app.GetPlayerFavoriteSkinsPos(m_iPad);
                    }
                }
                break;
            default:
                break;
        }
    }
    handleSkinIndexChanged();
    updatePackDisplay();
}

void UIScene_SkinSelectMenu::updatePackDisplay() {
    m_currentPackCount =
        app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin) +
        SKIN_SELECT_MAX_DEFAULTS;

    if (m_packIndex >= SKIN_SELECT_MAX_DEFAULTS) {
        DLCPack* thisPack = app.m_dlcManager.getPack(
            m_packIndex - SKIN_SELECT_MAX_DEFAULTS, DLCManager::e_DLCType_Skin);
        setCentreLabel(thisPack->getName().c_str());
    } else {
        switch (m_packIndex) {
            case SKIN_SELECT_PACK_DEFAULT:
                setCentreLabel(app.GetString(IDS_NO_SKIN_PACK));
                break;
            case SKIN_SELECT_PACK_FAVORITES:
                setCentreLabel(app.GetString(IDS_FAVORITES_SKIN_PACK));
                break;
        }
    }

    int nextPackIndex = getNextPackIndex(m_packIndex);
    if (nextPackIndex >= SKIN_SELECT_MAX_DEFAULTS) {
        DLCPack* thisPack =
            app.m_dlcManager.getPack(nextPackIndex - SKIN_SELECT_MAX_DEFAULTS,
                                     DLCManager::e_DLCType_Skin);
        setRightLabel(thisPack->getName().c_str());
    } else {
        switch (nextPackIndex) {
            case SKIN_SELECT_PACK_DEFAULT:
                setRightLabel(app.GetString(IDS_NO_SKIN_PACK));
                break;
            case SKIN_SELECT_PACK_FAVORITES:
                setRightLabel(app.GetString(IDS_FAVORITES_SKIN_PACK));
                break;
        }
    }

    int previousPackIndex = getPreviousPackIndex(m_packIndex);
    if (previousPackIndex >= SKIN_SELECT_MAX_DEFAULTS) {
        DLCPack* thisPack = app.m_dlcManager.getPack(
            previousPackIndex - SKIN_SELECT_MAX_DEFAULTS,
            DLCManager::e_DLCType_Skin);
        setLeftLabel(thisPack->getName().c_str());
    } else {
        switch (previousPackIndex) {
            case SKIN_SELECT_PACK_DEFAULT:
                setLeftLabel(app.GetString(IDS_NO_SKIN_PACK));
                break;
            case SKIN_SELECT_PACK_FAVORITES:
                setLeftLabel(app.GetString(IDS_FAVORITES_SKIN_PACK));
                break;
        }
    }
}

int UIScene_SkinSelectMenu::getNextPackIndex(int sourceIndex) {
    int nextPack = sourceIndex;
    ++nextPack;
    if (nextPack > app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin) -
                       1 + SKIN_SELECT_MAX_DEFAULTS) {
        nextPack = SKIN_SELECT_PACK_DEFAULT;
    }

    return nextPack;
}

int UIScene_SkinSelectMenu::getPreviousPackIndex(int sourceIndex) {
    int previousPack = sourceIndex;
    if (previousPack == SKIN_SELECT_PACK_DEFAULT) {
        int packCount =
            app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin);

        if (packCount > 0) {
            previousPack = packCount + SKIN_SELECT_MAX_DEFAULTS - 1;
        } else {
            previousPack = SKIN_SELECT_MAX_DEFAULTS - 1;
        }
    } else {
        --previousPack;
    }

    return previousPack;
}

void UIScene_SkinSelectMenu::setCharacterSelected(bool selected) {
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_boolean;
    value[0].boolval = selected;
    IggyResult out = IggyPlayerCallMethodRS(
        getMovie(), &result, IggyPlayerRootPath(getMovie()),
        m_funcSetPlayerCharacterSelected, 1, value);
}

void UIScene_SkinSelectMenu::setCharacterLocked(bool locked) {
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_boolean;
    value[0].boolval = locked;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetCharacterLocked, 1, value);
}

void UIScene_SkinSelectMenu::setLeftLabel(const std::string& label) {
    if (label.compare(m_leftLabel) != 0) {
        m_leftLabel = label;

        IggyDataValue result;
        IggyDataValue value[1];

        IggyStringUTF8 stringVal;
        stringVal.string = const_cast<char*>(label.c_str());
        stringVal.length = label.length();

        value[0].type = IGGY_DATATYPE_string_UTF8;
        value[0].string8 = stringVal;
        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcSetLeftLabel, 1, value);
    }
}

void UIScene_SkinSelectMenu::setCentreLabel(const std::string& label) {
    if (label.compare(m_centreLabel) != 0) {
        m_centreLabel = label;

        IggyDataValue result;
        IggyDataValue value[1];

        IggyStringUTF8 stringVal;
        stringVal.string = const_cast<char*>(label.c_str());
        stringVal.length = label.length();

        value[0].type = IGGY_DATATYPE_string_UTF8;
        value[0].string8 = stringVal;
        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcSetCentreLabel, 1, value);
    }
}

void UIScene_SkinSelectMenu::setRightLabel(const std::string& label) {
    if (label.compare(m_rightLabel) != 0) {
        m_rightLabel = label;

        IggyDataValue result;
        IggyDataValue value[1];

        IggyStringUTF8 stringVal;
        stringVal.string = const_cast<char*>(label.c_str());
        stringVal.length = label.length();

        value[0].type = IGGY_DATATYPE_string_UTF8;
        value[0].string8 = stringVal;
        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcSetRightLabel, 1, value);
    }
}

void UIScene_SkinSelectMenu::HandleDLCInstalled() {
    app.DebugPrintf(4, "UIScene_SkinSelectMenu::HandleDLCInstalled\n");
    // mounted DLC may have changed
    if (app.StartInstallDLCProcess(m_iPad) == false) {
        // not doing a mount, so re-enable input
        app.DebugPrintf(4,
                        "UIScene_SkinSelectMenu::HandleDLCInstalled - not "
                        "doing a mount, so re-enable input\n");
        m_bIgnoreInput = false;
    } else {
        m_bIgnoreInput = true;
        m_controlTimer.setVisible(true);
        m_controlIggyCharacters.setVisible(false);
        m_controlSkinNamePlate.setVisible(false);
    }

    // this will send a CustomMessage_DLCMountingComplete when done
}

void UIScene_SkinSelectMenu::HandleDLCMountingComplete() {
    app.DebugPrintf(4, "UIScene_SkinSelectMenu::HandleDLCMountingComplete\n");
    m_controlTimer.setVisible(false);
    m_controlIggyCharacters.setVisible(true);
    m_controlSkinNamePlate.setVisible(true);

    m_packIndex = SKIN_SELECT_PACK_DEFAULT;

    if (app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin) > 0) {
        m_currentPack =
            app.m_dlcManager.getPackContainingSkin(m_currentSkinPath);
        if (m_currentPack != nullptr) {
            bool bFound = false;
            m_packIndex =
                app.m_dlcManager.getPackIndex(m_currentPack, bFound,
                                              DLCManager::e_DLCType_Skin) +
                SKIN_SELECT_MAX_DEFAULTS;
        }
    }

    // If we have any favourites, set this to the favourites
    // first validate the favorite skins - we might have uninstalled the DLC
    // needed for them
    app.ValidateFavoriteSkins(m_iPad);

    if (app.GetPlayerFavoriteSkinsCount(m_iPad) > 0) {
        m_packIndex = SKIN_SELECT_PACK_FAVORITES;
    }

    handlePackIndexChanged();

    m_bIgnoreInput = false;
    app.m_dlcManager.checkForCorruptDLCAndAlert();
    bool bInGame = (Minecraft::GetInstance()->level != nullptr);

#if TO_BE_IMPLEMENTED
    if (bInGame) XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_AUTO);
#endif
}

void UIScene_SkinSelectMenu::showNotOnlineDialog(int iPad) {
    // need to be signed in to live. get them to sign in to online
}

int UIScene_SkinSelectMenu::UnlockSkinReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_SkinSelectMenu* pScene = (UIScene_SkinSelectMenu*)pParam;

    if ((result == IPlatformStorage::EMessage_ResultAccept) &&
        PlatformProfile.IsSignedIn(iPad)) {
        if (PlatformProfile.IsSignedInLive(iPad)) {
        } else  // Is signed in, but not live.
        {
            pScene->showNotOnlineDialog(iPad);
            pScene->m_bIgnoreInput = false;
        }
    } else {
        pScene->m_bIgnoreInput = false;
    }

    return 0;
}

int UIScene_SkinSelectMenu::RenableInput(void* lpVoid, int, int) {
    ((UIScene_SkinSelectMenu*)lpVoid)->m_bIgnoreInput = false;
    return 0;
}

void UIScene_SkinSelectMenu::AddFavoriteSkin(int iPad, int iSkinID) {
    // Is this favorite skin already in the array?
    unsigned int uiCurrentFavoriteSkinsCount =
        app.GetPlayerFavoriteSkinsCount(iPad);

    for (int i = 0; i < uiCurrentFavoriteSkinsCount; i++) {
        if (app.GetPlayerFavoriteSkin(m_iPad, i) == iSkinID) {
            app.SetPlayerFavoriteSkinsPos(m_iPad, i);
            return;
        }
    }

    unsigned char ucPos = app.GetPlayerFavoriteSkinsPos(m_iPad);
    if (ucPos == (MAX_FAVORITE_SKINS - 1)) {
        ucPos = 0;
    } else {
        if (uiCurrentFavoriteSkinsCount > 0) {
            ucPos++;
        } else {
            ucPos = 0;
        }
    }

    app.SetPlayerFavoriteSkin(iPad, (int)ucPos, iSkinID);
    app.SetPlayerFavoriteSkinsPos(m_iPad, ucPos);
}

void UIScene_SkinSelectMenu::handleReload() {
    // Reinitialise a few values to prevent problems on reload
    m_bIgnoreInput = false;

    m_currentNavigation = eSkinNavigation_Skin;
    m_currentPackCount = 0;

    m_labelSkinName.init("");
    m_labelSkinOrigin.init("");

    m_leftLabel = "";
    m_centreLabel = "";
    m_rightLabel = "";

    handlePackIndexChanged();
}
