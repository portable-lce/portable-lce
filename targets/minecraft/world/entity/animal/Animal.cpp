#include "Animal.h"

#include <cmath>
#include <numbers>
#include <string>
#include <typeinfo>
#include <vector>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/damageSource/EntityDamageSource.h"
#include "minecraft/world/entity/AgeableMob.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityEvent.h"
#include "minecraft/world/entity/ExperienceOrb.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/attributes/AttributeModifier.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"
#include "strings.h"

Animal::Animal(Level* level) : AgableMob(level) {
    //	inLove = 0;
    //// 4J removed - now synched data
    loveTime = 0;
    loveCause = std::shared_ptr<Player>();

    setDespawnProtected();
}

void Animal::defineSynchedData() {
    AgableMob::defineSynchedData();

    entityData->define(DATA_IN_LOVE, (int)0);  // 4J added
}

void Animal::serverAiMobStep() {
    if (getAge() != 0) setInLoveValue(0);
    AgableMob::serverAiMobStep();
}

void Animal::aiStep() {
    AgableMob::aiStep();

    if (getAge() != 0) setInLoveValue(0);

    if (getInLoveValue() > 0) {
        setInLoveValue(getInLoveValue() - 1);
        if (getInLoveValue() % 10 == 0) {
            double xa = random->nextGaussian() * 0.02;
            double ya = random->nextGaussian() * 0.02;
            double za = random->nextGaussian() * 0.02;
            level->addParticle(eParticleType_heart,
                               x + random->nextFloat() * bbWidth * 2 - bbWidth,
                               y + .5f + random->nextFloat() * bbHeight,
                               z + random->nextFloat() * bbWidth * 2 - bbWidth,
                               xa, ya, za);
        }
    } else {
        loveTime = 0;
    }

    updateDespawnProtectedState();  // 4J added
}

void Animal::checkHurtTarget(std::shared_ptr<Entity> target, float d) {
    // 4J-JEV: Changed from dynamic cast to use eINSTANCEOF
    if (target->instanceof (eTYPE_PLAYER)) {
        if (d < 3) {
            double xd = target->x - x;
            double zd = target->z - z;
            yRot = (float)(atan2(zd, xd) * 180 / std::numbers::pi) - 90;

            holdGround = true;
        }

        std::shared_ptr<Player> p = std::dynamic_pointer_cast<Player>(target);
        if (p->getSelectedItem() == nullptr || !isFood(p->getSelectedItem())) {
            attackTarget = nullptr;
        }

    }
    // 4J-JEV: Changed from dynamic cast to use eINSTANCEOF
    else if (target->instanceof (eTYPE_ANIMAL)) {
        std::shared_ptr<Animal> a = std::dynamic_pointer_cast<Animal>(target);
        if (getAge() > 0 && a->getAge() < 0) {
            if (d < 2.5) {
                holdGround = true;
            }
        } else if (getInLoveValue() > 0 && a->getInLoveValue() > 0) {
            if (a->attackTarget == nullptr)
                a->attackTarget = shared_from_this();

            if (a->attackTarget == shared_from_this() && d < 3.5) {
                a->setInLoveValue(a->getInLoveValue() + 1);
                setInLoveValue(getInLoveValue() + 1);
                loveTime++;
                if (loveTime % 4 == 0) {
                    level->addParticle(
                        eParticleType_heart,
                        x + random->nextFloat() * bbWidth * 2 - bbWidth,
                        y + .5f + random->nextFloat() * bbHeight,
                        z + random->nextFloat() * bbWidth * 2 - bbWidth, 0, 0,
                        0);
                }

                if (loveTime == 20 * 3) breedWith(a);
            } else
                loveTime = 0;
        } else {
            loveTime = 0;
            attackTarget = nullptr;
        }
    }
}

