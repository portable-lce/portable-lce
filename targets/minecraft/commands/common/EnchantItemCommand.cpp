#include "EnchantItemCommand.h"

#include <string>
#include <vector>

#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/commands/CommandsEnum.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/network/packet/GameCommandPacket.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "platform/PlatformTypes.h"

EGameCommand EnchantItemCommand::getId() { return eGameCommand_EnchantItem; }

int EnchantItemCommand::getPermissionLevel() { return LEVEL_GAMEMASTERS; }

void EnchantItemCommand::execute(std::shared_ptr<CommandSender> source,
                                 std::vector<uint8_t>& commandData) {
    ByteArrayInputStream bais(commandData);
    DataInputStream dis(&bais);

    PlayerUID uid = dis.readPlayerUID();
    int enchantmentId = dis.readInt();
    int enchantmentLevel = dis.readInt();

    bais.reset();

    std::shared_ptr<ServerPlayer> player = getPlayer(uid);

    if (player == nullptr) return;

    std::shared_ptr<ItemInstance> selectedItem = player->getSelectedItem();

    if (selectedItem == nullptr) return;

    Enchantment* e = Enchantment::enchantments[enchantmentId];

    if (e == nullptr) return;
    if (!e->canEnchant(selectedItem)) return;

    if (enchantmentLevel < e->getMinLevel())
        enchantmentLevel = e->getMinLevel();
    if (enchantmentLevel > e->getMaxLevel())
        enchantmentLevel = e->getMaxLevel();

    if (selectedItem->hasTag()) {
        ListTag<CompoundTag>* enchantmentTags =
            selectedItem->getEnchantmentTags();
        if (enchantmentTags != nullptr) {
            for (int i = 0; i < enchantmentTags->size(); i++) {
                int type = enchantmentTags->get(i)->getShort(
                    (char*)ItemInstance::TAG_ENCH_ID);

                if (Enchantment::enchantments[type] != nullptr) {
                    Enchantment* other = Enchantment::enchantments[type];
                    if (!other->isCompatibleWith(e)) {
                        return;
                        // throw new
                        // CommandException("commands.enchant.cantCombine",
                        // e.getFullname(level),
                        // other.getFullname(enchantmentTags.get(i).getShort(ItemInstance.TAG_ENCH_LEVEL)));
                    }
                }
            }
        }
    }

    selectedItem->enchant(e, enchantmentLevel);

    // logAdminAction(source, "commands.enchant.success");
    logAdminAction(source, ChatPacket::e_ChatCustom,
                   "commands.enchant.success");
}

std::shared_ptr<GameCommandPacket> EnchantItemCommand::preparePacket(
    std::shared_ptr<Player> player, int enchantmentId, int enchantmentLevel) {
    if (player == nullptr) return nullptr;

    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);

    dos.writePlayerUID(player->getXuid());
    dos.writeInt(enchantmentId);
    dos.writeInt(enchantmentLevel);

    return std::shared_ptr<GameCommandPacket>(
        new GameCommandPacket(eGameCommand_EnchantItem, baos.toByteArray()));
}