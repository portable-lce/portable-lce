#include "UIController.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "app/common/Audio/SoundEngine.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Components/UIComponent_DebugUIConsole.h"
#include "app/common/UI/Components/UIComponent_DebugUIMarketingGuide.h"
#include "app/common/UI/Components/UIComponent_PressStartToPlay.h"
#include "app/common/UI/Components/UIComponent_Tooltips.h"
#include "app/common/UI/Components/UIComponent_TutorialPopup.h"
#include "app/common/UI/Components/UIScene_HUD.h"
#include "app/common/UI/UIBitmapFont.h"
#include "app/common/UI/UIGroup.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/UI/UITTFFont.h"
#include "app/linux/Iggy/include/iggy.h"
#include "minecraft/GameEnums.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#ifndef _ENABLEIGGY
#include "app/linux/Stubs/iggy_stubs.h"
#endif
#include "UIFontData.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "java/System.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/client/title/TitleScreen.h"
#include "platform/C4JThread.h"
#include "platform/XboxStubs.h"
#include "strings.h"
#include "util/StringHelpers.h"
#include "util/Timer.h"

class Tutorial;

// 4J Stu - Enable this to override the Iggy Allocator
// #define ENABLE_IGGY_ALLOCATOR
// #define EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR

// #define ENABLE_IGGY_EXPLORER
#if defined(ENABLE_IGGY_EXPLORER)
#include "app/windows/Iggy/include/iggyexpruntime.h"
#endif

// #define ENABLE_IGGY_PERFMON
#if defined(ENABLE_IGGY_PERFMON)

#define PM_ORIGIN_X 24
#define PM_ORIGIN_Y 34

#if defined(__WINDOWS64)
#include "app/windows/Iggy/include/iggyperfmon.h"
#endif

#endif

std::mutex UIController::ms_reloadSkinCS;
bool UIController::ms_bReloadSkinCSInitialised = false;

std::uint32_t UIController::m_dwTrialTimerLimitSecs =
    /*DYNAMIC_CONFIG_DEFAULT_TRIAL_TIME*/ 2400;

static void RADLINK WarningCallback(void* user_callback_data, Iggy* player,
                                    IggyResult code, const char* message) {
    // enum IggyResult{    IGGY_RESULT_SUCCESS = 0,    IGGY_RESULT_Warning_None
    // = 0,
    //    IGGY_RESULT_Warning_Misc = 100,    IGGY_RESULT_Warning_GDraw = 101,
    //    IGGY_RESULT_Warning_ProgramFlow = 102,
    //    IGGY_RESULT_Warning_Actionscript = 103,
    //    IGGY_RESULT_Warning_Graphics = 104,    IGGY_RESULT_Warning_Font = 105,
    //    IGGY_RESULT_Warning_Timeline = 106,    IGGY_RESULT_Warning_Library =
    //    107, IGGY_RESULT_Warning_CannotSustainFrameRate = 201,
    //    IGGY_RESULT_Warning_ThrewException = 202,
    //    IGGY_RESULT_Error_Threshhold = 400,    IGGY_RESULT_Error_Misc = 400,
    //    IGGY_RESULT_Error_GDraw = 401,    IGGY_RESULT_Error_ProgramFlow = 402,
    //    IGGY_RESULT_Error_Actionscript = 403,    IGGY_RESULT_Error_Graphics =
    //    404, IGGY_RESULT_Error_Font = 405,    IGGY_RESULT_Error_Create = 406,
    //    IGGY_RESULT_Error_Library = 407,    IGGY_RESULT_Error_ValuePath = 408,
    //    IGGY_RESULT_Error_Audio = 409,    IGGY_RESULT_Error_Internal = 499,
    //    IGGY_RESULT_Error_InvalidIggy = 501,
    //    IGGY_RESULT_Error_InvalidArgument = 502,
    //    IGGY_RESULT_Error_InvalidEntity = 503,
    //    IGGY_RESULT_Error_UndefinedEntity = 504,
    //    IGGY_RESULT_Error_OutOfMemory = 1001,};

    if (message != nullptr) {
        // 4jcraft: Some Linux movie variants do not ship these optional
        // hooks/controls. We guard the call sites, so drop the residual Iggy
        // warning noise.
        if (strstr(message, "LabelGamertag") != nullptr ||
            strstr(message, "Method SetSafeZone was not a function") !=
                nullptr) {
            return;
        }
    }

    switch (code) {
        case IGGY_RESULT_Warning_CannotSustainFrameRate:
            // Ignore warning
            break;
        default:
            /* Normally, we'd want to issue this warning to some kind of
            logging system or error reporting system, but since this is a
            tutorial app, we just use Win32's default error stream.  Since
            ActionScript 3 exceptions are routed through this warning
            callback, it's definitely a good idea to make sure these
            warnings get printed somewhere that's easy for you to read and
            use for debugging, otherwise debugging errors in the
            ActionScript 3 code in your Flash content will be very
            difficult! */
            app.DebugPrintf(app.USER_SR, "[Iggy] ");
            app.DebugPrintf(app.USER_SR, message);
            app.DebugPrintf(app.USER_SR, "\n");
            break;
    };
}

/* Flash provides a way for ActionScript 3 code to print debug output
using a function called "trace".  It's very useful for debugging
Flash programs, so ideally, when using Iggy, we'd like to see any
trace output alongside our own debugging output.  To facilitate
this, Iggy allows us to install a callback that will be called
any time ActionScript code calls trace. */
static void RADLINK TraceCallback(void* user_callback_data, Iggy* player,
                                  char const* utf8_string,
                                  S32 length_in_bytes) {
    app.DebugPrintf(app.USER_UI, (char*)utf8_string);
}

#if defined(ENABLE_IGGY_PERFMON)
static void* RADLINK perf_malloc(void* handle, U32 size) {
    return malloc(size);
}

static void RADLINK perf_free(void* handle, void* ptr) { return free(ptr); }
#endif

#if defined(EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR)
extern "C" void* __real_malloc(size_t t);
extern "C" void __real_free(void* t);
#endif

int64_t UIController::iggyAllocCount = 0;
static std::unordered_map<void*, size_t> allocations;
static void* RADLINK AllocateFunction(void* alloc_callback_user_data,
                                      size_t size_requested,
                                      size_t* size_returned) {
    UIController* controller = (UIController*)alloc_callback_user_data;
    std::lock_guard<std::mutex> lock(controller->m_Allocatorlock);
#if defined(EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR)
    void* alloc = __real_malloc(size_requested);
#else
    void* alloc = malloc(size_requested);
#endif
    *size_returned = size_requested;
    UIController::iggyAllocCount += size_requested;
    allocations[alloc] = size_requested;
    app.DebugPrintf(app.USER_SR, "Allocating %d, new total: %d\n",
                    size_requested, UIController::iggyAllocCount);
    return alloc;
}

static void RADLINK DeallocateFunction(void* alloc_callback_user_data,
                                       void* ptr) {
    UIController* controller = (UIController*)alloc_callback_user_data;
    std::lock_guard<std::mutex> lock(controller->m_Allocatorlock);
    size_t size = allocations[ptr];
    UIController::iggyAllocCount -= size;
    allocations.erase(ptr);
    app.DebugPrintf(app.USER_SR, "Freeing %d, new total %d\n", size,
                    UIController::iggyAllocCount);
#if defined(EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR)
    __real_free(ptr);
#else
    free(ptr);
#endif
}

UIController::UIController() {
    m_uiDebugConsole = nullptr;
    m_reloadSkinThread = nullptr;

    m_navigateToHomeOnReload = false;

    m_bCleanupOnReload = false;
    m_mcTTFFont = nullptr;
    m_moj7 = nullptr;
    m_moj11 = nullptr;

    // 4J-JEV: It's important that these remain the same, unless
    // updateCurrentLanguage is going to be called.
    m_eCurrentFont = m_eTargetFont = eFont_NotLoaded;

    // 4J Stu - This is a bit of a hack until we change the Minecraft
    // initialisation to store the proper screen size for other platforms
#if defined(_WINDOWS64) || defined(__linux__)
    m_fScreenWidth = 1920.0f;
    m_fScreenHeight = 1080.0f;
    m_bScreenWidthSetup = true;
#else
    m_fScreenWidth = 1280.0f;
    m_fScreenHeight = 720.0f;
    m_bScreenWidthSetup = false;
#endif

    for (unsigned int i = 0; i < eLibrary_Count; ++i) {
        m_iggyLibraries[i] = IGGY_INVALID_LIBRARY;
    }

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        m_bMenuDisplayed[i] = false;
        m_iCountDown[i] = 0;
        m_bMenuToBeClosed[i] = false;

        for (unsigned int key = 0; key <= ACTION_MAX_MENU; ++key) {
            m_actionRepeatTimer[i][key] = {};
        }
    }

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_bCloseAllScenes[i] = false;
    }

    m_iPressStartQuadrantsMask = 0;

    m_currentRenderViewport = IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN;
    m_bCustomRenderPosition = false;
    m_winUserIndex = 0;
    m_accumulatedTicks = 0;
    m_lastUiSfx = 0;

    // m_bSysUIShowing=false;
    m_bSystemUIShowing = false;

    if (!ms_bReloadSkinCSInitialised) {
        // MGH - added to prevent crash loading Iggy movies while the skins were
        // being reloaded
        ms_bReloadSkinCSInitialised = true;
    }
}

