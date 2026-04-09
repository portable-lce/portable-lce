#include "EntityIO.h"

#include <utility>

#include "Entity.h"
#include "Painting.h"
#include "java/Class.h"
#include "java/JavaIntHash.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/ExperienceOrb.h"
#include "minecraft/world/entity/ItemFrame.h"
#include "minecraft/world/entity/LeashFenceKnotEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/ambient/Bat.h"
#include "minecraft/world/entity/animal/Chicken.h"
#include "minecraft/world/entity/animal/Cow.h"
#include "minecraft/world/entity/animal/EntityHorse.h"
#include "minecraft/world/entity/animal/MushroomCow.h"
#include "minecraft/world/entity/animal/Ocelot.h"
#include "minecraft/world/entity/animal/Pig.h"
#include "minecraft/world/entity/animal/Sheep.h"
#include "minecraft/world/entity/animal/SnowMan.h"
#include "minecraft/world/entity/animal/Squid.h"
#include "minecraft/world/entity/animal/VillagerGolem.h"
#include "minecraft/world/entity/animal/Wolf.h"
#include "minecraft/world/entity/boss/enderdragon/EnderCrystal.h"
#include "minecraft/world/entity/boss/enderdragon/EnderDragon.h"
#include "minecraft/world/entity/boss/wither/WitherBoss.h"
#include "minecraft/world/entity/item/Boat.h"
#include "minecraft/world/entity/item/FallingTile.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/entity/item/Minecart.h"
#include "minecraft/world/entity/item/MinecartChest.h"
#include "minecraft/world/entity/item/MinecartFurnace.h"
#include "minecraft/world/entity/item/MinecartHopper.h"
#include "minecraft/world/entity/item/MinecartRideable.h"
#include "minecraft/world/entity/item/MinecartSpawner.h"
#include "minecraft/world/entity/item/MinecartTNT.h"
#include "minecraft/world/entity/item/PrimedTnt.h"
#include "minecraft/world/entity/monster/Blaze.h"
#include "minecraft/world/entity/monster/CaveSpider.h"
#include "minecraft/world/entity/monster/Creeper.h"
#include "minecraft/world/entity/monster/EnderMan.h"
#include "minecraft/world/entity/monster/Ghast.h"
#include "minecraft/world/entity/monster/Giant.h"
#include "minecraft/world/entity/monster/LavaSlime.h"
#include "minecraft/world/entity/monster/Monster.h"
#include "minecraft/world/entity/monster/PigZombie.h"
#include "minecraft/world/entity/monster/Silverfish.h"
#include "minecraft/world/entity/monster/Skeleton.h"
#include "minecraft/world/entity/monster/Slime.h"
#include "minecraft/world/entity/monster/Spider.h"
#include "minecraft/world/entity/monster/Witch.h"
#include "minecraft/world/entity/monster/Zombie.h"
#include "minecraft/world/entity/npc/Villager.h"
#include "minecraft/world/entity/projectile/Arrow.h"
#include "minecraft/world/entity/projectile/DragonFireball.h"
#include "minecraft/world/entity/projectile/EyeOfEnderSignal.h"
#include "minecraft/world/entity/projectile/FireworksRocketEntity.h"
#include "minecraft/world/entity/projectile/LargeFireball.h"
#include "minecraft/world/entity/projectile/SmallFireball.h"
#include "minecraft/world/entity/projectile/Snowball.h"
#include "minecraft/world/entity/projectile/ThrownEnderpearl.h"
#include "minecraft/world/entity/projectile/ThrownExpBottle.h"
#include "minecraft/world/entity/projectile/ThrownPotion.h"
#include "minecraft/world/entity/projectile/WitherSkull.h"
#include "nbt/CompoundTag.h"
#include "strings.h"

std::unordered_map<std::string, entityCreateFn>* EntityIO::idCreateMap =
    new std::unordered_map<std::string, entityCreateFn>;
