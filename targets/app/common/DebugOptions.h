#pragma once

#include "minecraft/Console_Debug_enum.h"

class DebugOptions {
public:
    DebugOptions();

    bool settingsOn() const { return m_bDebugOptions; }
    void setDebugOptions(bool bVal) { m_bDebugOptions = bVal; }

    bool getLoadSavesFromFolderEnabled() const {
        return m_bLoadSavesFromFolderEnabled;
    }
    void setLoadSavesFromFolderEnabled(bool bVal) {
        m_bLoadSavesFromFolderEnabled = bVal;
    }

    bool getWriteSavesToFolderEnabled() const {
        return m_bWriteSavesToFolderEnabled;
    }
    void setWriteSavesToFolderEnabled(bool bVal) {
        m_bWriteSavesToFolderEnabled = bVal;
    }

    bool getMobsDontAttack() const { return m_bMobsDontAttack; }
    void setMobsDontAttack(bool bVal) { m_bMobsDontAttack = bVal; }

    bool getUseDPadForDebug() const { return m_bUseDPadForDebug; }
    void setUseDPadForDebug(bool bVal) { m_bUseDPadForDebug = bVal; }

    bool getMobsDontTick() const { return m_bMobsDontTick; }
    void setMobsDontTick(bool bVal) { m_bMobsDontTick = bVal; }

    bool getFreezePlayers() const { return m_bFreezePlayers; }
    void setFreezePlayers(bool bVal) { m_bFreezePlayers = bVal; }

#if defined(_DEBUG_MENUS_ENABLED)
    bool debugArtToolsOn(unsigned int debugMask);
#else
    bool debugArtToolsOn(unsigned int) { return false; }
#endif

private:
    bool m_bDebugOptions;
    bool m_bLoadSavesFromFolderEnabled;
    bool m_bWriteSavesToFolderEnabled;
    bool m_bMobsDontAttack;
    bool m_bUseDPadForDebug;
    bool m_bMobsDontTick;
    bool m_bFreezePlayers;
};
