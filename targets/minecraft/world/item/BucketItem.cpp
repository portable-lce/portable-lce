#include "BucketItem.h"

#include <memory>
#include <string>

#include "java/Class.h"
#include "java/JavaMath.h"
#include "java/Random.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/server/network/PlayerConnection.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/HitResult.h"

BucketItem::BucketItem(int id, int content) : Item(id) {
    maxStackSize = 1;
    this->content = content;
}

bool BucketItem::TestUse(std::shared_ptr<ItemInstance> itemInstance,
                         Level* level, std::shared_ptr<Player> player) {
    bool pickLiquid = content == 0;
    HitResult* hr = getPlayerPOVHitResult(level, player, pickLiquid);
    if (hr == nullptr) return false;

    if (hr->type == HitResult::TILE) {
        int xt = hr->x;
        int yt = hr->y;
        int zt = hr->z;

        if (!level->mayInteract(player, xt, yt, zt, content)) {
            delete hr;
            return false;
        }

        if (content == 0) {
            if (!player->mayUseItemAt(xt, yt, zt, hr->f, itemInstance))
                return false;
            if (level->getMaterial(xt, yt, zt) == Material::water &&
                level->getData(xt, yt, zt) == 0) {
                delete hr;
                return true;
            }
            if (level->getMaterial(xt, yt, zt) == Material::lava &&
                level->getData(xt, yt, zt) == 0) {
                delete hr;
                return true;
            }
        } else if (content < 0) {
            delete hr;
            return true;
        } else {
            if (hr->f == 0) yt--;
            if (hr->f == 1) yt++;
            if (hr->f == 2) zt--;
            if (hr->f == 3) zt++;
            if (hr->f == 4) xt--;
            if (hr->f == 5) xt++;

            if (!player->mayUseItemAt(xt, yt, zt, hr->f, itemInstance))
                return false;

            if (level->isEmptyTile(xt, yt, zt) ||
                !level->getMaterial(xt, yt, zt)->isSolid()) {
                delete hr;
                return true;
            }
        }
    } else {
        if (content == 0) {
            if (hr->entity->GetType() == eTYPE_COW) {
                delete hr;
                return true;
            }
        }
    }
    delete hr;

    return false;
}

std::shared_ptr<ItemInstance> BucketItem::use(
    std::shared_ptr<ItemInstance> itemInstance, Level* level,
    std::shared_ptr<Player> player) {
    float a = 1;

    double x = player->xo + (player->x - player->xo) * a;
    double y =
        player->yo + (player->y - player->yo) * a + 1.62 - player->heightOffset;
    double z = player->zo + (player->z - player->zo) * a;

    bool pickLiquid = content == 0;
    HitResult* hr = getPlayerPOVHitResult(level, player, pickLiquid);
    if (hr == nullptr) return itemInstance;

    if (hr->type == HitResult::TILE) {
        int xt = hr->x;
        int yt = hr->y;
        int zt = hr->z;

        if (!level->mayInteract(player, xt, yt, zt, content)) {
            Log::info("!!!!!!!!!!! Can't place that here\n");
            std::shared_ptr<ServerPlayer> servPlayer =
                std::dynamic_pointer_cast<ServerPlayer>(player);
            if (servPlayer != nullptr) {
                Log::info(
                    "Sending ChatPacket::e_ChatCannotPlaceLava to player\n");
                servPlayer->connection->send(std::shared_ptr<ChatPacket>(
                    new ChatPacket("", ChatPacket::e_ChatCannotPlaceLava)));
            }

            delete hr;
            return itemInstance;
        }

        if (content == 0) {
            if (!player->mayUseItemAt(xt, yt, zt, hr->f, itemInstance))
                return itemInstance;
            if (level->getMaterial(xt, yt, zt) == Material::water &&
                level->getData(xt, yt, zt) == 0) {
                level->removeTile(xt, yt, zt);
                delete hr;
                if (player->abilities.instabuild) {
                    return itemInstance;
                }

                if (--itemInstance->count <= 0) {
                    return std::shared_ptr<ItemInstance>(
                        new ItemInstance(Item::bucket_water));
                } else {
                    if (!player->inventory->add(std::shared_ptr<ItemInstance>(
                            new ItemInstance(Item::bucket_water)))) {
                        player->drop(std::shared_ptr<ItemInstance>(
                            new ItemInstance(Item::bucket_water_Id, 1, 0)));
                    }
                    return itemInstance;
                }
            }
            if (level->getMaterial(xt, yt, zt) == Material::lava &&
                level->getData(xt, yt, zt) == 0) {
                if (level->dimension->id == -1)
                    player->awardStat(GenericStats::netherLavaCollected(),
                                      GenericStats::param_noArgs());

                level->removeTile(xt, yt, zt);
                delete hr;
                if (player->abilities.instabuild) {
                    return itemInstance;
                }
                if (--itemInstance->count <= 0) {
                    return std::shared_ptr<ItemInstance>(
                        new ItemInstance(Item::bucket_lava));
                } else {
                    if (!player->inventory->add(std::shared_ptr<ItemInstance>(
                            new ItemInstance(Item::bucket_lava)))) {
                        player->drop(std::shared_ptr<ItemInstance>(
                            new ItemInstance(Item::bucket_lava_Id, 1, 0)));
                    }
                    return itemInstance;
                }
            }
        } else if (content < 0) {
            delete hr;
            return std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::bucket_empty));
        } else {
            if (hr->f == 0) yt--;
            if (hr->f == 1) yt++;
            if (hr->f == 2) zt--;
            if (hr->f == 3) zt++;
            if (hr->f == 4) xt--;
            if (hr->f == 5) xt++;

            if (!player->mayUseItemAt(xt, yt, zt, hr->f, itemInstance))
                return itemInstance;

            if (emptyBucket(level, xt, yt, zt) &&
                !player->abilities.instabuild) {
                return std::shared_ptr<ItemInstance>(
                    new ItemInstance(Item::bucket_empty));
            }
        }
    }
    delete hr;
    return itemInstance;
}

bool BucketItem::emptyBucket(Level* level, int xt, int yt, int zt) {
    if (content <= 0) return false;

    Material* material = level->getMaterial(xt, yt, zt);
    bool nonSolid = !material->isSolid();

    if (level->isEmptyTile(xt, yt, zt) || nonSolid) {
        if (level->dimension->ultraWarm && content == Tile::water_Id) {
            level->playSound(
                xt + 0.5f, yt + 0.5f, zt + 0.5f, eSoundType_RANDOM_FIZZ, 0.5f,
                2.6f +
                    (level->random->nextFloat() - level->random->nextFloat()) *
                        0.8f);

            for (int i = 0; i < 8; i++) {
                level->addParticle(eParticleType_largesmoke,
                                   xt + Math::random(), yt + Math::random(),
                                   zt + Math::random(), 0, 0, 0);
            }
        } else {
            if (!level->isClientSide && nonSolid && !material->isLiquid()) {
                level->destroyTile(xt, yt, zt, true);
            }
            level->setTileAndData(xt, yt, zt, content, 0, Tile::UPDATE_ALL);
        }

        return true;
    }

    return false;
}