void Animal::breedWith(std::shared_ptr<Animal> target) {
    std::shared_ptr<AgableMob> offspring = getBreedOffspring(target);

    setInLoveValue(0);
    loveTime = 0;
    attackTarget = nullptr;
    target->attackTarget = nullptr;
    target->loveTime = 0;
    target->setInLoveValue(0);

    // 4J - we have offspring of nullptr returned when we have hit our limits of
    // spawning any particular type of animal. In these cases try and do
    // everything we can apart from actually spawning the entity.
    if (offspring != nullptr) {
        // Only want to set the age to this +ve value if something is actually
        // spawned, as during this period the animal will attempt to follow
        // offspring and ignore players.
        setAge(5 * 60 * 20);
        target->setAge(5 * 60 * 20);

        offspring->setAge(-20 * 60 * 20);
        offspring->moveTo(x, y, z, yRot, xRot);
        offspring->setDespawnProtected();
        for (int i = 0; i < 7; i++) {
            double xa = random->nextGaussian() * 0.02;
            double ya = random->nextGaussian() * 0.02;
            double za = random->nextGaussian() * 0.02;
            level->addParticle(eParticleType_heart,
                               x + random->nextFloat() * bbWidth * 2 - bbWidth,
                               y + .5f + random->nextFloat() * bbHeight,
                               z + random->nextFloat() * bbWidth * 2 - bbWidth,
                               xa, ya, za);
        }
        level->addEntity(offspring);

        level->addEntity(std::shared_ptr<ExperienceOrb>(
            new ExperienceOrb(level, x, y, z, random->nextInt(4) + 1)));
    }

    setDespawnProtected();
}

float Animal::getWalkTargetValue(int x, int y, int z) {
    if (level->getTile(x, y - 1, z) == Tile::grass_Id) return 10;
    return level->getBrightness(x, y, z) - 0.5f;
}

bool Animal::hurt(DamageSource* dmgSource, float dmg) {
    if (isInvulnerable()) return false;
    if (dynamic_cast<EntityDamageSource*>(dmgSource) != nullptr) {
        std::shared_ptr<Entity> source = dmgSource->getDirectEntity();

        // 4J-JEV: Changed from dynamic cast to use eINSTANCEOF
        if (source->instanceof
            (eTYPE_PLAYER) && !std::dynamic_pointer_cast<Player>(source)
                                   ->isAllowedToAttackAnimals()) {
            return false;
        }

        if ((source != nullptr) && source->instanceof (eTYPE_ARROW)) {
            std::shared_ptr<Arrow> arrow =
                std::dynamic_pointer_cast<Arrow>(source);

            // 4J: Check that the arrow's owner can attack animals (dispenser
            // arrows are not owned)
            if (arrow->owner != nullptr && arrow->owner->instanceof
                (eTYPE_PLAYER) &&
                    !std::dynamic_pointer_cast<Player>(arrow->owner)
                         ->isAllowedToAttackAnimals()) {
                return false;
            }
        }
    }

    fleeTime = 20 * 3;

    if (!useNewAi()) {
        AttributeInstance* speed =
            getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
        if (speed->getModifier(eModifierId_MOB_FLEEING) == nullptr) {
            speed->addModifier(
                new AttributeModifier(*Animal::SPEED_MODIFIER_FLEEING));
        }
    }

    attackTarget = nullptr;
    setInLoveValue(0);

    return AgableMob::hurt(dmgSource, dmg);
}

void Animal::addAdditonalSaveData(CompoundTag* tag) {
    AgableMob::addAdditonalSaveData(tag);
    tag->putInt("InLove", getInLoveValue());
}

void Animal::readAdditionalSaveData(CompoundTag* tag) {
    AgableMob::readAdditionalSaveData(tag);
    setInLoveValue(tag->getInt("InLove"));
    setDespawnProtected();
}

