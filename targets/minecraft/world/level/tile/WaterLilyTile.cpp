#include "WaterLilyTile.h"

#include <memory>
#include <optional>

#include "java/Class.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/PlantTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"

class Random;

WaterlilyTile::WaterlilyTile(int id) : Bush(id) { this->updateDefaultShape(); }

// 4J Added override
void WaterlilyTile::updateDefaultShape() {
    float ss = 0.5f;
    float hh = 0.25f / 16.0f;
    setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, hh, 0.5f + ss);
}

int WaterlilyTile::getRenderShape() { return Tile::SHAPE_LILYPAD; }

void WaterlilyTile::addAABBs(Level* level, int x, int y, int z, AABB* box,
                             std::vector<AABB>* boxes,
                             std::shared_ptr<Entity> source) {
    if (source == nullptr || !source->instanceof (eTYPE_BOAT)) {
        Bush::addAABBs(level, x, y, z, box, boxes, source);
    }
}

std::optional<AABB> WaterlilyTile::getAABB(Level* level, int x, int y, int z) {
    ThreadStorage* tls = m_tlsShape;
    // 4J Stu - Added this so that the TLS shape is correct for this tile
    if (tls->tileId != this->id) updateDefaultShape();
    return AABB(x + tls->xx0, y + tls->yy0, z + tls->zz0, x + tls->xx1,
                y + tls->yy1, z + tls->zz1);
}

int WaterlilyTile::getColor() const {
    return Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_Tile_WaterLily);  // 0x208030
}

int WaterlilyTile::getColor(int auxData) {
    return Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_Tile_WaterLily);  // 0x208030
}

int WaterlilyTile::getColor(LevelSource* level, int x, int y, int z) {
    return Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_Tile_WaterLily);  // 0x208030
}

int WaterlilyTile::getColor(LevelSource* level, int x, int y, int z,
                            int data)  // 0x208030
{
    return getColor(level, x, y, z);
}

bool WaterlilyTile::mayPlaceOn(int tile) { return tile == Tile::calmWater_Id; }

bool WaterlilyTile::canSurvive(Level* level, int x, int y, int z) {
    if (y < 0 || y >= Level::maxBuildHeight) return false;
    return level->getMaterial(x, y - 1, z) == Material::water &&
           level->getData(x, y - 1, z) == 0;
}

bool WaterlilyTile::growTree(Level* level, int x, int y, int z,
                             Random* random) {
    return false;
}
