#include "minecraft/GameHostOptions.h"

namespace GameHostOptions {

unsigned int get(unsigned int settings, eGameHostOption option) {
    switch (option) {
        case eGameHostOption_FriendsOfFriends:
            return (settings & GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS);
        case eGameHostOption_Difficulty:
            return (settings & GAME_HOST_OPTION_BITMASK_DIFFICULTY);
        case eGameHostOption_Gamertags:
            return (settings & GAME_HOST_OPTION_BITMASK_GAMERTAGS);
        case eGameHostOption_GameType:
            return (settings & GAME_HOST_OPTION_BITMASK_GAMETYPE) >> 4;
        case eGameHostOption_All:
            return (settings & GAME_HOST_OPTION_BITMASK_ALL);
        case eGameHostOption_Tutorial:
            return ((settings & GAME_HOST_OPTION_BITMASK_GAMERTAGS) |
                    GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS |
                    GAME_HOST_OPTION_BITMASK_FIRESPREADS |
                    GAME_HOST_OPTION_BITMASK_TNT |
                    GAME_HOST_OPTION_BITMASK_PVP |
                    GAME_HOST_OPTION_BITMASK_STRUCTURES | 1);
        case eGameHostOption_LevelType:
            return (settings & GAME_HOST_OPTION_BITMASK_LEVELTYPE);
        case eGameHostOption_Structures:
            return (settings & GAME_HOST_OPTION_BITMASK_STRUCTURES);
        case eGameHostOption_BonusChest:
            return (settings & GAME_HOST_OPTION_BITMASK_BONUSCHEST);
        case eGameHostOption_HasBeenInCreative:
            return (settings & GAME_HOST_OPTION_BITMASK_BEENINCREATIVE);
        case eGameHostOption_PvP:
            return (settings & GAME_HOST_OPTION_BITMASK_PVP);
        case eGameHostOption_TrustPlayers:
            return (settings & GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS);
        case eGameHostOption_TNT:
            return (settings & GAME_HOST_OPTION_BITMASK_TNT);
        case eGameHostOption_FireSpreads:
            return (settings & GAME_HOST_OPTION_BITMASK_FIRESPREADS);
        case eGameHostOption_CheatsEnabled:
            return (settings & (GAME_HOST_OPTION_BITMASK_HOSTFLY |
                                GAME_HOST_OPTION_BITMASK_HOSTHUNGER |
                                GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE));
        case eGameHostOption_HostCanFly:
            return (settings & GAME_HOST_OPTION_BITMASK_HOSTFLY);
        case eGameHostOption_HostCanChangeHunger:
            return (settings & GAME_HOST_OPTION_BITMASK_HOSTHUNGER);
        case eGameHostOption_HostCanBeInvisible:
            return (settings & GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE);
        case eGameHostOption_BedrockFog:
            return (settings & GAME_HOST_OPTION_BITMASK_BEDROCKFOG);
        case eGameHostOption_DisableSaving:
            return (settings & GAME_HOST_OPTION_BITMASK_DISABLESAVE);
        case eGameHostOption_WasntSaveOwner:
            return (settings & GAME_HOST_OPTION_BITMASK_NOTOWNER);
        case eGameHostOption_WorldSize:
            return (settings & GAME_HOST_OPTION_BITMASK_WORLDSIZE) >>
                   GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT;
        case eGameHostOption_MobGriefing:
            return !(settings & GAME_HOST_OPTION_BITMASK_MOBGRIEFING);
        case eGameHostOption_KeepInventory:
            return (settings & GAME_HOST_OPTION_BITMASK_KEEPINVENTORY);
        case eGameHostOption_DoMobSpawning:
            return !(settings & GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING);
        case eGameHostOption_DoMobLoot:
            return !(settings & GAME_HOST_OPTION_BITMASK_DOMOBLOOT);
        case eGameHostOption_DoTileDrops:
            return !(settings & GAME_HOST_OPTION_BITMASK_DOTILEDROPS);
        case eGameHostOption_NaturalRegeneration:
            return !(settings & GAME_HOST_OPTION_BITMASK_NATURALREGEN);
        case eGameHostOption_DoDaylightCycle:
            return !(settings & GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE);
        default:
            return 0;
    }
}

void set(unsigned int& settings, eGameHostOption option, unsigned int value) {
    auto setBit = [&](unsigned int mask) {
        if (value != 0)
            settings |= mask;
        else
            settings &= ~mask;
    };
    auto setInvertedBit = [&](unsigned int mask) {
        if (value != 1)
            settings |= mask;
        else
            settings &= ~mask;
    };

    switch (option) {
        case eGameHostOption_FriendsOfFriends:
            setBit(GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS);
            break;
        case eGameHostOption_Difficulty:
            settings &= ~GAME_HOST_OPTION_BITMASK_DIFFICULTY;
            settings |= (GAME_HOST_OPTION_BITMASK_DIFFICULTY & value);
            break;
        case eGameHostOption_Gamertags:
            setBit(GAME_HOST_OPTION_BITMASK_GAMERTAGS);
            break;
        case eGameHostOption_GameType:
            settings &= ~GAME_HOST_OPTION_BITMASK_GAMETYPE;
            settings |= (GAME_HOST_OPTION_BITMASK_GAMETYPE & (value << 4));
            break;
        case eGameHostOption_LevelType:
            setBit(GAME_HOST_OPTION_BITMASK_LEVELTYPE);
            break;
        case eGameHostOption_Structures:
            setBit(GAME_HOST_OPTION_BITMASK_STRUCTURES);
            break;
        case eGameHostOption_BonusChest:
            setBit(GAME_HOST_OPTION_BITMASK_BONUSCHEST);
            break;
        case eGameHostOption_HasBeenInCreative:
            setBit(GAME_HOST_OPTION_BITMASK_BEENINCREATIVE);
            break;
        case eGameHostOption_PvP:
            setBit(GAME_HOST_OPTION_BITMASK_PVP);
            break;
        case eGameHostOption_TrustPlayers:
            setBit(GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS);
            break;
        case eGameHostOption_TNT:
            setBit(GAME_HOST_OPTION_BITMASK_TNT);
            break;
        case eGameHostOption_FireSpreads:
            setBit(GAME_HOST_OPTION_BITMASK_FIRESPREADS);
            break;
        case eGameHostOption_CheatsEnabled:
            if (value != 0) {
                settings |= GAME_HOST_OPTION_BITMASK_HOSTFLY;
                settings |= GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
                settings |= GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
            } else {
                settings &= ~GAME_HOST_OPTION_BITMASK_HOSTFLY;
                settings &= ~GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
                settings &= ~GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
            }
            break;
        case eGameHostOption_HostCanFly:
            setBit(GAME_HOST_OPTION_BITMASK_HOSTFLY);
            break;
        case eGameHostOption_HostCanChangeHunger:
            setBit(GAME_HOST_OPTION_BITMASK_HOSTHUNGER);
            break;
        case eGameHostOption_HostCanBeInvisible:
            setBit(GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE);
            break;
        case eGameHostOption_BedrockFog:
            setBit(GAME_HOST_OPTION_BITMASK_BEDROCKFOG);
            break;
        case eGameHostOption_DisableSaving:
            setBit(GAME_HOST_OPTION_BITMASK_DISABLESAVE);
            break;
        case eGameHostOption_WasntSaveOwner:
            setBit(GAME_HOST_OPTION_BITMASK_NOTOWNER);
            break;
        case eGameHostOption_MobGriefing:
            setInvertedBit(GAME_HOST_OPTION_BITMASK_MOBGRIEFING);
            break;
        case eGameHostOption_KeepInventory:
            setBit(GAME_HOST_OPTION_BITMASK_KEEPINVENTORY);
            break;
        case eGameHostOption_DoMobSpawning:
            setInvertedBit(GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING);
            break;
        case eGameHostOption_DoMobLoot:
            setInvertedBit(GAME_HOST_OPTION_BITMASK_DOMOBLOOT);
            break;
        case eGameHostOption_DoTileDrops:
            setInvertedBit(GAME_HOST_OPTION_BITMASK_DOTILEDROPS);
            break;
        case eGameHostOption_NaturalRegeneration:
            setInvertedBit(GAME_HOST_OPTION_BITMASK_NATURALREGEN);
            break;
        case eGameHostOption_DoDaylightCycle:
            setInvertedBit(GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE);
            break;
        case eGameHostOption_WorldSize:
            settings &= ~GAME_HOST_OPTION_BITMASK_WORLDSIZE;
            settings |=
                (GAME_HOST_OPTION_BITMASK_WORLDSIZE &
                 (value << GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT));
            break;
        case eGameHostOption_All:
            settings = value;
            break;
        default:
            break;
    }
}

}  // namespace GameHostOptions
