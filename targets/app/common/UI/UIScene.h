#pragma once
// 4J-PB - remove the inherits via dominance warnings

// using namespace std;
//  A scene map directly to an Iggy movie (or more accurately a collection of
//  different sized movies)

#include <stddef.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "platform/renderer/renderer.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class ItemRenderer;
class UILayer;
class ItemInstance;
class UIControl_Base;
class UIControl;

// 4J Stu - Setup some defines for quickly mapping elements in the scene

#define UI_BEGIN_MAP_ELEMENTS_AND_NAMES(parentClass) \
    virtual bool mapElementsAndNames() {             \
        parentClass::mapElementsAndNames();          \
        IggyValuePath* currentRoot = IggyPlayerRootPath(getMovie());

#define UI_END_MAP_ELEMENTS_AND_NAMES() \
    return true;                        \
    }

#define UI_MAP_ELEMENT(var, name)                  \
    {                                              \
        var.setupControl(this, currentRoot, name); \
        m_controls.push_back(&var);                \
    }

#define UI_BEGIN_MAP_CHILD_ELEMENTS(parent)    \
    {                                          \
        IggyValuePath* lastRoot = currentRoot; \
        currentRoot = parent.getIggyValuePath();

#define UI_END_MAP_CHILD_ELEMENTS() \
    currentRoot = lastRoot;         \
    }

#define UI_MAP_NAME(var, name)        \
    {                                 \
        var = registerFastName(name); \
    }

class UIScene {
    friend class UILayer;

public:
    IggyValuePath* m_rootPath;

private:
    Iggy* swf;
    IggyName m_funcRemoveObject, m_funcSlideLeft, m_funcSlideRight,
        m_funcSetSafeZone, m_funcSetFocus, m_funcHorizontalResizeCheck;
    IggyName m_funcSetAlpha;

    ItemRenderer* m_pItemRenderer;
    std::unordered_map<std::string, IggyName> m_fastNames;
    std::unordered_map<std::string, bool> m_registeredTextures;

    typedef struct _TimerInfo {
        int duration;
        int targetTime;
        bool running;
    } TimerInfo;
    std::unordered_map<int, TimerInfo> m_timers;

    int m_iFocusControl, m_iFocusChild;
    float m_lastOpacity;
    bool m_bUpdateOpacity;
    bool m_bVisible;
    bool m_bCanHandleInput;
    bool m_hasSetSafeZoneMethod;
    UIScene* m_backScene;

    size_t m_callbackUniqueId;

public:
    enum ESceneResolution {
        eSceneResolution_1080,
        eSceneResolution_720,
        eSceneResolution_480,
        eSceneResolution_Vita,
    };

protected:
    ESceneResolution m_loadedResolution;

    bool m_bIsReloading;
    bool m_bFocussedOnce;

    int m_movieWidth, m_movieHeight;
    int m_renderWidth, m_renderHeight;
    std::vector<UIControl*> m_controls;

protected:
    UILayer* m_parentLayer;
    bool bHasFocus;
    int m_iPad;
    bool m_hasTickedOnce;

public:
    virtual Iggy* getMovie() { return swf; }

    void destroyMovie();
    virtual void reloadMovie(bool force = false);
    virtual bool needsReloaded();
    virtual bool hasMovie();
    virtual void updateSafeZone();

    int getRenderWidth() { return m_renderWidth; }
    int getRenderHeight() { return m_renderHeight; }

protected:
    virtual F64 getSafeZoneHalfHeight();
    virtual F64 getSafeZoneHalfWidth();
    void setSafeZone(S32 top, S32 bottom, S32 left, S32 right);
    void doHorizontalResizeCheck();
    virtual std::string getMoviePath() = 0;

    virtual bool mapElementsAndNames();
    void initialiseMovie();
    void loadMovie();

private:
    void getDebugMemoryUseRecursive(const std::string& moviePath,
                                    IggyMemoryUseInfo& memoryInfo);

public:
    void PrintTotalMemoryUsage(int64_t& totalStatic, int64_t& totalDynamic);

public:
    UIScene(int iPad, UILayer* parentLayer);
    virtual ~UIScene();

    virtual EUIScene getSceneType() = 0;
    ESceneResolution getSceneResolution() { return m_loadedResolution; }

    virtual void tick();

    IggyName registerFastName(const std::string& name);

protected:
    void addTimer(int id, int ms);
    void killTimer(int id);
    void tickTimers();
    TimerInfo* getTimer(int id) { return &m_timers[id]; }
    virtual void handleTimerComplete(int id) {}

public:
    // FOCUS
    // Returns true if this scene handles input
    virtual bool stealsFocus() { return true; }

