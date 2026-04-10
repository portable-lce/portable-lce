#include "UILayer.h"

#include <algorithm>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Components/UIComponent_Chat.h"
#include "app/common/UI/Components/UIComponent_DebugUIConsole.h"
#include "app/common/UI/Components/UIComponent_DebugUIMarketingGuide.h"
#include "app/common/UI/Components/UIComponent_Logo.h"
#include "app/common/UI/Components/UIComponent_MenuBackground.h"
#include "app/common/UI/Components/UIComponent_Panorama.h"
#include "app/common/UI/Components/UIComponent_PressStartToPlay.h"
#include "app/common/UI/Components/UIComponent_Tooltips.h"
#include "app/common/UI/Components/UIComponent_TutorialPopup.h"
#include "app/common/UI/Components/UIScene_HUD.h"
#include "app/common/UI/Scenes/Debug/UIScene_DebugCreateSchematic.h"
#include "app/common/UI/Scenes/Debug/UIScene_DebugOptions.h"
#include "app/common/UI/Scenes/Debug/UIScene_DebugOverlay.h"
#include "app/common/UI/Scenes/Debug/UIScene_DebugSetCamera.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_CreateWorldMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_DLCMainMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_DLCOffersMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_EULA.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_Intro.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_JoinMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_LaunchMoreOptionsMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_LeaderboardsMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_LoadMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_LoadOrJoinMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_MainMenu.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_NewUpdateMessage.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_SaveMessage.h"
#include "app/common/UI/Scenes/Frontend Menu screens/UIScene_TrialExitUpsell.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_ControlsMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_Credits.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_HelpAndOptionsMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_HowToPlay.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_HowToPlayMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_LanguageSelector.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_ReinstallMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SettingsAudioMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SettingsControlMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SettingsGraphicsMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SettingsMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SettingsOptionsMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SettingsUIMenu.h"
#include "app/common/UI/Scenes/Help & Options/UIScene_SkinSelectMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_AnvilMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_BeaconMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_BrewingStandMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_ContainerMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_CreativeMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_DispenserMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_EnchantingMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_FireworksMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_FurnaceMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_HopperMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_HorseInventoryMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_InventoryMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_TradingMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_CraftingMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_DeathMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_EndPoem.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_InGameHostOptionsMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_InGameInfoMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_InGamePlayerOptionsMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_PauseMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_SignEntryMenu.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/UIScene_TeleportMenu.h"
#include "app/common/UI/Scenes/UIScene_ConnectingProgress.h"
#include "app/common/UI/Scenes/UIScene_FullscreenProgress.h"
#include "app/common/UI/Scenes/UIScene_Keyboard.h"
#include "app/common/UI/Scenes/UIScene_MessageBox.h"
#include "app/common/UI/Scenes/UIScene_QuadrantSignin.h"
#include "app/common/UI/Scenes/UIScene_Timer.h"
#include "app/common/UI/UIGroup.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "app/common/Game.h"
#include "app/linux/Linux_UIController.h"
#include "platform/renderer/renderer.h"

UILayer::UILayer(UIGroup* parent) {
    m_parentGroup = parent;
    m_hasFocus = false;
    m_bMenuDisplayed = false;
    m_bPauseMenuDisplayed = false;
    m_bContainerMenuDisplayed = false;
    m_bIgnoreAutosaveMenuDisplayed = false;
    m_bIgnorePlayerJoinMenuDisplayed = false;
}