std::unordered_map<eINSTANCEOF, std::string, eINSTANCEOFKeyHash,
                   eINSTANCEOFKeyEq>* EntityIO::classIdMap =
    new std::unordered_map<eINSTANCEOF, std::string, eINSTANCEOFKeyHash,
                           eINSTANCEOFKeyEq>;
std::unordered_map<int, entityCreateFn>* EntityIO::numCreateMap =
    new std::unordered_map<int, entityCreateFn>;
std::unordered_map<int, eINSTANCEOF>* EntityIO::numClassMap =
    new std::unordered_map<int, eINSTANCEOF>;
std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash, eINSTANCEOFKeyEq>*
    EntityIO::classNumMap =
        new std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash,
                               eINSTANCEOFKeyEq>;
std::unordered_map<std::string, int>* EntityIO::idNumMap =
    new std::unordered_map<std::string, int>;
std::unordered_map<int, EntityIO::SpawnableMobInfo*>
    EntityIO::idsSpawnableInCreative;

void EntityIO::setId(entityCreateFn createFn, eINSTANCEOF clas,
                     const std::string& id, int idNum) {
    idCreateMap->insert(
        std::unordered_map<std::string, entityCreateFn>::value_type(id,
                                                                    createFn));
    classIdMap->insert(
        std::unordered_map<eINSTANCEOF, std::string, eINSTANCEOFKeyHash,
                           eINSTANCEOFKeyEq>::value_type(clas, id));
    numCreateMap->insert(
        std::unordered_map<int, entityCreateFn>::value_type(idNum, createFn));
    numClassMap->insert(
        std::unordered_map<int, eINSTANCEOF>::value_type(idNum, clas));
    classNumMap->insert(
        std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash,
                           eINSTANCEOFKeyEq>::value_type(clas, idNum));
    idNumMap->insert(
        std::unordered_map<std::string, int>::value_type(id, idNum));
}

void EntityIO::setId(entityCreateFn createFn, eINSTANCEOF clas,
                     const std::string& id, int idNum, eMinecraftColour color1,
                     eMinecraftColour color2, int nameId) {
    setId(createFn, clas, id, idNum);

    idsSpawnableInCreative.insert(
        std::unordered_map<int, SpawnableMobInfo*>::value_type(
            idNum, new SpawnableMobInfo(idNum, color1, color2, nameId)));
}

