#include "minecraft/client/resources/Colours/ColourTable.h"

#include <cstring>
#include <utility>
#include <vector>

#include "minecraft/GameEnums.h"
#include "util/StringHelpers.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/DataInputStream.h"

std::unordered_map<std::string, eMinecraftColour>
    ColourTable::s_colourNamesMap;

const char* ColourTable::ColourTableElements[eMinecraftColour_COUNT] = {
    "NOTSET",

    "Foliage_Evergreen",
    "Foliage_Birch",
    "Foliage_Default",
    "Foliage_Common",
    "Foliage_Ocean",
    "Foliage_Plains",
    "Foliage_Desert",
    "Foliage_ExtremeHills",
    "Foliage_Forest",
    "Foliage_Taiga",
    "Foliage_Swampland",
    "Foliage_River",
    "Foliage_Hell",
    "Foliage_Sky",
    "Foliage_FrozenOcean",
    "Foliage_FrozenRiver",
    "Foliage_IcePlains",
    "Foliage_IceMountains",
    "Foliage_MushroomIsland",
    "Foliage_MushroomIslandShore",
    "Foliage_Beach",
    "Foliage_DesertHills",
    "Foliage_ForestHills",
    "Foliage_TaigaHills",
    "Foliage_ExtremeHillsEdge",
    "Foliage_Jungle",
    "Foliage_JungleHills",

    "Grass_Common",
    "Grass_Ocean",
    "Grass_Plains",
    "Grass_Desert",
    "Grass_ExtremeHills",
    "Grass_Forest",
    "Grass_Taiga",
    "Grass_Swampland",
    "Grass_River",
    "Grass_Hell",
    "Grass_Sky",
    "Grass_FrozenOcean",
    "Grass_FrozenRiver",
    "Grass_IcePlains",
    "Grass_IceMountains",
    "Grass_MushroomIsland",
    "Grass_MushroomIslandShore",
    "Grass_Beach",
    "Grass_DesertHills",
    "Grass_ForestHills",
    "Grass_TaigaHills",
    "Grass_ExtremeHillsEdge",
    "Grass_Jungle",
    "Grass_JungleHills",

    "Water_Ocean",
    "Water_Plains",
    "Water_Desert",
    "Water_ExtremeHills",
    "Water_Forest",
    "Water_Taiga",
    "Water_Swampland",
    "Water_River",
    "Water_Hell",
    "Water_Sky",
    "Water_FrozenOcean",
    "Water_FrozenRiver",
    "Water_IcePlains",
    "Water_IceMountains",
    "Water_MushroomIsland",
    "Water_MushroomIslandShore",
    "Water_Beach",
    "Water_DesertHills",
    "Water_ForestHills",
    "Water_TaigaHills",
    "Water_ExtremeHillsEdge",
    "Water_Jungle",
    "Water_JungleHills",

    "Sky_Ocean",
    "Sky_Plains",
    "Sky_Desert",
    "Sky_ExtremeHills",
    "Sky_Forest",
    "Sky_Taiga",
    "Sky_Swampland",
    "Sky_River",
    "Sky_Hell",
    "Sky_Sky",
    "Sky_FrozenOcean",
    "Sky_FrozenRiver",
    "Sky_IcePlains",
    "Sky_IceMountains",
    "Sky_MushroomIsland",
    "Sky_MushroomIslandShore",
    "Sky_Beach",
    "Sky_DesertHills",
    "Sky_ForestHills",
    "Sky_TaigaHills",
    "Sky_ExtremeHillsEdge",
    "Sky_Jungle",
    "Sky_JungleHills",

    "Tile_RedstoneDust",
    "Tile_RedstoneDustUnlit",
    "Tile_RedstoneDustLitMin",
    "Tile_RedstoneDustLitMax",
    "Tile_StemMin",
    "Tile_StemMax",
    "Tile_WaterLily",

    "Sky_Dawn_Dark",
    "Sky_Dawn_Bright",

    "Material_None",
    "Material_Grass",
    "Material_Sand",
    "Material_Cloth",
    "Material_Fire",
    "Material_Ice",
    "Material_Metal",
    "Material_Plant",
    "Material_Snow",
    "Material_Clay",
    "Material_Dirt",
    "Material_Stone",
    "Material_Water",
    "Material_Wood",
    "Material_Emerald",

    "Particle_Note_00",
    "Particle_Note_01",
    "Particle_Note_02",
    "Particle_Note_03",
    "Particle_Note_04",
    "Particle_Note_05",
    "Particle_Note_06",
    "Particle_Note_07",
    "Particle_Note_08",
    "Particle_Note_09",
    "Particle_Note_10",
    "Particle_Note_11",
    "Particle_Note_12",
    "Particle_Note_13",
    "Particle_Note_14",
    "Particle_Note_15",
    "Particle_Note_16",
    "Particle_Note_17",
    "Particle_Note_18",
    "Particle_Note_19",
    "Particle_Note_20",
    "Particle_Note_21",
    "Particle_Note_22",
    "Particle_Note_23",
    "Particle_Note_24",

    "Particle_NetherPortal",
    "Particle_EnderPortal",
    "Particle_Smoke",
    "Particle_Ender",

    "Particle_Explode",
    "Particle_HugeExplosion",

    "Particle_DripWater",
    "Particle_DripLavaStart",
    "Particle_DripLavaEnd",

    "Particle_EnchantmentTable",
    "Particle_DragonBreathMin",
    "Particle_DragonBreathMax",
    "Particle_Suspend",

    "Particle_CritStart",  // arrow in air
    "Particle_CritEnd",    // arrow in air

    "Effect_MovementSpeed",
    "Effect_MovementSlowDown",
    "Effect_DigSpeed",
    "Effect_DigSlowdown",
    "Effect_DamageBoost",
    "Effect_Heal",
    "Effect_Harm",
    "Effect_Jump",
    "Effect_Confusion",
    "Effect_Regeneration",
    "Effect_DamageResistance",
    "Effect_FireResistance",
    "Effect_WaterBreathing",
    "Effect_Invisiblity",
    "Effect_Blindness",
    "Effect_NightVision",
    "Effect_Hunger",
    "Effect_Weakness",
    "Effect_Poison",
    "Effect_Wither",
    "Effect_HealthBoost",
    "Effect_Absorption",
    "Effect_Saturation",

    "Potion_BaseColour",

    "Mob_Creeper_Colour1",
    "Mob_Creeper_Colour2",
    "Mob_Skeleton_Colour1",
    "Mob_Skeleton_Colour2",
    "Mob_Spider_Colour1",
    "Mob_Spider_Colour2",
    "Mob_Zombie_Colour1",
    "Mob_Zombie_Colour2",
    "Mob_Slime_Colour1",
    "Mob_Slime_Colour2",
    "Mob_Ghast_Colour1",
    "Mob_Ghast_Colour2",
    "Mob_PigZombie_Colour1",
    "Mob_PigZombie_Colour2",
    "Mob_Enderman_Colour1",
    "Mob_Enderman_Colour2",
    "Mob_CaveSpider_Colour1",
    "Mob_CaveSpider_Colour2",
    "Mob_Silverfish_Colour1",
    "Mob_Silverfish_Colour2",
    "Mob_Blaze_Colour1",
    "Mob_Blaze_Colour2",
    "Mob_LavaSlime_Colour1",
    "Mob_LavaSlime_Colour2",
    "Mob_Pig_Colour1",
    "Mob_Pig_Colour2",
    "Mob_Sheep_Colour1",
    "Mob_Sheep_Colour2",
    "Mob_Cow_Colour1",
    "Mob_Cow_Colour2",
    "Mob_Chicken_Colour1",
    "Mob_Chicken_Colour2",
    "Mob_Squid_Colour1",
    "Mob_Squid_Colour2",
    "Mob_Wolf_Colour1",
    "Mob_Wolf_Colour2",
    "Mob_MushroomCow_Colour1",
    "Mob_MushroomCow_Colour2",
    "Mob_Ocelot_Colour1",
    "Mob_Ocelot_Colour2",
    "Mob_Villager_Colour1",
    "Mob_Villager_Colour2",
    "Mob_Bat_Colour1",
    "Mob_Bat_Colour2",
    "Mob_Witch_Colour1",
    "Mob_Witch_Colour2",
    "Mob_Horse_Colour1",
    "Mob_Horse_Colour2",

    "Armour_Default_Leather_Colour",
    "Under_Water_Clear_Colour",
    "Under_Lava_Clear_Colour",
    "In_Cloud_Base_Colour",

    "Under_Water_Fog_Colour",
    "Under_Lava_Fog_Colour",
    "In_Cloud_Fog_Colour",

    "Default_Fog_Colour",
    "Nether_Fog_Colour",
    "End_Fog_Colour",

    "Sign_Text",
    "Map_Text",

    "Leash_Light_Colour",
    "Leash_Dark_Colour",

    "Fire_Overlay",

    "HTMLColor_0",
    "HTMLColor_1",
    "HTMLColor_2",
    "HTMLColor_3",
    "HTMLColor_4",
    "HTMLColor_5",
    "HTMLColor_6",
    "HTMLColor_7",
    "HTMLColor_8",
    "HTMLColor_9",
    "HTMLColor_a",
    "HTMLColor_b",
    "HTMLColor_c",
    "HTMLColor_d",
    "HTMLColor_e",
    "HTMLColor_f",
    "HTMLColor_dark_0",
    "HTMLColor_dark_1",
    "HTMLColor_dark_2",
    "HTMLColor_dark_3",
    "HTMLColor_dark_4",
    "HTMLColor_dark_5",
    "HTMLColor_dark_6",
    "HTMLColor_dark_7",
    "HTMLColor_dark_8",
    "HTMLColor_dark_9",
    "HTMLColor_dark_a",
    "HTMLColor_dark_b",
    "HTMLColor_dark_c",
    "HTMLColor_dark_d",
    "HTMLColor_dark_e",
    "HTMLColor_dark_f",
    "HTMLColor_T1",
    "HTMLColor_T2",
    "HTMLColor_T3",
    "HTMLColor_Black",
    "HTMLColor_White",
    "Color_EnchantText",
    "Color_EnchantTextFocus",
    "Color_EnchantTextDisabled",
    "Color_RenamedItemTitle",
};