void UILayer::tick() {
    // Delete old scenes - deleting a scene can cause a new scene to be deleted,
    // so we need to make a copy of the scenes that we are going to try and
    // destroy this tick
    std::vector<UIScene*> scenesToDeleteCopy;
    for (auto it = m_scenesToDelete.begin(); it != m_scenesToDelete.end();
         it++) {
        UIScene* scene = (*it);
        scenesToDeleteCopy.push_back(scene);
    }
    m_scenesToDelete.clear();

    // Delete the scenes in our copy if they are ready to delete, otherwise add
    // back to the ones that are still to be deleted. Actually deleting a scene
    // might also add something back into m_scenesToDelete.
    for (auto it = scenesToDeleteCopy.begin(); it != scenesToDeleteCopy.end();
         it++) {
        UIScene* scene = (*it);
        if (scene->isReadyToDelete()) {
            delete scene;
        } else {
            m_scenesToDelete.push_back(scene);
        }
    }

    while (!m_scenesToDestroy.empty()) {
        UIScene* scene = m_scenesToDestroy.back();
        m_scenesToDestroy.pop_back();
        scene->destroyMovie();
    }
    m_scenesToDestroy.clear();

    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        (*it)->tick();
    }
    // Note: reverse iterator, the last element is the top of the stack
    int sceneIndex = m_sceneStack.size() - 1;
    // for(auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it)
    while (sceneIndex >= 0 && sceneIndex < m_sceneStack.size()) {
        //(*it)->tick();
        UIScene* scene = m_sceneStack[sceneIndex];
        scene->tick();
        --sceneIndex;
        // TODO: We may wish to ignore ticking the rest of the stack based on
        // this scene
    }
}

void UILayer::render(S32 width, S32 height,
                     IPlatformRenderer::eViewportType viewport) {
    if (!ui.IsExpectingOrReloadingSkin()) {
        for (auto it = m_components.begin(); it != m_components.end(); ++it) {
            auto itRef = m_componentRefCount.find((*it)->getSceneType());
            if (itRef != m_componentRefCount.end() && itRef->second.second) {
                if ((*it)->isVisible()) {
                    (*it)->render(width, height, viewport);
                }
            }
        }
    }
    if (!m_sceneStack.empty()) {
        int lowestRenderable = m_sceneStack.size() - 1;
        for (; lowestRenderable >= 0; --lowestRenderable) {
            if (m_sceneStack[lowestRenderable]->hidesLowerScenes()) break;
        }
        if (lowestRenderable < 0) lowestRenderable = 0;
        for (; lowestRenderable < m_sceneStack.size(); ++lowestRenderable) {
            if (m_sceneStack[lowestRenderable]->isVisible() &&
                (!ui.IsExpectingOrReloadingSkin() ||
                 m_sceneStack[lowestRenderable]->getSceneType() ==
                     eUIScene_Timer)) {
                m_sceneStack[lowestRenderable]->render(width, height, viewport);
            }
        }
    }
}

bool UILayer::IsSceneInStack(EUIScene scene) {
    bool inStack = false;
    for (int i = m_sceneStack.size() - 1; i >= 0; --i) {
        if (m_sceneStack[i]->getSceneType() == scene) {
            inStack = true;
            break;
        }
    }
    return inStack;
}

bool UILayer::HasFocus(int iPad) {
    bool hasFocus = false;
    if (m_hasFocus) {
        for (int i = m_sceneStack.size() - 1; i >= 0; --i) {
            if (m_sceneStack[i]->stealsFocus()) {
                if (m_sceneStack[i]->hasFocus(iPad)) {
                    hasFocus = true;
                }
                break;
            }
        }
    }
    return hasFocus;
}

bool UILayer::hidesLowerScenes() {
    bool hidesScenes = false;
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        if ((*it)->hidesLowerScenes()) {
            hidesScenes = true;
            break;
        }
    }
    if (!hidesScenes && !m_sceneStack.empty()) {
        for (int i = m_sceneStack.size() - 1; i >= 0; --i) {
            if (m_sceneStack[i]->hidesLowerScenes()) {
                hidesScenes = true;
                break;
            }
        }
    }
    return hidesScenes;
}

void UILayer::getRenderDimensions(S32& width, S32& height) {
    m_parentGroup->getRenderDimensions(width, height);
}

void UILayer::DestroyAll() {
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        (*it)->destroyMovie();
    }
    for (auto it = m_sceneStack.begin(); it != m_sceneStack.end(); ++it) {
        (*it)->destroyMovie();
    }
}

void UILayer::ReloadAll(bool force) {
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        (*it)->reloadMovie(force);
    }
    if (!m_sceneStack.empty()) {
        int lowestRenderable = 0;
        for (; lowestRenderable < m_sceneStack.size(); ++lowestRenderable) {
            m_sceneStack[lowestRenderable]->reloadMovie(force);
        }
    }
}

