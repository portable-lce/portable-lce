#include "FurnaceTile.h"

#include <stdio.h>

#include <memory>
#include <string>

#include "java/Random.h"
#include "minecraft/Facing.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/Container.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/inventory/AbstractContainerMenu.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/BaseEntityTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/FurnaceTileEntity.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "nbt/CompoundTag.h"

bool FurnaceTile::noDrop = false;

FurnaceTile::FurnaceTile(int id, bool lit)
    : BaseEntityTile(id, Material::stone) {
    random = new Random();
    this->lit = lit;

    iconTop = nullptr;
    iconFront = nullptr;
}

int FurnaceTile::getResource(int data, Random* random, int playerBonusLevel) {
    return Tile::furnace_Id;
}

void FurnaceTile::onPlace(Level* level, int x, int y, int z) {
    BaseEntityTile::onPlace(level, x, y, z);
    recalcLockDir(level, x, y, z);
}

void FurnaceTile::recalcLockDir(Level* level, int x, int y, int z) {
    if (level->isClientSide) {
        return;
    }

    int n = level->getTile(x, y, z - 1);  // face = 2
    int s = level->getTile(x, y, z + 1);  // face = 3
    int w = level->getTile(x - 1, y, z);  // face = 4
    int e = level->getTile(x + 1, y, z);  // face = 5

    int lockDir = 3;
    if (Tile::solid[n] && !Tile::solid[s]) lockDir = 3;
    if (Tile::solid[s] && !Tile::solid[n]) lockDir = 2;
    if (Tile::solid[w] && !Tile::solid[e]) lockDir = 5;
    if (Tile::solid[e] && !Tile::solid[w]) lockDir = 4;
    level->setData(x, y, z, lockDir, Tile::UPDATE_CLIENTS);
}

Icon* FurnaceTile::getTexture(int face, int data) {
    if (face == Facing::UP) return iconTop;
    if (face == Facing::DOWN) return iconTop;

    if (face != data) return icon;
    return iconFront;
}

void FurnaceTile::registerIcons(IconRegister* iconRegister) {
    icon = iconRegister->registerIcon("furnace_side");
    iconFront =
        iconRegister->registerIcon(lit ? "furnace_front_lit" : "furnace_front");
    iconTop = iconRegister->registerIcon("furnace_top");
}

void FurnaceTile::animateTick(Level* level, int xt, int yt, int zt,
                              Random* random) {
    if (!lit) return;

    int dir = level->getData(xt, yt, zt);

    float x = xt + 0.5f;
    float y = yt + 0.0f + random->nextFloat() * 6 / 16.0f;
    float z = zt + 0.5f;
    float r = 0.52f;
    float ss = random->nextFloat() * 0.6f - 0.3f;

    if (dir == 4) {
        level->addParticle(eParticleType_smoke, x - r, y, z + ss, 0, 0, 0);
        level->addParticle(eParticleType_flame, x - r, y, z + ss, 0, 0, 0);
    } else if (dir == 5) {
        level->addParticle(eParticleType_smoke, x + r, y, z + ss, 0, 0, 0);
        level->addParticle(eParticleType_flame, x + r, y, z + ss, 0, 0, 0);
    } else if (dir == 2) {
        level->addParticle(eParticleType_smoke, x + ss, y, z - r, 0, 0, 0);
        level->addParticle(eParticleType_flame, x + ss, y, z - r, 0, 0, 0);
    } else if (dir == 3) {
        level->addParticle(eParticleType_smoke, x + ss, y, z + r, 0, 0, 0);
        level->addParticle(eParticleType_flame, x + ss, y, z + r, 0, 0, 0);
    }
}

// 4J-PB - Adding a TestUse for tooltip display
bool FurnaceTile::TestUse() { return true; }

bool FurnaceTile::use(Level* level, int x, int y, int z,
                      std::shared_ptr<Player> player, int clickedFace,
                      float clickX, float clickY, float clickZ,
                      bool soundOnly /*=false*/)  // 4J added soundOnly param
{
    if (soundOnly) return false;

    if (level->isClientSide) {
        return true;
    }
    std::shared_ptr<FurnaceTileEntity> furnace =
        std::dynamic_pointer_cast<FurnaceTileEntity>(
            level->getTileEntity(x, y, z));
    if (furnace != nullptr) player->openFurnace(furnace);
    return true;
}

