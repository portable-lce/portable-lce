#include "TallGrassPlantTile.h"

#include <memory>

#include "minecraft/GameEnums.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "java/Random.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/ShearsItem.h"
#include "minecraft/world/level/FoliageColor.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelSource.h"
#include "minecraft/world/level/biome/Biome.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/PlantTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "strings.h"

class Icon;

const unsigned int
    TallGrass::TALL_GRASS_TILE_NAMES[TALL_GRASS_TILE_NAMES_LENGTH] = {
        IDS_TILE_SHRUB,
        IDS_TILE_TALL_GRASS,
        IDS_TILE_FERN,
};

const std::string TallGrass::TEXTURE_NAMES[] = {"deadbush", "tallgrass",
                                                 "fern"};

TallGrass::TallGrass(int id) : Bush(id, Material::replaceable_plant) {
    this->updateDefaultShape();
}

// 4J Added override
void TallGrass::updateDefaultShape() {
    float ss = 0.4f;
    this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.8f, 0.5f + ss);
}

Icon* TallGrass::getTexture(int face, int data) {
    if (data >= TALL_GRASS_TILE_NAMES_LENGTH) data = 0;
    return icons[data];
}

int TallGrass::getColor(int auxData) {
    if (auxData == DEAD_SHRUB) return 0xffffff;

    return FoliageColor::getDefaultColor();
}

int TallGrass::getColor() const {
    // 4J Stu - Not using this any more
    // double temp = 0.5;
    // double rain = 1.0;

    // return GrassColor::get(temp, rain);

    return Minecraft::GetInstance()->getColourTable()->getColor(
        eMinecraftColour_Grass_Common);
}

int TallGrass::getColor(LevelSource* level, int x, int y, int z) {
    return getColor(level, x, y, z, level->getData(x, y, z));
}

// 4J - changed interface to have data passed in, and put existing interface as
// wrapper above
int TallGrass::getColor(LevelSource* level, int x, int y, int z, int data) {
    int d = data;
    if (d == DEAD_SHRUB) return 0xffffff;

    return level->getBiome(x, z)->getGrassColor();
}

int TallGrass::getResource(int data, Random* random, int playerBonusLevel) {
    if (random->nextInt(8) == 0) {
        return Item::seeds_wheat->id;
    }

    return -1;
}

int TallGrass::getResourceCountForLootBonus(int bonusLevel, Random* random) {
    return 1 + random->nextInt(bonusLevel * 2 + 1);
}

void TallGrass::playerDestroy(Level* level, std::shared_ptr<Player> player,
                              int x, int y, int z, int data) {
    if (!level->isClientSide && player->getSelectedItem() != nullptr &&
        player->getSelectedItem()->id == Item::shears->id) {
        player->awardStat(GenericStats::blocksMined(id),
                          GenericStats::param_blocksMined(id, data, 1));

        // drop leaf block instead of sapling
        popResource(level, x, y, z,
                    std::shared_ptr<ItemInstance>(
                        new ItemInstance(Tile::tallgrass, 1, data)));
    } else {
        Bush::playerDestroy(level, player, x, y, z, data);
    }
}

int TallGrass::cloneTileData(Level* level, int x, int y, int z) {
    return level->getData(x, y, z);
}

unsigned int TallGrass::getDescriptionId(int iData /*= -1*/) {
    if (iData < 0) iData = 0;
    return TallGrass::TALL_GRASS_TILE_NAMES[iData];
}

void TallGrass::registerIcons(IconRegister* iconRegister) {
    icons = new Icon*[TALL_GRASS_TILE_NAMES_LENGTH];

    for (int i = 0; i < TALL_GRASS_TILE_NAMES_LENGTH; i++) {
        icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
    }
}