bool UILayer::GetMenuDisplayed() { return m_bMenuDisplayed; }

bool UILayer::NavigateToScene(int iPad, EUIScene scene, void* initData) {
    UIScene* newScene = nullptr;
    switch (scene) {
        // Debug
#if defined(_DEBUG_MENUS_ENABLED)
        case eUIScene_DebugOverlay:
            newScene = new UIScene_DebugOverlay(iPad, initData, this);
            break;
        case eUIScene_DebugSetCamera:
            newScene = new UIScene_DebugSetCamera(iPad, initData, this);
            break;
        case eUIScene_DebugCreateSchematic:
            newScene = new UIScene_DebugCreateSchematic(iPad, initData, this);
            break;
#endif
        case eUIScene_DebugOptions:
            newScene = new UIScene_DebugOptionsMenu(iPad, initData, this);
            break;

            // Containers
        case eUIScene_InventoryMenu:
            newScene = new UIScene_InventoryMenu(iPad, initData, this);
            break;
        case eUIScene_CreativeMenu:
            newScene = new UIScene_CreativeMenu(iPad, initData, this);
            break;
        case eUIScene_ContainerMenu:
        case eUIScene_LargeContainerMenu:
            newScene = new UIScene_ContainerMenu(iPad, initData, this);
            break;
        case eUIScene_BrewingStandMenu:
            newScene = new UIScene_BrewingStandMenu(iPad, initData, this);
            break;
        case eUIScene_DispenserMenu:
            newScene = new UIScene_DispenserMenu(iPad, initData, this);
            break;
        case eUIScene_EnchantingMenu:
            newScene = new UIScene_EnchantingMenu(iPad, initData, this);
            break;
        case eUIScene_FurnaceMenu:
            newScene = new UIScene_FurnaceMenu(iPad, initData, this);
            break;
        case eUIScene_Crafting2x2Menu:
        case eUIScene_Crafting3x3Menu:
            newScene = new UIScene_CraftingMenu(iPad, initData, this);
            break;
        case eUIScene_TradingMenu:
            newScene = new UIScene_TradingMenu(iPad, initData, this);
            break;
        case eUIScene_AnvilMenu:
            newScene = new UIScene_AnvilMenu(iPad, initData, this);
            break;
        case eUIScene_HopperMenu:
            newScene = new UIScene_HopperMenu(iPad, initData, this);
            break;
        case eUIScene_BeaconMenu:
            newScene = new UIScene_BeaconMenu(iPad, initData, this);
            break;
        case eUIScene_HorseMenu:
            newScene = new UIScene_HorseInventoryMenu(iPad, initData, this);
            break;
        case eUIScene_FireworksMenu:
            newScene = new UIScene_FireworksMenu(iPad, initData, this);
            break;

            // Help and Options
        case eUIScene_HelpAndOptionsMenu:
            newScene = new UIScene_HelpAndOptionsMenu(iPad, initData, this);
            break;
        case eUIScene_SettingsMenu:
            newScene = new UIScene_SettingsMenu(iPad, initData, this);
            break;
        case eUIScene_SettingsOptionsMenu:
            newScene = new UIScene_SettingsOptionsMenu(iPad, initData, this);
            break;
        case eUIScene_SettingsAudioMenu:
            newScene = new UIScene_SettingsAudioMenu(iPad, initData, this);
            break;
        case eUIScene_SettingsControlMenu:
            newScene = new UIScene_SettingsControlMenu(iPad, initData, this);
            break;
        case eUIScene_SettingsGraphicsMenu:
            newScene = new UIScene_SettingsGraphicsMenu(iPad, initData, this);
            break;
        case eUIScene_SettingsUIMenu:
            newScene = new UIScene_SettingsUIMenu(iPad, initData, this);
            break;
        case eUIScene_SkinSelectMenu:
            newScene = new UIScene_SkinSelectMenu(iPad, initData, this);
            break;
        case eUIScene_HowToPlayMenu:
            newScene = new UIScene_HowToPlayMenu(iPad, initData, this);
            break;
        case eUIScene_LanguageSelector:
            newScene = new UIScene_LanguageSelector(iPad, initData, this);
            break;
        case eUIScene_HowToPlay:
            newScene = new UIScene_HowToPlay(iPad, initData, this);
            break;
        case eUIScene_ControlsMenu:
            newScene = new UIScene_ControlsMenu(iPad, initData, this);
            break;
        case eUIScene_ReinstallMenu:
            newScene = new UIScene_ReinstallMenu(iPad, initData, this);
            break;
        case eUIScene_Credits:
            newScene = new UIScene_Credits(iPad, initData, this);
            break;

            // Other in-game
        case eUIScene_PauseMenu:
            newScene = new UIScene_PauseMenu(iPad, initData, this);
            break;
        case eUIScene_DeathMenu:
            newScene = new UIScene_DeathMenu(iPad, initData, this);
            break;
        case eUIScene_ConnectingProgress:
            newScene = new UIScene_ConnectingProgress(iPad, initData, this);
            break;
        case eUIScene_SignEntryMenu:
            newScene = new UIScene_SignEntryMenu(iPad, initData, this);
            break;
        case eUIScene_InGameInfoMenu:
            newScene = new UIScene_InGameInfoMenu(iPad, initData, this);
            break;
        case eUIScene_InGameHostOptionsMenu:
            newScene = new UIScene_InGameHostOptionsMenu(iPad, initData, this);
            break;
        case eUIScene_InGamePlayerOptionsMenu:
            newScene =
                new UIScene_InGamePlayerOptionsMenu(iPad, initData, this);
            break;
        case eUIScene_TeleportMenu:
            newScene = new UIScene_TeleportMenu(iPad, initData, this);
            break;
        case eUIScene_EndPoem:
            if (IsSceneInStack(eUIScene_EndPoem)) {
                app.DebugPrintf("Skipped EndPoem as one was already showing\n");
                return false;
            } else {
                newScene = new UIScene_EndPoem(iPad, initData, this);
            }
            break;

            // Frontend
        case eUIScene_TrialExitUpsell:
            newScene = new UIScene_TrialExitUpsell(iPad, initData, this);
            break;
        case eUIScene_Intro:
            newScene = new UIScene_Intro(iPad, initData, this);
            break;
        case eUIScene_SaveMessage:
            newScene = new UIScene_SaveMessage(iPad, initData, this);
            break;
        case eUIScene_MainMenu:
            newScene = new UIScene_MainMenu(iPad, initData, this);
            break;
        case eUIScene_LoadOrJoinMenu:
            newScene = new UIScene_LoadOrJoinMenu(iPad, initData, this);
            break;
        case eUIScene_LoadMenu:
            newScene = new UIScene_LoadMenu(iPad, initData, this);
            break;
        case eUIScene_JoinMenu:
            newScene = new UIScene_JoinMenu(iPad, initData, this);
            break;
        case eUIScene_CreateWorldMenu:
            newScene = new UIScene_CreateWorldMenu(iPad, initData, this);
            break;
        case eUIScene_LaunchMoreOptionsMenu:
            newScene = new UIScene_LaunchMoreOptionsMenu(iPad, initData, this);
            break;
        case eUIScene_FullscreenProgress:
            newScene = new UIScene_FullscreenProgress(iPad, initData, this);
            break;
        case eUIScene_LeaderboardsMenu:
            newScene = new UIScene_LeaderboardsMenu(iPad, initData, this);
            break;
        case eUIScene_DLCMainMenu:
            newScene = new UIScene_DLCMainMenu(iPad, initData, this);
            break;
        case eUIScene_DLCOffersMenu:
            newScene = new UIScene_DLCOffersMenu(iPad, initData, this);
            break;
        case eUIScene_EULA:
            newScene = new UIScene_EULA(iPad, initData, this);
            break;
        case eUIScene_NewUpdateMessage:
            newScene = new UIScene_NewUpdateMessage(iPad, initData, this);
            break;

            // Other
        case eUIScene_Keyboard:
            newScene = new UIScene_Keyboard(iPad, initData, this);
            break;
        case eUIScene_QuadrantSignin:
            newScene = new UIScene_QuadrantSignin(iPad, initData, this);
            break;
        case eUIScene_MessageBox:
            if (IsSceneInStack(eUIScene_MessageBox)) {
                app.DebugPrintf(
                    "Skipped MessageBox as one was already showing\n");
                return false;
            } else {
                newScene = new UIScene_MessageBox(iPad, initData, this);
            }
            break;
        case eUIScene_Timer:
            newScene = new UIScene_Timer(iPad, initData, this);
            break;
        default:
            break;
    };

    if (newScene == nullptr) {
        app.DebugPrintf(
            "WARNING: Scene %d was not created. Add it to "
            "UILayer::NavigateToScene\n",
            scene);
        return false;
    }

    if (m_sceneStack.size() > 0) {
        newScene->setBackScene(m_sceneStack[m_sceneStack.size() - 1]);
    }

    m_sceneStack.push_back(newScene);

    updateFocusState();

    newScene->tick();

    return true;
}

