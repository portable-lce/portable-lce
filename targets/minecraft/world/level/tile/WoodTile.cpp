#include "WoodTile.h"

#include "minecraft/world/IconRegister.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/Tile.h"
#include "strings.h"

class Icon;

const unsigned int WoodTile::WOOD_NAMES[WOOD_NAMES_LENGTH] = {
    IDS_TILE_OAKWOOD_PLANKS,
    IDS_TILE_SPRUCEWOOD_PLANKS,
    IDS_TILE_BIRCHWOOD_PLANKS,
    IDS_TILE_JUNGLE_PLANKS,
};

const std::string WoodTile::TEXTURE_NAMES[] = {"oak", "spruce", "birch",
                                               "jungle"};

// 	public static final String[] WOOD_NAMES = {
// 		"oak", "spruce", "birch", "jungle"
// 	};

WoodTile::WoodTile(int id) : Tile(id, Material::wood) { icons = nullptr; }

unsigned int WoodTile::getDescriptionId(int iData) {
    if (iData < 0 || iData >= WOOD_NAMES_LENGTH) iData = 0;

    return WOOD_NAMES[iData];
}

Icon* WoodTile::getTexture(int face, int data) {
    if (data < 0 || data >= WOOD_NAMES_LENGTH) {
        data = 0;
    }
    return icons[data];
}

int WoodTile::getSpawnResourcesAuxValue(int data) { return data; }

void WoodTile::registerIcons(IconRegister* iconRegister) {
    icons = new Icon*[WOOD_NAMES_LENGTH];

    for (int i = 0; i < WOOD_NAMES_LENGTH; i++) {
        icons[i] =
            iconRegister->registerIcon(getIconName() + "_" + TEXTURE_NAMES[i]);
    }
}