void UIController::SetSysUIShowing(bool bVal) {
    if (bVal)
        app.DebugPrintf("System UI showing\n");
    else
        app.DebugPrintf("System UI stopped showing\n");
    m_bSystemUIShowing = bVal;
}

void UIController::SetSystemUIShowing(void* lpParam, bool bVal) {
    UIController* pClass = (UIController*)lpParam;
    pClass->SetSysUIShowing(bVal);
}

// SETUP
void UIController::preInit(S32 width, S32 height) {
    m_fScreenWidth = width;
    m_fScreenHeight = height;
    m_bScreenWidthSetup = true;

#if defined(ENABLE_IGGY_ALLOCATOR)
    IggyAllocator allocator;
    allocator.user_callback_data = this;
    allocator.mem_alloc = &AllocateFunction;
    allocator.mem_free = &DeallocateFunction;
    IggyInit(&allocator);
#else
    IggyInit(0);
#endif

    IggySetWarningCallback(WarningCallback, 0);
    IggySetTraceCallbackUTF8(TraceCallback, 0);

    setFontCachingCalculationBuffer(-1);
}

void UIController::postInit() {
    // set up a custom rendering callback
    IggySetCustomDrawCallback(&UIController::CustomDrawCallback, this);
    IggySetAS3ExternalFunctionCallbackUTF16(
        &UIController::ExternalFunctionCallback, this);
    IggySetTextureSubstitutionCallbacks(
        &UIController::TextureSubstitutionCreateCallback,
        &UIController::TextureSubstitutionDestroyCallback, this);

    SetupFont();
    //
    loadSkins();

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i] = new UIGroup((EUIGroup)i, i - 1);
    }

#if defined(ENABLE_IGGY_EXPLORER)
    iggy_explorer = IggyExpCreate(
        "127.0.0.1", 9190, malloc(IGGYEXP_MIN_STORAGE), IGGYEXP_MIN_STORAGE);
    if (iggy_explorer == nullptr) {
        // not normally an error, just an error for this demo!
        app.DebugPrintf(
            "Couldn't connect to Iggy Explorer, did you run it first?");
    } else {
        IggyUseExplorer(m_groups[1]->getHUD()->getMovie(), iggy_explorer);
    }
#endif

#if defined(ENABLE_IGGY_PERFMON)
    m_iggyPerfmonEnabled = false;
    iggy_perfmon = IggyPerfmonCreate(perf_malloc, perf_free, nullptr);
    IggyInstallPerfmon(iggy_perfmon);
#endif

    NavigateToScene(0, eUIScene_Intro);
}

UIController::EFont UIController::getFontForLanguage(int language) {
    switch (language) {
        case XC_LANGUAGE_JAPANESE:
            return eFont_Japanese;
        case XC_LANGUAGE_TCHINESE:
            return eFont_TradChinese;
        case XC_LANGUAGE_KOREAN:
            return eFont_Korean;
        default:
            return eFont_Bitmap;
    }
}

UITTFFont* UIController::createFont(EFont fontLanguage) {
    switch (fontLanguage) {
        case eFont_Japanese:
            return new UITTFFont(
                "Mojangles_TTF_jaJP",
                "app/common/Media/font/JPN/DFGMaruGothic-Md.ttf",
                0x2022);  // JPN
        case eFont_TradChinese:
            return new UITTFFont("Mojangles_TTF_cnTD",
                                 "app/common/Media/font/CHT/DFHeiMedium-B5.ttf",
                                 0x2022);  // CHT
        case eFont_Korean:
            return new UITTFFont("Mojangles_TTF_koKR",
                                 "app/common/Media/font/KOR/BOKMSD.ttf",
                                 0x2022);  // KOR
        // 4J-JEV, Cyrillic characters have been added to this font now,
        // (4/July/14) XC_LANGUAGE_RUSSIAN and XC_LANGUAGE_GREEK:
        default:
            return nullptr;
    }
}

void UIController::SetupFont() {
    // 4J-JEV: Language hasn't changed or is already changing.
    if ((m_eCurrentFont != m_eTargetFont) || !UIString::setCurrentLanguage())
        return;

    uint32_t nextLanguage = UIString::getCurrentLanguage();
    m_eTargetFont = getFontForLanguage(nextLanguage);

    // flag a language change to reload the string tables in the DLC
    app.m_dlcManager.LanguageChanged();

    app.loadStringTable();  // Switch to use new string table,

    if (m_eTargetFont == m_eCurrentFont) {
        // 4J-JEV: If we're ingame, reload the font to update all the text.
        if (app.GetGameStarted())
            app.SetAction(PlatformProfile.GetPrimaryPad(),
                          eAppAction_ReloadFont);
        return;
    }

    if (m_eCurrentFont != eFont_NotLoaded)
        app.DebugPrintf(
            "[UIController] Font switch required for language transition to "
            "%i.\n",
            nextLanguage);
    else
        app.DebugPrintf("[UIController] Initialising font for language %i.\n",
                        nextLanguage);

    if (m_mcTTFFont != nullptr) {
        delete m_mcTTFFont;
        m_mcTTFFont = nullptr;
    }

    if (m_eTargetFont == eFont_Bitmap) {
        // these may have been set up by a previous language being chosen
        if (m_moj7 == nullptr)
            m_moj7 = new UIBitmapFont(SFontData::Mojangles_7);
        if (m_moj11 == nullptr)
            m_moj11 = new UIBitmapFont(SFontData::Mojangles_11);

        // 4J-JEV: Ensure we redirect to them correctly, even if the objects
        // were previously initialised.
        m_moj7->registerFont();
        m_moj11->registerFont();
    } else if (m_eTargetFont != eFont_NotLoaded) {
        m_mcTTFFont = createFont(m_eTargetFont);

        app.DebugPrintf("[Iggy] Set font indirect to '%hs'.\n",
                        m_mcTTFFont->getFontName().c_str());
        IggyFontSetIndirectUTF8("Mojangles7", -1, IGGY_FONTFLAG_all,
                                m_mcTTFFont->getFontName().c_str(), -1,
                                IGGY_FONTFLAG_none);
        IggyFontSetIndirectUTF8("Mojangles11", -1, IGGY_FONTFLAG_all,
                                m_mcTTFFont->getFontName().c_str(), -1,
                                IGGY_FONTFLAG_none);
    } else {
        assert(false);
    }

    // Reload ui to set new font.
    if (m_eCurrentFont != eFont_NotLoaded) {
        app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_ReloadFont);
    } else {
        updateCurrentFont();
    }
}

bool UIController::PendingFontChange() {
    return getFontForLanguage(XGetLanguage()) != m_eCurrentFont;
}

void UIController::setCleanupOnReload() { m_bCleanupOnReload = true; }

void UIController::updateCurrentFont() { m_eCurrentFont = m_eTargetFont; }

bool UIController::UsingBitmapFont() { return m_eCurrentFont == eFont_Bitmap; }

// TICKING
void UIController::tick() {
    SetupFont();  // If necessary, change font.

    if ((m_navigateToHomeOnReload || m_bCleanupOnReload) &&
        !ui.IsReloadingSkin()) {
        ui.CleanUpSkinReload();

        if (m_navigateToHomeOnReload || !g_NetworkManager.IsInSession()) {
            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_MainMenu);
        } else {
            ui.CloseAllPlayersScenes();
        }

        updateCurrentFont();

        m_navigateToHomeOnReload = false;
        m_bCleanupOnReload = false;
    }

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        if (m_bCloseAllScenes[i]) {
            m_groups[i]->closeAllScenes();
            m_groups[i]->getTooltips()->SetTooltips(-1);
            m_bCloseAllScenes[i] = false;
        }
    }

    if (m_accumulatedTicks == 0) tickInput();
    m_accumulatedTicks = 0;

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->tick();

        // TODO: May wish to skip ticking other groups here
    }

    // Clear out the cached movie file data
    int64_t currentTime = System::currentTimeMillis();
    for (auto it = m_cachedMovieData.begin(); it != m_cachedMovieData.end();) {
        if (it->second.m_expiry < currentTime) {
            it = m_cachedMovieData.erase(it);
        } else {
            ++it;
        }
    }
}

