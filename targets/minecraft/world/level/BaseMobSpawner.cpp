#include "BaseMobSpawner.h"

#include <vector>

#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/util/WeighedRandom.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityIO.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/item/Minecart.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/LevelEvent.h"
#include "minecraft/world/phys/AABB.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "nbt/Tag.h"

BaseMobSpawner::BaseMobSpawner() {
    spawnPotentials = nullptr;
    spawnDelay = 20;
    entityId = "Pig";
    nextSpawnData = nullptr;
    spin = oSpin = 0.0;

    minSpawnDelay = SharedConstants::TICKS_PER_SECOND * 10;
    maxSpawnDelay = SharedConstants::TICKS_PER_SECOND * 40;
    spawnCount = 4;
    displayEntity = nullptr;
    maxNearbyEntities = 6;
    requiredPlayerRange = 16;
    spawnRange = 4;
}

BaseMobSpawner::~BaseMobSpawner() {
    if (spawnPotentials) {
        for (auto it = spawnPotentials->begin(); it != spawnPotentials->end();
             ++it) {
            delete *it;
        }
        delete spawnPotentials;
    }
}

std::string BaseMobSpawner::getEntityId() {
    if (getNextSpawnData() == nullptr) {
        if (entityId.compare("Minecart") == 0) {
            entityId = "MinecartRideable";
        }
        return entityId;
    } else {
        return getNextSpawnData()->type;
    }
}

void BaseMobSpawner::setEntityId(const std::string& entityId) {
    this->entityId = entityId;
}

bool BaseMobSpawner::isNearPlayer() {
    return getLevel()->getNearestPlayer(getX() + 0.5, getY() + 0.5,
                                        getZ() + 0.5,
                                        requiredPlayerRange) != nullptr;
}

void BaseMobSpawner::tick() {
    if (!isNearPlayer()) {
        return;
    }

    if (getLevel()->isClientSide) {
        double xP = getX() + getLevel()->random->nextFloat();
        double yP = getY() + getLevel()->random->nextFloat();
        double zP = getZ() + getLevel()->random->nextFloat();
        getLevel()->addParticle(eParticleType_smoke, xP, yP, zP, 0, 0, 0);
        getLevel()->addParticle(eParticleType_flame, xP, yP, zP, 0, 0, 0);

        if (spawnDelay > 0) spawnDelay--;
        oSpin = spin;
        spin = (int)(spin + 1000 / (spawnDelay + 200.0f)) % 360;
    } else {
        if (spawnDelay == -1) delay();

        if (spawnDelay > 0) {
            spawnDelay--;
            return;
        }

        bool _delay = false;

        for (int c = 0; c < spawnCount; c++) {
            std::shared_ptr<Entity> entity =
                EntityIO::newEntity(getEntityId(), getLevel());
            if (entity == nullptr) return;

            AABB grown =
                AABB(getX(), getY(), getZ(), getX() + 1, getY() + 1, getZ() + 1)
                    .grow(spawnRange * 2, 4, spawnRange * 2);

            int nearBy = getLevel()
                             ->getEntitiesOfClass(typeid(entity.get()), &grown)
                             ->size();
            if (nearBy >= maxNearbyEntities) {
                delay();
                return;
            }

            double xp = getX() + (getLevel()->random->nextDouble() -
                                  getLevel()->random->nextDouble()) *
                                     spawnRange;
            double yp = getY() + getLevel()->random->nextInt(3) - 1;
            double zp = getZ() + (getLevel()->random->nextDouble() -
                                  getLevel()->random->nextDouble()) *
                                     spawnRange;
            std::shared_ptr<Mob> mob = entity->instanceof
                (eTYPE_MOB) ? std::dynamic_pointer_cast<Mob>(entity) : nullptr;

            entity->moveTo(xp, yp, zp, getLevel()->random->nextFloat() * 360,
                           0);

            if (mob == nullptr || mob->canSpawn()) {
                loadDataAndAddEntity(entity);
                getLevel()->levelEvent(LevelEvent::PARTICLES_MOBTILE_SPAWN,
                                       getX(), getY(), getZ(), 0);

                if (mob != nullptr) {
                    mob->spawnAnim();
                }

                _delay = true;
            }
        }

        if (_delay) delay();
    }
}

