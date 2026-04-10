#pragma once
#include <stddef.h>
// using namespace std;
#include <cstdint>
#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/Timer.h"

#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIGroup.h"
#include "app/common/UI/All Platforms/IUIController.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/PlatformTypes.h"
#include "platform/input/input.h"
#include "platform/renderer/renderer.h"
#include "platform/storage/storage.h"

class UIAbstractBitmapFont;
class UIBitmapFont;
class UITTFFont;
class UIComponent_DebugUIConsole;
class UIComponent_DebugUIMarketingGuide;
class C4JThread;
class Tutorial;
class UIScene;

// 4jcraft, used to be D3D11_RECT.
// This was the only class that used it, so it's here now.
struct RECT {
    long left;
    long top;
    long right;
    long bottom;
};

// Base class for all shared functions between UIControllers
class UIController : public IUIController {
public:
    static int64_t iggyAllocCount;

    // MGH - added to prevent crash loading Iggy movies while the skins were
    // being reloaded
    static std::mutex ms_reloadSkinCS;
    static bool ms_bReloadSkinCSInitialised;

protected:
    UIComponent_DebugUIConsole* m_uiDebugConsole;
    UIComponent_DebugUIMarketingGuide* m_uiDebugMarketingGuide;

private:
    std::mutex m_navigationLock;

    static constexpr int UI_REPEAT_KEY_DELAY_MS =
        300;  // How long from press until the first repeat
    static constexpr int UI_REPEAT_KEY_REPEAT_RATE_MS =
        100;  // How long in between repeats
    time_util::time_point m_actionRepeatTimer[XUSER_MAX_COUNT]
                                             [ACTION_MAX_MENU + 1];

    float m_fScreenWidth;
    float m_fScreenHeight;
    bool m_bScreenWidthSetup;

    S32 m_tileOriginX, m_tileOriginY;

    enum EFont {
        eFont_NotLoaded = 0,

        eFont_Bitmap,
        eFont_Japanese,
        eFont_SimpChinese,
        eFont_TradChinese,
        eFont_Korean,

    };

    // 4J-JEV: It's important that currentFont == targetFont, unless
    // updateCurrentLanguage is going to be called.
    EFont m_eCurrentFont, m_eTargetFont;

    // 4J-JEV: Behaves like navigateToHome when not ingame. When in-game, it
    // closes all player scenes instead.
    bool m_bCleanupOnReload;

    EFont getFontForLanguage(int language);
    UITTFFont* createFont(EFont fontLanguage);

    UIAbstractBitmapFont* m_mcBitmapFont;
    UITTFFont* m_mcTTFFont;
    UIBitmapFont *m_moj7, *m_moj11;

public:
    void setCleanupOnReload();
    void updateCurrentFont();

private:
    // 4J-PB - ui element type for PSVita touch control
    // 4J Stu - These should be in the order that they reference each other
    // (i.e. they can only reference one with a lower value in the enum)
    enum ELibraries {
        eLibrary_Platform,
        eLibrary_GraphicsDefault,
        eLibrary_GraphicsHUD,
        eLibrary_GraphicsInGame,
        eLibrary_GraphicsTooltips,
        eLibrary_GraphicsLabels,
        eLibrary_Labels,
        eLibrary_InGame,
        eLibrary_HUD,
        eLibrary_Tooltips,
        eLibrary_Default,

#if defined(_WINDOWS64)
    // 4J Stu - Load the 720/480 skins so that we have something to fallback on
    // during development
#if !defined(_FINAL_BUILD)
        eLibraryFallback_Platform,
        eLibraryFallback_GraphicsDefault,
        eLibraryFallback_GraphicsHUD,
        eLibraryFallback_GraphicsInGame,
        eLibraryFallback_GraphicsTooltips,
        eLibraryFallback_GraphicsLabels,
        eLibraryFallback_Labels,
        eLibraryFallback_InGame,
        eLibraryFallback_HUD,
        eLibraryFallback_Tooltips,
        eLibraryFallback_Default,
#endif
#endif

        eLibrary_Count,
    };

    IggyLibrary m_iggyLibraries[eLibrary_Count];

protected:
    GDrawFunctions* gdraw_funcs;

private:
    HIGGYEXP iggy_explorer;
    HIGGYPERFMON iggy_perfmon;
    bool m_iggyPerfmonEnabled;

