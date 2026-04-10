#include "CauldronTile.h"

#include <memory>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/Facing.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ArmorItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/Tile.h"

class Icon;

const std::string CauldronTile::TEXTURE_INSIDE = "cauldron_inner";
const std::string CauldronTile::TEXTURE_BOTTOM = "cauldron_bottom";

CauldronTile::CauldronTile(int id) : Tile(id, Material::metal, false) {
    iconInner = nullptr;
    iconTop = nullptr;
    iconBottom = nullptr;
}

Icon* CauldronTile::getTexture(int face, int data) {
    if (face == Facing::UP) {
        return iconTop;
    }
    if (face == Facing::DOWN) {
        return iconBottom;
    }
    return icon;
}

void CauldronTile::registerIcons(IconRegister* iconRegister) {
    iconInner = iconRegister->registerIcon("cauldron_inner");
    iconTop = iconRegister->registerIcon("cauldron_top");
    iconBottom = iconRegister->registerIcon("cauldron_bottom");
    icon = iconRegister->registerIcon("cauldron_side");
}

Icon* CauldronTile::getTexture(const std::string& name) {
    if (name.compare(TEXTURE_INSIDE) == 0) return Tile::cauldron->iconInner;
    if (name.compare(TEXTURE_BOTTOM) == 0) return Tile::cauldron->iconBottom;
    return nullptr;
}

void CauldronTile::addAABBs(Level* level, int x, int y, int z, AABB* box,
                            std::vector<AABB>* boxes,
                            std::shared_ptr<Entity> source) {
    setShape(0, 0, 0, 1, 5.0f / 16.0f, 1);
    Tile::addAABBs(level, x, y, z, box, boxes, source);
    float thickness = 2.0f / 16.0f;
    setShape(0, 0, 0, thickness, 1, 1);
    Tile::addAABBs(level, x, y, z, box, boxes, source);
    setShape(0, 0, 0, 1, 1, thickness);
    Tile::addAABBs(level, x, y, z, box, boxes, source);
    setShape(1 - thickness, 0, 0, 1, 1, 1);
    Tile::addAABBs(level, x, y, z, box, boxes, source);
    setShape(0, 0, 1 - thickness, 1, 1, 1);
    Tile::addAABBs(level, x, y, z, box, boxes, source);

    updateDefaultShape();
}

void CauldronTile::updateDefaultShape() { setShape(0, 0, 0, 1, 1, 1); }

bool CauldronTile::isSolidRender(bool isServerLevel) { return false; }

int CauldronTile::getRenderShape() { return SHAPE_CAULDRON; }

bool CauldronTile::isCubeShaped() { return false; }

bool CauldronTile::use(Level* level, int x, int y, int z,
                       std::shared_ptr<Player> player, int clickedFace,
                       float clickX, float clickY, float clickZ,
                       bool soundOnly /*=false*/)  // 4J added soundOnly param
{
    if (soundOnly) return false;

    if (level->isClientSide) {
        return true;
    }

    std::shared_ptr<ItemInstance> item = player->inventory->getSelected();
    if (item == nullptr) {
        return true;
    }

    int currentData = level->getData(x, y, z);
    int fillLevel = getFillLevel(currentData);

    if (item->id == Item::bucket_water_Id) {
        if (fillLevel < 3) {
            if (!player->abilities.instabuild) {
                player->inventory->setItem(
                    player->inventory->selected,
                    std::shared_ptr<ItemInstance>(
                        new ItemInstance(Item::bucket_empty)));
            }

            level->setData(x, y, z, 3, Tile::UPDATE_CLIENTS);
            level->updateNeighbourForOutputSignal(x, y, z, id);
        }
        return true;
    } else if (item->id == Item::glassBottle_Id) {
        if (fillLevel > 0) {
            std::shared_ptr<ItemInstance> potion =
                std::shared_ptr<ItemInstance>(
                    new ItemInstance(Item::potion, 1, 0));
            if (!player->inventory->add(potion)) {
                level->addEntity(std::shared_ptr<ItemEntity>(
                    new ItemEntity(level, x + 0.5, y + 1.5, z + 0.5, potion)));
            }
            // 4J Stu - Brought forward change to update inventory when filling
            // bottles with water
            else if (player->instanceof(eTYPE_SERVERPLAYER)) {
                std::dynamic_pointer_cast<ServerPlayer>(player)
                    ->refreshContainer(player->inventoryMenu);
            }
            // 4J-PB - don't lose the water in creative mode
            if (player->abilities.instabuild == false) {
                item->count--;
                if (item->count <= 0) {
                    player->inventory->setItem(player->inventory->selected,
                                               nullptr);
                }
            }
            level->setData(x, y, z, fillLevel - 1, Tile::UPDATE_CLIENTS);
            level->updateNeighbourForOutputSignal(x, y, z, id);
        }
    } else if (fillLevel > 0) {
        ArmorItem* armor = dynamic_cast<ArmorItem*>(item->getItem());
        if (armor && armor->getMaterial() == ArmorItem::ArmorMaterial::CLOTH) {
            armor->clearColor(item);
            level->setData(x, y, z, fillLevel - 1, Tile::UPDATE_CLIENTS);
            level->updateNeighbourForOutputSignal(x, y, z, id);
            return true;
        }
    }

    return true;
}

void CauldronTile::handleRain(Level* level, int x, int y, int z) {
    if (level->random->nextInt(20) != 1) return;

    int data = level->getData(x, y, z);

    if (data < 3) {
        level->setData(x, y, z, data + 1, Tile::UPDATE_CLIENTS);
    }
}

int CauldronTile::getResource(int data, Random* random, int playerBonusLevel) {
    return Item::cauldron_Id;
}

int CauldronTile::cloneTileId(Level* level, int x, int y, int z) {
    return Item::cauldron_Id;
}

bool CauldronTile::hasAnalogOutputSignal() { return true; }

int CauldronTile::getAnalogOutputSignal(Level* level, int x, int y, int z,
                                        int dir) {
    int data = level->getData(x, y, z);

    return getFillLevel(data);
}

int CauldronTile::getFillLevel(int data) { return data; }