void EntityIO::staticCtor() {
    setId(ItemEntity::create, eTYPE_ITEMENTITY, "Item", 1);
    setId(ExperienceOrb::create, eTYPE_EXPERIENCEORB, "XPOrb", 2);

    setId(LeashFenceKnotEntity::create, eTYPE_LEASHFENCEKNOT, "LeashKnot", 8);
    setId(Painting::create, eTYPE_PAINTING, "Painting", 9);
    setId(Arrow::create, eTYPE_ARROW, "Arrow", 10);
    setId(Snowball::create, eTYPE_SNOWBALL, "Snowball", 11);
    setId(LargeFireball::create, eTYPE_FIREBALL, "Fireball", 12);
    setId(SmallFireball::create, eTYPE_SMALL_FIREBALL, "SmallFireball", 13);
    setId(ThrownEnderpearl::create, eTYPE_THROWNENDERPEARL, "ThrownEnderpearl",
          14);
    setId(EyeOfEnderSignal::create, eTYPE_EYEOFENDERSIGNAL, "EyeOfEnderSignal",
          15);
    setId(ThrownPotion::create, eTYPE_THROWNPOTION, "ThrownPotion", 16);
    setId(ThrownExpBottle::create, eTYPE_THROWNEXPBOTTLE, "ThrownExpBottle",
          17);
    setId(ItemFrame::create, eTYPE_ITEM_FRAME, "ItemFrame", 18);
    setId(WitherSkull::create, eTYPE_WITHER_SKULL, "WitherSkull", 19);

    setId(PrimedTnt::create, eTYPE_PRIMEDTNT, "PrimedTnt", 20);
    setId(FallingTile::create, eTYPE_FALLINGTILE, "FallingSand", 21);

    setId(FireworksRocketEntity::create, eTYPE_FIREWORKS_ROCKET,
          "FireworksRocketEntity", 22);

    setId(Boat::create, eTYPE_BOAT, "Boat", 41);
    setId(MinecartRideable::create, eTYPE_MINECART_RIDEABLE, "MinecartRideable",
          42);
    setId(MinecartChest::create, eTYPE_MINECART_CHEST, "MinecartChest", 43);
    setId(MinecartFurnace::create, eTYPE_MINECART_FURNACE, "MinecartFurnace",
          44);
    setId(MinecartTNT::create, eTYPE_MINECART_TNT, "MinecartTNT", 45);
    setId(MinecartHopper::create, eTYPE_MINECART_HOPPER, "MinecartHopper", 46);
    setId(MinecartSpawner::create, eTYPE_MINECART_SPAWNER, "MinecartSpawner",
          47);

    setId(Mob::create, eTYPE_MOB, "Mob", 48);
    setId(Monster::create, eTYPE_MONSTER, "Monster", 49);

    setId(Creeper::create, eTYPE_CREEPER, "Creeper", 50,
          eMinecraftColour_Mob_Creeper_Colour1,
          eMinecraftColour_Mob_Creeper_Colour2, IDS_CREEPER);
    setId(Skeleton::create, eTYPE_SKELETON, "Skeleton", 51,
          eMinecraftColour_Mob_Skeleton_Colour1,
          eMinecraftColour_Mob_Skeleton_Colour2, IDS_SKELETON);
    setId(Spider::create, eTYPE_SPIDER, "Spider", 52,
          eMinecraftColour_Mob_Spider_Colour1,
          eMinecraftColour_Mob_Spider_Colour2, IDS_SPIDER);
    setId(Giant::create, eTYPE_GIANT, "Giant", 53);
    setId(Zombie::create, eTYPE_ZOMBIE, "Zombie", 54,
          eMinecraftColour_Mob_Zombie_Colour1,
          eMinecraftColour_Mob_Zombie_Colour2, IDS_ZOMBIE);
    setId(Slime::create, eTYPE_SLIME, "Slime", 55,
          eMinecraftColour_Mob_Slime_Colour1,
          eMinecraftColour_Mob_Slime_Colour2, IDS_SLIME);
    setId(Ghast::create, eTYPE_GHAST, "Ghast", 56,
          eMinecraftColour_Mob_Ghast_Colour1,
          eMinecraftColour_Mob_Ghast_Colour2, IDS_GHAST);
    setId(PigZombie::create, eTYPE_PIGZOMBIE, "PigZombie", 57,
          eMinecraftColour_Mob_PigZombie_Colour1,
          eMinecraftColour_Mob_PigZombie_Colour2, IDS_PIGZOMBIE);
    setId(EnderMan::create, eTYPE_ENDERMAN, "Enderman", 58,
          eMinecraftColour_Mob_Enderman_Colour1,
          eMinecraftColour_Mob_Enderman_Colour2, IDS_ENDERMAN);
    setId(CaveSpider::create, eTYPE_CAVESPIDER, "CaveSpider", 59,
          eMinecraftColour_Mob_CaveSpider_Colour1,
          eMinecraftColour_Mob_CaveSpider_Colour2, IDS_CAVE_SPIDER);
    setId(Silverfish::create, eTYPE_SILVERFISH, "Silverfish", 60,
          eMinecraftColour_Mob_Silverfish_Colour1,
          eMinecraftColour_Mob_Silverfish_Colour2, IDS_SILVERFISH);
    setId(Blaze::create, eTYPE_BLAZE, "Blaze", 61,
          eMinecraftColour_Mob_Blaze_Colour1,
          eMinecraftColour_Mob_Blaze_Colour2, IDS_BLAZE);
    setId(LavaSlime::create, eTYPE_LAVASLIME, "LavaSlime", 62,
          eMinecraftColour_Mob_LavaSlime_Colour1,
          eMinecraftColour_Mob_LavaSlime_Colour2, IDS_LAVA_SLIME);
    setId(EnderDragon::create, eTYPE_ENDERDRAGON, "EnderDragon", 63,
          eMinecraftColour_Mob_Enderman_Colour1,
          eMinecraftColour_Mob_Enderman_Colour1, IDS_ENDERDRAGON);
    setId(WitherBoss::create, eTYPE_WITHERBOSS, "WitherBoss", 64);
    setId(Bat::create, eTYPE_BAT, "Bat", 65, eMinecraftColour_Mob_Bat_Colour1,
          eMinecraftColour_Mob_Bat_Colour2, IDS_BAT);
    setId(Witch::create, eTYPE_WITCH, "Witch", 66,
          eMinecraftColour_Mob_Witch_Colour1,
          eMinecraftColour_Mob_Witch_Colour2, IDS_WITCH);

    setId(Pig::create, eTYPE_PIG, "Pig", 90, eMinecraftColour_Mob_Pig_Colour1,
          eMinecraftColour_Mob_Pig_Colour2, IDS_PIG);
    setId(Sheep::create, eTYPE_SHEEP, "Sheep", 91,
          eMinecraftColour_Mob_Sheep_Colour1,
          eMinecraftColour_Mob_Sheep_Colour2, IDS_SHEEP);
    setId(Cow::create, eTYPE_COW, "Cow", 92, eMinecraftColour_Mob_Cow_Colour1,
          eMinecraftColour_Mob_Cow_Colour2, IDS_COW);
    setId(Chicken::create, eTYPE_CHICKEN, "Chicken", 93,
          eMinecraftColour_Mob_Chicken_Colour1,
          eMinecraftColour_Mob_Chicken_Colour2, IDS_CHICKEN);
    setId(Squid::create, eTYPE_SQUID, "Squid", 94,
          eMinecraftColour_Mob_Squid_Colour1,
          eMinecraftColour_Mob_Squid_Colour2, IDS_SQUID);
    setId(Wolf::create, eTYPE_WOLF, "Wolf", 95,
          eMinecraftColour_Mob_Wolf_Colour1, eMinecraftColour_Mob_Wolf_Colour2,
          IDS_WOLF);
    setId(MushroomCow::create, eTYPE_MUSHROOMCOW, "MushroomCow", 96,
          eMinecraftColour_Mob_MushroomCow_Colour1,
          eMinecraftColour_Mob_MushroomCow_Colour2, IDS_MUSHROOM_COW);
    setId(SnowMan::create, eTYPE_SNOWMAN, "SnowMan", 97);
    setId(Ocelot::create, eTYPE_OCELOT, "Ozelot", 98,
          eMinecraftColour_Mob_Ocelot_Colour1,
          eMinecraftColour_Mob_Ocelot_Colour2, IDS_OZELOT);
    setId(VillagerGolem::create, eTYPE_VILLAGERGOLEM, "VillagerGolem", 99);
    setId(EntityHorse::create, eTYPE_HORSE, "EntityHorse", 100,
          eMinecraftColour_Mob_Horse_Colour1,
          eMinecraftColour_Mob_Horse_Colour2, IDS_HORSE);

    setId(Villager::create, eTYPE_VILLAGER, "Villager", 120,
          eMinecraftColour_Mob_Villager_Colour1,
          eMinecraftColour_Mob_Villager_Colour2, IDS_VILLAGER);

    setId(EnderCrystal::create, eTYPE_ENDER_CRYSTAL, "EnderCrystal", 200);

    // 4J Added
    setId(DragonFireball::create, eTYPE_DRAGON_FIREBALL, "DragonFireball",
          1000);

    // 4J-PB - moved to allow the eggs to be named and coloured in the Creative
    // Mode menu 4J Added for custom spawn eggs
    setId(EntityHorse::create, eTYPE_HORSE, "EntityHorse",
          100 | ((EntityHorse::TYPE_DONKEY + 1) << 12),
          eMinecraftColour_Mob_Horse_Colour1,
          eMinecraftColour_Mob_Horse_Colour2, IDS_DONKEY);
    setId(EntityHorse::create, eTYPE_HORSE, "EntityHorse",
          100 | ((EntityHorse::TYPE_MULE + 1) << 12),
          eMinecraftColour_Mob_Horse_Colour1,
          eMinecraftColour_Mob_Horse_Colour2, IDS_MULE);

#ifndef _CONTENT_PACKAGE
    setId(EntityHorse::create, eTYPE_HORSE, "EntityHorse",
          100 | ((EntityHorse::TYPE_SKELETON + 1) << 12),
          eMinecraftColour_Mob_Horse_Colour1,
          eMinecraftColour_Mob_Horse_Colour2, IDS_SKELETON_HORSE);
    setId(EntityHorse::create, eTYPE_HORSE, "EntityHorse",
          100 | ((EntityHorse::TYPE_UNDEAD + 1) << 12),
          eMinecraftColour_Mob_Horse_Colour1,
          eMinecraftColour_Mob_Horse_Colour2, IDS_ZOMBIE_HORSE);
    setId(Ocelot::create, eTYPE_OCELOT, "Ozelot",
          98 | ((Ocelot::TYPE_BLACK + 1) << 12),
          eMinecraftColour_Mob_Ocelot_Colour1,
          eMinecraftColour_Mob_Ocelot_Colour2, IDS_OZELOT);
    setId(Ocelot::create, eTYPE_OCELOT, "Ozelot",
          98 | ((Ocelot::TYPE_RED + 1) << 12),
          eMinecraftColour_Mob_Ocelot_Colour1,
          eMinecraftColour_Mob_Ocelot_Colour2, IDS_OZELOT);
    setId(Ocelot::create, eTYPE_OCELOT, "Ozelot",
          98 | ((Ocelot::TYPE_SIAMESE + 1) << 12),
          eMinecraftColour_Mob_Ocelot_Colour1,
          eMinecraftColour_Mob_Ocelot_Colour2, IDS_OZELOT);
    setId(Spider::create, eTYPE_SPIDER, "Spider", 52 | (2 << 12),
          eMinecraftColour_Mob_Spider_Colour1,
          eMinecraftColour_Mob_Spider_Colour2, IDS_SKELETON);
#endif
}

