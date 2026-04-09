
// EntityDamageSource::EntityDamageSource(const string &msgId,
// shared_ptr<Entity> entity) : DamageSource(msgId)
#include "minecraft/world/damageSource/EntityDamageSource.h"

#include <memory>
#include <string>

#include "java/Class.h"
#include "minecraft/network/packet/ChatPacket.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ItemInstance.h"

EntityDamageSource::EntityDamageSource(
    ChatPacket::EChatPacketMessage msgId,
    ChatPacket::EChatPacketMessage msgWithItemId,
    std::shared_ptr<Entity> entity)
    : DamageSource(msgId, msgWithItemId) {
    this->entity = entity;
}

std::shared_ptr<Entity> EntityDamageSource::getEntity() { return entity; }

// string EntityDamageSource::getLocalizedDeathMessage(shared_ptr<Player>
// player)
//{
//	return "death." + msgId + player->name + entity->getAName();
//	//return I18n.get("death." + msgId, player.name, entity.getAName());
// }

std::shared_ptr<ChatPacket> EntityDamageSource::getDeathMessagePacket(
    std::shared_ptr<LivingEntity> player) {
    std::shared_ptr<ItemInstance> held =
        (entity != nullptr) && entity->instanceof
        (eTYPE_LIVINGENTITY)
            ? std::dynamic_pointer_cast<LivingEntity>(entity)->getCarriedItem()
            : nullptr;
    std::string additional = "";

    if (entity->instanceof (eTYPE_SERVERPLAYER)) {
        additional = std::dynamic_pointer_cast<Player>(entity)->name;
    } else if (entity->instanceof (eTYPE_MOB)) {
        std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(entity);
        if (mob->hasCustomName()) {
            additional = mob->getCustomName();
        }
    }

    if ((held != nullptr) && held->hasCustomHoverName()) {
        return std::make_shared<ChatPacket>(player->getNetworkName(),
                                            m_msgWithItemId, entity->GetType(),
                                            additional, held->getHoverName());
    } else {
        return std::make_shared<ChatPacket>(player->getNetworkName(), m_msgId,
                                            entity->GetType(), additional);
    }
}

bool EntityDamageSource::scalesWithDifficulty() {
    return (entity != nullptr) && entity->instanceof
        (eTYPE_LIVINGENTITY) && !entity->instanceof (eTYPE_PLAYER);
}

// 4J: Copy function
DamageSource* EntityDamageSource::copy() {
    return new EntityDamageSource(*this);
}