void UIController::loadSkins() {
    std::string platformSkinPath = "";

#if defined(_WINDOWS64) || defined(__linux__)
    if (m_fScreenHeight == 1080.0f) {
        platformSkinPath = "skinHDWin.swf";
    } else {
        platformSkinPath = "skinWin.swf";
    }
#endif
    // Every platform has one of these, so nothing shared
    if (m_fScreenHeight == 1080.0f) {
        m_iggyLibraries[eLibrary_Platform] =
            loadSkin(platformSkinPath, "platformskinHD.swf");
    } else {
        m_iggyLibraries[eLibrary_Platform] =
            loadSkin(platformSkinPath, "platformskin.swf");
    }

#if defined(_WINDOWS64) || defined(__linux__)

#if defined(_WINDOWS64)
    // 4J Stu - Load the 720/480 skins so that we have something to fallback on
    // during development
#if !defined(_FINAL_BUILD)
    m_iggyLibraries[eLibraryFallback_GraphicsDefault] =
        loadSkin("skinGraphics.swf", "skinGraphics.swf");
    m_iggyLibraries[eLibraryFallback_GraphicsHUD] =
        loadSkin("skinGraphicsHud.swf", "skinGraphicsHud.swf");
    m_iggyLibraries[eLibraryFallback_GraphicsInGame] =
        loadSkin("skinGraphicsInGame.swf", "skinGraphicsInGame.swf");
    m_iggyLibraries[eLibraryFallback_GraphicsTooltips] =
        loadSkin("skinGraphicsTooltips.swf", "skinGraphicsTooltips.swf");
    m_iggyLibraries[eLibraryFallback_GraphicsLabels] =
        loadSkin("skinGraphicsLabels.swf", "skinGraphicsLabels.swf");
    m_iggyLibraries[eLibraryFallback_Labels] =
        loadSkin("skinLabels.swf", "skinLabels.swf");
    m_iggyLibraries[eLibraryFallback_InGame] =
        loadSkin("skinInGame.swf", "skinInGame.swf");
    m_iggyLibraries[eLibraryFallback_HUD] =
        loadSkin("skinHud.swf", "skinHud.swf");
    m_iggyLibraries[eLibraryFallback_Tooltips] =
        loadSkin("skinTooltips.swf", "skinTooltips.swf");
    m_iggyLibraries[eLibraryFallback_Default] =
        loadSkin("skin.swf", "skin.swf");
#endif
#endif

    m_iggyLibraries[eLibrary_GraphicsDefault] =
        loadSkin("skinHDGraphics.swf", "skinHDGraphics.swf");
    m_iggyLibraries[eLibrary_GraphicsHUD] =
        loadSkin("skinHDGraphicsHud.swf", "skinHDGraphicsHud.swf");
    m_iggyLibraries[eLibrary_GraphicsInGame] =
        loadSkin("skinHDGraphicsInGame.swf", "skinHDGraphicsInGame.swf");
    m_iggyLibraries[eLibrary_GraphicsTooltips] =
        loadSkin("skinHDGraphicsTooltips.swf", "skinHDGraphicsTooltips.swf");
    m_iggyLibraries[eLibrary_GraphicsLabels] =
        loadSkin("skinHDGraphicsLabels.swf", "skinHDGraphicsLabels.swf");
    m_iggyLibraries[eLibrary_Labels] =
        loadSkin("skinHDLabels.swf", "skinHDLabels.swf");
    m_iggyLibraries[eLibrary_InGame] =
        loadSkin("skinHDInGame.swf", "skinHDInGame.swf");
    m_iggyLibraries[eLibrary_HUD] = loadSkin("skinHDHud.swf", "skinHDHud.swf");
    m_iggyLibraries[eLibrary_Tooltips] =
        loadSkin("skinHDTooltips.swf", "skinHDTooltips.swf");
    m_iggyLibraries[eLibrary_Default] = loadSkin("skinHD.swf", "skinHD.swf");
#endif
}

IggyLibrary UIController::loadSkin(const std::string& skinPath,
                                   const std::string& skinName) {
    IggyLibrary lib = IGGY_INVALID_LIBRARY;
    // 4J Stu - We need to load the platformskin before the normal skin, as the
    // normal skin requires some elements from the platform skin
    if (!skinPath.empty() && app.hasArchiveFile(skinPath)) {
        std::vector<uint8_t> baFile = app.getArchiveFile(skinPath);

        const std::u16string convSkinName = string_to_u16string(skinName);

        // 4jcraft: shiggy has no IggyLibraryCreateFromMemory unfortunately
        lib = IggyLibraryCreateFromMemoryUTF16(
            convSkinName.data(), (void*)baFile.data(), baFile.size(), nullptr);

#if defined(_DEBUG)
        IggyMemoryUseInfo memoryInfo;
        rrbool res;
        int iteration = 0;
        int64_t totalStatic = 0;
        while ((res = IggyDebugGetMemoryUseInfo(nullptr, lib, "", 0, iteration,
                                                &memoryInfo))) {
            totalStatic += memoryInfo.static_allocation_bytes;
            app.DebugPrintf(
                app.USER_SR, "%s - %.*s, static: %dB, dynamic: %dB\n",
                skinPath.c_str(), memoryInfo.subcategory_stringlen,
                memoryInfo.subcategory, memoryInfo.static_allocation_bytes,
                memoryInfo.dynamic_allocation_bytes);
            ++iteration;
        }

        app.DebugPrintf(app.USER_SR, "%s - Total static: %dB (%dKB)\n",
                        skinPath.c_str(), totalStatic, totalStatic / 1024);
#endif
    }
    return lib;
}

void UIController::ReloadSkin() {
    // Destroy all scene swf
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        // m_bCloseAllScenes[i] = true;
        m_groups[i]->DestroyAll();
    }

    // Unload the current libraries
    // Some libraries reference others, so we destroy in reverse order
    for (int i = eLibrary_Count - 1; i >= 0; --i) {
        if (m_iggyLibraries[i] != IGGY_INVALID_LIBRARY)
            IggyLibraryDestroy(m_iggyLibraries[i]);
        m_iggyLibraries[i] = IGGY_INVALID_LIBRARY;
    }

#if defined(_WINDOWS64) || defined(__linux__)
    // 4J Stu - Don't load on a thread on windows. I haven't investigated this
    // in detail, so a quick fix
    reloadSkinThreadProc(this);
#else

    m_reloadSkinThread =
        new C4JThread(reloadSkinThreadProc, (void*)this, "Reload skin thread");

    // Navigate to the timer scene so that we can display something while the
    // loading is happening
    ui.NavigateToScene(0, eUIScene_Timer, (void*)1, eUILayer_Tooltips,
                       eUIGroup_Fullscreen);
    // m_reloadSkinThread->run();

    //// Load new skin
    // loadSkins();

    //// Reload all scene swf
    // for(int i = eUIGroup_Player1; i <= eUIGroup_Player4; ++i)
    //{
    //	m_groups[i]->ReloadAll();
    // }

    //// Always reload the fullscreen group
    // m_groups[eUIGroup_Fullscreen]->ReloadAll();
#endif
}

void UIController::StartReloadSkinThread() {
    if (m_reloadSkinThread) m_reloadSkinThread->run();
}

int UIController::reloadSkinThreadProc(void* lpParam) {
    {
        std::lock_guard<std::mutex> lock(
            ms_reloadSkinCS);  // MGH - added to prevent crash loading Iggy
                               // movies while the skins were being reloaded
        UIController* controller = (UIController*)lpParam;
        // Load new skin
        controller->loadSkins();

        // Reload all scene swf
        for (int i = eUIGroup_Player1; i < eUIGroup_COUNT; ++i) {
            controller->m_groups[i]->ReloadAll();
        }

        // Always reload the fullscreen group
        controller->m_groups[eUIGroup_Fullscreen]->ReloadAll();

        // 4J Stu - Don't do this on windows, as we never navigated forwards to
        // start with
#if !(defined(_WINDOWS64) || defined(__linux__))
        controller->NavigateBack(0, false, eUIScene_COUNT, eUILayer_Tooltips);
#endif
    }

    return 0;
}

bool UIController::IsReloadingSkin() {
    return m_reloadSkinThread && (!m_reloadSkinThread->hasStarted() ||
                                  m_reloadSkinThread->isRunning());
}

bool UIController::IsExpectingOrReloadingSkin() {
    return Minecraft::GetInstance()->skins->getSelected()->isLoadingData() ||
           Minecraft::GetInstance()->skins->needsUIUpdate() ||
           IsReloadingSkin() || PendingFontChange();
}

void UIController::CleanUpSkinReload() {
    delete m_reloadSkinThread;
    m_reloadSkinThread = nullptr;

    if (!Minecraft::GetInstance()->skins->isUsingDefaultSkin()) {
        if (!Minecraft::GetInstance()->skins->getSelected()->hasAudio()) {
            const unsigned int result =
                PlatformStorage.UnmountInstalledDLC("TPACK");
        }
    }

    for (auto it = m_queuedMessageBoxData.begin();
         it != m_queuedMessageBoxData.end(); ++it) {
        QueuedMessageBoxData* queuedData = *it;
        ui.NavigateToScene(queuedData->iPad, eUIScene_MessageBox,
                           &queuedData->info, queuedData->layer,
                           eUIGroup_Fullscreen);
        delete queuedData->info.uiOptionA;
        delete queuedData;
    }
    m_queuedMessageBoxData.clear();
}

std::vector<uint8_t> UIController::getMovieData(const std::string& filename) {
    // Cache everything we load in the current tick
    int64_t targetTime = System::currentTimeMillis() + (1000LL * 60);
    auto it = m_cachedMovieData.find(filename);
    if (it == m_cachedMovieData.end()) {
        std::vector<uint8_t> baFile = app.getArchiveFile(filename);
        CachedMovieData cmd;
        cmd.m_ba = baFile;
        cmd.m_expiry = targetTime;
        m_cachedMovieData[filename] = cmd;
        return baFile;
    } else {
        it->second.m_expiry = targetTime;
        return it->second.m_ba;
    }
}

// INPUT
void UIController::tickInput() {
    // If system/commerce UI up, don't handle input
    // if(!m_bSysUIShowing && !m_bSystemUIShowing)
    if (!m_bSystemUIShowing) {
#if defined(ENABLE_IGGY_PERFMON)
        if (m_iggyPerfmonEnabled) {
            if (PlatformInput.ButtonPressed(PlatformProfile.GetPrimaryPad(),
                                            ACTION_MENU_STICK_PRESS))
                m_iggyPerfmonEnabled = !m_iggyPerfmonEnabled;
        } else
#endif
        {
            handleInput();
            ++m_accumulatedTicks;
        }
    }
}