std::shared_ptr<Entity> EntityIO::newEntity(const std::string& id,
                                            Level* level) {
    std::shared_ptr<Entity> entity;

    auto it = idCreateMap->find(id);
    if (it != idCreateMap->end()) {
        entityCreateFn create = it->second;
        if (create != nullptr) entity = std::shared_ptr<Entity>(create(level));
        if ((entity != nullptr) && entity->GetType() == eTYPE_ENDERDRAGON) {
            std::dynamic_pointer_cast<EnderDragon>(entity)
                ->AddParts();  // 4J added to finalise creation
        }
    }

    return entity;
}

std::shared_ptr<Entity> EntityIO::loadStatic(CompoundTag* tag, Level* level) {
    std::shared_ptr<Entity> entity;

    if (tag->getString("id").compare("Minecart") == 0) {
        // I don't like this any more than you do. Sadly, compatibility...

        switch (tag->getInt("Type")) {
            case Minecart::TYPE_CHEST:
                tag->putString("id", "MinecartChest");
                break;
            case Minecart::TYPE_FURNACE:
                tag->putString("id", "MinecartFurnace");
                break;
            case Minecart::TYPE_RIDEABLE:
                tag->putString("id", "MinecartRideable");
                break;
        }

        tag->remove("Type");
    }

    auto it = idCreateMap->find(tag->getString("id"));
    if (it != idCreateMap->end()) {
        entityCreateFn create = it->second;
        if (create != nullptr) entity = std::shared_ptr<Entity>(create(level));
        if ((entity != nullptr) && entity->GetType() == eTYPE_ENDERDRAGON) {
            std::dynamic_pointer_cast<EnderDragon>(entity)
                ->AddParts();  // 4J added to finalise creation
        }
    }

    if (entity != nullptr) {
        entity->load(tag);
    } else {
#ifdef _DEBUG
        Log::info("Skipping Entity with id %s\n", tag->getString("id").c_str());
#endif
    }
    return entity;
}