std::shared_ptr<Entity> Animal::findAttackTarget() {
    if (fleeTime > 0) return nullptr;

    float r = 8;
    if (getInLoveValue() > 0) {
        AABB grown = bb.grow(r, r, r);
        std::vector<std::shared_ptr<Entity> >* others =
            level->getEntitiesOfClass(typeid(*this), &grown);
        // for (int i = 0; i < others->size(); i++)
        for (auto it = others->begin(); it != others->end(); ++it) {
            std::shared_ptr<Animal> p = std::dynamic_pointer_cast<Animal>(*it);
            if (p != shared_from_this() && p->getInLoveValue() > 0) {
                delete others;
                return p;
            }
        }
        delete others;
    } else {
        if (getAge() == 0) {
            AABB grown = bb.grow(r, r, r);
            std::vector<std::shared_ptr<Entity> >* players =
                level->getEntitiesOfClass(typeid(Player), &grown);
            // for (int i = 0; i < players.size(); i++)
            for (auto it = players->begin(); it != players->end(); ++it) {
                setDespawnProtected();

                std::shared_ptr<Player> p =
                    std::dynamic_pointer_cast<Player>(*it);
                if (p->getSelectedItem() != nullptr &&
                    this->isFood(p->getSelectedItem())) {
                    delete players;
                    return p;
                }
            }
            delete players;
        } else if (getAge() > 0) {
            AABB grown = bb.grow(r, r, r);
            std::vector<std::shared_ptr<Entity> >* others =
                level->getEntitiesOfClass(typeid(*this), &grown);
            // for (int i = 0; i < others.size(); i++)
            for (auto it = others->begin(); it != others->end(); ++it) {
                std::shared_ptr<Animal> p =
                    std::dynamic_pointer_cast<Animal>(*it);
                if (p != shared_from_this() && p->getAge() < 0) {
                    delete others;
                    return p;
                }
            }
            delete others;
        }
    }
    return nullptr;
}

bool Animal::canSpawn() {
    int xt = Mth::floor(x);
    int yt = Mth::floor(bb.y0);
    int zt = Mth::floor(z);
    return level->getTile(xt, yt - 1, zt) == Tile::grass_Id &&
           level->getDaytimeRawBrightness(xt, yt, zt) > 8 &&
           AgableMob::canSpawn();
}

int Animal::getAmbientSoundInterval() { return 20 * 6; }

bool Animal::removeWhenFarAway() {
    return !isDespawnProtected();  // 4J changed - was false
}

int Animal::getExperienceReward(std::shared_ptr<Player> killedBy) {
    return 1 + level->random->nextInt(3);
}

bool Animal::isFood(std::shared_ptr<ItemInstance> itemInstance) {
    return itemInstance->id == Item::wheat_Id;
}

bool Animal::mobInteract(std::shared_ptr<Player> player) {
    std::shared_ptr<ItemInstance> item = player->inventory->getSelected();
    if (item != nullptr && isFood(item) && getAge() == 0 &&
        getInLoveValue() <= 0) {
        if (!player->abilities.instabuild) {
            item->count--;
            if (item->count <= 0) {
                player->inventory->setItem(player->inventory->selected,
                                           nullptr);
            }
        }

        // 4J-PB - If we can't produce another animal through breeding because
        // of the spawn limits, display a message here
        if (!level->isClientSide) {
            switch (GetType()) {
                case eTYPE_CHICKEN:
                    if (!level->canCreateMore(eTYPE_CHICKEN,
                                              Level::eSpawnType_Breed)) {
                        player->displayClientMessage(IDS_MAX_CHICKENS_BRED);
                        return false;
                    }
                    break;
                case eTYPE_WOLF:
                    if (!level->canCreateMore(eTYPE_WOLF,
                                              Level::eSpawnType_Breed)) {
                        player->displayClientMessage(IDS_MAX_WOLVES_BRED);
                        return false;
                    }
                    break;
                case eTYPE_MUSHROOMCOW:
                    if (!level->canCreateMore(eTYPE_MUSHROOMCOW,
                                              Level::eSpawnType_Breed)) {
                        player->displayClientMessage(IDS_MAX_MUSHROOMCOWS_BRED);
                        return false;
                    }
                    break;
                default:
                    if ((GetType() & eTYPE_ANIMALS_SPAWN_LIMIT_CHECK) ==
                        eTYPE_ANIMALS_SPAWN_LIMIT_CHECK) {
                        if (!level->canCreateMore(GetType(),
                                                  Level::eSpawnType_Breed)) {
                            player->displayClientMessage(
                                IDS_MAX_PIGS_SHEEP_COWS_CATS_BRED);

                            return false;
                        }
                    } else if (instanceof (eTYPE_MONSTER)) {
                    }
                    break;
            }
            setInLove(player);
        }
        setInLove();

        return true;
    }
    return AgableMob::mobInteract(player);
}