void UIController::handleInput() {
    // For each user, loop over each key type and send messages based on the
    // state
    for (unsigned int iPad = 0; iPad < XUSER_MAX_COUNT; ++iPad) {
        for (unsigned int key = 0; key <= ACTION_MAX_MENU; ++key) {
            handleKeyPress(iPad, key);
        }
    }
}

void UIController::handleKeyPress(unsigned int iPad, unsigned int key) {
    bool down = false;
    bool pressed = false;   // Toggle
    bool released = false;  // Toggle
    bool repeat = false;

    down = PlatformInput.ButtonDown(iPad, key);
    pressed = PlatformInput.ButtonPressed(iPad, key);    // Toggle
    released = PlatformInput.ButtonReleased(iPad, key);  // Toggle

    if (pressed) app.DebugPrintf("Pressed %d\n", key);
    if (released) app.DebugPrintf("Released %d\n", key);
    // Repeat handling
    if (pressed) {
        // Start repeat timer
        m_actionRepeatTimer[iPad][key] =
            time_util::clock::now() +
            std::chrono::milliseconds(UI_REPEAT_KEY_DELAY_MS);
    } else if (released) {
        // Stop repeat timer
        m_actionRepeatTimer[iPad][key] = {};
    } else if (down) {
        // Check is enough time has elapsed to be a repeat key
        auto now = time_util::clock::now();
        if (m_actionRepeatTimer[iPad][key] != time_util::time_point{} &&
            now > m_actionRepeatTimer[iPad][key]) {
            repeat = true;
            pressed = true;
            m_actionRepeatTimer[iPad][key] =
                now + std::chrono::milliseconds(UI_REPEAT_KEY_REPEAT_RATE_MS);
        }
    }

#if !defined(_CONTENT_PACKAGE)

#if defined(ENABLE_IGGY_PERFMON)
    if (pressed && !repeat && key == ACTION_MENU_STICK_PRESS) {
        m_iggyPerfmonEnabled = !m_iggyPerfmonEnabled;
    }
#endif

    // 4J Stu - Removed this function
#endif
    // #endif
    if (repeat || pressed || released) {
        bool handled = false;

        // Send the key to the fullscreen group first
        m_groups[(int)eUIGroup_Fullscreen]->handleInput(
            iPad, key, repeat, pressed, released, handled);
        if (!handled) {
            // If it's not been handled yet, then pass the event onto the
            // players specific group
            m_groups[(iPad + 1)]->handleInput(iPad, key, repeat, pressed,
                                              released, handled);
        }
    }
}

rrbool RADLINK
UIController::ExternalFunctionCallback(void* user_callback_data, Iggy* player,
                                       IggyExternalFunctionCallUTF16* call) {
    UIScene* scene = (UIScene*)IggyPlayerGetUserdata(player);

    if (scene != nullptr) {
        scene->externalCallback(call);
    }

    return true;
}

// RENDERING
void UIController::renderScenes() {
    // Only render player scenes if the game is started
    if (app.GetGameStarted() &&
        !m_groups[eUIGroup_Fullscreen]->hidesLowerScenes()) {
        for (int i = eUIGroup_Player1; i < eUIGroup_COUNT; ++i) {
            m_groups[i]->render();
        }
    }

    // Always render the fullscreen group
    m_groups[eUIGroup_Fullscreen]->render();

#if defined(ENABLE_IGGY_PERFMON)
    if (m_iggyPerfmonEnabled) {
        IggyPerfmonPad pm_pad;

        pm_pad.bits = 0;
        pm_pad.field.dpad_up = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_UP);
        pm_pad.field.dpad_down = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_DOWN);
        pm_pad.field.dpad_left = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_LEFT);
        pm_pad.field.dpad_right = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_RIGHT);
        pm_pad.field.button_up = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_Y);
        pm_pad.field.button_down = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_A);
        pm_pad.field.button_left = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_X);
        pm_pad.field.button_right = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_B);
        pm_pad.field.shoulder_left_hi = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_LEFT_SCROLL);
        pm_pad.field.shoulder_right_hi = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_RIGHT_SCROLL);
        pm_pad.field.trigger_left_low = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_PAGEUP);
        pm_pad.field.trigger_right_low = PlatformInput.ButtonPressed(
            PlatformProfile.GetPrimaryPad(), ACTION_MENU_PAGEDOWN);
        // IggyPerfmonPadFromXInputStatePointer(pm_pad, &xi_pad);

        // gdraw_D3D_SetTileOrigin( fb,
        //	zb,
        //	PM_ORIGIN_X,
        //	PM_ORIGIN_Y );
        IggyPerfmonTickAndDraw(
            iggy_perfmon, gdraw_funcs, &pm_pad, PM_ORIGIN_X, PM_ORIGIN_Y,
            getScreenWidth(),
            getScreenHeight());  // perfmon draw area in window coords
    }
#endif
}

void UIController::getRenderDimensions(
    IPlatformRenderer::eViewportType viewport, S32& width, S32& height) {
    switch (viewport) {
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
            width = (S32)(getScreenWidth());
            height = (S32)(getScreenHeight());
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
            width = (S32)(getScreenWidth() / 2);
            height = (S32)(getScreenHeight() / 2);
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            width = (S32)(getScreenWidth() / 2);
            height = (S32)(getScreenHeight() / 2);
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
            width = (S32)(getScreenWidth() / 2);
            height = (S32)(getScreenHeight() / 2);
            break;
        default:
            break;
    }
}