std::shared_ptr<Entity> EntityIO::newById(int id, Level* level) {
    std::shared_ptr<Entity> entity;

    auto it = numCreateMap->find(id);
    if (it != numCreateMap->end()) {
        entityCreateFn create = it->second;
        if (create != nullptr) entity = std::shared_ptr<Entity>(create(level));
        if ((entity != nullptr) && entity->GetType() == eTYPE_ENDERDRAGON) {
            std::dynamic_pointer_cast<EnderDragon>(entity)
                ->AddParts();  // 4J added to finalise creation
        }
    }

    if (entity != nullptr) {
    } else {
        // printf("Skipping Entity with id %d\n", id ) ;
    }
    return entity;
}

std::shared_ptr<Entity> EntityIO::newByEnumType(eINSTANCEOF eType,
                                                Level* level) {
    std::shared_ptr<Entity> entity;

    std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash,
                       eINSTANCEOFKeyEq>::iterator it =
        classNumMap->find(eType);
    if (it != classNumMap->end()) {
        auto it2 = numCreateMap->find(it->second);
        if (it2 != numCreateMap->end()) {
            entityCreateFn create = it2->second;
            if (create != nullptr)
                entity = std::shared_ptr<Entity>(create(level));
            if ((entity != nullptr) && entity->GetType() == eTYPE_ENDERDRAGON) {
                std::dynamic_pointer_cast<EnderDragon>(entity)
                    ->AddParts();  // 4J added to finalise creation
            }
        }
    }

    return entity;
}