    // Returns true if this scene has focus for the pad passed in
    virtual bool hasFocus(int iPad) { return bHasFocus && iPad == m_iPad; }

    void gainFocus();
    void loseFocus();

    virtual void updateTooltips();
    virtual void updateComponents() {}
    virtual void handleGainFocus(bool navBack);
    virtual void handleLoseFocus() {}

    // Returns true if lower scenes in this scenes layer, or in any layer below
    // this scenes layers should be hidden
    virtual bool hidesLowerScenes() { return m_hasTickedOnce; }

    // Returns true if this scene should block input to lower scenes (works like
    // hidesLowerScenes but doesn't interfere with rendering)
    virtual bool blocksInput() { return false; }

    // returns main panel if controls are not living in the root
    virtual UIControl* GetMainPanel();

    void removeControl(UIControl_Base* control, bool centreScene);
    void slideLeft();
    void slideRight();

    // RENDERING
    virtual void render(S32 width, S32 height,
                        IPlatformRenderer::eViewportType viewpBort);

    virtual void customDraw(IggyCustomDrawCallbackRegion* region);

    void setOpacity(float percent);
    void setVisible(bool visible);
    bool isVisible() { return m_bVisible; }

protected:
    // void customDrawSlotControl(IggyCustomDrawCallbackRegion *region, int
    // iPad, int iID, int iCount, int iAuxVal, float fAlpha, bool isFoil, bool
    // bDecorations);
    void customDrawSlotControl(IggyCustomDrawCallbackRegion* region, int iPad,
                               std::shared_ptr<ItemInstance> item, float fAlpha,
                               bool isFoil, bool bDecorations);

    bool m_cacheSlotRenders;
    bool m_needsCacheRendered;
    int m_expectedCachedSlotCount;

private:
    typedef struct _CachedSlotDrawData {
        CustomDrawData* customDrawRegion;
        std::shared_ptr<ItemInstance> item;
        float fAlpha;
        bool isFoil;
        bool bDecorations;
    } CachedSlotDrawData;
    std::vector<CachedSlotDrawData*> m_cachedSlotDraw;

    void _customDrawSlotControl(CustomDrawData* region, int iPad,
                                std::shared_ptr<ItemInstance> item,
                                float fAlpha, bool isFoil, bool bDecorations,
                                bool usingCommandBuffer);

public:
    // INPUT
    bool canHandleInput() { return m_bCanHandleInput; }
    virtual bool allowRepeat(int key);
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled) {}
    void externalCallback(IggyExternalFunctionCallUTF16* call);

    virtual void handleDestroy() {}

protected:
    void sendInputToMovie(int key, bool repeat, bool pressed, bool released);
    virtual void handlePreReload() {}
    virtual void handleReload() {}
    virtual void handlePress(F64 controlId, F64 childId) {}
    virtual void handleFocusChange(F64 controlId, F64 childId) {}
    virtual void handleInitFocus(F64 controlId, F64 childId) {}
    virtual void handleCheckboxToggled(F64 controlId, bool selected) {}
    virtual void handleSliderMove(F64 sliderId, F64 currentValue) {}
    virtual void handleAnimationEnd() {}
    virtual void handleSelectionChanged(F64 selectedId) {}
    virtual void handleRequestMoreData(F64 startIndex, bool up) {}
    virtual void handleTouchBoxRebuild() {}

private:
    void _handleFocusChange(F64 controlId, F64 childId);
    void _handleInitFocus(F64 controlId, F64 childId);

    int convertGameActionToIggyKeycode(int action);

public:
    bool controlHasFocus(int iControlId);
    bool controlHasFocus(UIControl_Base* control);
    int getControlFocus();
    int getControlChildFocus();

    // NAVIGATION
protected:
    // void navigateForward(int iPad, EUIScene scene, void *initData = nullptr);
    void navigateBack();

public:
    void setBackScene(UIScene* scene);
    UIScene* getBackScene();
    virtual void HandleDLCMountingComplete() {}
    virtual void HandleDLCInstalled() {}

    virtual void HandleMessage(EUIMessage message, void* data);

    void registerSubstitutionTexture(const std::string& textureName,
                                     std::uint8_t* pbData,
                                     unsigned int dwLength,
                                     bool deleteData = false);
    bool hasRegisteredSubstitutionTexture(const std::string& textureName);

    virtual void handleUnlockFullVersion() {}

    virtual void handleTouchInput(unsigned int iPad, S32 x, S32 y, int iId,
                                  bool bPressed, bool bRepeat, bool bReleased) {
    }

protected:
    size_t GetCallbackUniqueId();

    virtual bool isReadyToDelete();

    static int parseSlotId(const char16_t* s);
};
