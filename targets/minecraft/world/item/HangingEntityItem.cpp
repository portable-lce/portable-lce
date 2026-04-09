#include "HangingEntityItem.h"

#include <string.h>
#include <wchar.h>

#include <string>
#include <vector>

#include "Direction.h"
#include "Facing.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/world/entity/HangingEntity.h"
#include "minecraft/world/entity/ItemFrame.h"
#include "minecraft/world/entity/Painting.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "strings.h"

HangingEntityItem::HangingEntityItem(int id, eINSTANCEOF eClassType)
    : Item(id) {
    this->eType = eClassType;
}

bool HangingEntityItem::useOn(std::shared_ptr<ItemInstance> instance,
                              std::shared_ptr<Player> player, Level* level,
                              int xt, int yt, int zt, int face, float clickX,
                              float clickY, float clickZ, bool bTestOnly) {
    if (face == Facing::DOWN) return false;
    if (face == Facing::UP) return false;

    if (bTestOnly) {
        if (!player->mayUseItemAt(xt, yt, zt, face, instance)) return false;

        return true;
    }

    int dir = Direction::FACING_DIRECTION[face];

    std::shared_ptr<HangingEntity> entity =
        createEntity(level, xt, yt, zt, dir, instance->getAuxValue());

    if (!player->mayUseItemAt(xt, yt, zt, face, instance)) return false;

    if (entity != nullptr && entity->survives()) {
        if (!level->isClientSide) {
            if (level->addEntity(entity) == true) {
                // 4J-JEV: Hook for durango 'BlockPlaced' event.
                if (eType == eTYPE_PAINTING)
                    player->awardStat(
                        GenericStats::blocksPlaced(Item::painting_Id),
                        GenericStats::param_blocksPlaced(
                            Item::painting_Id, instance->getAuxValue(), 1));
                else if (eType == eTYPE_ITEM_FRAME)
                    player->awardStat(
                        GenericStats::blocksPlaced(Item::itemFrame_Id),
                        GenericStats::param_blocksPlaced(
                            Item::itemFrame_Id, instance->getAuxValue(), 1));

                instance->count--;
            } else {
                player->displayClientMessage(IDS_MAX_HANGINGENTITIES);
                return false;
            }
        } else {
            instance->count--;
        }
    }
    return true;
}

std::shared_ptr<HangingEntity> HangingEntityItem::createEntity(
    Level* level, int x, int y, int z, int dir,
    int auxValue)  // 4J added auxValue
{
    if (eType == eTYPE_PAINTING) {
        std::shared_ptr<Painting> painting =
            std::make_shared<Painting>(level, x, y, z, dir);

#ifndef _CONTENT_PACKAGE
        if (gameServices().debugArtToolsOn() && auxValue > 0) {
            painting->PaintingPostConstructor(dir, auxValue - 1);
        } else
#endif
        {
            painting->PaintingPostConstructor(dir);
        }

        return std::dynamic_pointer_cast<HangingEntity>(painting);
    } else if (eType == eTYPE_ITEM_FRAME) {
        std::shared_ptr<ItemFrame> itemFrame =
            std::make_shared<ItemFrame>(level, x, y, z, dir);

        return std::dynamic_pointer_cast<HangingEntity>(itemFrame);
    } else {
        return nullptr;
    }
}

// 4J Adding overrides for art tools
void HangingEntityItem::appendHoverText(
    std::shared_ptr<ItemInstance> itemInstance, std::shared_ptr<Player> player,
    std::vector<HtmlString>* lines, bool advanced) {
#ifndef _CONTENT_PACKAGE
    if (eType == eTYPE_PAINTING && gameServices().debugArtToolsOn() &&
        itemInstance->getAuxValue() > 0) {
        int motive = itemInstance->getAuxValue() - 1;

        char formatted[256];
        memset(formatted, 0, 256 * sizeof(char));
        snprintf(formatted, 256, "** %s %dx%d",
                 Painting::Motive::values[motive]->name.c_str(),
                 Painting::Motive::values[motive]->w / 16,
                 Painting::Motive::values[motive]->h / 16);

        std::string motiveName = formatted;

        lines->push_back(HtmlString(motiveName.c_str(), eHTMLColor_c));
    } else
#endif
    {
        return Item::appendHoverText(itemInstance, player, lines, advanced);
    }
}