    bool
        m_bMenuDisplayed[XUSER_MAX_COUNT];  // track each players menu displayed
    bool m_bMenuToBeClosed[XUSER_MAX_COUNT];  // actioned at the end of the game
                                              // loop
    int m_iCountDown[XUSER_MAX_COUNT];        // ticks to block input

    bool m_bCloseAllScenes[eUIGroup_COUNT];

    int m_iPressStartQuadrantsMask;

    IPlatformRenderer::eViewportType m_currentRenderViewport;
    bool m_bCustomRenderPosition;

    static std::uint32_t m_dwTrialTimerLimitSecs;

    std::unordered_map<std::string, std::vector<uint8_t>>
        m_substitutionTextures;

    typedef struct _CachedMovieData {
        std::vector<uint8_t> m_ba;
        int64_t m_expiry;
    } CachedMovieData;
    std::unordered_map<std::string, CachedMovieData> m_cachedMovieData;

    typedef struct _QueuedMessageBoxData {
        MessageBoxInfo info;
        int iPad;
        EUILayer layer;
    } QueuedMessageBoxData;
    std::vector<QueuedMessageBoxData*> m_queuedMessageBoxData;

    unsigned int m_winUserIndex;
    // bool m_bSysUIShowing;
    bool m_bSystemUIShowing;
    C4JThread* m_reloadSkinThread;
    bool m_navigateToHomeOnReload;
    int m_accumulatedTicks;
    uint64_t m_lastUiSfx;  // Tracks time (ms) of last UI sound effect

    RECT m_customRenderingClearRect;

    std::unordered_map<size_t, UIScene*>
        m_registeredCallbackScenes;  // A collection of scenes and unique id's
                                     // that are used in async callbacks so we
                                     // can safely handle when they get
                                     // destroyed
    std::mutex m_registeredCallbackScenesCS;
    ;

public:
    UIController();

protected:
    UIGroup* m_groups[eUIGroup_COUNT];

public:
    void showComponent(int iPad, EUIScene scene, EUILayer layer, EUIGroup group,
                       bool show) {
        m_groups[group]->showComponent(iPad, scene, layer, show);
    }

    void removeComponent(EUIScene scene, EUILayer layer, EUIGroup group) {
        m_groups[group]->removeComponent(scene, layer);
    }

protected:
    // Should be called from the platforms init function
    void preInit(S32 width, S32 height);
    void postInit();

public:
    std::mutex m_Allocatorlock;
    void SetupFont();
    bool PendingFontChange();
    bool UsingBitmapFont();

public:
    // TICKING
    virtual void tick();

private:
    void loadSkins();
    IggyLibrary loadSkin(const std::string& skinPath,
                         const std::string& skinName);

public:
    void ReloadSkin();
    virtual void StartReloadSkinThread();
    virtual bool IsReloadingSkin();
    virtual bool IsExpectingOrReloadingSkin();
    virtual void CleanUpSkinReload();

private:
    static int reloadSkinThreadProc(void* lpParam);

public:
    std::vector<uint8_t> getMovieData(const std::string& filename);

    // INPUT
private:
    void tickInput();
    void handleInput();
    void handleKeyPress(unsigned int iPad, unsigned int key);

protected:
    static rrbool RADLINK
    ExternalFunctionCallback(void* user_callback_data, Iggy* player,
                             IggyExternalFunctionCallUTF16* call);

public:
    // RENDERING
    float getScreenWidth() { return m_fScreenWidth; }
    float getScreenHeight() { return m_fScreenHeight; }
    void setScreenSize(S32 w, S32 h) {
        m_fScreenWidth = (float)w;
        m_fScreenHeight = (float)h;
    }

    virtual void render() = 0;
    void getRenderDimensions(IPlatformRenderer::eViewportType viewport,
                             S32& width, S32& height);
    void setupRenderPosition(IPlatformRenderer::eViewportType viewport);
    void setupRenderPosition(S32 xOrigin, S32 yOrigin);

