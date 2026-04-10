#include "UIComponent_TutorialPopup.h"

#include <wchar.h>

#include <vector>

#include "app/common/Game.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "platform/profile/profile.h"
#include "strings.h"
#include "util/StringHelpers.h"

UIComponent_TutorialPopup::UIComponent_TutorialPopup(int iPad, void* initData,
                                                     UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_interactScene = nullptr;
    m_lastInteractSceneMoved = nullptr;
    m_lastSceneMovedLeft = false;
    m_bAllowFade = false;
    m_iconItem = nullptr;
    m_iconIsFoil = false;

    m_bContainerMenuVisible = false;
    m_bSplitscreenGamertagVisible = false;
    m_iconType = e_ICON_TYPE_IGGY;

    m_labelDescription.init("");

    // 4jcraft added
    m_tutorial = nullptr;
}

std::string UIComponent_TutorialPopup::getMoviePath() {
    switch (m_parentLayer->getViewport()) {
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
            return "TutorialPopupSplit";
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        default:
            return "TutorialPopup";
            break;
    }
}

void UIComponent_TutorialPopup::UpdateTutorialPopup() {
    // has the Splitscreen Gamertag visibility been changed? Re-Adjust Layout to
    // prevent overlaps!
    if (m_bSplitscreenGamertagVisible !=
        (bool)(app.GetGameSettings(PlatformProfile.GetPrimaryPad(),
                                   eGameSetting_DisplaySplitscreenGamertags) !=
               0)) {
        m_bSplitscreenGamertagVisible =
            (bool)(app.GetGameSettings(
                       PlatformProfile.GetPrimaryPad(),
                       eGameSetting_DisplaySplitscreenGamertags) != 0);
        handleReload();
    }
}

void UIComponent_TutorialPopup::handleReload() {
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_boolean;
    value[0].boolval =
        (bool)((app.GetGameSettings(PlatformProfile.GetPrimaryPad(),
                                    eGameSetting_DisplaySplitscreenGamertags) !=
                0) &&
               !m_bContainerMenuVisible);  // 4J - TomK - Offset for splitscreen
                                           // gamertag?
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcAdjustLayout, 1, value);

    setupIconHolder(m_iconType);
}

void UIComponent_TutorialPopup::SetTutorialDescription(
    TutorialPopupInfo* info) {
    m_interactScene = info->interactScene;

    // 4jcraft added
    m_tutorial = info->tutorial;

    std::string parsed =
        _SetIcon(info->icon, info->iAuxVal, info->isFoil, info->desc);
    parsed = _SetImage(parsed);
    parsed = ParseDescription(m_iPad, parsed);

    if (parsed.empty()) {
        _SetDescription(info->interactScene, "", "", info->allowFade,
                        info->isReminder);
    } else {
        _SetDescription(info->interactScene, parsed, info->title,
                        info->allowFade, info->isReminder);
    }
}

void UIComponent_TutorialPopup::RemoveInteractSceneReference(UIScene* scene) {
    if (m_interactScene == scene) {
        m_interactScene = nullptr;
    }
}

void UIComponent_TutorialPopup::SetVisible(bool visible) {
    m_parentLayer->showComponent(0, eUIComponent_TutorialPopup, visible);

    if (visible && m_bAllowFade) {
        // Initialise a timer to fade us out again
        app.DebugPrintf(
            "UIComponent_TutorialPopup::SetVisible: setting "
            "TUTORIAL_POPUP_FADE_TIMER_ID to %d\n",
            m_tutorial->GetTutorialDisplayMessageTime());
        addTimer(TUTORIAL_POPUP_FADE_TIMER_ID,
                 m_tutorial->GetTutorialDisplayMessageTime());
    }
}

bool UIComponent_TutorialPopup::IsVisible() {
    return m_parentLayer->isComponentVisible(eUIComponent_TutorialPopup);
}

void UIComponent_TutorialPopup::handleTimerComplete(int id) {
    switch (id) {
        case TUTORIAL_POPUP_FADE_TIMER_ID:
            SetVisible(false);
            killTimer(id);
            app.DebugPrintf(
                "handleTimerComplete: setting "
                "TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID\n");
            addTimer(TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID,
                     TUTORIAL_POPUP_MOVE_SCENE_TIME);
            break;
        case TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID:
            UpdateInteractScenePosition(IsVisible());
            killTimer(id);
            break;
    }
}

