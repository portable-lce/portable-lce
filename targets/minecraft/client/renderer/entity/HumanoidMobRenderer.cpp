#include "HumanoidMobRenderer.h"
#include "platform/stubs.h"

#include <utility>
#include <vector>


#include "platform/renderer/renderer.h"
#include "EntityRenderDispatcher.h"
#include "util/StringHelpers.h"
#include "minecraft/Facing.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/renderer/tileentity/SkullTileRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/item/ArmorItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"
#include "nbt/CompoundTag.h"

const std::string HumanoidMobRenderer::MATERIAL_NAMES[5] = {
    "cloth", "chain", "iron", "diamond", "gold"};
std::map<std::string, ResourceLocation>
    HumanoidMobRenderer::ARMOR_LOCATION_CACHE;

void HumanoidMobRenderer::_init(HumanoidModel* humanoidModel, float scale) {
    this->humanoidModel = humanoidModel;
    this->_scale = scale;
    armorParts1 = nullptr;
    armorParts2 = nullptr;

    createArmorParts();
}

HumanoidMobRenderer::HumanoidMobRenderer(HumanoidModel* humanoidModel,
                                         float shadow)
    : MobRenderer(humanoidModel, shadow) {
    _init(humanoidModel, 1.0f);
}

HumanoidMobRenderer::HumanoidMobRenderer(HumanoidModel* humanoidModel,
                                         float shadow, float scale)
    : MobRenderer(humanoidModel, shadow) {
    _init(humanoidModel, scale);
}

ResourceLocation* HumanoidMobRenderer::getArmorLocation(ArmorItem* armorItem,
                                                        int layer) {
    return getArmorLocation(armorItem, layer, false);
}

ResourceLocation* HumanoidMobRenderer::getArmorLocation(ArmorItem* armorItem,
                                                        int layer,
                                                        bool overlay) {
    switch (armorItem->modelIndex) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
    };
    std::string path =
        std::string("armor/" + MATERIAL_NAMES[armorItem->modelIndex])
            .append("_")
            .append(toWString<int>(layer == 2 ? 2 : 1))
            .append((overlay ? "_b" : ""))
            .append(".png");

    std::map<std::string, ResourceLocation>::iterator it =
        ARMOR_LOCATION_CACHE.find(path);

    ResourceLocation* location;
    if (it != ARMOR_LOCATION_CACHE.end()) {
        location = &it->second;
    } else {
        ARMOR_LOCATION_CACHE.insert(std::pair<std::string, ResourceLocation>(
            path, ResourceLocation(path)));

        it = ARMOR_LOCATION_CACHE.find(path);
        location = &it->second;
    }

    return location;
}

void HumanoidMobRenderer::prepareSecondPassArmor(
    std::shared_ptr<LivingEntity> mob, int layer, float a) {
    std::shared_ptr<ItemInstance> itemInstance = mob->getArmor(3 - layer);
    if (itemInstance != nullptr) {
        Item* item = itemInstance->getItem();
        if (dynamic_cast<ArmorItem*>(item) != nullptr) {
            bindTexture(
                getArmorLocation(dynamic_cast<ArmorItem*>(item), layer, true));

            float brightness =
                SharedConstants::TEXTURE_LIGHTING ? 1 : mob->getBrightness(a);
            glColor3f(brightness, brightness, brightness);
        }
    }
}

void HumanoidMobRenderer::createArmorParts() {
    armorParts1 = new HumanoidModel(1.0f);
    armorParts2 = new HumanoidModel(0.5f);
}

int HumanoidMobRenderer::prepareArmor(std::shared_ptr<LivingEntity> _mob,
                                      int layer, float a) {
    std::shared_ptr<LivingEntity> mob =
        std::dynamic_pointer_cast<LivingEntity>(_mob);

    std::shared_ptr<ItemInstance> itemInstance = mob->getArmor(3 - layer);
    if (itemInstance != nullptr) {
        Item* item = itemInstance->getItem();
        if (dynamic_cast<ArmorItem*>(item) != nullptr) {
            ArmorItem* armorItem = dynamic_cast<ArmorItem*>(item);
            bindTexture(getArmorLocation(armorItem, layer));

            HumanoidModel* armor = layer == 2 ? armorParts2 : armorParts1;

            armor->head->visible = layer == 0;
            armor->hair->visible = layer == 0;
            armor->body->visible = layer == 1 || layer == 2;
            armor->arm0->visible = layer == 1;
            armor->arm1->visible = layer == 1;
            armor->leg0->visible = layer == 2 || layer == 3;
            armor->leg1->visible = layer == 2 || layer == 3;

            setArmor(armor);
            armor->attackTime = model->attackTime;
            armor->riding = model->riding;
            armor->young = model->young;

            float brightness =
                SharedConstants::TEXTURE_LIGHTING ? 1 : mob->getBrightness(a);
            if (armorItem->getMaterial() == ArmorItem::ArmorMaterial::CLOTH) {
                int color = armorItem->getColor(itemInstance);
                float red = (float)((color >> 16) & 0xFF) / 0xFF;
                float green = (float)((color >> 8) & 0xFF) / 0xFF;
                float blue = (float)(color & 0xFF) / 0xFF;
                glColor3f(brightness * red, brightness * green,
                          brightness * blue);

                if (itemInstance->isEnchanted()) return 0x1f;
                return 0x10;

            } else {
                glColor3f(brightness, brightness, brightness);
            }

            if (itemInstance->isEnchanted()) return 15;

            return 1;
        }
    }
    return -1;
}