int EntityIO::getId(std::shared_ptr<Entity> entity) {
    std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash,
                       eINSTANCEOFKeyEq>::iterator it =
        classNumMap->find(entity->GetType());
    return (*it).second;
}

std::string EntityIO::getEncodeId(std::shared_ptr<Entity> entity) {
    std::unordered_map<eINSTANCEOF, std::string, eINSTANCEOFKeyHash,
                       eINSTANCEOFKeyEq>::iterator it =
        classIdMap->find(entity->GetType());
    if (it != classIdMap->end())
        return (*it).second;
    else
        return "";
}

int EntityIO::getId(const std::string& encodeId) {
    auto it = idNumMap->find(encodeId);
    if (it == idNumMap->end()) {
        // defaults to pig...
        return 90;
    }
    return it->second;
}

std::string EntityIO::getEncodeId(int entityIoValue) {
    // Class<? extends Entity> class1 = numClassMap.get(entityIoValue);
    // if (class1 != null)
    //{
    // return classIdMap.get(class1);
    // }

    auto it = numClassMap->find(entityIoValue);
    if (it != numClassMap->end()) {
        std::unordered_map<eINSTANCEOF, std::string, eINSTANCEOFKeyHash,
                           eINSTANCEOFKeyEq>::iterator classIdIt =
            classIdMap->find(it->second);
        if (classIdIt != classIdMap->end())
            return (*classIdIt).second;
        else
            return "";
    }

    return "";
}

int EntityIO::getNameId(int entityIoValue) {
    int id = -1;

    auto it = idsSpawnableInCreative.find(entityIoValue);
    if (it != idsSpawnableInCreative.end()) {
        id = it->second->nameId;
    }

    return id;
}

eINSTANCEOF EntityIO::getType(const std::string& idString) {
    auto it = numClassMap->find(getId(idString));
    if (it != numClassMap->end()) {
        return it->second;
    }
    return eTYPE_NOTSET;
}

eINSTANCEOF EntityIO::getClass(int id) {
    auto it = numClassMap->find(id);
    if (it != numClassMap->end()) {
        return it->second;
    }
    return eTYPE_NOTSET;
}

int EntityIO::eTypeToIoid(eINSTANCEOF eType) {
    std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash,
                       eINSTANCEOFKeyEq>::iterator it =
        classNumMap->find(eType);
    if (it != classNumMap->end()) return it->second;
    return -1;
}
