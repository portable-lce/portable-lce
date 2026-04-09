#include "app/common/DebugOptions.h"

DebugOptions::DebugOptions() {
#if defined(_DEBUG_MENUS_ENABLED)
#if defined(_CONTENT_PACKAGE)
    m_bDebugOptions =
        false;  // make them off by default in a content package build
#else
    m_bDebugOptions = true;
#endif
#else
    m_bDebugOptions = false;
#endif

    m_bLoadSavesFromFolderEnabled = false;
    m_bWriteSavesToFolderEnabled = false;
    m_bMobsDontAttack = false;
    m_bMobsDontTick = false;
    m_bFreezePlayers = false;

#if defined(_CONTENT_PACAKGE)
    m_bUseDPadForDebug = false;
#else
    m_bUseDPadForDebug = true;
#endif
}

#if defined(_DEBUG_MENUS_ENABLED)
bool DebugOptions::debugArtToolsOn(unsigned int debugMask) {
    return settingsOn() && (debugMask & (1L << eDebugSetting_ArtTools)) != 0;
}
#endif