std::shared_ptr<Entity> BaseMobSpawner::loadDataAndAddEntity(
    std::shared_ptr<Entity> entity) {
    if (getNextSpawnData() != nullptr) {
        CompoundTag* data = new CompoundTag();
        entity->save(data);

        std::vector<Tag*> tags = getNextSpawnData()->tag->getAllTags();
        for (auto it = tags.begin(); it != tags.end(); ++it) {
            Tag* tag = *it;
            data->put(tag->getName(), tag->copy());
        }

        entity->load(data);
        if (entity->level != nullptr) entity->level->addEntity(entity);

        // add mounts
        std::shared_ptr<Entity> rider = entity;
        while (data->contains(Entity::RIDING_TAG)) {
            CompoundTag* ridingTag = data->getCompound(Entity::RIDING_TAG);
            std::shared_ptr<Entity> mount =
                EntityIO::newEntity(ridingTag->getString("id"), entity->level);
            if (mount != nullptr) {
                CompoundTag* mountData = new CompoundTag();
                mount->save(mountData);

                std::vector<Tag*> ridingTags = ridingTag->getAllTags();
                for (auto it = ridingTags.begin(); it != ridingTags.end();
                     ++it) {
                    Tag* tag = *it;
                    mountData->put(tag->getName(), tag->copy());
                }
                mount->load(mountData);
                mount->moveTo(rider->x, rider->y, rider->z, rider->yRot,
                              rider->xRot);

                if (entity->level != nullptr) entity->level->addEntity(mount);
                rider->ride(mount);
            }
            rider = mount;
            data = ridingTag;
        }

    } else if (entity->instanceof
               (eTYPE_LIVINGENTITY) && entity->level != nullptr) {
        std::dynamic_pointer_cast<Mob>(entity)->finalizeMobSpawn(nullptr);
        getLevel()->addEntity(entity);
    }

    return entity;
}

void BaseMobSpawner::delay() {
    if (maxSpawnDelay <= minSpawnDelay) {
        spawnDelay = minSpawnDelay;
    } else {
        spawnDelay = minSpawnDelay +
                     getLevel()->random->nextInt(maxSpawnDelay - minSpawnDelay);
    }

    if ((spawnPotentials != nullptr) && (spawnPotentials->size() > 0)) {
        setNextSpawnData((SpawnData*)WeighedRandom::getRandomItem(
            (Random*)getLevel()->random,
            (std::vector<WeighedRandomItem*>*)spawnPotentials));
    }

    broadcastEvent(EVENT_SPAWN);
}

void BaseMobSpawner::load(CompoundTag* tag) {
    entityId = tag->getString("EntityId");
    spawnDelay = tag->getShort("Delay");

    if (tag->contains("SpawnPotentials")) {
        spawnPotentials = new std::vector<SpawnData*>();
        ListTag<CompoundTag>* potentials =
            (ListTag<CompoundTag>*)tag->getList("SpawnPotentials");

        for (int i = 0; i < potentials->size(); i++) {
            spawnPotentials->push_back(new SpawnData(potentials->get(i)));
        }
    } else {
        spawnPotentials = nullptr;
    }

    if (tag->contains("SpawnData")) {
        setNextSpawnData(
            new SpawnData(tag->getCompound("SpawnData"), entityId));
    } else {
        setNextSpawnData(nullptr);
    }

    if (tag->contains("MinSpawnDelay")) {
        minSpawnDelay = tag->getShort("MinSpawnDelay");
        maxSpawnDelay = tag->getShort("MaxSpawnDelay");
        spawnCount = tag->getShort("SpawnCount");
    }

    if (tag->contains("MaxNearbyEntities")) {
        maxNearbyEntities = tag->getShort("MaxNearbyEntities");
        requiredPlayerRange = tag->getShort("RequiredPlayerRange");
    }

    if (tag->contains("SpawnRange")) spawnRange = tag->getShort("SpawnRange");

    if (getLevel() != nullptr && getLevel()->isClientSide) {
        displayEntity = nullptr;
    }
}