void UIController::setupRenderPosition(
    IPlatformRenderer::eViewportType viewport) {
    if (m_bCustomRenderPosition || m_currentRenderViewport != viewport) {
        m_currentRenderViewport = viewport;
        m_bCustomRenderPosition = false;
        S32 xPos = 0;
        S32 yPos = 0;
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
                xPos = (S32)(getScreenWidth() / 4);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
                xPos = (S32)(getScreenWidth() / 4);
                yPos = (S32)(getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
                yPos = (S32)(getScreenHeight() / 4);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
                xPos = (S32)(getScreenWidth() / 2);
                yPos = (S32)(getScreenHeight() / 4);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
                xPos = (S32)(getScreenWidth() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
                yPos = (S32)(getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
                xPos = (S32)(getScreenWidth() / 2);
                yPos = (S32)(getScreenHeight() / 2);
                break;
            default:
                break;
        }
        m_tileOriginX = xPos;
        m_tileOriginY = yPos;
        setTileOrigin(xPos, yPos);
    }
}

void UIController::setupRenderPosition(S32 xOrigin, S32 yOrigin) {
    m_bCustomRenderPosition = true;
    m_tileOriginX = xOrigin;
    m_tileOriginY = yOrigin;
    setTileOrigin(xOrigin, yOrigin);
}

void UIController::setupCustomDrawGameState() {
    // Rest the clear rect
    m_customRenderingClearRect.left = LONG_MAX;
    m_customRenderingClearRect.right = LONG_MIN;
    m_customRenderingClearRect.top = LONG_MAX;
    m_customRenderingClearRect.bottom = LONG_MIN;

#if defined(_WINDOWS64)
    PlatformRenderer.StartFrame();

    gdraw_D3D11_setViewport_4J();
#elif defined(__linux__)
    PlatformRenderer.StartFrame();
#endif
    PlatformRenderer.Set_matrixDirty();

    // 4J Stu - We don't need to clear this here as iggy hasn't written anything
    // to the depth buffer. We DO however clear after we render which is why we
    // still setup the rectangle here
    // PlatformRenderer.Clear(GL_DEPTH_BUFFER_BIT, &m_customRenderingClearRect);
    // glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_fScreenWidth, m_fScreenHeight, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(true);
}

void UIController::setupCustomDrawMatrices(UIScene* scene,
                                           CustomDrawData* customDrawRegion) {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    // Clear just the region required for this control.
    float sceneWidth = (float)scene->getRenderWidth();
    float sceneHeight = (float)scene->getRenderHeight();

    int32_t left, right, top, bottom;
    {
        left =
            m_tileOriginX +
            (sceneWidth + customDrawRegion->mat[(0 * 4) + 3] * sceneWidth) / 2;
        right = left + ((sceneWidth * customDrawRegion->mat[0]) / 2) *
                           customDrawRegion->x1;
    }

    top = m_tileOriginY +
          (sceneHeight - customDrawRegion->mat[(1 * 4) + 3] * sceneHeight) / 2;
    bottom = top + (sceneHeight * -customDrawRegion->mat[(1 * 4) + 1]) / 2 *
                       customDrawRegion->y1;

    m_customRenderingClearRect.left =
        std::min<long>(m_customRenderingClearRect.left, left);
    m_customRenderingClearRect.right =
        std::max<long>(m_customRenderingClearRect.right, right);
    ;
    m_customRenderingClearRect.top =
        std::min<long>(m_customRenderingClearRect.top, top);
    m_customRenderingClearRect.bottom =
        std::max<long>(m_customRenderingClearRect.bottom, bottom);

    if (!m_bScreenWidthSetup) {
        Minecraft* pMinecraft = Minecraft::GetInstance();
        if (pMinecraft != nullptr) {
            m_fScreenWidth = (float)pMinecraft->width_phys;
            m_fScreenHeight = (float)pMinecraft->height_phys;
            m_bScreenWidthSetup = true;
        }
    }

    glLoadIdentity();
    glTranslatef(0, 0, -2000);
    // Iggy translations are based on a double-size target, with the origin in
    // the centre
    glTranslatef(
        (m_fScreenWidth + customDrawRegion->mat[(0 * 4) + 3] * m_fScreenWidth) /
            2,
        (m_fScreenHeight -
         customDrawRegion->mat[(1 * 4) + 3] * m_fScreenHeight) /
            2,
        0);
    // Iggy scales are based on a double-size target
    glScalef((m_fScreenWidth * customDrawRegion->mat[0]) / 2,
             (m_fScreenHeight * -customDrawRegion->mat[(1 * 4) + 1]) / 2, 1.0f);
}

void UIController::setupCustomDrawGameStateAndMatrices(
    UIScene* scene, CustomDrawData* customDrawRegion) {
    setupCustomDrawGameState();
    setupCustomDrawMatrices(scene, customDrawRegion);
}

void UIController::endCustomDrawGameState() {
#if defined(__linux__)
    PlatformRenderer.Clear(GL_DEPTH_BUFFER_BIT);
#else
    PlatformRenderer.Clear(GL_DEPTH_BUFFER_BIT, &m_customRenderingClearRect);
#endif
    // glClear(GL_DEPTH_BUFFER_BIT);
    glDepthMask(false);
    glDisable(GL_ALPHA_TEST);
}

void UIController::endCustomDrawMatrices() {}

void UIController::endCustomDrawGameStateAndMatrices() {
    endCustomDrawMatrices();
    endCustomDrawGameState();
}

void RADLINK
UIController::CustomDrawCallback(void* user_callback_data, Iggy* player,
                                 IggyCustomDrawCallbackRegion* region) {
    UIScene* scene = (UIScene*)IggyPlayerGetUserdata(player);

    if (scene != nullptr) {
        scene->customDraw(region);
    }
}

// Description
// Callback to create a user-defined texture to replace SWF-defined textures.
// Parameters
// width - Input value: optional number of pixels wide specified from AS3, or -1
// if not defined. Output value: the number of pixels wide to pretend to Iggy
// that the bitmap is. SWF and AS3 scales bitmaps based on their pixel
// dimensions, so you can use this to substitute a texture that is higher or
// lower resolution that ActionScript thinks it is. height - Input value:
// optional number of pixels high specified from AS3, or -1 if not defined.
// Output value: the number of pixels high to pretend to Iggy that the bitmap
// is. SWF and AS3 scales bitmaps based on their pixel dimensions, so you can
// use this to substitute a texture that is higher or lower resolution that
// ActionScript thinks it is. destroy_callback_data - Optional additional output
// value you can set; the value will be passed along to the corresponding
// Iggy_TextureSubstitutionDestroyCallback (e.g. you can store the pointer to
// your own internal structure here). return - A platform-independent wrapped
// texture handle provided by GDraw, or nullptr (nullptr with throw an
// ActionScript 3 ArgumentError that the Flash developer can catch) Use by
// calling IggySetTextureSubstitutionCallbacks.
//
// Discussion
//
// If your texture includes an alpha channel, you must use a premultiplied alpha
// (where the R,G, and B channels have been multiplied by the alpha value); all
// Iggy shaders assume premultiplied alpha (and it looks better anyway).
GDrawTexture* RADLINK UIController::TextureSubstitutionCreateCallback(
    void* user_callback_data, IggyUTF16* texture_name, S32* width, S32* height,
    void** destroy_callback_data) {
    UIController* uiController = (UIController*)user_callback_data;
    auto it = uiController->m_substitutionTextures.find((char*)texture_name);

    if (it != uiController->m_substitutionTextures.end()) {
        app.DebugPrintf("Found substitution texture %s, with %d bytes\n",
                        (char*)texture_name, it->second.size());

        BufferedImage image(it->second.data(), it->second.size());
        if (image.getData() != nullptr) {
            image.preMultiplyAlpha();
            Textures* t = Minecraft::GetInstance()->textures;
            int id = t->getTexture(
                &image, IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw, false);

            // 4J Stu - All our flash controls that allow replacing textures use
            // a special 64x64 symbol Force this size here so that our images
            // don't get scaled wildly
            *width = 64;
            *height = 64;

            *destroy_callback_data = (void*)(intptr_t)id;

            app.DebugPrintf("Found substitution texture %s (%d) - %dx%d\n",
                            (char*)texture_name, id, image.getWidth(),
                            image.getHeight());
            return ui.getSubstitutionTexture(id);
        } else {
            return nullptr;
        }
    } else {
        app.DebugPrintf("Could not find substitution texture %s\n",
                        (char*)texture_name);
        return nullptr;
    }
}

// Description
// Callback received from Iggy when it stops using a user-defined texture.
void RADLINK UIController::TextureSubstitutionDestroyCallback(
    void* user_callback_data, void* destroy_callback_data,
    GDrawTexture* handle) {
    // Orbis complains about casting a pointer to an int
    int64_t llVal = (int64_t)destroy_callback_data;
    int id = (int)llVal;
    app.DebugPrintf("Destroying iggy texture %d\n", id);

    ui.destroySubstitutionTexture(user_callback_data, handle);

    Textures* t = Minecraft::GetInstance()->textures;
    t->releaseTexture(id);
}

void UIController::registerSubstitutionTexture(const std::string& textureName,
                                               std::uint8_t* pbData,
                                               unsigned int dwLength) {
    // Remove it if it already exists
    unregisterSubstitutionTexture(textureName, false);

    m_substitutionTextures[textureName] =
        std::vector<uint8_t>(pbData, pbData + dwLength);
}

void UIController::unregisterSubstitutionTexture(const std::string& textureName,
                                                 bool deleteData) {
    auto it = m_substitutionTextures.find(textureName);

    if (it != m_substitutionTextures.end()) {
        m_substitutionTextures.erase(it);
    }
}

// NAVIGATION
bool UIController::NavigateToScene(int iPad, EUIScene scene, void* initData,
                                   EUILayer layer, EUIGroup group) {
    static bool bSeenUpdateTextThisSession = false;
    // If you're navigating to the multigamejoinload, and the player hasn't seen
    // the updates message yet, display it now display this message the first 3
    // times
    if ((scene == eUIScene_LoadOrJoinMenu) &&
        (bSeenUpdateTextThisSession == false) &&
        (app.GetGameSettings(PlatformProfile.GetPrimaryPad(),
                             eGameSetting_DisplayUpdateMessage) != 0)) {
        scene = eUIScene_NewUpdateMessage;
        bSeenUpdateTextThisSession = true;
    }

    // if you're trying to navigate to the inventory,the crafting, pause or game
    // info or any of the trigger scenes and there's already a menu up (because
    // you were pressing a few buttons at the same time) then ignore the
    // navigate
    if (GetMenuDisplayed(iPad)) {
        switch (scene) {
            case eUIScene_PauseMenu:
            case eUIScene_Crafting2x2Menu:
            case eUIScene_Crafting3x3Menu:
            case eUIScene_FurnaceMenu:
            case eUIScene_ContainerMenu:
            case eUIScene_LargeContainerMenu:
            case eUIScene_InventoryMenu:
            case eUIScene_CreativeMenu:
            case eUIScene_DispenserMenu:
            case eUIScene_SignEntryMenu:
            case eUIScene_InGameInfoMenu:
            case eUIScene_EnchantingMenu:
            case eUIScene_BrewingStandMenu:
            case eUIScene_AnvilMenu:
            case eUIScene_TradingMenu:
            case eUIScene_BeaconMenu:
            case eUIScene_HorseMenu:
                app.DebugPrintf(
                    "IGNORING NAVIGATE - we're trying to navigate to a user "
                    "selected scene when there's already a scene up: pad:%d, "
                    "scene:%d\n",
                    iPad, scene);
                return false;
                break;
            default:
                break;
        }
    }

    switch (scene) {
        case eUIScene_FullscreenProgress: {
            // 4J Stu - The fullscreen progress scene should not interfere with
            // any other scene stack, so should be placed in it's own
            // group/layer
            layer = eUILayer_Fullscreen;
            group = eUIGroup_Fullscreen;
        } break;
        case eUIScene_ConnectingProgress: {
            // The connecting progress scene shouldn't interfere with other
            // scenes
            layer = eUILayer_Fullscreen;
        } break;
        case eUIScene_EndPoem: {
            // The end poem scene shouldn't interfere with other scenes, but
            // will be underneath the autosave progress
            group = eUIGroup_Fullscreen;
            layer = eUILayer_Scene;
        } break;
        default:
            break;
    };
    int menuDisplayedPad = XUSER_INDEX_ANY;
    if (group == eUIGroup_PAD) {
        if (app.GetGameStarted()) {
            // If the game isn't running treat as user 0, otherwise map index
            // directly from pad
            if ((iPad != 255) && (iPad >= 0)) {
                menuDisplayedPad = iPad;
                group = (EUIGroup)(iPad + 1);
            } else
                group = eUIGroup_Fullscreen;
        } else {
            layer = eUILayer_Fullscreen;
            group = eUIGroup_Fullscreen;
        }
    }

    time_util::Timer timer;

    bool success;
    {
        std::lock_guard<std::mutex> lock(m_navigationLock);
        SetMenuDisplayed(menuDisplayedPad, true);
        success =
            m_groups[(int)group]->NavigateToScene(iPad, scene, initData, layer);
        if (success && group == eUIGroup_Fullscreen)
            setFullscreenMenuDisplayed(true);
    }

    std::println(stderr, "TIMER: Navigate to scene: Elapsed time {:.6f}",
                 timer.elapsed_seconds());

    return success;
    // return true;
}

bool UIController::NavigateBack(int iPad, bool forceUsePad, EUIScene eScene,
                                EUILayer eLayer) {
    bool navComplete = false;
    if (app.GetGameStarted()) {
        bool navComplete = m_groups[(int)eUIGroup_Fullscreen]->NavigateBack(
            iPad, eScene, eLayer);

        if (!navComplete && (iPad != 255) && (iPad >= 0)) {
            EUIGroup group = (EUIGroup)(iPad + 1);
            navComplete =
                m_groups[(int)group]->NavigateBack(iPad, eScene, eLayer);
            if (!m_groups[(int)group]->GetMenuDisplayed())
                SetMenuDisplayed(iPad, false);
        }
        // 4J-PB - autosave in fullscreen doesn't clear the menuDisplayed flag
        else {
            if (!m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed()) {
                setFullscreenMenuDisplayed(false);
                for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                    SetMenuDisplayed(i, m_groups[i + 1]->GetMenuDisplayed());
                }
            }
        }
    } else {
        navComplete = m_groups[(int)eUIGroup_Fullscreen]->NavigateBack(
            iPad, eScene, eLayer);
        if (!m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed())
            SetMenuDisplayed(XUSER_INDEX_ANY, false);
    }
    return navComplete;
}

void UIController::NavigateToHomeMenu() {
    ui.CloseAllPlayersScenes();

    // Alert the app the we no longer want to be informed of ethernet
    // connections
    app.SetLiveLinkRequired(false);

    Minecraft* pMinecraft = Minecraft::GetInstance();

    // 4J-PB - just about to switched to the default texture pack , so clean up
    // anything texture pack related here

    // unload any texture pack audio
    // if there is audio in use, clear out the audio, and unmount the pack
    TexturePack* pTexPack = Minecraft::GetInstance()->skins->getSelected();

    DLCTexturePack* pDLCTexPack = nullptr;
    if (pTexPack->hasAudio()) {
        // get the dlc texture pack, and store it
        pDLCTexPack = (DLCTexturePack*)pTexPack;
    }

    // change to the default texture pack
    pMinecraft->skins->selectTexturePackById(
        TexturePackRepository::DEFAULT_TEXTURE_PACK_ID);

    if (pTexPack->hasAudio()) {
        // need to stop the streaming audio - by playing streaming audio from
        // the default texture pack now reset the streaming sounds back to the
        // normal ones
        pMinecraft->soundEngine->SetStreamingSounds(
            eStream_Overworld_Calm1, eStream_Overworld_piano3, eStream_Nether1,
            eStream_Nether4, eStream_end_dragon, eStream_end_end, eStream_CD_1);
        pMinecraft->soundEngine->playStreaming("", 0, 0, 0, 1, 1);

        // 		if(pDLCTexPack->m_pStreamedWaveBank!=nullptr)
        // 		{
        // 			pDLCTexPack->m_pStreamedWaveBank->Destroy();
        // 		}
        // 		if(pDLCTexPack->m_pSoundBank!=nullptr)
        // 		{
        // 			pDLCTexPack->m_pSoundBank->Destroy();
        // 		}
        const unsigned int result =
            PlatformStorage.UnmountInstalledDLC("TPACK");

        app.DebugPrintf("Unmount result is %d\n", result);
    }

    g_NetworkManager.ForceFriendsSessionRefresh();

    if (pMinecraft->skins->needsUIUpdate()) {
        m_navigateToHomeOnReload = true;
    } else {
        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(), eUIScene_MainMenu);
#if defined(ENABLE_JAVA_GUIS)
        pMinecraft->setScreen(new TitleScreen());
#endif
    }
}

