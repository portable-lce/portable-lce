#include "FireworksItem.h"

#include <memory>
#include <vector>

#include "minecraft/IGameServices.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/projectile/FireworksRocketEntity.h"
#include "minecraft/world/item/FireworksChargeItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "strings.h"
#include "util/StringHelpers.h"

const std::string FireworksItem::TAG_FIREWORKS = "Fireworks";
const std::string FireworksItem::TAG_EXPLOSION = "Explosion";
const std::string FireworksItem::TAG_EXPLOSIONS = "Explosions";
const std::string FireworksItem::TAG_FLIGHT = "Flight";
const std::string FireworksItem::TAG_E_TYPE = "Type";
const std::string FireworksItem::TAG_E_TRAIL = "Trail";
const std::string FireworksItem::TAG_E_FLICKER = "Flicker";
const std::string FireworksItem::TAG_E_COLORS = "Colors";
const std::string FireworksItem::TAG_E_FADECOLORS = "FadeColors";

FireworksItem::FireworksItem(int id) : Item(id) {}

bool FireworksItem::useOn(std::shared_ptr<ItemInstance> instance,
                          std::shared_ptr<Player> player, Level* level, int x,
                          int y, int z, int face, float clickX, float clickY,
                          float clickZ, bool bTestUseOnOnly) {
    // 4J-JEV: Fix for xb1 #173493 - CU7: Content: UI: Missing tooltip for
    // Firework Rocket.
    if (bTestUseOnOnly) return true;

    if (!level->isClientSide) {
        std::shared_ptr<FireworksRocketEntity> f =
            std::make_shared<FireworksRocketEntity>(
                level, x + clickX, y + clickY, z + clickZ, instance);
        level->addEntity(f);

        if (!player->abilities.instabuild) {
            instance->count--;
        }
        return true;
    }

    return false;
}

void FireworksItem::appendHoverText(std::shared_ptr<ItemInstance> itemInstance,
                                    std::shared_ptr<Player> player,
                                    std::vector<HtmlString>* lines,
                                    bool advanced) {
    if (!itemInstance->hasTag()) {
        return;
    }
    CompoundTag* fireTag = itemInstance->getTag()->getCompound(TAG_FIREWORKS);
    if (fireTag == nullptr) {
        return;
    }
    if (fireTag->contains(TAG_FLIGHT)) {
        lines->push_back(
            std::string(gameServices().getString(IDS_ITEM_FIREWORKS_FLIGHT)) +
            " " + toWString<int>((fireTag->getByte(TAG_FLIGHT))));
    }

    ListTag<CompoundTag>* explosions =
        (ListTag<CompoundTag>*)fireTag->getList(TAG_EXPLOSIONS);
    if (explosions != nullptr && explosions->size() > 0) {
        for (int i = 0; i < explosions->size(); i++) {
            CompoundTag* expTag = explosions->get(i);

            std::vector<HtmlString> eLines;
            FireworksChargeItem::appendHoverText(expTag, &eLines);

            if (eLines.size() > 0) {
                // Indent lines after first line
                for (int i = 1; i < eLines.size(); i++) {
                    eLines[i].indent = true;
                }

                lines->insert(lines->end(), eLines.begin(), eLines.end());
            }
        }
    }
}