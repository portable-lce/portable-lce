#include "Enemy.h"

#include <memory>

#include "java/Class.h"
#include "minecraft/world/entity/Entity.h"

class EntitySelector;

EntitySelector* Enemy::ENEMY_SELECTOR = new Enemy::EnemyEntitySelector();

bool Enemy::EnemyEntitySelector::matches(std::shared_ptr<Entity> entity) const {
    return (entity != nullptr) && entity->instanceof(eTYPE_ENEMY);
}