void UIComponent_TutorialPopup::_SetDescription(UIScene* interactScene,
                                                const std::string& desc,
                                                const std::string& title,
                                                bool allowFade,
                                                bool isReminder) {
    m_interactScene = interactScene;
    app.DebugPrintf("Setting m_interactScene to %08x\n", m_interactScene);
    if (interactScene != m_lastInteractSceneMoved)
        m_lastInteractSceneMoved = nullptr;
    if (desc.empty()) {
        SetVisible(false);
        app.DebugPrintf(
            "_SetDescription1: setting TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID\n");
        addTimer(TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID,
                 TUTORIAL_POPUP_MOVE_SCENE_TIME);
        killTimer(TUTORIAL_POPUP_FADE_TIMER_ID);
    } else {
        SetVisible(true);
        app.DebugPrintf(
            "_SetDescription2: setting TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID\n");
        addTimer(TUTORIAL_POPUP_MOVE_SCENE_TIMER_ID,
                 TUTORIAL_POPUP_MOVE_SCENE_TIME);

        if (allowFade) {
            // Initialise a timer to fade us out again
            app.DebugPrintf(
                "_SetDescription: setting TUTORIAL_POPUP_FADE_TIMER_ID\n");
            addTimer(TUTORIAL_POPUP_FADE_TIMER_ID,
                     m_tutorial->GetTutorialDisplayMessageTime());
        } else {
            app.DebugPrintf(
                "_SetDescription: killing TUTORIAL_POPUP_FADE_TIMER_ID\n");
            killTimer(TUTORIAL_POPUP_FADE_TIMER_ID);
        }
        m_bAllowFade = allowFade;

        if (isReminder) {
            std::string text(app.GetString(IDS_TUTORIAL_REMINDER));
            text.append(desc);
            stripWhitespaceForHtml(text);
            // set the text colour
            char formatting[40];
            // 4J Stu - Don't set HTML font size, that's set at design time in
            // flash
            // snprintf(formatting, 40, "<font color=\"#%08x\"
            // size=\"%d\">",app.GetHTMLColour(eHTMLColor_White),m_textFontSize);
            snprintf(formatting, 40, "<font color=\"#%08x\">",
                     app.GetHTMLColour(eHTMLColor_White));
            text = formatting + text;

            m_labelDescription.setLabel(text, true);
        } else {
            std::string text(desc);
            stripWhitespaceForHtml(text);
            // set the text colour
            char formatting[40];
            // 4J Stu - Don't set HTML font size, that's set at design time in
            // flash
            // snprintf(formatting, 40, "<font color=\"#%08x\"
            // size=\"%d\">",app.GetHTMLColour(eHTMLColor_White),m_textFontSize);
            snprintf(formatting, 40, "<font color=\"#%08x\">",
                     app.GetHTMLColour(eHTMLColor_White));
            text = formatting + text;

            m_labelDescription.setLabel(text, true);
        }

        m_labelTitle.setLabel(title, true);
        m_labelTitle.setVisible(!title.empty());

        // read host setting if gamertag is visible or not and pass on to Adjust
        // Layout function (so we can offset it to stay clear of the gamertag)
        m_bSplitscreenGamertagVisible =
            (bool)(app.GetGameSettings(
                       PlatformProfile.GetPrimaryPad(),
                       eGameSetting_DisplaySplitscreenGamertags) != 0);
        IggyDataValue result;
        IggyDataValue value[1];
        value[0].type = IGGY_DATATYPE_boolean;
        value[0].boolval =
            (m_bSplitscreenGamertagVisible &&
             !m_bContainerMenuVisible);  // 4J - TomK - Offset for splitscreen
                                         // gamertag?
        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcAdjustLayout, 1, value);
    }
}