void BaseMobSpawner::save(CompoundTag* tag) {
    tag->putString("EntityId", getEntityId());
    tag->putShort("Delay", (short)spawnDelay);
    tag->putShort("MinSpawnDelay", (short)minSpawnDelay);
    tag->putShort("MaxSpawnDelay", (short)maxSpawnDelay);
    tag->putShort("SpawnCount", (short)spawnCount);
    tag->putShort("MaxNearbyEntities", (short)maxNearbyEntities);
    tag->putShort("RequiredPlayerRange", (short)requiredPlayerRange);
    tag->putShort("SpawnRange", (short)spawnRange);

    if (getNextSpawnData() != nullptr) {
        tag->putCompound("SpawnData",
                         (CompoundTag*)getNextSpawnData()->tag->copy());
    }

    if (getNextSpawnData() != nullptr ||
        (spawnPotentials != nullptr && spawnPotentials->size() > 0)) {
        ListTag<CompoundTag>* list = new ListTag<CompoundTag>();

        if (spawnPotentials != nullptr && spawnPotentials->size() > 0) {
            for (auto it = spawnPotentials->begin();
                 it != spawnPotentials->end(); ++it) {
                SpawnData* data = *it;
                list->add(data->save());
            }
        } else {
            list->add(getNextSpawnData()->save());
        }

        tag->put("SpawnPotentials", list);
    }
}

std::shared_ptr<Entity> BaseMobSpawner::getDisplayEntity() {
    if (displayEntity == nullptr) {
        std::shared_ptr<Entity> e = EntityIO::newEntity(getEntityId(), nullptr);
        e = loadDataAndAddEntity(e);
        displayEntity = e;
    }

    return displayEntity;
}

bool BaseMobSpawner::onEventTriggered(int id) {
    if (id == EVENT_SPAWN && getLevel()->isClientSide) {
        spawnDelay = minSpawnDelay;
        return true;
    }
    return false;
}

BaseMobSpawner::SpawnData* BaseMobSpawner::getNextSpawnData() {
    return nextSpawnData;
}

void BaseMobSpawner::setNextSpawnData(SpawnData* nextSpawnData) {
    this->nextSpawnData = nextSpawnData;
}

BaseMobSpawner::SpawnData::SpawnData(CompoundTag* base)
    : WeighedRandomItem(base->getInt("Weight")) {
    CompoundTag* tag = base->getCompound("Properties");
    std::string _type = base->getString("Type");

    if (_type.compare("Minecart") == 0) {
        if (tag != nullptr) {
            switch (tag->getInt("Type")) {
                case Minecart::TYPE_CHEST:
                    type = "MinecartChest";
                    break;
                case Minecart::TYPE_FURNACE:
                    type = "MinecartFurnace";
                    break;
                case Minecart::TYPE_RIDEABLE:
                    type = "MinecartRideable";
                    break;
            }
        } else {
            type = "MinecartRideable";
        }
    }

    this->tag = tag;
    this->type = _type;
}

BaseMobSpawner::SpawnData::SpawnData(CompoundTag* tag, std::string _type)
    : WeighedRandomItem(1) {
    if (_type.compare("Minecart") == 0) {
        if (tag != nullptr) {
            switch (tag->getInt("Type")) {
                case Minecart::TYPE_CHEST:
                    _type = "MinecartChest";
                    break;
                case Minecart::TYPE_FURNACE:
                    _type = "MinecartFurnace";
                    break;
                case Minecart::TYPE_RIDEABLE:
                    _type = "MinecartRideable";
                    break;
            }
        } else {
            _type = "MinecartRideable";
        }
    }

    this->tag = tag;
    this->type = _type;
}

BaseMobSpawner::SpawnData::~SpawnData() { delete tag; }

CompoundTag* BaseMobSpawner::SpawnData::save() {
    CompoundTag* result = new CompoundTag();

    result->putCompound("Properties", tag);
    result->putString("Type", type);
    result->putInt("Weight", randomWeight);

    return result;
}