UIScene* UIController::GetTopScene(int iPad, EUILayer layer, EUIGroup group) {
    if (group == eUIGroup_PAD) {
        if (app.GetGameStarted()) {
            // If the game isn't running treat as user 0, otherwise map index
            // directly from pad
            if ((iPad != 255) && (iPad >= 0)) {
                group = (EUIGroup)(iPad + 1);
            } else
                group = eUIGroup_Fullscreen;
        } else {
            layer = eUILayer_Fullscreen;
            group = eUIGroup_Fullscreen;
        }
    }
    return m_groups[(int)group]->GetTopScene(layer);
}

size_t UIController::RegisterForCallbackId(UIScene* scene) {
    std::lock_guard<std::mutex> lock(m_registeredCallbackScenesCS);
    static std::atomic<std::uint32_t> s_nextId{1};
    size_t newId = s_nextId.fetch_add(1, std::memory_order_relaxed) & 0xFFFFFF;
    newId |= (scene->getSceneType()
              << 24);  // Add in the scene's type to help keep this unique
    m_registeredCallbackScenes[newId] = scene;
    return newId;
}

void UIController::UnregisterCallbackId(size_t id) {
    std::lock_guard<std::mutex> lock(m_registeredCallbackScenesCS);
    auto it = m_registeredCallbackScenes.find(id);
    if (it != m_registeredCallbackScenes.end()) {
        m_registeredCallbackScenes.erase(it);
    }
}

UIScene* UIController::GetSceneFromCallbackId(size_t id) {
    UIScene* scene = nullptr;
    auto it = m_registeredCallbackScenes.find(id);
    if (it != m_registeredCallbackScenes.end()) {
        scene = it->second;
    }
    return scene;
}

void UIController::lockCallbackScenes() { m_registeredCallbackScenesCS.lock(); }

void UIController::unlockCallbackScenes() {
    m_registeredCallbackScenesCS.unlock();
}

void UIController::CloseAllPlayersScenes() {
    m_groups[(int)eUIGroup_Fullscreen]->getTooltips()->SetTooltips(-1);
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        // m_bCloseAllScenes[i] = true;
        m_groups[i]->closeAllScenes();
        m_groups[i]->getTooltips()->SetTooltips(-1);
    }

    if (!m_groups[eUIGroup_Fullscreen]->GetMenuDisplayed()) {
        for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
            SetMenuDisplayed(i, false);
        }
    }
    setFullscreenMenuDisplayed(false);
}

void UIController::CloseUIScenes(int iPad, bool forceIPad) {
    EUIGroup group;
    if (app.GetGameStarted() || forceIPad) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }

    m_groups[(int)group]->closeAllScenes();
    m_groups[(int)group]->getTooltips()->SetTooltips(-1);

    // This should cause the popup to dissappear
    TutorialPopupInfo popupInfo;
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->SetTutorialDescription(
            &popupInfo);

    if (group == eUIGroup_Fullscreen) setFullscreenMenuDisplayed(false);

    SetMenuDisplayed((group == eUIGroup_Fullscreen ? XUSER_INDEX_ANY : iPad),
                     m_groups[(int)group]->GetMenuDisplayed());
}

void UIController::setFullscreenMenuDisplayed(bool displayed) {
    // Show/hide the tooltips for the fullscreen group
    m_groups[(int)eUIGroup_Fullscreen]->showComponent(
        PlatformProfile.GetPrimaryPad(), eUIComponent_Tooltips,
        eUILayer_Tooltips, displayed);

    // Show/hide tooltips for the other layers
    for (unsigned int i = (eUIGroup_Fullscreen + 1); i < eUIGroup_COUNT; ++i) {
        m_groups[i]->showComponent(i, eUIComponent_Tooltips, eUILayer_Tooltips,
                                   !displayed);
    }
}

bool UIController::IsPauseMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsPauseMenuDisplayed();
}

bool UIController::IsContainerMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsContainerMenuDisplayed();
}

bool UIController::IsIgnorePlayerJoinMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsIgnorePlayerJoinMenuDisplayed();
}

bool UIController::IsIgnoreAutosaveMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)eUIGroup_Fullscreen]
               ->IsIgnoreAutosaveMenuDisplayed() ||
           (group != eUIGroup_Fullscreen &&
            m_groups[(int)group]->IsIgnoreAutosaveMenuDisplayed());
}

void UIController::SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed) {
    app.DebugPrintf(
        app.USER_SR,
        "UIController::SetIgnoreAutosaveMenuDisplayed is not implemented\n");
}

bool UIController::IsSceneInStack(int iPad, EUIScene eScene) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsSceneInStack(eScene);
}

bool UIController::GetMenuDisplayed(int iPad) { return m_bMenuDisplayed[iPad]; }

void UIController::SetMenuDisplayed(int iPad, bool bVal) {
    if (bVal) {
        if (iPad == XUSER_INDEX_ANY) {
            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                PlatformInput.SetMenuDisplayed(i, true);
                m_bMenuDisplayed[i] = true;
                // 4J Stu - Fix for #11018 - Functional: When the controller is
                // unplugged during active gameplay and plugged back in at the
                // resulting pause menu, it will demonstrate dual-functionality.
                m_bMenuToBeClosed[i] = false;
            }
        } else {
            PlatformInput.SetMenuDisplayed(iPad, true);
            m_bMenuDisplayed[iPad] = true;
            // 4J Stu - Fix for #11018 - Functional: When the controller is
            // unplugged during active gameplay and plugged back in at the
            // resulting pause menu, it will demonstrate dual-functionality.
            m_bMenuToBeClosed[iPad] = false;
        }
    } else {
        if (iPad == XUSER_INDEX_ANY) {
            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                m_bMenuToBeClosed[i] = true;
                m_iCountDown[i] = 10;
            }
        } else {
            m_bMenuToBeClosed[iPad] = true;
            m_iCountDown[iPad] = 10;
        }
    }
}

