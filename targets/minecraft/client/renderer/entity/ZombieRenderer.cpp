#include "ZombieRenderer.h"

#include <math.h>

#include <numbers>

#include "java/Class.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/model/VillagerZombieModel.h"
#include "minecraft/client/model/ZombieModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/HumanoidMobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/Zombie.h"

ResourceLocation ZombieRenderer::ZOMBIE_PIGMAN_LOCATION(TN_MOB_PIGZOMBIE);
ResourceLocation ZombieRenderer::ZOMBIE_LOCATION(TN_MOB_ZOMBIE);
ResourceLocation ZombieRenderer::ZOMBIE_VILLAGER_LOCATION(
    TN_MOB_ZOMBIE_VILLAGER);

ZombieRenderer::ZombieRenderer()
    : HumanoidMobRenderer(new ZombieModel(), .5f, 1.0f) {
    modelVersion = 1;
    defaultModel = humanoidModel;
    villagerModel = new VillagerZombieModel();

    defaultArmorParts1 = nullptr;
    defaultArmorParts2 = nullptr;

    villagerArmorParts1 = nullptr;
    villagerArmorParts2 = nullptr;

    createArmorParts();
}

void ZombieRenderer::createArmorParts() {
    delete armorParts1;
    delete armorParts2;

    armorParts1 = new ZombieModel(1.0f, true);
    armorParts2 = new ZombieModel(0.5f, true);

    defaultArmorParts1 = armorParts1;
    defaultArmorParts2 = armorParts2;

    villagerArmorParts1 = new VillagerZombieModel(1.0f, 0, true);
    villagerArmorParts2 = new VillagerZombieModel(0.5f, 0, true);
}

int ZombieRenderer::prepareArmor(std::shared_ptr<LivingEntity> _mob, int layer,
                                 float a) {
    std::shared_ptr<Zombie> mob = std::dynamic_pointer_cast<Zombie>(_mob);
    swapArmor(mob);
    return HumanoidMobRenderer::prepareArmor(_mob, layer, a);
}

void ZombieRenderer::render(std::shared_ptr<Entity> _mob, double x, double y,
                            double z, float rot, float a) {
    std::shared_ptr<Zombie> mob = std::dynamic_pointer_cast<Zombie>(_mob);
    swapArmor(mob);
    HumanoidMobRenderer::render(_mob, x, y, z, rot, a);
}

ResourceLocation* ZombieRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    std::shared_ptr<Zombie> mob = std::dynamic_pointer_cast<Zombie>(entity);

    // TODO Extract this clusterfck into 3 renderers
    if (entity->instanceof (eTYPE_PIGZOMBIE)) {
        return &ZOMBIE_PIGMAN_LOCATION;
    }

    if (mob->isVillager()) {
        return &ZOMBIE_VILLAGER_LOCATION;
    }
    return &ZOMBIE_LOCATION;
}

void ZombieRenderer::additionalRendering(std::shared_ptr<LivingEntity> _mob,
                                         float a) {
    std::shared_ptr<Zombie> mob = std::dynamic_pointer_cast<Zombie>(_mob);
    swapArmor(mob);
    HumanoidMobRenderer::additionalRendering(_mob, a);
}

void ZombieRenderer::swapArmor(std::shared_ptr<Zombie> mob) {
    if (mob->isVillager()) {
        // if (modelVersion != villagerModel->version())
        //{
        //	villagerModel = new VillagerZombieModel();
        //	modelVersion = villagerModel->version();
        //	villagerArmorParts1 = new VillagerZombieModel(1.0f, 0, true);
        //	villagerArmorParts2 = new VillagerZombieModel(0.5f, 0, true);
        // }
        model = villagerModel;
        armorParts1 = villagerArmorParts1;
        armorParts2 = villagerArmorParts2;
    } else {
        model = defaultModel;
        armorParts1 = defaultArmorParts1;
        armorParts2 = defaultArmorParts2;
    }

    humanoidModel = (HumanoidModel*)model;
}

void ZombieRenderer::setupRotations(std::shared_ptr<LivingEntity> _mob,
                                    float bob, float bodyRot, float a) {
    std::shared_ptr<Zombie> mob = std::dynamic_pointer_cast<Zombie>(_mob);
    if (mob->isConverting()) {
        bodyRot +=
            (float)(cos(mob->tickCount * 3.25) * std::numbers::pi * .25f);
    }
    HumanoidMobRenderer::setupRotations(mob, bob, bodyRot, a);
}