std::string UIComponent_TutorialPopup::_SetIcon(int icon, int iAuxVal,
                                                bool isFoil, const char* desc) {
    std::string temp(desc);

    bool isFixedIcon = false;

    m_iconIsFoil = isFoil;
    if (icon != TUTORIAL_NO_ICON) {
        m_iconIsFoil = false;
        m_iconItem =
            std::shared_ptr<ItemInstance>(new ItemInstance(icon, 1, iAuxVal));
    } else {
        m_iconItem = nullptr;
        std::string openTag("{*ICON*}");
        std::string closeTag("{*/ICON*}");
        int iconTagStartPos = (int)temp.find(openTag);
        int iconStartPos = iconTagStartPos + (int)openTag.length();
        if (iconTagStartPos > 0 && iconStartPos < (int)temp.length()) {
            int iconEndPos = (int)temp.find(closeTag, iconStartPos);

            if (iconEndPos > iconStartPos && iconEndPos < (int)temp.length()) {
                std::string id =
                    temp.substr(iconStartPos, iconEndPos - iconStartPos);

                std::vector<std::string> idAndAux = stringSplit(id, ':');

                int iconId = fromWString<int>(idAndAux[0]);

                if (idAndAux.size() > 1) {
                    iAuxVal = fromWString<int>(idAndAux[1]);
                } else {
                    iAuxVal = 0;
                }
                m_iconItem = std::shared_ptr<ItemInstance>(
                    new ItemInstance(iconId, 1, iAuxVal));

                temp.replace(iconTagStartPos,
                             iconEndPos - iconTagStartPos + closeTag.length(),
                             "");
            }
        }

        // remove any icon text
        else if (temp.find("{*CraftingTableIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::workBench_Id, 1, 0));
        } else if (temp.find("{*SticksIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::stick_Id, 1, 0));
        } else if (temp.find("{*PlanksIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::wood_Id, 1, 0));
        } else if (temp.find("{*WoodenShovelIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::shovel_wood_Id, 1, 0));
        } else if (temp.find("{*WoodenHatchetIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::hatchet_wood_Id, 1, 0));
        } else if (temp.find("{*WoodenPickaxeIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::pickAxe_wood_Id, 1, 0));
        } else if (temp.find("{*FurnaceIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::furnace_Id, 1, 0));
        } else if (temp.find("{*WoodenDoorIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::door_wood, 1, 0));
        } else if (temp.find("{*TorchIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::torch_Id, 1, 0));
        } else if (temp.find("{*BoatIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::boat_Id, 1, 0));
        } else if (temp.find("{*FishingRodIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::fishingRod_Id, 1, 0));
        } else if (temp.find("{*FishIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::fish_raw_Id, 1, 0));
        } else if (temp.find("{*MinecartIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::minecart_Id, 1, 0));
        } else if (temp.find("{*RailIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::rail_Id, 1, 0));
        } else if (temp.find("{*PoweredRailIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::goldenRail_Id, 1, 0));
        } else if (temp.find("{*StructuresIcon*}") != std::string::npos) {
            isFixedIcon = true;
            setupIconHolder(e_ICON_TYPE_STRUCTURES);
        } else if (temp.find("{*ToolsIcon*}") != std::string::npos) {
            isFixedIcon = true;
            setupIconHolder(e_ICON_TYPE_TOOLS);
        } else if (temp.find("{*StoneIcon*}") != std::string::npos) {
            m_iconItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Tile::stone_Id, 1, 0));
        } else {
            m_iconItem = nullptr;
        }
    }
    if (!isFixedIcon && m_iconItem != nullptr)
        setupIconHolder(e_ICON_TYPE_IGGY);
    m_controlIconHolder.setVisible(isFixedIcon || m_iconItem != nullptr);

    return temp;
}

std::string UIComponent_TutorialPopup::_SetImage(std::string& desc) {
    // 4J Stu - Unused
    return desc;
}

std::string UIComponent_TutorialPopup::ParseDescription(int iPad,
                                                        std::string& text) {
    text = replaceAll(text, "{*CraftingTableIcon*}", "");
    text = replaceAll(text, "{*SticksIcon*}", "");
    text = replaceAll(text, "{*PlanksIcon*}", "");
    text = replaceAll(text, "{*WoodenShovelIcon*}", "");
    text = replaceAll(text, "{*WoodenHatchetIcon*}", "");
    text = replaceAll(text, "{*WoodenPickaxeIcon*}", "");
    text = replaceAll(text, "{*FurnaceIcon*}", "");
    text = replaceAll(text, "{*WoodenDoorIcon*}", "");
    text = replaceAll(text, "{*TorchIcon*}", "");
    text = replaceAll(text, "{*MinecartIcon*}", "");
    text = replaceAll(text, "{*BoatIcon*}", "");
    text = replaceAll(text, "{*FishingRodIcon*}", "");
    text = replaceAll(text, "{*FishIcon*}", "");
    text = replaceAll(text, "{*RailIcon*}", "");
    text = replaceAll(text, "{*PoweredRailIcon*}", "");
    text = replaceAll(text, "{*StructuresIcon*}", "");
    text = replaceAll(text, "{*ToolsIcon*}", "");
    text = replaceAll(text, "{*StoneIcon*}", "");

    bool exitScreenshot = false;
    size_t pos = text.find("{*EXIT_PICTURE*}");
    if (pos != std::string::npos) exitScreenshot = true;
    text = replaceAll(text, "{*EXIT_PICTURE*}", "");
    m_controlExitScreenshot.setVisible(exitScreenshot);
    /*
#define MINECRAFT_ACTION_RENDER_DEBUG		ACTION_INGAME_13
#define MINECRAFT_ACTION_PAUSEMENU			ACTION_INGAME_15
#define MINECRAFT_ACTION_SNEAK_TOGGLE		ACTION_INGAME_17
*/

    return app.FormatHTMLString(iPad, text);
}

void UIComponent_TutorialPopup::UpdateInteractScenePosition(bool visible) {
    if (m_interactScene == nullptr) return;

    // 4J-PB - check this players screen section to see if we should allow the
    // animation
    bool bAllowAnim = false;
    bool isCraftingScene =
        (m_interactScene->getSceneType() == eUIScene_Crafting2x2Menu) ||
        (m_interactScene->getSceneType() == eUIScene_Crafting3x3Menu);
    bool isCreativeScene =
        (m_interactScene->getSceneType() == eUIScene_CreativeMenu);
    bool isTradingScene =
        (m_interactScene->getSceneType() == eUIScene_TradingMenu);
    switch (Minecraft::GetInstance()->localplayers[m_iPad]->m_iScreenSection) {
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
            bAllowAnim = true;
            break;
        default:
            // anim allowed for everything except the crafting 2x2 and 3x3, and
            // the creative menu
            if (!isCraftingScene && !isCreativeScene && !isTradingScene) {
                bAllowAnim = true;
            }
            break;
    }

    if (bAllowAnim) {
        bool movingLeft = visible;

        if ((m_lastInteractSceneMoved != m_interactScene && movingLeft) ||
            (m_lastInteractSceneMoved == m_interactScene &&
             m_lastSceneMovedLeft != movingLeft)) {
            if (movingLeft) {
                m_interactScene->slideLeft();
            } else {
                m_interactScene->slideRight();
            }

            m_lastInteractSceneMoved = m_interactScene;
            m_lastSceneMovedLeft = movingLeft;
        }
    }
}

void UIComponent_TutorialPopup::render(
    S32 width, S32 height, IPlatformRenderer::eViewportType viewport) {
    if (viewport != IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN) {
        S32 xPos = 0;
        S32 yPos = 0;
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
                xPos = (S32)(ui.getScreenWidth() / 2);
                yPos = (S32)(ui.getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
                yPos = (S32)(ui.getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
                xPos = (S32)(ui.getScreenWidth() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
                xPos = (S32)(ui.getScreenWidth() / 2);
                yPos = (S32)(ui.getScreenHeight() / 2);
                break;
            default:
                break;
        }
        // Adjust for safezone
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
                yPos += getSafeZoneHalfHeight();
                break;
            default:
                break;
        }
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
                xPos -= getSafeZoneHalfWidth();
                break;
            default:
                break;
        }
        ui.setupRenderPosition(xPos, yPos);

        IggyPlayerSetDisplaySize(getMovie(), width, height);
        IggyPlayerDraw(getMovie());
    } else {
        UIScene::render(width, height, viewport);
    }
}

void UIComponent_TutorialPopup::customDraw(
    IggyCustomDrawCallbackRegion* region) {
    if (m_iconItem != nullptr)
        customDrawSlotControl(region, m_iPad, m_iconItem, 1.0f,
                              m_iconItem->isFoil() || m_iconIsFoil, false);
}

void UIComponent_TutorialPopup::setupIconHolder(EIcons icon) {
    app.DebugPrintf("Setting icon holder to %d\n", icon);
    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = (F64)icon;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetupIconHolder, 1, value);

    m_iconType = icon;
}