void HumanoidMobRenderer::render(std::shared_ptr<Entity> _mob, double x,
                                 double y, double z, float rot, float a) {
    std::shared_ptr<LivingEntity> mob =
        std::dynamic_pointer_cast<LivingEntity>(_mob);

    float brightness =
        SharedConstants::TEXTURE_LIGHTING ? 1 : mob->getBrightness(a);
    glColor3f(brightness, brightness, brightness);
    std::shared_ptr<ItemInstance> item = mob->getCarriedItem();

    prepareCarriedItem(mob, item);

    double yp = y - mob->heightOffset;
    if (mob->isSneaking()) {
        yp -= 2 / 16.0f;
    }
    MobRenderer::render(mob, x, yp, z, rot, a);
    armorParts1->bowAndArrow = armorParts2->bowAndArrow =
        humanoidModel->bowAndArrow = false;
    armorParts1->sneaking = armorParts2->sneaking = humanoidModel->sneaking =
        false;
    armorParts1->holdingRightHand = armorParts2->holdingRightHand =
        humanoidModel->holdingRightHand = 0;
}

ResourceLocation* HumanoidMobRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    // TODO -- Figure out of we need some data in here
    return nullptr;
}

void HumanoidMobRenderer::prepareCarriedItem(
    std::shared_ptr<Entity> mob, std::shared_ptr<ItemInstance> item) {
    armorParts1->holdingRightHand = armorParts2->holdingRightHand =
        humanoidModel->holdingRightHand = item != nullptr ? 1 : 0;
    armorParts1->sneaking = armorParts2->sneaking = humanoidModel->sneaking =
        mob->isSneaking();
}

void HumanoidMobRenderer::additionalRendering(std::shared_ptr<LivingEntity> mob,
                                              float a) {
    float brightness =
        SharedConstants::TEXTURE_LIGHTING ? 1 : mob->getBrightness(a);
    glColor3f(brightness, brightness, brightness);
    std::shared_ptr<ItemInstance> item = mob->getCarriedItem();
    std::shared_ptr<ItemInstance> headGear = mob->getArmor(3);

    if (headGear != nullptr) {
        // don't render the pumpkin of skulls for the skins with that disabled
        // 4J-PB - need to disable rendering armour/skulls/pumpkins for some
        // special skins (Daleks)

        if ((mob->getAnimOverrideBitmask() &
             (1 << HumanoidModel::eAnim_DontRenderArmour)) == 0) {
            glPushMatrix();
            humanoidModel->head->translateTo(1 / 16.0f);

            if (headGear->getItem()->id < 256) {
                if (Tile::tiles[headGear->id] != nullptr &&
                    TileRenderer::canRender(
                        Tile::tiles[headGear->id]->getRenderShape())) {
                    float s = 10 / 16.0f;
                    glTranslatef(-0 / 16.0f, -4 / 16.0f, 0 / 16.0f);
                    glRotatef(90, 0, 1, 0);
                    glScalef(s, -s, -s);
                }

                this->entityRenderDispatcher->itemInHandRenderer->renderItem(
                    mob, headGear, 0);
            } else if (headGear->getItem()->id == Item::skull_Id) {
                float s = 17 / 16.0f;
                glScalef(s, -s, -s);

                std::string extra = "";
                if (headGear->hasTag() &&
                    headGear->getTag()->contains("SkullOwner")) {
                    extra = headGear->getTag()->getString("SkullOwner");
                }
                SkullTileRenderer::instance->renderSkull(
                    -0.5f, 0, -0.5f, Facing::UP, 180, headGear->getAuxValue(),
                    extra);
            }

            glPopMatrix();
        }
    }

    if (item != nullptr) {
        glPushMatrix();

        if (model->young) {
            float s = 0.5f;
            glTranslatef(0 / 16.0f, 10 / 16.0f, 0 / 16.0f);
            glRotatef(-20, -1, 0, 0);
            glScalef(s, s, s);
        }

        humanoidModel->arm0->translateTo(1 / 16.0f);
        glTranslatef(-1 / 16.0f, 7 / 16.0f, 1 / 16.0f);

        if (item->id < 256 &&
            TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
            float s = 8 / 16.0f;
            glTranslatef(-0 / 16.0f, 3 / 16.0f, -5 / 16.0f);
            s *= 0.75f;
            glRotatef(20, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
            glScalef(-s, -s, s);
        } else if (item->id == Item::bow_Id) {
            float s = 10 / 16.0f;
            glTranslatef(0 / 16.0f, 2 / 16.0f, 5 / 16.0f);
            glRotatef(-20, 0, 1, 0);
            glScalef(s, -s, s);
            glRotatef(-100, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
        } else if (Item::items[item->id]->isHandEquipped()) {
            float s = 10 / 16.0f;
            glTranslatef(0, 3 / 16.0f, 0);
            glScalef(s, -s, s);
            glRotatef(-100, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
        } else {
            float s = 6 / 16.0f;
            glTranslatef(+4 / 16.0f, +3 / 16.0f, -3 / 16.0f);
            glScalef(s, s, s);
            glRotatef(60, 0, 0, 1);
            glRotatef(-90, 1, 0, 0);
            glRotatef(20, 0, 0, 1);
        }

        this->entityRenderDispatcher->itemInHandRenderer->renderItem(mob, item,
                                                                     0);
        if (item->getItem()->hasMultipleSpriteLayers()) {
            this->entityRenderDispatcher->itemInHandRenderer->renderItem(
                mob, item, 1);
        }

        glPopMatrix();
    }
}

void HumanoidMobRenderer::scale(std::shared_ptr<LivingEntity> mob, float a) {
    glScalef(_scale, _scale, _scale);
}