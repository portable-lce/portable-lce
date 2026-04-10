#pragma once
#include <stdint.h>

#include "UILayer.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "platform/renderer/renderer.h"

class UIComponent_Tooltips;
class UIComponent_TutorialPopup;
class UIScene_HUD;
class UIComponent_PressStartToPlay;
class UILayer;
class UIScene;

// A group contains a collection of layers for a specific context (e.g. each
// player has 1 group)
class UIGroup {
private:
    UILayer* m_layers[eUILayer_COUNT];

    UIComponent_Tooltips* m_tooltips;
    UIComponent_TutorialPopup* m_tutorialPopup;
    UIComponent_PressStartToPlay* m_pressStartToPlay;
    UIScene_HUD* m_hud;

    IPlatformRenderer::eViewportType m_viewportType;

    EUIGroup m_group;
    int m_iPad;

    bool m_bMenuDisplayed;
    bool m_bPauseMenuDisplayed;
    bool m_bContainerMenuDisplayed;
    bool m_bIgnoreAutosaveMenuDisplayed;
    bool m_bIgnorePlayerJoinMenuDisplayed;

    // Countdown in ticks to update focus state
    int m_updateFocusStateCountdown;

    int m_commandBufferList;

public:
    UIGroup(EUIGroup group, int iPad);

    UIComponent_Tooltips* getTooltips() { return m_tooltips; }
    UIComponent_TutorialPopup* getTutorialPopup() { return m_tutorialPopup; }
    UIScene_HUD* getHUD() { return m_hud; }
    UIComponent_PressStartToPlay* getPressStartToPlay() {
        return m_pressStartToPlay;
    }

    void DestroyAll();
    void ReloadAll();

    void tick();
    void render();
    bool hidesLowerScenes();
    void getRenderDimensions(S32& width, S32& height);

    // NAVIGATION
    bool NavigateToScene(int iPad, EUIScene scene, void* initData,
                         EUILayer layer);
    bool NavigateBack(int iPad, EUIScene eScene,
                      EUILayer eLayer = eUILayer_COUNT);
    void closeAllScenes();
    UIScene* GetTopScene(EUILayer layer);

    bool IsSceneInStack(EUIScene scene);
    bool HasFocus(int iPad);

    bool RequestFocus(UILayer* layerPtr);
    void UpdateFocusState();

    bool GetMenuDisplayed();
    bool IsPauseMenuDisplayed() { return m_bPauseMenuDisplayed; }
    bool IsContainerMenuDisplayed() { return m_bContainerMenuDisplayed; }
    bool IsIgnoreAutosaveMenuDisplayed() {
        return m_bIgnoreAutosaveMenuDisplayed;
    }
    bool IsIgnorePlayerJoinMenuDisplayed() {
        return m_bIgnorePlayerJoinMenuDisplayed;
    }

    // INPUT
    void handleInput(int iPad, int key, bool repeat, bool pressed,
                     bool released, bool& handled);

    // FOCUS
    bool getFocusState();

    // A component is an element on a layer that displays BELOW other scenes in
    // this layer, but does not engage in any navigation E.g. you can keep a
    // component active while performing navigation with other scenes on this
    // layer
    void showComponent(int iPad, EUIScene scene, EUILayer layer, bool show);
    UIScene* addComponent(int iPad, EUIScene scene, EUILayer layer);
    void removeComponent(EUIScene scene, EUILayer layer);

    void SetViewportType(IPlatformRenderer::eViewportType type);
    IPlatformRenderer::eViewportType GetViewportType();

    virtual void HandleDLCMountingComplete();
    virtual void HandleDLCInstalled();
    virtual void HandleMessage(EUIMessage message, void* data);

    bool IsFullscreenGroup();

    void handleUnlockFullVersion();

    void PrintTotalMemoryUsage(int64_t& totalStatic, int64_t& totalDynamic);

    unsigned int GetLayerIndex(UILayer* layerPtr);

    int getCommandBufferList();
    UIScene* FindScene(EUIScene sceneType);

private:
    void _UpdateFocusState();
    void updateStackStates();
};