// 4J added
int Animal::getInLoveValue() { return entityData->getInteger(DATA_IN_LOVE); }

void Animal::setInLoveValue(int value) { entityData->set(DATA_IN_LOVE, value); }

// 4J added
void Animal::setInLove(std::shared_ptr<Player> player) {
    loveCause = player;
    setInLoveValue(20 * 30);
}

std::shared_ptr<Player> Animal::getLoveCause() { return loveCause.lock(); }

void Animal::setInLove() {
    entityData->set(DATA_IN_LOVE, 20 * 30);

    attackTarget = nullptr;
    level->broadcastEntityEvent(shared_from_this(),
                                EntityEvent::IN_LOVE_HEARTS);
}

bool Animal::isInLove() { return entityData->getInteger(DATA_IN_LOVE) > 0; }

void Animal::resetLove() { entityData->set(DATA_IN_LOVE, 0); }

bool Animal::canMate(std::shared_ptr<Animal> partner) {
    if (partner == nullptr) return false;
    if (partner == shared_from_this()) return false;
    Animal* partnerAnimal = partner.get();
    if (typeid(*partnerAnimal) != typeid(*this)) return false;
    return isInLove() && partner->isInLove();
}

void Animal::handleEntityEvent(uint8_t id) {
    if (id == EntityEvent::IN_LOVE_HEARTS) {
        for (int i = 0; i < 7; i++) {
            double xa = random->nextGaussian() * 0.02;
            double ya = random->nextGaussian() * 0.02;
            double za = random->nextGaussian() * 0.02;
            level->addParticle(eParticleType_heart,
                               x + random->nextFloat() * bbWidth * 2 - bbWidth,
                               y + .5f + random->nextFloat() * bbHeight,
                               z + random->nextFloat() * bbWidth * 2 - bbWidth,
                               xa, ya, za);
        }
    } else {
        AgableMob::handleEntityEvent(id);
    }
}

void Animal::updateDespawnProtectedState() {
    if (level->isClientSide) return;

    if (m_isDespawnProtected) {
        int xt = Mth::floor(x);
        int zt = Mth::floor(z);

        if (xt > m_maxWanderX) m_maxWanderX = xt;
        if (xt < m_minWanderX) m_minWanderX = xt;
        if (zt > m_maxWanderZ) m_maxWanderZ = zt;
        if (zt < m_minWanderZ) m_minWanderZ = zt;

        if (((m_maxWanderX - m_minWanderX) > MAX_WANDER_DISTANCE) ||
            ((m_maxWanderZ - m_minWanderZ) > MAX_WANDER_DISTANCE)) {
            //			printf("Unprotecting : %d to %d, %d to %d\n",
            // m_minWanderX, m_maxWanderX, m_minWanderZ, m_maxWanderZ );
            m_isDespawnProtected = false;
        }

        /*
                        if( isExtraWanderingEnabled() )
                        {
                                printf("%d: %d %d, %d\n",entityId,m_maxWanderX -
           m_minWanderX, m_maxWanderZ - m_minWanderZ, getWanderingQuadrant());
                        }
                        */
    }
}

bool Animal::isDespawnProtected() { return m_isDespawnProtected; }

void Animal::setDespawnProtected() {
    if (level && level->isClientSide) return;

    int xt = Mth::floor(x);
    int zt = Mth::floor(z);

    m_minWanderX = xt;
    m_maxWanderX = xt;
    m_minWanderZ = zt;
    m_maxWanderZ = zt;

    m_isDespawnProtected = true;
}
