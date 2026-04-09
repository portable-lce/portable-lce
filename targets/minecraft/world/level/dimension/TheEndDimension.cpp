#include "TheEndDimension.h"

#include <math.h>

#include <numbers>

#include "minecraft/GameEnums.h"
#include "minecraft/Pos.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/biome/FixedBiomeSource.h"
#include "minecraft/world/level/levelgen/TheEndLevelRandomLevelSource.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/Vec3.h"

void TheEndDimension::init() {
    biomeSource = new FixedBiomeSource(Biome::sky, 0.5f, 0);
    id = 1;
    hasCeiling = true;
}

ChunkSource* TheEndDimension::createRandomLevelSource() const {
    return new TheEndLevelRandomLevelSource(level, level->getSeed());
}

float TheEndDimension::getTimeOfDay(int64_t time, float a) const {
    return 0.0f;
}

float* TheEndDimension::getSunriseColor(float td, float a) { return nullptr; }

Vec3 TheEndDimension::getFogColor(float td, float a) const {
    int fogColor = Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_End_Fog_Colour);  // 0xa080a0;
    float br = cosf(td * std::numbers::pi * 2) * 2 + 0.5f;
    if (br < 0.0f) br = 0.0f;
    if (br > 1.0f) br = 1.0f;

    float r = ((fogColor >> 16) & 0xff) / 255.0f;
    float g = ((fogColor >> 8) & 0xff) / 255.0f;
    float b = ((fogColor) & 0xff) / 255.0f;
    r *= br * 0.0f + 0.15f;
    g *= br * 0.0f + 0.15f;
    b *= br * 0.0f + 0.15f;

    return Vec3(r, g, b);
}

bool TheEndDimension::hasGround() { return false; }

bool TheEndDimension::mayRespawn() const { return false; }

bool TheEndDimension::isNaturalDimension() { return false; }

float TheEndDimension::getCloudHeight() { return 8; }

bool TheEndDimension::isValidSpawn(int x, int z) const {
    int topTile = level->getTopTile(x, z);

    if (topTile == 0) return false;

    return Tile::tiles[topTile]->material->blocksMotion();
}

Pos* TheEndDimension::getSpawnPos() { return new Pos(100, 50, 0); }

bool TheEndDimension::isFoggyAt(int x, int z) { return true; }

int TheEndDimension::getSpawnYPosition() { return 50; }
