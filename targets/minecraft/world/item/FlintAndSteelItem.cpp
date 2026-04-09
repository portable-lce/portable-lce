#include "FlintAndSteelItem.h"

#include <memory>

#include "java/Random.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/PortalTile.h"
#include "minecraft/world/level/tile/Tile.h"

FlintAndSteelItem::FlintAndSteelItem(int id) : Item(id) {
    maxStackSize = 1;
    setMaxDamage(64);
}

bool FlintAndSteelItem::useOn(std::shared_ptr<ItemInstance> instance,
                              std::shared_ptr<Player> player, Level* level,
                              int x, int y, int z, int face, float clickX,
                              float clickY, float clickZ, bool bTestUseOnOnly) {
    // 4J-PB - Adding a test only version to allow tooltips to be displayed
    if (face == 0) y--;
    if (face == 1) y++;
    if (face == 2) z--;
    if (face == 3) z++;
    if (face == 4) x--;
    if (face == 5) x++;

    if (!player->mayUseItemAt(x, y, z, face, instance)) return false;

    int targetType = level->getTile(x, y, z);

    if (!bTestUseOnOnly) {
        if (targetType == 0) {
            if (level->getTile(x, y - 1, z) == Tile::obsidian_Id) {
                if (Tile::portalTile->trySpawnPortal(level, x, y, z, false)) {
                    player->awardStat(GenericStats::portalsCreated(),
                                      GenericStats::param_noArgs());

                    // 4J : WESTY : Added for achievement.
                    player->awardStat(GenericStats::InToTheNether(),
                                      GenericStats::param_InToTheNether());
                }
            }

            level->playSound(x + 0.5, y + 0.5, z + 0.5,
                             eSoundType_FIRE_NEWIGNITE, 1,
                             random->nextFloat() * 0.4f + 0.8f);
            level->setTileAndUpdate(x, y, z, Tile::fire_Id);
        }

        instance->hurtAndBreak(1, player);
    } else {
        if (targetType == 0) {
            return true;
        } else {
            return false;
        }
    }

    // 4J-PB - this function shouldn't really return true all the time, but I've
    // added a special case for my test use for the tooltips display and will
    // leave it as is for the game use

    return true;
}
