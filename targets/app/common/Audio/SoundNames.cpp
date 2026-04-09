#include "minecraft/sounds/ConsoleSoundEngine.h"
#include "minecraft/sounds/SoundTypes.h"

const char* ConsoleSoundEngine::wchSoundNames[eSoundType_MAX] = {
    "mob/chicken/chicken",        //	eSoundType_MOB_CHICKEN_AMBIENT
    "mob/chicken/chickenhurt",    //	eSoundType_MOB_CHICKEN_HURT
    "mob/chicken/chickenplop",    //	eSoundType_MOB_CHICKENPLOP
    "mob/cow/say",                //	eSoundType_MOB_COW_AMBIENT
    "mob/cow/hurt",               //	eSoundType_MOB_COW_HURT
    "mob/pig/pig",                //	eSoundType_MOB_PIG_AMBIENT
    "mob/pig/pigdeath",           //	eSoundType_MOB_PIG_DEATH
    "mob/sheep/sheep",            //	eSoundType_MOB_SHEEP_AMBIENT
    "mob/wolf/growl",             //	eSoundType_MOB_WOLF_GROWL
    "mob/wolf/whine",             //	eSoundType_MOB_WOLF_WHINE
    "mob/wolf/panting",           //	eSoundType_MOB_WOLF_PANTING
    "mob/wolf/bark",              //	eSoundType_MOB_WOLF_BARK
    "mob/wolf/hurt",              //	eSoundType_MOB_WOLF_HURT
    "mob/wolf/death",             //	eSoundType_MOB_WOLF_DEATH
    "mob/wolf/shake",             //	eSoundType_MOB_WOLF_SHAKE
    "mob/blaze/breathe",          //	eSoundType_MOB_BLAZE_BREATHE
    "mob/blaze/hit",              //	eSoundType_MOB_BLAZE_HURT
    "mob/blaze/death",            //	eSoundType_MOB_BLAZE_DEATH
    "mob/ghast/moan",             //	eSoundType_MOB_GHAST_MOAN
    "mob/ghast/scream",           //	eSoundType_MOB_GHAST_SCREAM
    "mob/ghast/death",            //	eSoundType_MOB_GHAST_DEATH
    "mob/ghast/fireball",         //	eSoundType_MOB_GHAST_FIREBALL
    "mob/ghast/charge",           //	eSoundType_MOB_GHAST_CHARGE
    "mob/endermen/idle",          //	eSoundType_MOB_ENDERMEN_IDLE
    "mob/endermen/hit",           //	eSoundType_MOB_ENDERMEN_HIT
    "mob/endermen/death",         //	eSoundType_MOB_ENDERMEN_DEATH
    "mob/endermen/portal",        //	eSoundType_MOB_ENDERMEN_PORTAL
    "mob/zombiepig/zpig",         //	eSoundType_MOB_ZOMBIEPIG_AMBIENT
    "mob/zombiepig/zpighurt",     //	eSoundType_MOB_ZOMBIEPIG_HURT
    "mob/zombiepig/zpigdeath",    //	eSoundType_MOB_ZOMBIEPIG_DEATH
    "mob/zombiepig/zpigangry",    //	eSoundType_MOB_ZOMBIEPIG_ZPIGANGRY
    "mob/silverfish/say",         //	eSoundType_MOB_SILVERFISH_AMBIENT,
    "mob/silverfish/hit",         //	eSoundType_MOB_SILVERFISH_HURT
    "mob/silverfish/kill",        //	eSoundType_MOB_SILVERFISH_DEATH,
    "mob/silverfish/step",        //	eSoundType_MOB_SILVERFISH_STEP,
    "mob/skeleton/skeleton",      //	eSoundType_MOB_SKELETON_AMBIENT,
    "mob/skeleton/skeletonhurt",  //	eSoundType_MOB_SKELETON_HURT,
    "mob/spider/spider",          //	eSoundType_MOB_SPIDER_AMBIENT,
    "mob/spider/spiderdeath",     //	eSoundType_MOB_SPIDER_DEATH,
    "mob/slime/slime",            //	eSoundType_MOB_SLIME,
    "mob/slime/slimeattack",      //	eSoundType_MOB_SLIME_ATTACK,
    "mob/creeper/creeper",        //	eSoundType_MOB_CREEPER_HURT,
    "mob/creeper/creeperdeath",   //	eSoundType_MOB_CREEPER_DEATH,
    "mob/zombie/zombie",          //	eSoundType_MOB_ZOMBIE_AMBIENT,
    "mob/zombie/zombiehurt",      //	eSoundType_MOB_ZOMBIE_HURT,
    "mob/zombie/zombiedeath",     //	eSoundType_MOB_ZOMBIE_DEATH,
    "mob/zombie/wood",            //	eSoundType_MOB_ZOMBIE_WOOD,
    "mob/zombie/woodbreak",       //	eSoundType_MOB_ZOMBIE_WOOD_BREAK,
    "mob/zombie/metal",           //	eSoundType_MOB_ZOMBIE_METAL,
    "mob/magmacube/big",          //	eSoundType_MOB_MAGMACUBE_BIG,
    "mob/magmacube/small",        //	eSoundType_MOB_MAGMACUBE_SMALL,
    "mob/cat/purr",               //  eSoundType_MOB_CAT_PURR
    "mob/cat/purreow",            //  eSoundType_MOB_CAT_PURREOW
    "mob/cat/meow",               //  eSoundType_MOB_CAT_MEOW
    // 4J-PB - correct the name of the event for hitting ocelots
    "mob/cat/hitt",  //  eSoundType_MOB_CAT_HITT
                     //	"mob.irongolem.throw", //
    // eSoundType_MOB_IRONGOLEM_THROW 	"mob.irongolem.hit",
    ////  eSoundType_MOB_IRONGOLEM_HIT 	"mob.irongolem.death",
    ////  eSoundType_MOB_IRONGOLEM_DEATH 	"mob.irongolem.walk",
    ////  eSoundType_MOB_IRONGOLEM_WALK
    "random/bow",               //	eSoundType_RANDOM_BOW,
    "random/bowhit",            //	eSoundType_RANDOM_BOW_HIT,
    "random/explode",           //	eSoundType_RANDOM_EXPLODE,
    "random/fizz",              //	eSoundType_RANDOM_FIZZ,
    "random/pop",               //	eSoundType_RANDOM_POP,
    "random/fuse",              //	eSoundType_RANDOM_FUSE,
    "random/drink",             //	eSoundType_RANDOM_DRINK,
    "random/eat",               //	eSoundType_RANDOM_EAT,
    "random/burp",              //	eSoundType_RANDOM_BURP,
    "random/splash",            //	eSoundType_RANDOM_SPLASH,
    "random/click",             //	eSoundType_RANDOM_CLICK,
    "random/glass",             //	eSoundType_RANDOM_GLASS,
    "random/orb",               //	eSoundType_RANDOM_ORB,
    "random/break",             //	eSoundType_RANDOM_BREAK,
    "random/chestopen",         //	eSoundType_RANDOM_CHEST_OPEN,
    "random/chestclosed",       //	eSoundType_RANDOM_CHEST_CLOSE,
    "random/door_open",         //	eSoundType_RANDOM_DOOR_OPEN,
    "random/door_close",        //	eSoundType_RANDOM_DOOR_CLOSE,
    "ambient/weather/rain",     //	eSoundType_AMBIENT_WEATHER_RAIN,
    "ambient/weather/thunder",  //	eSoundType_AMBIENT_WEATHER_THUNDER,
    "ambient/cave/cave",  //	eSoundType_CAVE_CAVE, DON'T USE FOR XBOX 360!!!
    "portal/portal",      //	eSoundType_PORTAL_PORTAL,
    // 4J-PB - added a couple that were still using std::string
    "portal/trigger",  //	eSoundType_PORTAL_TRIGGER
    "portal/travel",   //	eSoundType_PORTAL_TRAVEL

    "fire/ignite",       //	eSoundType_FIRE_IGNITE,
    "fire/fire",         //	eSoundType_FIRE_FIRE,
    "damage/hit",        //	eSoundType_DAMAGE_HURT,
    "damage/fallsmall",  //	eSoundType_DAMAGE_FALL_SMALL,
    "damage/fallbig",    //	eSoundType_DAMAGE_FALL_BIG,
    "note/harp",         //	eSoundType_NOTE_HARP,
    "note/bd",           //	eSoundType_NOTE_BD,
    "note/snare",        //	eSoundType_NOTE_SNARE,
    "note/hat",          //	eSoundType_NOTE_HAT,
    "note/bassattack",   //	eSoundType_NOTE_BASSATTACK,
    "tile/piston.in",    //	eSoundType_TILE_PISTON_IN,
    "tile/piston.out",   //	eSoundType_TILE_PISTON_OUT,
    "liquid/water",      //	eSoundType_LIQUID_WATER,
    "liquid/lavapop",    //	eSoundType_LIQUID_LAVA_POP,
    "liquid/lava",       //	eSoundType_LIQUID_LAVA,
    "step/stone",        //	eSoundType_STEP_STONE,
    "step/wood",         //	eSoundType_STEP_WOOD,
    "step/gravel",       //	eSoundType_STEP_GRAVEL,
    "step/grass",        //	eSoundType_STEP_GRASS,
    "step/metal",        //	eSoundType_STEP_METAL,
    "step/cloth",        //	eSoundType_STEP_CLOTH,
    "step/sand",         //	eSoundType_STEP_SAND,

    // below this are the additional sounds from the second soundbank
    "mob/enderdragon/end",    //	eSoundType_MOB_ENDERDRAGON_END
    "mob/enderdragon/growl",  //	eSoundType_MOB_ENDERDRAGON_GROWL
    "mob/enderdragon/hit",    //	eSoundType_MOB_ENDERDRAGON_HIT
    "mob/enderdragon/wings",  //	eSoundType_MOB_ENDERDRAGON_MOVE
    "mob/irongolem/throw",    //  eSoundType_MOB_IRONGOLEM_THROW
    "mob/irongolem/hit",      //  eSoundType_MOB_IRONGOLEM_HIT
    "mob/irongolem/death",    //  eSoundType_MOB_IRONGOLEM_DEATH
    "mob/irongolem/walk",     //  eSoundType_MOB_IRONGOLEM_WALK

    // TU14
    "damage/thorns",        //  eSoundType_DAMAGE_THORNS
    "random/anvil_break",   //  eSoundType_RANDOM_ANVIL_BREAK
    "random/anvil_land",    //  eSoundType_RANDOM_ANVIL_LAND
    "random/anvil_use",     //  eSoundType_RANDOM_ANVIL_USE
    "mob/villager/haggle",  //  eSoundType_MOB_VILLAGER_HAGGLE
    "mob/villager/idle",    //  eSoundType_MOB_VILLAGER_IDLE
    "mob/villager/hit",     //  eSoundType_MOB_VILLAGER_HIT
    "mob/villager/death",   //  eSoundType_MOB_VILLAGER_DEATH
    "mob/villager/yes",     //  eSoundType_MOB_VILLAGER_YES
    "mob/villager/no",      //  eSoundType_MOB_VILLAGER_NO
    "mob/zombie/infect",    //  eSoundType_MOB_ZOMBIE_INFECT
    "mob/zombie/unfect",    //  eSoundType_MOB_ZOMBIE_UNFECT
    "mob/zombie/remedy",    //  eSoundType_MOB_ZOMBIE_REMEDY
    "step/snow",            //  eSoundType_STEP_SNOW
    "step/ladder",          //  eSoundType_STEP_LADDER
    "dig/cloth",            //  eSoundType_DIG_CLOTH
    "dig/grass",            //  eSoundType_DIG_GRASS
    "dig/gravel",           //  eSoundType_DIG_GRAVEL
    "dig/sand",             //  eSoundType_DIG_SAND
    "dig/snow",             //  eSoundType_DIG_SNOW
    "dig/stone",            //  eSoundType_DIG_STONE
    "dig/wood",             //  eSoundType_DIG_WOOD

    // 1.6.4
    "fireworks/launch",           // eSoundType_FIREWORKS_LAUNCH,
    "fireworks/blast",            // eSoundType_FIREWORKS_BLAST,
    "fireworks/blast_far",        // eSoundType_FIREWORKS_BLAST_FAR,
    "fireworks/large_blast",      // eSoundType_FIREWORKS_LARGE_BLAST,
    "fireworks/large_blast_far",  // eSoundType_FIREWORKS_LARGE_BLAST_FAR,
    "fireworks/twinkle",          // eSoundType_FIREWORKS_TWINKLE,
    "fireworks/twinkle_far",      // eSoundType_FIREWORKS_TWINKLE_FAR,

    "mob/bat/idle",     // eSoundType_MOB_BAT_IDLE,
    "mob/bat/hurt",     // eSoundType_MOB_BAT_HURT,
    "mob/bat/death",    // eSoundType_MOB_BAT_DEATH,
    "mob/bat/takeoff",  // eSoundType_MOB_BAT_TAKEOFF,

    "mob/wither/spawn",  // eSoundType_MOB_WITHER_SPAWN,
    "mob/wither/idle",   // eSoundType_MOB_WITHER_IDLE,
    "mob/wither/hurt",   // eSoundType_MOB_WITHER_HURT,
    "mob/wither/death",  // eSoundType_MOB_WITHER_DEATH,
    "mob/wither/shoot",  // eSoundType_MOB_WITHER_SHOOT,

    "mob/cow/step",         // eSoundType_MOB_COW_STEP,
    "mob/chicken/step",     // eSoundType_MOB_CHICKEN_STEP,
    "mob/pig/step",         // eSoundType_MOB_PIG_STEP,
    "mob/enderman/stare",   // eSoundType_MOB_ENDERMAN_STARE,
    "mob/enderman/scream",  // eSoundType_MOB_ENDERMAN_SCREAM,
    "mob/sheep/shear",      // eSoundType_MOB_SHEEP_SHEAR,
    "mob/sheep/step",       // eSoundType_MOB_SHEEP_STEP,
    "mob/skeleton.death",   // eSoundType_MOB_SKELETON_DEATH,
    "mob/skeleton/step",    // eSoundType_MOB_SKELETON_STEP,
    "mob/spider/step",      // eSoundType_MOB_SPIDER_STEP,
    "mob/wolf/step",        // eSoundType_MOB_WOLF_STEP,
    "mob/zombie/step",      // eSoundType_MOB_ZOMBIE_STEP,

    "liquid/swim",  // eSoundType_LIQUID_SWIM,

    "mob/horse/land",            // eSoundType_MOB_HORSE_LAND,
    "mob/horse/armor",           // eSoundType_MOB_HORSE_ARMOR,
    "mob/horse/leather",         // eSoundType_MOB_HORSE_LEATHER,
    "mob/horse/zombie.death",    // eSoundType_MOB_HORSE_ZOMBIE_DEATH,
    "mob/horse/skeleton.death",  // eSoundType_MOB_HORSE_SKELETON_DEATH,
    "mob/horse/donkey.death",    // eSoundType_MOB_HORSE_DONKEY_DEATH,
    "mob/horse/death",           // eSoundType_MOB_HORSE_DEATH,
    "mob/horse/zombie.hit",      // eSoundType_MOB_HORSE_ZOMBIE_HIT,
    "mob/horse/skeleton.hit",    // eSoundType_MOB_HORSE_SKELETON_HIT,
    "mob/horse/donkey.hit",      // eSoundType_MOB_HORSE_DONKEY_HIT,
    "mob/horse/hit",             // eSoundType_MOB_HORSE_HIT,
    "mob/horse/zombie.idle",     // eSoundType_MOB_HORSE_ZOMBIE_IDLE,
    "mob/horse/skeleton.idle",   // eSoundType_MOB_HORSE_SKELETON_IDLE,
    "mob/horse/donkey.idle",     // eSoundType_MOB_HORSE_DONKEY_IDLE,
    "mob/horse/idle",            // eSoundType_MOB_HORSE_IDLE,
    "mob/horse/donkey.angry",    // eSoundType_MOB_HORSE_DONKEY_ANGRY,
    "mob/horse/angry",           // eSoundType_MOB_HORSE_ANGRY,
    "mob/horse/gallop",          // eSoundType_MOB_HORSE_GALLOP,
    "mob/horse/breathe",         // eSoundType_MOB_HORSE_BREATHE,
    "mob/horse/wood",            // eSoundType_MOB_HORSE_WOOD,
    "mob/horse/soft",            // eSoundType_MOB_HORSE_SOFT,
    "mob/horse/jump",            // eSoundType_MOB_HORSE_JUMP,

    "mob/witch/idle",   // eSoundType_MOB_WITCH_IDLE,			<---
                        // missing
    "mob/witch/hurt",   // eSoundType_MOB_WITCH_HURT,			<---
                        // missing
    "mob/witch/death",  // eSoundType_MOB_WITCH_DEATH,			<---
                        // missing

    "mob/slime/big",    // eSoundType_MOB_SLIME_BIG,
    "mob/slime/small",  // eSoundType_MOB_SLIME_SMALL,

    "eating",          // eSoundType_EATING <--- missing
    "random/levelup",  // eSoundType_RANDOM_LEVELUP

    // 4J-PB  - Some sounds were updated, but we can't do that for the 360 or we
    // have to do a new sound bank instead, we'll add the sounds as new ones and
    // change the code to reference them
    "fire/new_ignite",
};

const char* ConsoleSoundEngine::wchUISoundNames[eSFX_MAX] = {
    "back", "craft", "craftfail", "focus", "press", "scroll",
};