    void SetSysUIShowing(bool bVal);
    static void SetSystemUIShowing(void* lpParam, bool bVal);

protected:
    virtual void setTileOrigin(S32 xPos, S32 yPos) = 0;

public:
    virtual CustomDrawData* setupCustomDraw(
        UIScene* scene, IggyCustomDrawCallbackRegion* region) = 0;
    virtual CustomDrawData* calculateCustomDraw(
        IggyCustomDrawCallbackRegion* region) = 0;
    virtual void endCustomDraw(IggyCustomDrawCallbackRegion* region) = 0;

protected:
    // Should be called from the platforms render function
    void renderScenes();

public:
    virtual void beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion* region,
                                       CustomDrawData* customDrawRegion) = 0;
    void setupCustomDrawGameState();
    void endCustomDrawGameState();
    void setupCustomDrawMatrices(UIScene* scene,
                                 CustomDrawData* customDrawRegion);
    void setupCustomDrawGameStateAndMatrices(UIScene* scene,
                                             CustomDrawData* customDrawRegion);
    void endCustomDrawMatrices();
    void endCustomDrawGameStateAndMatrices();

protected:
    static void RADLINK
    CustomDrawCallback(void* user_callback_data, Iggy* player,
                       IggyCustomDrawCallbackRegion* Region);
    static GDrawTexture* RADLINK TextureSubstitutionCreateCallback(
        void* user_callback_data, IggyUTF16* texture_name, S32* width,
        S32* height, void** destroy_callback_data);
    static void RADLINK TextureSubstitutionDestroyCallback(
        void* user_callback_data, void* destroy_callback_data,
        GDrawTexture* handle);

    virtual GDrawTexture* getSubstitutionTexture(int textureId) {
        return nullptr;
    }
    virtual void destroySubstitutionTexture(void* destroyCallBackData,
                                            GDrawTexture* handle) {}

public:
    void registerSubstitutionTexture(const std::string& textureName,
                                     std::uint8_t* pbData,
                                     unsigned int dwLength);
    void unregisterSubstitutionTexture(const std::string& textureName,
                                       bool deleteData);

public:
    // NAVIGATION
    bool NavigateToScene(int iPad, EUIScene scene, void* initData = nullptr,
                         EUILayer layer = eUILayer_Scene,
                         EUIGroup group = eUIGroup_PAD);
    bool NavigateBack(int iPad, bool forceUsePad = false,
                      EUIScene eScene = eUIScene_COUNT,
                      EUILayer eLayer = eUILayer_COUNT);
    void NavigateToHomeMenu();
    UIScene* GetTopScene(int iPad, EUILayer layer = eUILayer_Scene,
                         EUIGroup group = eUIGroup_PAD);

    size_t RegisterForCallbackId(UIScene* scene);
    void UnregisterCallbackId(size_t id);
    UIScene* GetSceneFromCallbackId(size_t id);
    void lockCallbackScenes();
    void unlockCallbackScenes();

private:
    void setFullscreenMenuDisplayed(bool displayed);

