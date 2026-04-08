#pragma once

#include <cstdint>

#include "minecraft/GameEnums.h"

// Vocabulary types and small game constants shared across minecraft/.
// Anything that used to live in app/common/App_Defines.h and represents a
// game-side concept (asset name sizes, default control indices, etc.) lives
// here.

#define MAX_CAPENAME_SIZE 32
#define MAX_BANNERNAME_SIZE 32
#define MAX_TMSFILENAME_SIZE 40
#define MAX_TYPE_SIZE 32
#define MAX_EXTENSION_TYPES 3

// Default UI controller index for non-splitscreen menus.
#define DEFAULT_XUI_MENU_USER 0

// Default sound/music volume level (0-100).
#define DEFAULT_VOLUME_LEVEL 100

struct MOJANG_DATA {
    eXUID eXuid;
    char wchCape[MAX_CAPENAME_SIZE];
    char wchSkin[MAX_CAPENAME_SIZE];
};

struct FEATURE_DATA {
    int x, z;
    _eTerrainFeatureType eTerrainFeature;
};