bool UILayer::NavigateBack(int iPad, EUIScene eScene) {
    if (m_sceneStack.size() == 0) return false;

    bool navigated = false;
    if (eScene < eUIScene_COUNT) {
        UIScene* scene = nullptr;
        do {
            scene = m_sceneStack.back();
            if (scene->getSceneType() == eScene) {
                navigated = true;
                break;
            } else {
                if (scene->hasFocus(iPad)) {
                    removeScene(scene);
                } else {
                    // No focus on the top scene, so this use shouldn't be
                    // navigating!
                    break;
                }
            }
        } while (m_sceneStack.size() > 0);

    } else {
        UIScene* scene = m_sceneStack.back();
        if (scene->hasFocus(iPad)) {
            removeScene(scene);
            navigated = true;
        }
    }
    return navigated;
}

void UILayer::showComponent(int iPad, EUIScene scene, bool show) {
    auto it = m_componentRefCount.find(scene);
    if (it != m_componentRefCount.end()) {
        it->second.second = show;
        return;
    }
    if (show) addComponent(iPad, scene);
}

bool UILayer::isComponentVisible(EUIScene scene) {
    bool visible = false;
    auto it = m_componentRefCount.find(scene);
    if (it != m_componentRefCount.end()) {
        visible = it->second.second;
    }
    return visible;
}

