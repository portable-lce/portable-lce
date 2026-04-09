#include "TntTile.h"

#include <string>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/Facing.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/item/PrimedTnt.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Explosion.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/Tile.h"

TntTile::TntTile(int id) : Tile(id, Material::explosive) {
    iconTop = nullptr;
    iconBottom = nullptr;
}

Icon* TntTile::getTexture(int face, int data) {
    if (face == Facing::DOWN) return iconBottom;
    if (face == Facing::UP) return iconTop;
    return icon;
}

void TntTile::onPlace(Level* level, int x, int y, int z) {
    Tile::onPlace(level, x, y, z);
    if (level->hasNeighborSignal(x, y, z) &&
        gameServices().getGameHostOption(eGameHostOption_TNT)) {
        destroy(level, x, y, z, EXPLODE_BIT);
        level->removeTile(x, y, z);
    }
}

void TntTile::neighborChanged(Level* level, int x, int y, int z, int type) {
    if (level->hasNeighborSignal(x, y, z) &&
        gameServices().getGameHostOption(eGameHostOption_TNT)) {
        destroy(level, x, y, z, EXPLODE_BIT);
        level->removeTile(x, y, z);
    }
}

int TntTile::getResourceCount(Random* random) { return 1; }

void TntTile::wasExploded(Level* level, int x, int y, int z,
                          Explosion* explosion) {
    // 4J - added - don't every create on the client, I think this must be the
    // cause of a bug reported in the java version where white tnts are created
    // in the network game
    if (level->isClientSide) return;

    // 4J - added condition to have finite limit of these
    // 4J-JEV: Fix for #90934 - Customer Encountered: TU11: Content: Gameplay:
    // TNT blocks are triggered by explosions even though "TNT explodes" option
    // is unchecked.
    if (level->newPrimedTntAllowed() &&
        gameServices().getGameHostOption(eGameHostOption_TNT)) {
        std::shared_ptr<PrimedTnt> primed = std::shared_ptr<PrimedTnt>(
            new PrimedTnt(level, x + 0.5f, y + 0.5f, z + 0.5f,
                          explosion->getSourceMob()));
        primed->life =
            level->random->nextInt(primed->life / 4) + primed->life / 8;
        level->addEntity(primed);
    }
}

void TntTile::destroy(Level* level, int x, int y, int z, int data) {
    destroy(level, x, y, z, data, nullptr);
}

void TntTile::destroy(Level* level, int x, int y, int z, int data,
                      std::shared_ptr<LivingEntity> source) {
    if (level->isClientSide) return;

    if ((data & EXPLODE_BIT) == 1) {
        // 4J - added condition to have finite limit of these
        if (level->newPrimedTntAllowed() &&
            gameServices().getGameHostOption(eGameHostOption_TNT)) {
            std::shared_ptr<PrimedTnt> tnt = std::shared_ptr<PrimedTnt>(
                new PrimedTnt(level, x + 0.5f, y + 0.5f, z + 0.5f, source));
            level->addEntity(tnt);
            level->playEntitySound(tnt, eSoundType_RANDOM_FUSE, 1, 1.0f);
        }
    }
}

bool TntTile::use(Level* level, int x, int y, int z,
                  std::shared_ptr<Player> player, int clickedFace, float clickX,
                  float clickY, float clickZ,
                  bool soundOnly /*=false*/)  // 4J added soundOnly param
{
    if (soundOnly) return false;
    if (player->getSelectedItem() != nullptr &&
        player->getSelectedItem()->id == Item::flintAndSteel_Id) {
        destroy(level, x, y, z, EXPLODE_BIT, player);
        level->removeTile(x, y, z);
        player->getSelectedItem()->hurtAndBreak(1, player);
        return true;
    }
    return Tile::use(level, x, y, z, player, clickedFace, clickX, clickY,
                     clickZ);
}

void TntTile::entityInside(Level* level, int x, int y, int z,
                           std::shared_ptr<Entity> entity) {
    if (entity->GetType() == eTYPE_ARROW && !level->isClientSide) {
        if (entity->isOnFire()) {
            std::shared_ptr<Arrow> arrow =
                std::dynamic_pointer_cast<Arrow>(entity);
            destroy(level, x, y, z, EXPLODE_BIT, arrow->owner->instanceof
                    (eTYPE_LIVINGENTITY)
                        ? std::dynamic_pointer_cast<LivingEntity>(arrow->owner)
                        : nullptr);
            level->removeTile(x, y, z);
        }
    }
}

void TntTile::registerIcons(IconRegister* iconRegister) {
    icon = iconRegister->registerIcon("tnt_side");
    iconTop = iconRegister->registerIcon("tnt_top");
    iconBottom = iconRegister->registerIcon("tnt_bottom");
}

bool TntTile::dropFromExplosion(Explosion* explosion) { return false; }