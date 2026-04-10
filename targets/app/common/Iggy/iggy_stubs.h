#ifndef IGGYSTUBS_H
#define IGGYSTUBS_H

#pragma once

#include <cstdio>
#include <cstring>

#include "app/common/Iggy/include/iggy.h"

#define STUBBED \
    {           \
    }

RADEXPFUNC inline IggyValuePath* RADEXPLINK IggyPlayerRootPath(Iggy* f) {
    STUBBED;
    return nullptr;
}

RADEXPFUNC inline IggyResult RADEXPLINK
IggyPlayerCallMethodRS(Iggy* f, IggyDataValue* result, IggyValuePath* target,
                       IggyName methodname, S32 numargs, IggyDataValue* args) {
    STUBBED;
    return IGGY_RESULT_SUCCESS;
}

RADEXPFUNC inline void RADEXPLINK IggyPlayerDestroy(Iggy* player) { STUBBED; }
RADEXPFUNC inline void RADEXPLINK IggyPlayerSetDisplaySize(Iggy* f, S32 w,
                                                           S32 h) {
    STUBBED;
}

RADEXPFUNC inline void RADEXPLINK IggyPlayerDrawTilesStart(Iggy* f) { STUBBED; }

RADEXPFUNC inline void RADEXPLINK IggyPlayerDrawTile(Iggy* f, S32 x0, S32 y0,
                                                     S32 x1, S32 y1,
                                                     S32 padding) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggyPlayerDrawTilesEnd(Iggy* f) { STUBBED; }

// Each fake Iggy player gets its own state block
struct FakeIggyPlayer {
    int tickCount;
    bool needsTick;
    IggyProperties props;
    void* userdata;
};

// Simple player pool
static FakeIggyPlayer s_fakePlayers[64];
static int s_fakePlayerCount = 0;

RADEXPFUNC inline Iggy* RADEXPLINK IggyPlayerCreateFromMemory(
    void const* data, U32 data_size_in_bytes, IggyPlayerConfig* config) {
    if (s_fakePlayerCount >= 64) return nullptr;
    FakeIggyPlayer* fp = &s_fakePlayers[s_fakePlayerCount++];
    fp->tickCount = 0;
    fp->needsTick = true;
    fp->userdata = nullptr;
    // Default to 1920x1080 at 30fps
    memset(&fp->props, 0, sizeof(fp->props));
    fp->props.movie_width_in_pixels = 1920;
    fp->props.movie_height_in_pixels = 1080;
    fp->props.movie_frame_rate_from_file_in_fps = 30.0f;
    fp->props.movie_frame_rate_current_in_fps = 30.0f;
    fprintf(stderr, "[Iggy Stub] Created fake player %d (data=%p, size=%u)\n",
            s_fakePlayerCount - 1, data, data_size_in_bytes);
    return (Iggy*)fp;
}

static FakeIggyPlayer* getFakePlayer(Iggy* player) {
    return (FakeIggyPlayer*)player;
}

RADEXPFUNC inline void RADEXPLINK IggyPlayerInitializeAndTickRS(Iggy* player) {
    FakeIggyPlayer* fp = getFakePlayer(player);
    if (fp) {
        fp->tickCount = 0;
        fp->needsTick = true;
    }
}

