
// IndirectEntityDamageSource::IndirectEntityDamageSource(const string &msgId,
// shared_ptr<Entity> entity, shared_ptr<Entity> owner) :
// EntityDamageSource(msgId, entity)
#include "minecraft/world/damageSource/IndirectEntityDamageSource.h"

#include <memory>
#include <string>

#include "java/Class.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/world/damageSource/EntityDamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ItemInstance.h"

IndirectEntityDamageSource::IndirectEntityDamageSource(
    ChatPacket::EChatPacketMessage msgId,
    ChatPacket::EChatPacketMessage msgWithItemId,
    std::shared_ptr<Entity> entity, std::shared_ptr<Entity> owner)
    : EntityDamageSource(msgId, msgWithItemId, entity) {
    this->owner = owner;
}

// 4J Stu - Brought forward from 1.2.3 to fix #46422
std::shared_ptr<Entity> IndirectEntityDamageSource::getDirectEntity() {
    return entity;
}

std::shared_ptr<Entity> IndirectEntityDamageSource::getEntity() {
    return owner;
}

// string
// IndirectEntityDamageSource::getLocalizedDeathMessage(shared_ptr<Player>
// player)
//{
//	return "death." + msgId + player->name + owner->getAName();
//	//return I18n.get("death." + msgId, player.name, owner.getAName());
// }

std::shared_ptr<ChatPacket> IndirectEntityDamageSource::getDeathMessagePacket(
    std::shared_ptr<LivingEntity> player) {
    std::shared_ptr<ItemInstance> held = entity->instanceof
        (eTYPE_LIVINGENTITY)
            ? std::dynamic_pointer_cast<LivingEntity>(entity)->getCarriedItem()
            : nullptr;
    std::string additional = "";
    int type;
    if (owner != nullptr) {
        type = owner->GetType();
        if (type == eTYPE_SERVERPLAYER) {
            std::shared_ptr<Player> sourcePlayer =
                std::dynamic_pointer_cast<Player>(owner);
            if (sourcePlayer != nullptr) additional = sourcePlayer->name;
        }
    } else {
        type = entity->GetType();
    }
    if (held != nullptr && held->hasCustomHoverName()) {
        return std::shared_ptr<ChatPacket>(
            new ChatPacket(player->getNetworkName(), m_msgWithItemId, type,
                           additional, held->getHoverName()));
    } else {
        return std::make_shared<ChatPacket>(player->getNetworkName(), m_msgId,
                                            type, additional);
    }
}

// 4J: Copy function
DamageSource* IndirectEntityDamageSource::copy() {
    return new IndirectEntityDamageSource(*this);
}