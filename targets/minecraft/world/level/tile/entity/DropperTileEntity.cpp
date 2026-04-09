#include "DropperTileEntity.h"

#include <memory>
#include <string>

#include "minecraft/IGameServices.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "strings.h"

std::string DropperTileEntity::getName() {
    return hasCustomName() ? name
                           : gameServices().getString(IDS_CONTAINER_DROPPER);
}

// 4J Added
std::shared_ptr<TileEntity> DropperTileEntity::clone() {
    std::shared_ptr<DropperTileEntity> result =
        std::make_shared<DropperTileEntity>();
    TileEntity::clone(result);

    result->name = name;

    return result;
}