UIScene* UILayer::addComponent(int iPad, EUIScene scene, void* initData) {
    auto it = m_componentRefCount.find(scene);
    if (it != m_componentRefCount.end()) {
        ++it->second.first;

        for (auto itComp = m_components.begin(); itComp != m_components.end();
             ++itComp) {
            if ((*itComp)->getSceneType() == scene) {
                return *itComp;
            }
        }
        return nullptr;
    }
    UIScene* newScene = nullptr;

    switch (scene) {
        case eUIComponent_Panorama:
            newScene = new UIComponent_Panorama(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_DebugUIConsole:
            newScene = new UIComponent_DebugUIConsole(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_DebugUIMarketingGuide:
            newScene =
                new UIComponent_DebugUIMarketingGuide(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_Logo:
            newScene = new UIComponent_Logo(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_Tooltips:
            newScene = new UIComponent_Tooltips(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_TutorialPopup:
            newScene = new UIComponent_TutorialPopup(iPad, initData, this);
            // Start hidden
            m_componentRefCount[scene] = std::pair<int, bool>(1, false);
            break;
        case eUIScene_HUD:
            newScene = new UIScene_HUD(iPad, initData, this);
            // Start hidden
            m_componentRefCount[scene] = std::pair<int, bool>(1, false);
            break;
        case eUIComponent_Chat:
            newScene = new UIComponent_Chat(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_PressStartToPlay:
            newScene = new UIComponent_PressStartToPlay(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        case eUIComponent_MenuBackground:
            newScene = new UIComponent_MenuBackground(iPad, initData, this);
            m_componentRefCount[scene] = std::pair<int, bool>(1, true);
            break;
        default:
            break;
    };

    if (newScene == nullptr) return nullptr;

    m_components.push_back(newScene);

    return newScene;
}

void UILayer::removeComponent(EUIScene scene) {
    auto it = m_componentRefCount.find(scene);
    if (it != m_componentRefCount.end()) {
        --it->second.first;

        if (it->second.first <= 0) {
            m_componentRefCount.erase(it);
            for (auto compIt = m_components.begin();
                 compIt != m_components.end();) {
                if ((*compIt)->getSceneType() == scene) {
                    m_scenesToDelete.push_back((*compIt));
                    (*compIt)->handleDestroy();  // For anything that might
                                                 // require the pointer be valid
                    compIt = m_components.erase(compIt);
                } else {
                    ++compIt;
                }
            }
        }
    }
}

void UILayer::removeScene(UIScene* scene) {
    auto newEnd = std::remove(m_sceneStack.begin(), m_sceneStack.end(), scene);
    m_sceneStack.erase(newEnd, m_sceneStack.end());

    m_scenesToDelete.push_back(scene);

    scene->handleDestroy();  // For anything that might require the pointer be
                             // valid

    bool hadFocus = m_hasFocus;
    updateFocusState();

    // If this layer has focus, pass it on
    if (m_hasFocus || hadFocus) {
        m_hasFocus = false;
        m_parentGroup->UpdateFocusState();
    }
}

void UILayer::closeAllScenes() {
    std::vector<UIScene*> temp;
    temp.insert(temp.end(), m_sceneStack.begin(), m_sceneStack.end());
    m_sceneStack.clear();
    for (auto it = temp.begin(); it != temp.end(); ++it) {
        m_scenesToDelete.push_back(*it);
        (*it)->handleDestroy();  // For anything that might require the pointer
                                 // be valid
    }

    updateFocusState();

    // If this layer has focus, pass it on
    if (m_hasFocus) {
        m_hasFocus = false;
        m_parentGroup->UpdateFocusState();
    }
}

// Get top scene on stack (or nullptr if stack is empty)
UIScene* UILayer::GetTopScene() {
    if (m_sceneStack.size() == 0) {
        return nullptr;
    } else {
        return m_sceneStack[m_sceneStack.size() - 1];
    }
}

// Updates layer focus state if no error message is present (unless this is the
// error layer)
bool UILayer::updateFocusState(bool allowedFocus /* = false */) {
    // If haveFocus is false, request it
    if (!allowedFocus) {
        // To update focus in this layer we need to request focus from group
        // Focus will be denied if there's an upper layer that needs focus
        allowedFocus = m_parentGroup->RequestFocus(this);
    }

    m_bMenuDisplayed = false;
    m_bPauseMenuDisplayed = false;
    m_bContainerMenuDisplayed = false;
    m_bIgnoreAutosaveMenuDisplayed = false;
    m_bIgnorePlayerJoinMenuDisplayed = false;

    bool layerFocusSet = false;
    for (auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it) {
        UIScene* scene = *it;

        // UPDATE FOCUS STATES
        if (!layerFocusSet && allowedFocus && scene->stealsFocus()) {
            scene->gainFocus();
            layerFocusSet = true;
        } else {
            scene->loseFocus();
            if (allowedFocus && app.GetGameStarted()) {
                // 4J Stu - This is a memory optimisation so we don't keep
                // scenes loaded in memory all the time This is required for PS3
                // (and likely Vita), but I'm removing it on XboxOne so that we
                // can avoid the scene creation time (which can be >0.5s) since
                // we have the memory to spare
                m_scenesToDestroy.push_back(scene);
            }

            if (scene->getSceneType() == eUIScene_SettingsOptionsMenu) {
                scene->loseFocus();
                m_scenesToDestroy.push_back(scene);
            }
        }

        /// UPDATE STACK STATES

        // 4J-PB - this should just be true
        m_bMenuDisplayed = true;

        EUIScene sceneType = scene->getSceneType();
        switch (sceneType) {
            case eUIScene_PauseMenu:
                m_bPauseMenuDisplayed = true;
                break;
            case eUIScene_Crafting2x2Menu:
            case eUIScene_Crafting3x3Menu:
            case eUIScene_FurnaceMenu:
            case eUIScene_ContainerMenu:
            case eUIScene_LargeContainerMenu:
            case eUIScene_InventoryMenu:
            case eUIScene_CreativeMenu:
            case eUIScene_DispenserMenu:
            case eUIScene_BrewingStandMenu:
            case eUIScene_EnchantingMenu:
            case eUIScene_TradingMenu:
            case eUIScene_HopperMenu:
            case eUIScene_HorseMenu:
            case eUIScene_FireworksMenu:
            case eUIScene_BeaconMenu:
            case eUIScene_AnvilMenu:
                m_bContainerMenuDisplayed = true;

                // Intentional fall-through
            case eUIScene_DeathMenu:
            case eUIScene_FullscreenProgress:
            case eUIScene_SignEntryMenu:
            case eUIScene_EndPoem:
                m_bIgnoreAutosaveMenuDisplayed = true;
                break;
            default:
                break;
        }

        switch (sceneType) {
            case eUIScene_FullscreenProgress:
            case eUIScene_EndPoem:
            case eUIScene_Credits:
            case eUIScene_LeaderboardsMenu:
                m_bIgnorePlayerJoinMenuDisplayed = true;
                break;
            default:
                break;
        }
    }
    m_hasFocus = layerFocusSet;

    return m_hasFocus;
}

void UILayer::handleInput(int iPad, int key, bool repeat, bool pressed,
                          bool released, bool& handled) {
    // Note: reverse iterator, the last element is the top of the stack
    for (auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it) {
        UIScene* scene = *it;
        if (scene->hasFocus(iPad) && scene->canHandleInput()) {
            // 4J-PB - ignore repeats of action ABXY buttons
            // fix for PS3 213 - [MAIN MENU] Holding down buttons will continue
            // to activate every prompt. 4J Stu - Changed this slightly to add
            // the allowRepeat function so we can allow repeats in the crafting
            // menu
            if (repeat && !scene->allowRepeat(key)) {
                return;
            }
            scene->handleInput(iPad, key, repeat, pressed, released, handled);
        }

        // Fix for PS3 #444 - [IN GAME] If the user keeps pressing CROSS while
        // on the 'Save Game' screen the title will crash.
        handled = handled || scene->hidesLowerScenes() || scene->blocksInput();
        if (handled) break;
    }

    // Components can't take input or focus
}

void UILayer::HandleDLCMountingComplete() {
    for (auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it) {
        UIScene* topScene = *it;
        app.DebugPrintf("UILayer::HandleDLCMountingComplete - topScene\n");
        topScene->HandleDLCMountingComplete();
    }
}

void UILayer::HandleDLCInstalled() {
    for (auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it) {
        UIScene* topScene = *it;
        topScene->HandleDLCInstalled();
    }
}

void UILayer::HandleMessage(EUIMessage message, void* data) {
    for (auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it) {
        UIScene* topScene = *it;
        topScene->HandleMessage(message, data);
    }
}

bool UILayer::IsFullscreenGroup() { return m_parentGroup->IsFullscreenGroup(); }

IPlatformRenderer::eViewportType UILayer::getViewport() {
    return m_parentGroup->GetViewportType();
}

void UILayer::handleUnlockFullVersion() {
    for (auto it = m_sceneStack.begin(); it != m_sceneStack.end(); ++it) {
        (*it)->handleUnlockFullVersion();
    }
}

void UILayer::PrintTotalMemoryUsage(int64_t& totalStatic,
                                    int64_t& totalDynamic) {
    int64_t layerStatic = 0;
    int64_t layerDynamic = 0;
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        (*it)->PrintTotalMemoryUsage(layerStatic, layerDynamic);
    }
    for (auto it = m_sceneStack.begin(); it != m_sceneStack.end(); ++it) {
        (*it)->PrintTotalMemoryUsage(layerStatic, layerDynamic);
    }
    app.DebugPrintf(app.USER_SR, "  \\- Layer static: %d , Layer dynamic: %d\n",
                    layerStatic, layerDynamic);
    totalStatic += layerStatic;
    totalDynamic += layerDynamic;
}

// Returns the first scene of given type if it exists, nullptr otherwise
UIScene* UILayer::FindScene(EUIScene sceneType) {
    for (int i = 0; i < m_sceneStack.size(); i++) {
        if (m_sceneStack[i]->getSceneType() == sceneType) {
            return m_sceneStack[i];
        }
    }

    return nullptr;
}