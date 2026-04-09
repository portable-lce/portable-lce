#include "GiveItemCommand.h"

#include <vector>

#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/ByteArrayOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/commands/CommandsEnum.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/network/packet/GameCommandPacket.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "platform/PlatformTypes.h"

EGameCommand GiveItemCommand::getId() { return eGameCommand_Give; }

int GiveItemCommand::getPermissionLevel() { return LEVEL_GAMEMASTERS; }

void GiveItemCommand::execute(std::shared_ptr<CommandSender> source,
                              std::vector<uint8_t>& commandData) {
    ByteArrayInputStream bais(commandData);
    DataInputStream dis(&bais);

    PlayerUID uid = dis.readPlayerUID();
    int item = dis.readInt();
    int amount = dis.readInt();
    int aux = dis.readInt();
    std::string tag = dis.readUTF();

    bais.reset();

    std::shared_ptr<ServerPlayer> player = getPlayer(uid);
    if (player != nullptr && item > 0 && Item::items[item] != nullptr) {
        std::shared_ptr<ItemInstance> itemInstance =
            std::make_shared<ItemInstance>(item, amount, aux);
        std::shared_ptr<ItemEntity> drop = player->drop(itemInstance);
        drop->throwTime = 0;
        // logAdminAction(source, "commands.give.success",
        // ChatPacket::e_ChatCustom, Item::items[item]->getName(itemInstance),
        // item, amount, player->getAName());
        logAdminAction(source, ChatPacket::e_ChatCustom,
                       "commands.give.success", item, player->getAName());
    }
}

std::shared_ptr<GameCommandPacket> GiveItemCommand::preparePacket(
    std::shared_ptr<Player> player, int item, int amount, int aux,
    const std::string& tag) {
    if (player == nullptr) return nullptr;

    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);

    dos.writePlayerUID(player->getXuid());
    dos.writeInt(item);
    dos.writeInt(amount);
    dos.writeInt(aux);
    dos.writeUTF(tag);

    return std::shared_ptr<GameCommandPacket>(
        new GameCommandPacket(eGameCommand_Give, baos.toByteArray()));
}