public:
    void CloseAllPlayersScenes();
    void CloseUIScenes(int iPad, bool forceIPad = false);

    virtual bool IsPauseMenuDisplayed(int iPad);
    virtual bool IsContainerMenuDisplayed(int iPad);
    virtual bool IsIgnorePlayerJoinMenuDisplayed(int iPad);
    virtual bool IsIgnoreAutosaveMenuDisplayed(int iPad);
    virtual void SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed);
    virtual bool IsSceneInStack(int iPad, EUIScene eScene);
    bool GetMenuDisplayed(int iPad);
    void SetMenuDisplayed(int iPad, bool bVal);
    virtual void CheckMenuDisplayed();
    void AnimateKeyPress(int iPad, int iAction, bool bRepeat, bool bPressed,
                         bool bReleased);
    void OverrideSFX(int iPad, int iAction, bool bVal);

    // TOOLTIPS
    virtual void SetTooltipText(unsigned int iPad, unsigned int tooltip,
                                int iTextID);
    virtual void SetEnableTooltips(unsigned int iPad, bool bVal);
    virtual void ShowTooltip(unsigned int iPad, unsigned int tooltip,
                             bool show);
    virtual void SetTooltips(unsigned int iPad, int iA, int iB = -1,
                             int iX = -1, int iY = -1, int iLT = -1,
                             int iRT = -1, int iLB = -1, int iRB = -1,
                             int iLS = -1, int iRS = -1, int iBack = -1,
                             bool forceUpdate = false);
    virtual void EnableTooltip(unsigned int iPad, unsigned int tooltip,
                               bool enable);
    virtual void RefreshTooltips(unsigned int iPad);

    virtual void PlayUISFX(ESoundEffect eSound);

    virtual void DisplayGamertag(unsigned int iPad, bool show);
    virtual void SetSelectedItem(unsigned int iPad, const std::string& name);
    virtual void UpdateSelectedItemPos(unsigned int iPad);

    virtual void HandleDLCMountingComplete();
    virtual void HandleDLCInstalled(int iPad);
    virtual void HandleTMSDLCFileRetrieved(int iPad);
    virtual void HandleTMSBanFileRetrieved(int iPad);
    virtual void HandleInventoryUpdated(int iPad);
    virtual void HandleGameTick();

    virtual void SetTutorial(int iPad, Tutorial* tutorial);
    virtual void SetTutorialDescription(int iPad, TutorialPopupInfo* info);
    virtual void RemoveInteractSceneReference(int iPad, UIScene* scene);
    virtual void SetTutorialVisible(int iPad, bool visible);
    virtual bool IsTutorialVisible(int iPad);

    virtual void UpdatePlayerBasePositions();
    virtual void SetEmptyQuadrantLogo(int iSection);
    virtual void HideAllGameUIElements();
    virtual void ShowOtherPlayersBaseScene(unsigned int iPad, bool show);

    virtual void ShowTrialTimer(bool show);
    virtual void SetTrialTimerLimitSecs(unsigned int uiSeconds);
    virtual void UpdateTrialTimer(unsigned int iPad);
    virtual void ReduceTrialTimerValue();

    virtual void ShowAutosaveCountdownTimer(bool show);
    virtual void UpdateAutosaveCountdownTimer(unsigned int uiSeconds);
    virtual void ShowSavingMessage(unsigned int iPad,
                                   IPlatformStorage::ESavingMessage eVal);

    virtual void ShowPlayerDisplayname(bool show);
    virtual bool PressStartPlaying(unsigned int iPad);
    virtual void ShowPressStart(unsigned int iPad);
    virtual void HidePressStart();
    void ClearPressStart();

    virtual IPlatformStorage::EMessageResult RequestAlertMessage(
        uint32_t uiTitle, uint32_t uiText, uint32_t* uiOptionA,
        uint32_t uiOptionC, uint32_t dwPad = XUSER_INDEX_ANY,
        int (*Func)(void*, int,
                    const IPlatformStorage::EMessageResult) = nullptr,
        void* lpParam = nullptr, char* pwchFormatString = nullptr);
    virtual IPlatformStorage::EMessageResult RequestErrorMessage(
        uint32_t uiTitle, uint32_t uiText, uint32_t* uiOptionA,
        uint32_t uiOptionC, uint32_t dwPad = XUSER_INDEX_ANY,
        int (*Func)(void*, int,
                    const IPlatformStorage::EMessageResult) = nullptr,
        void* lpParam = nullptr, char* pwchFormatString = nullptr);

private:
    virtual IPlatformStorage::EMessageResult RequestMessageBox(
        uint32_t uiTitle, uint32_t uiText, uint32_t* uiOptionA,
        uint32_t uiOptionC, uint32_t dwPad,
        int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
        void* lpParam, char* pwchFormatString, uint32_t dwFocusButton,
        bool bIsError);

public:
    IPlatformStorage::EMessageResult RequestUGCMessageBox(
        int title = -1, int message = -1, int iPad = -1,
        int (*Func)(void*, int,
                    const IPlatformStorage::EMessageResult) = nullptr,
        void* lpParam = nullptr);
    IPlatformStorage::EMessageResult RequestContentRestrictedMessageBox(
        int title = -1, int message = -1, int iPad = -1,
        int (*Func)(void*, int,
                    const IPlatformStorage::EMessageResult) = nullptr,
        void* lpParam = nullptr);

    virtual void SetWinUserIndex(unsigned int iPad);
    unsigned int GetWinUserIndex();

    virtual void ShowUIDebugConsole(bool show);
    virtual void ShowUIDebugMarketingGuide(bool show);
    void logDebugString(const std::string& text);
    UIScene* FindScene(EUIScene sceneType);

public:
    char *m_defaultBuffer, *m_tempBuffer;
    void setFontCachingCalculationBuffer(int length);
};