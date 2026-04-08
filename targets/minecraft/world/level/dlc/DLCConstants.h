#pragma once

// DLC type and parameter enums extracted from app/common/DLC/DLCManager
// so minecraft/ consumers can reference them without including the app
// header. DLCManager re-exports these as nested aliases for backward
// compatibility with app-side call sites.

namespace minecraft::dlc {

enum EDLCType {
    e_DLCType_Skin = 0,
    e_DLCType_Cape,
    e_DLCType_Texture,
    e_DLCType_UIData,
    e_DLCType_PackConfig,
    e_DLCType_TexturePack,
    e_DLCType_LocalisationData,
    e_DLCType_GameRules,
    e_DLCType_Audio,
    e_DLCType_ColourTable,
    e_DLCType_GameRulesHeader,

    e_DLCType_Max,
    e_DLCType_All,
};

enum EDLCParameterType {
    e_DLCParamType_Invalid = -1,

    e_DLCParamType_DisplayName = 0,
    e_DLCParamType_ThemeName,
    e_DLCParamType_Free,    // identify free skins
    e_DLCParamType_Credit,  // legal credits for DLC
    e_DLCParamType_Cape,
    e_DLCParamType_Box,
    e_DLCParamType_Anim,
    e_DLCParamType_PackId,
    e_DLCParamType_NetherParticleColour,
    e_DLCParamType_EnchantmentTextColour,
    e_DLCParamType_EnchantmentTextFocusColour,
    e_DLCParamType_DataPath,
    e_DLCParamType_PackVersion,

    e_DLCParamType_Max,
};

}  // namespace minecraft::dlc