void UIController::CheckMenuDisplayed() {
    for (int iPad = 0; iPad < XUSER_MAX_COUNT; iPad++) {
        if (m_bMenuToBeClosed[iPad]) {
            if (m_iCountDown[iPad] != 0) {
                m_iCountDown[iPad]--;
            } else {
                m_bMenuToBeClosed[iPad] = false;
                m_bMenuDisplayed[iPad] = false;
                PlatformInput.SetMenuDisplayed(iPad, false);
            }
        }
    }
}

void UIController::SetTooltipText(unsigned int iPad, unsigned int tooltip,
                                  int iTextID) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->SetTooltipText(tooltip, iTextID);
}

void UIController::SetEnableTooltips(unsigned int iPad, bool bVal) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->SetEnableTooltips(bVal);
}

void UIController::ShowTooltip(unsigned int iPad, unsigned int tooltip,
                               bool show) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->ShowTooltip(tooltip, show);
}

void UIController::SetTooltips(unsigned int iPad, int iA, int iB, int iX,
                               int iY, int iLT, int iRT, int iLB, int iRB,
                               int iLS, int iRS, int iBack, bool forceUpdate) {
    EUIGroup group;

    // 4J-PB - strip out any that are not applicable on the platform
    if (iX == IDS_TOOLTIPS_SELECTDEVICE) iX = -1;
    if (iX == IDS_TOOLTIPS_CHANGEDEVICE) iX = -1;

    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->SetTooltips(
            iA, iB, iX, iY, iLT, iRT, iLB, iRB, iLS, iRS, iBack, forceUpdate);
}

void UIController::EnableTooltip(unsigned int iPad, unsigned int tooltip,
                                 bool enable) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->EnableTooltip(tooltip, enable);
}

void UIController::RefreshTooltips(unsigned int iPad) {
    app.DebugPrintf(app.USER_SR,
                    "UIController::RefreshTooltips is not implemented\n");
}

void UIController::AnimateKeyPress(int iPad, int iAction, bool bRepeat,
                                   bool bPressed, bool bReleased) {
    EUIGroup group;
    if (bPressed == false) {
        // only animating button press
        return;
    }
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool handled = false;
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->handleInput(
            iPad, iAction, bRepeat, bPressed, bReleased, handled);
}

void UIController::OverrideSFX(int iPad, int iAction, bool bVal) {
    EUIGroup group;

    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool handled = false;
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->overrideSFX(iPad, iAction, bVal);
}

void UIController::PlayUISFX(ESoundEffect eSound) {
    uint64_t time = System::currentTimeMillis();

    // Don't play multiple SFX on the same tick
    // (prevents horrible sounds when programmatically setting multiple
    // checkboxes)
    if (time - m_lastUiSfx < 10) {
        return;
    }
    m_lastUiSfx = time;

    Minecraft::GetInstance()->soundEngine->playUI(eSound, 1.0f, 1.0f);
}

void UIController::DisplayGamertag(unsigned int iPad, bool show) {
    // The host decides whether these are on or off
    if (app.GetGameSettings(PlatformProfile.GetPrimaryPad(),
                            eGameSetting_DisplaySplitscreenGamertags) == 0) {
        show = false;
    }
    EUIGroup group = (EUIGroup)(iPad + 1);
    if (m_groups[(int)group]->getHUD())
        m_groups[(int)group]->getHUD()->ShowDisplayName(show);

    // Update TutorialPopup in Splitscreen if no container is displayed (to make
    // sure the Popup does not overlap with the Gamertag!)
    if (app.GetLocalPlayerCount() > 1 &&
        m_groups[(int)group]->getTutorialPopup() &&
        !m_groups[(int)group]->IsContainerMenuDisplayed()) {
        m_groups[(int)group]->getTutorialPopup()->UpdateTutorialPopup();
    }
}

void UIController::SetSelectedItem(unsigned int iPad, const std::string& name) {
    EUIGroup group;

    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool handled = false;
    if (m_groups[(int)group]->getHUD())
        m_groups[(int)group]->getHUD()->SetSelectedLabel(name);
}

void UIController::UpdateSelectedItemPos(unsigned int iPad) {
    app.DebugPrintf(app.USER_SR,
                    "UIController::UpdateSelectedItemPos not implemented\n");
}

void UIController::HandleDLCMountingComplete() {
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        app.DebugPrintf(
            "UIController::HandleDLCMountingComplete - m_groups[%d]\n", i);
        m_groups[i]->HandleDLCMountingComplete();
    }
}

void UIController::HandleDLCInstalled(int iPad) {
    // app.DebugPrintf(app.USER_SR, "UIController::HandleDLCInstalled not
    // implemented\n");
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->HandleDLCInstalled();
    }
}

void UIController::HandleTMSDLCFileRetrieved(int iPad) {
    app.DebugPrintf(
        app.USER_SR,
        "UIController::HandleTMSDLCFileRetrieved not implemented\n");
}

void UIController::HandleTMSBanFileRetrieved(int iPad) {
    app.DebugPrintf(
        app.USER_SR,
        "UIController::HandleTMSBanFileRetrieved not implemented\n");
}

void UIController::HandleInventoryUpdated(int iPad) {
    EUIGroup group = eUIGroup_Fullscreen;
    if (app.GetGameStarted() && (iPad != 255) && (iPad >= 0)) {
        group = (EUIGroup)(iPad + 1);
    }

    m_groups[group]->HandleMessage(eUIMessage_InventoryUpdated, nullptr);
}

void UIController::HandleGameTick() {
    tickInput();

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        if (m_groups[i]->getHUD()) m_groups[i]->getHUD()->handleGameTick();
    }
}

void UIController::SetTutorial(int iPad, Tutorial* tutorial) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->SetTutorial(tutorial);
}

void UIController::SetTutorialDescription(int iPad, TutorialPopupInfo* info) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }

    if (m_groups[(int)group]->getTutorialPopup()) {
        // tutorial popup needs to know if a container menu is being displayed
        m_groups[(int)group]->getTutorialPopup()->SetContainerMenuVisible(
            m_groups[(int)group]->IsContainerMenuDisplayed());
        m_groups[(int)group]->getTutorialPopup()->SetTutorialDescription(info);
    }
}

void UIController::RemoveInteractSceneReference(int iPad, UIScene* scene) {
    EUIGroup group;
    if ((iPad != 255) && (iPad >= 0))
        group = (EUIGroup)(iPad + 1);
    else
        group = eUIGroup_Fullscreen;
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->RemoveInteractSceneReference(
            scene);
}

void UIController::SetTutorialVisible(int iPad, bool visible) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->SetVisible(visible);
}

bool UIController::IsTutorialVisible(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        // If the game isn't running treat as user 0, otherwise map index
        // directly from pad
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool visible = false;
    if (m_groups[(int)group]->getTutorialPopup())
        visible = m_groups[(int)group]->getTutorialPopup()->IsVisible();
    return visible;
}

void UIController::UpdatePlayerBasePositions() {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
        if (pMinecraft->localplayers[idx] != nullptr) {
            if (pMinecraft->localplayers[idx]->m_iScreenSection ==
                IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN) {
                DisplayGamertag(idx, false);
            } else {
                DisplayGamertag(idx, true);
            }
            m_groups[idx + 1]->SetViewportType(
                (IPlatformRenderer::eViewportType)pMinecraft->localplayers[idx]
                    ->m_iScreenSection);
        } else {
            // 4J Stu - This is a legacy thing from our XUI implementation that
            // we don't need Changing the viewport to fullscreen for users that
            // no longer exist is SLOW This should probably be on all platforms,
            // but I don't have time to test them all just now!
            m_groups[idx + 1]->SetViewportType(
                IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN);
            DisplayGamertag(idx, false);
        }
    }
}

void UIController::SetEmptyQuadrantLogo(int iSection) {
    // 4J Stu - We shouldn't need to implement this
}

void UIController::HideAllGameUIElements() {
    // 4J Stu - We might not need to implement this
    app.DebugPrintf(app.USER_SR,
                    "UIController::HideAllGameUIElements not implemented\n");
}

void UIController::ShowOtherPlayersBaseScene(unsigned int iPad, bool show) {
    // 4J Stu - We shouldn't need to implement this
}

void UIController::ShowTrialTimer(bool show) {
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showTrialTimer(show);
}

void UIController::SetTrialTimerLimitSecs(unsigned int uiSeconds) {
    UIController::m_dwTrialTimerLimitSecs = uiSeconds;
}

void UIController::UpdateTrialTimer(unsigned int iPad) {
    char wcTime[20];

    std::uint32_t timeTicks = (std::uint32_t)app.getTrialTimer();

    if (timeTicks > m_dwTrialTimerLimitSecs) {
        timeTicks = m_dwTrialTimerLimitSecs;
    }

    timeTicks = m_dwTrialTimerLimitSecs - timeTicks;

#if !defined(_CONTENT_PACKAGE)
    if (true)
#else
    // display the time - only if there's less than 3 minutes
    if (timeTicks < 180)
#endif
    {
        int iMins = timeTicks / 60;
        int iSeconds = timeTicks % 60;
        snprintf(wcTime, 20, "%d:%02d", iMins, iSeconds);
        if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
            m_groups[(int)eUIGroup_Fullscreen]
                ->getPressStartToPlay()
                ->setTrialTimer(wcTime);
    } else {
        if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
            m_groups[(int)eUIGroup_Fullscreen]
                ->getPressStartToPlay()
                ->setTrialTimer("");
    }

    // are we out of time?
    if (timeTicks == 0) {
        // Trial over
        // bring up the pause menu to stop the trial over message box being
        // called again?
        if (!ui.GetMenuDisplayed(iPad)) {
            ui.NavigateToScene(iPad, eUIScene_PauseMenu, nullptr,
                               eUILayer_Scene);

            app.SetAction(iPad, eAppAction_TrialOver);
        }
    }
}