RADEXPFUNC inline IggyProperties* RADEXPLINK
IggyPlayerProperties(Iggy* player) {
    FakeIggyPlayer* fp = getFakePlayer(player);
    if (fp) return &fp->props;
    static IggyProperties defaultProps = {};
    return &defaultProps;
}
RADEXPFUNC inline void RADEXPLINK IggyPlayerSetUserdata(Iggy* player,
                                                        void* userdata) {
    FakeIggyPlayer* fp = getFakePlayer(player);
    if (fp) fp->userdata = userdata;
}
RADEXPFUNC inline IggyName RADEXPLINK
IggyPlayerCreateFastName(Iggy* f, IggyUTF16 const* name, S32 len) {
    STUBBED;
    return 0;
}
RADEXPFUNC inline rrbool RADEXPLINK IggyDebugGetMemoryUseInfo(
    Iggy* player, IggyLibrary lib, char const* category_string,
    S32 category_stringlen, S32 iteration, IggyMemoryUseInfo* data) {
    STUBBED;
    return false;
}
RADEXPFUNC inline rrbool RADEXPLINK IggyPlayerReadyToTick(Iggy* player) {
    FakeIggyPlayer* fp = getFakePlayer(player);
    if (fp && fp->needsTick) return true;
    return false;
}
RADEXPFUNC inline void RADEXPLINK IggyPlayerTickRS(Iggy* player) {
    FakeIggyPlayer* fp = getFakePlayer(player);
    if (fp) {
        fp->tickCount++;
        // Allow one tick per frame cycle
        fp->needsTick = false;
    }
}
RADEXPFUNC inline void RADEXPLINK IggyPlayerDraw(Iggy* f) {
    // Re-arm tick for next frame
    FakeIggyPlayer* fp = getFakePlayer(f);
    if (fp) fp->needsTick = true;
}
RADEXPFUNC inline void RADEXPLINK IggyMakeEventKey(IggyEvent* event,
                                                   IggyKeyevent event_type,
                                                   IggyKeycode keycode,
                                                   IggyKeyloc keyloc) {
    STUBBED;
}
RADEXPFUNC inline rrbool RADEXPLINK IggyPlayerDispatchEventRS(
    Iggy* player, IggyEvent* event, IggyEventResult* result) {
    STUBBED;
    return false;
}
RADEXPFUNC inline void RADEXPLINK IggyFontRemoveUTF8(const char* fontname,
                                                     S32 namelen_in_bytes,
                                                     U32 fontflags) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggyFontInstallBitmapUTF8(
    const IggyBitmapFontProvider* bmf, const char* fontname,
    S32 namelen_in_bytes, U32 fontflags) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggyFontSetIndirectUTF8(
    const char* request_name, S32 request_namelen, U32 request_flags,
    const char* result_name, S32 result_namelen, U32 result_flags) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggyFontInstallTruetypeUTF8(
    const void* truetype_storage, S32 ttc_index, const char* fontname,
    S32 namelen_in_bytes, U32 fontflags) {
    STUBBED;
}
RADEXPFUNC inline rrbool RADEXPLINK IggyValuePathMakeNameRef(
    IggyValuePath* result, IggyValuePath* parent, char const* text_utf8) {
    STUBBED;
    return false;
}
RADEXPFUNC inline IggyResult RADEXPLINK
IggyValueGetBooleanRS(IggyValuePath* var, IggyName sub_name,
                      char const* sub_name_utf8, rrbool* result) {
    STUBBED;
    return IGGY_RESULT_SUCCESS;
}
RADEXPFUNC inline void RADEXPLINK IggyFontInstallTruetypeFallbackCodepointUTF8(
    const char* fontname, S32 len, U32 fontflags, S32 fallback_codepoint) {
    STUBBED;
}
RADEXPFUNC inline IggyResult RADEXPLINK
IggyValueGetF64RS(IggyValuePath* var, IggyName sub_name,
                  char const* sub_name_utf8, F64* result) {
    STUBBED;
    return IGGY_RESULT_SUCCESS;
}
RADEXPFUNC inline rrbool RADEXPLINK
IggyValueSetBooleanRS(IggyValuePath* var, IggyName sub_name,
                      char const* sub_name_utf8, rrbool value) {
    STUBBED;
    return true;
}
RADEXPFUNC inline void RADEXPLINK IggyInit(IggyAllocator* allocator) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK
IggySetWarningCallback(Iggy_WarningFunction* error, void* user_callback_data) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggySetTraceCallbackUTF8(
    Iggy_TraceFunctionUTF8* trace_utf8, void* user_callback_data) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK
IggySetFontCachingCalculationBuffer(S32 max_chars, void* optional_temp_buffer,
                                    S32 optional_temp_buffer_size_in_bytes) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggySetCustomDrawCallback(
    Iggy_CustomDrawCallback* custom_draw, void* user_callback_data) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggySetAS3ExternalFunctionCallbackUTF16(
    Iggy_AS3ExternalFunctionUTF16* as3_external_function_utf16,
    void* user_callback_data) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggyMakeEventMouseMove(IggyEvent* event,
                                                         S32 x, S32 y) {
    STUBBED;
}
RADEXPFUNC inline void RADEXPLINK IggySetTextureSubstitutionCallbacks(
    Iggy_TextureSubstitutionCreateCallback* texture_create,
    Iggy_TextureSubstitutionDestroyCallback* texture_destroy,
    void* user_callback_data) {
    STUBBED;
}
RADEXPFUNC inline void* RADEXPLINK IggyPlayerGetUserdata(Iggy* player) {
    FakeIggyPlayer* fp = getFakePlayer(player);
    if (fp) return fp->userdata;
    return 0;
}
RADEXPFUNC inline IggyLibrary RADEXPLINK IggyLibraryCreateFromMemoryUTF16(
    IggyUTF16 const* url_utf16_null_terminated, void const* data,
    U32 data_size_in_bytes, IggyPlayerConfig* config) {
    STUBBED;
    return 0;
}
RADEXPFUNC inline void RADEXPLINK IggyLibraryDestroy(IggyLibrary lib) {
    STUBBED;
}

// Iggy is fake
static GDrawFunctions* s_iggy_gdraw_funcs = 0;
RADEXPFUNC inline void RADEXPLINK IggySetGDraw(GDrawFunctions* gdraw_funcs) {
    s_iggy_gdraw_funcs = gdraw_funcs;
}

// Audio stubs
RADEXPFUNC inline void RADEXPLINK IggyAudioUseDefault(void) { STUBBED; }

// Explorer/Perfmon, shit implmentation
RADEXPFUNC inline void* RADEXPLINK IggyExpCreate(const char* host, int port,
                                                 void* storage,
                                                 int storage_size) {
    STUBBED;
    return 0;
}
RADEXPFUNC inline void RADEXPLINK IggyUseExplorer(Iggy* player,
                                                  void* explorer) {
    STUBBED;
}
RADEXPFUNC inline void* RADEXPLINK IggyPerfmonCreate(
    void* (*alloc_func)(unsigned long), void (*free_func)(void*), void* user) {
    STUBBED;
    return 0;
}
RADEXPFUNC inline void RADEXPLINK IggyInstallPerfmon(void* perfmon) { STUBBED; }

RADEXPFUNC inline IggyResult RADEXPLINK
IggyValueGetTypeRS(IggyValuePath* var, IggyName sub_name,
                   char const* sub_name_utf8, IggyDatatype* result) {
    STUBBED;
    return IGGY_RESULT_SUCCESS;
}

// GDraw memory/warning functions are defined in gdraw_glfw.c (C linkage)
// Juicey you stupid idiot do NOT define them here

#endif  // IGGYSTUBS_H