void FurnaceTile::setLit(bool lit, Level* level, int x, int y, int z) {
    int data = level->getData(x, y, z);
    std::shared_ptr<TileEntity> te = level->getTileEntity(x, y, z);

    noDrop = true;
    if (lit)
        level->setTileAndUpdate(x, y, z, Tile::furnace_lit_Id);
    else
        level->setTileAndUpdate(x, y, z, Tile::furnace_Id);
    noDrop = false;

    level->setData(x, y, z, data, Tile::UPDATE_CLIENTS);
    if (te != nullptr) {
        te->clearRemoved();
        level->setTileEntity(x, y, z, te);
    }
}

std::shared_ptr<TileEntity> FurnaceTile::newTileEntity(Level* level) {
    return std::make_shared<FurnaceTileEntity>();
}

void FurnaceTile::setPlacedBy(Level* level, int x, int y, int z,
                              std::shared_ptr<LivingEntity> by,
                              std::shared_ptr<ItemInstance> itemInstance) {
    int dir = (Mth::floor(by->yRot * 4 / (360) + 0.5)) & 3;

    if (dir == 0) level->setData(x, y, z, Facing::NORTH, Tile::UPDATE_CLIENTS);
    if (dir == 1) level->setData(x, y, z, Facing::EAST, Tile::UPDATE_CLIENTS);
    if (dir == 2) level->setData(x, y, z, Facing::SOUTH, Tile::UPDATE_CLIENTS);
    if (dir == 3) level->setData(x, y, z, Facing::WEST, Tile::UPDATE_CLIENTS);

    if (itemInstance->hasCustomHoverName()) {
        std::dynamic_pointer_cast<FurnaceTileEntity>(
            level->getTileEntity(x, y, z))
            ->setCustomName(itemInstance->getHoverName());
    }
}

void FurnaceTile::onRemove(Level* level, int x, int y, int z, int id,
                           int data) {
    if (!noDrop) {
        std::shared_ptr<Container> container =
            std::dynamic_pointer_cast<FurnaceTileEntity>(
                level->getTileEntity(x, y, z));
        if (container != nullptr) {
            for (unsigned int i = 0; i < container->getContainerSize(); i++) {
                std::shared_ptr<ItemInstance> item = container->getItem(i);
                if (item != nullptr) {
                    float xo = random->nextFloat() * 0.8f + 0.1f;
                    float yo = random->nextFloat() * 0.8f + 0.1f;
                    float zo = random->nextFloat() * 0.8f + 0.1f;

                    while (item->count > 0) {
                        int count = random->nextInt(21) + 10;
                        if (count > item->count) count = item->count;
                        item->count -= count;

#ifndef _CONTENT_PACKAGE
                        if (level->isClientSide) {
                            printf("Client furnace dropping %d of %d/%d\n",
                                   count, item->id, item->getAuxValue());
                        } else {
                            printf("Server furnace dropping %d of %d/%d\n",
                                   count, item->id, item->getAuxValue());
                        }
#endif

                        std::shared_ptr<ItemInstance> newItem =
                            std::make_shared<ItemInstance>(item->id, count,
                                                           item->getAuxValue());
                        newItem->set4JData(item->get4JData());
                        std::shared_ptr<ItemEntity> itemEntity =
                            std::make_shared<ItemEntity>(level, x + xo, y + yo,
                                                         z + zo, newItem);
                        float pow = 0.05f;
                        itemEntity->xd = (float)random->nextGaussian() * pow;
                        itemEntity->yd =
                            (float)random->nextGaussian() * pow + 0.2f;
                        itemEntity->zd = (float)random->nextGaussian() * pow;
                        if (item->hasTag()) {
                            itemEntity->getItem()->setTag(
                                (CompoundTag*)item->getTag()->copy());
                        }
                        level->addEntity(itemEntity);
                    }

                    // 4J Stu - Fix for duplication glitch
                    container->setItem(i, nullptr);
                }
            }
            level->updateNeighbourForOutputSignal(x, y, z, id);
        }
    }
    BaseEntityTile::onRemove(level, x, y, z, id, data);
}

bool FurnaceTile::hasAnalogOutputSignal() { return true; }

int FurnaceTile::getAnalogOutputSignal(Level* level, int x, int y, int z,
                                       int dir) {
    return AbstractContainerMenu::getRedstoneSignalFromContainer(
        std::dynamic_pointer_cast<Container>(level->getTileEntity(x, y, z)));
}

int FurnaceTile::cloneTileId(Level* level, int x, int y, int z) {
    return Tile::furnace_Id;
}