void UIController::ReduceTrialTimerValue() {
    std::uint32_t timeTicks = (std::uint32_t)app.getTrialTimer();

    if (timeTicks > m_dwTrialTimerLimitSecs) {
        timeTicks = m_dwTrialTimerLimitSecs;
    }

    m_dwTrialTimerLimitSecs -= timeTicks;
}

void UIController::ShowAutosaveCountdownTimer(bool show) {
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showTrialTimer(show);
}

void UIController::UpdateAutosaveCountdownTimer(unsigned int uiSeconds) {
    char wcAutosaveCountdown[100];
    snprintf(wcAutosaveCountdown, 100, app.GetString(IDS_AUTOSAVE_COUNTDOWN),
             uiSeconds);
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->setTrialTimer(wcAutosaveCountdown);
}

void UIController::ShowSavingMessage(unsigned int iPad,
                                     IPlatformStorage::ESavingMessage eVal) {
    bool show = false;
    switch (eVal) {
        case IPlatformStorage::ESavingMessage_None:
            show = false;
            break;
        case IPlatformStorage::ESavingMessage_Short:
        case IPlatformStorage::ESavingMessage_Long:
            show = true;
            break;
    }
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showSaveIcon(
            show);
}

void UIController::ShowPlayerDisplayname(bool show) {
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showPlayerDisplayName(show);
}

void UIController::SetWinUserIndex(unsigned int iPad) { m_winUserIndex = iPad; }

unsigned int UIController::GetWinUserIndex() { return m_winUserIndex; }

void UIController::ShowUIDebugConsole(bool show) {
#if !defined(_CONTENT_PACKAGE)

    if (show) {
        m_uiDebugConsole =
            (UIComponent_DebugUIConsole*)m_groups[eUIGroup_Fullscreen]
                ->addComponent(0, eUIComponent_DebugUIConsole, eUILayer_Debug);
    } else {
        m_groups[eUIGroup_Fullscreen]->removeComponent(
            eUIComponent_DebugUIConsole, eUILayer_Debug);
        m_uiDebugConsole = nullptr;
    }
#endif
}

void UIController::ShowUIDebugMarketingGuide(bool show) {
#if !defined(_CONTENT_PACKAGE)

    if (show) {
        m_uiDebugMarketingGuide =
            (UIComponent_DebugUIMarketingGuide*)m_groups[eUIGroup_Fullscreen]
                ->addComponent(0, eUIComponent_DebugUIMarketingGuide,
                               eUILayer_Debug);
    } else {
        m_groups[eUIGroup_Fullscreen]->removeComponent(
            eUIComponent_DebugUIMarketingGuide, eUILayer_Debug);
        m_uiDebugMarketingGuide = nullptr;
    }
#endif
}

void UIController::logDebugString(const std::string& text) {
    if (m_uiDebugConsole) m_uiDebugConsole->addText(text);
}

bool UIController::PressStartPlaying(unsigned int iPad) {
    return m_iPressStartQuadrantsMask & (1 << iPad) ? true : false;
}

void UIController::ShowPressStart(unsigned int iPad) {
    m_iPressStartQuadrantsMask |= 1 << iPad;
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showPressStart(iPad, true);
}

void UIController::HidePressStart() {
    ClearPressStart();
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showPressStart(0, false);
}

void UIController::ClearPressStart() { m_iPressStartQuadrantsMask = 0; }

IPlatformStorage::EMessageResult UIController::RequestAlertMessage(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int dwPad,
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
    void* lpParam, char* pwchFormatString) {
    return RequestMessageBox(uiTitle, uiText, uiOptionA, uiOptionC, dwPad, Func,
                             lpParam, pwchFormatString, 0, false);
}

IPlatformStorage::EMessageResult UIController::RequestErrorMessage(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int dwPad,
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
    void* lpParam, char* pwchFormatString) {
    return RequestMessageBox(uiTitle, uiText, uiOptionA, uiOptionC, dwPad, Func,
                             lpParam, pwchFormatString, 0, true);
}

IPlatformStorage::EMessageResult UIController::RequestMessageBox(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int dwPad,
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
    void* lpParam, char* pwchFormatString, unsigned int dwFocusButton,
    bool bIsError)

{
    MessageBoxInfo param;
    param.uiTitle = uiTitle;
    param.uiText = uiText;
    param.uiOptionA = uiOptionA;
    param.uiOptionC = uiOptionC;
    param.dwPad = dwPad;
    param.Func = Func;
    param.lpParam = lpParam;
    param.pwchFormatString = pwchFormatString;
    param.dwFocusButton = dwFocusButton;

    EUILayer layer = bIsError ? eUILayer_Error : eUILayer_Alert;

    bool completed = false;
    if (ui.IsReloadingSkin()) {
        // Queue this message box
        QueuedMessageBoxData* queuedData = new QueuedMessageBoxData();
        queuedData->info = param;
        queuedData->info.uiOptionA = new unsigned int[param.uiOptionC];
        memcpy(queuedData->info.uiOptionA, param.uiOptionA,
               param.uiOptionC * sizeof(unsigned int));
        queuedData->iPad = dwPad;
        queuedData->layer =
            eUILayer_Error;  // Ensures that these don't get wiped out by a
                             // CloseAllScenes call
        m_queuedMessageBoxData.push_back(queuedData);
    } else {
        completed = ui.NavigateToScene(dwPad, eUIScene_MessageBox, &param,
                                       layer, eUIGroup_Fullscreen);
    }

    if (completed) {
        // This may happen if we had to queue the message box, or there was
        // already a message box displaying and so the NavigateToScene returned
        // false;
        return IPlatformStorage::EMessage_Pending;
    } else {
        return IPlatformStorage::EMessage_Busy;
    }
}

IPlatformStorage::EMessageResult UIController::RequestUGCMessageBox(
    int title /* = -1 */, int message /* = -1 */, int iPad /* = -1*/,
    int (*Func)(void*, int,
                const IPlatformStorage::EMessageResult) /* = nullptr*/,
    void* lpParam /* = nullptr*/) {
    // Default title / messages
    if (title == -1) {
        title = IDS_FAILED_TO_CREATE_GAME_TITLE;
    }

    if (message == -1) {
        message = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE;
    }

    // Default pad to primary player
    if (iPad == -1) iPad = PlatformProfile.GetPrimaryPad();

    unsigned int uiIDA[1];
    uiIDA[0] = IDS_CONFIRM_OK;
    return ui.RequestAlertMessage(title, message, uiIDA, 1, iPad, Func,
                                  lpParam);
}

IPlatformStorage::EMessageResult
UIController::RequestContentRestrictedMessageBox(
    int title /* = -1 */, int message /* = -1 */, int iPad /* = -1*/,
    int (*Func)(void*, int,
                const IPlatformStorage::EMessageResult) /* = nullptr*/,
    void* lpParam /* = nullptr*/) {
    // Default title / messages
    if (title == -1) {
        title = IDS_FAILED_TO_CREATE_GAME_TITLE;
    }

    if (message == -1) {
#if defined(_WINDOWS64) || defined(__linux__)
        // IDS_CONTENT_RESTRICTION doesn't exist on XB1
        message = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE;
#else
        message = IDS_CONTENT_RESTRICTION;
#endif
    }

    // Default pad to primary player
    if (iPad == -1) iPad = PlatformProfile.GetPrimaryPad();

    unsigned int uiIDA[1];
    uiIDA[0] = IDS_CONFIRM_OK;
    return ui.RequestAlertMessage(title, message, uiIDA, 1, iPad, Func,
                                  lpParam);
}

void UIController::setFontCachingCalculationBuffer(int length) {
    /* 4J-JEV: As described in an email from Sean.
    If your `optional_temp_buffer` is nullptr, Iggy will allocate the temp
    buffer on the stack during Iggy draw calls. The size of the buffer it
    will allocate is 16 bytes times `max_chars` in 32-bit, and 24 bytes
    times `max_chars` in 64-bit. If the stack of the thread making the
    draw call is not large enough, Iggy will crash or otherwise behave
    incorrectly.
    */
#if defined(_WIN64) || defined(__linux__)
    static const int CHAR_SIZE = 24;
#else
    static const int CHAR_SIZE = 16;
#endif

    if (m_tempBuffer != nullptr) delete[] m_tempBuffer;
    if (length < 0) {
        if (m_defaultBuffer == nullptr)
            m_defaultBuffer = new char[CHAR_SIZE * 5000];
        IggySetFontCachingCalculationBuffer(5000, m_defaultBuffer,
                                            CHAR_SIZE * 5000);
    } else {
        m_tempBuffer = new char[CHAR_SIZE * length];
        IggySetFontCachingCalculationBuffer(length, m_tempBuffer,
                                            CHAR_SIZE * length);
    }
}

// Returns the first scene of given type if it exists, nullptr otherwise
UIScene* UIController::FindScene(EUIScene sceneType) {
    UIScene* pScene = nullptr;

    for (int i = 0; i < eUIGroup_COUNT; i++) {
        pScene = m_groups[i]->FindScene(sceneType);
        if (pScene != nullptr) return pScene;
    }

    return pScene;
}