void ColourTable::staticCtor() {
    for (unsigned int i = eMinecraftColour_NOT_SET; i < eMinecraftColour_COUNT;
         ++i) {
        s_colourNamesMap.insert(
            std::unordered_map<std::string, eMinecraftColour>::value_type(
                ColourTableElements[i], (eMinecraftColour)i));
    }
}

ColourTable::ColourTable(std::uint8_t* pbData, std::uint32_t dataLength) {
    loadColoursFromData(pbData, dataLength);
}

ColourTable::ColourTable(ColourTable* defaultColours, std::uint8_t* pbData,
                         std::uint32_t dataLength) {
    // 4J Stu - Default the colours that of the table passed in
    memcpy((void*)m_colourValues, (void*)defaultColours->m_colourValues,
           sizeof(int) * eMinecraftColour_COUNT);
    loadColoursFromData(pbData, dataLength);
}
void ColourTable::loadColoursFromData(std::uint8_t* pbData,
                                      std::uint32_t dataLength) {
    std::vector<uint8_t> src(pbData, pbData + dataLength);

    ByteArrayInputStream bais(src);
    DataInputStream dis(&bais);

    int versionNumber = dis.readInt();
    int coloursCount = dis.readInt();

    for (int i = 0; i < coloursCount; ++i) {
        std::string colourId = dis.readUTF();
        int colourValue = dis.readInt();
        setColour(colourId, colourValue);
        auto it = s_colourNamesMap.find(colourId);
    }

    bais.reset();
}

void ColourTable::setColour(const std::string& colourName, int value) {
    auto it = s_colourNamesMap.find(colourName);
    if (it != s_colourNamesMap.end()) {
        m_colourValues[(int)it->second] = value;
    }
}

void ColourTable::setColour(const std::string& colourName,
                            const std::string& value) {
    setColour(colourName, fromHexWString<int>(value));
}

unsigned int ColourTable::getColour(eMinecraftColour id) {
    return m_colourValues[(int)id];
}
