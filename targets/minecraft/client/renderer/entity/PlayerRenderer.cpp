#include "minecraft/IGameServices.h"
#include "platform/stubs.h"
#include "PlayerRenderer.h"

#include <cmath>
#include <numbers>
#include <vector>


#include "platform/renderer/renderer.h"
#include "EntityRenderDispatcher.h"
#include "HumanoidMobRenderer.h"
#include "minecraft/GameEnums.h"
#include "java/Class.h"
#include "minecraft/Facing.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/model/geom/Model.h"
#include "minecraft/client/model/geom/ModelPart.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/EntityRenderer.h"
#include "minecraft/client/renderer/tileentity/SkullTileRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ArmorItem.h"
#include "minecraft/world/item/BowItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/UseAnim.h"
#include "minecraft/world/level/tile/Tile.h"
#include "nbt/CompoundTag.h"

const unsigned int PlayerRenderer::s_nametagColors[MINECRAFT_NET_MAX_PLAYERS] =
    {
        0xff000000,  // WHITE (represents the "white" player, but using black as
                     // the colour)
        0xff33cc33,  // GREEN
        0xffcc3333,  // RED
        0xff3333cc,  // BLUE
        0xffcc33cc,  // M_PINK
        0xffcc6633,  // ORANGE
        0xffcccc33,  // YELLOW
        0xff33dccc,  // TURQUOISE
};

ResourceLocation PlayerRenderer::DEFAULT_LOCATION =
    ResourceLocation(TN_MOB_CHAR);

PlayerRenderer::PlayerRenderer()
    : LivingEntityRenderer(new HumanoidModel(0), 0.5f) {
    humanoidModel = (HumanoidModel*)model;

    armorParts1 = new HumanoidModel(1.0f);
    armorParts2 = new HumanoidModel(0.5f);
}

unsigned int PlayerRenderer::getNametagColour(int index) {
    if (index >= 0 && index < MINECRAFT_NET_MAX_PLAYERS) {
        return s_nametagColors[index];
    }
    return 0xFF000000;
}

int PlayerRenderer::prepareArmor(std::shared_ptr<LivingEntity> _player,
                                 int layer, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(_player);

    // 4J-PB - need to disable rendering armour for some special skins (Daleks)
    unsigned int uiAnimOverrideBitmask = player->getAnimOverrideBitmask();
    if (uiAnimOverrideBitmask & (1 << HumanoidModel::eAnim_DontRenderArmour)) {
        return -1;
    }

    std::shared_ptr<ItemInstance> itemInstance =
        player->inventory->getArmor(3 - layer);
    if (itemInstance != nullptr) {
        Item* item = itemInstance->getItem();
        if (dynamic_cast<ArmorItem*>(item)) {
            ArmorItem* armorItem = dynamic_cast<ArmorItem*>(item);
            bindTexture(
                HumanoidMobRenderer::getArmorLocation(armorItem, layer));

            HumanoidModel* armor = layer == 2 ? armorParts2 : armorParts1;

            armor->head->visible = layer == 0;
            armor->hair->visible = layer == 0;
            armor->body->visible = layer == 1 || layer == 2;
            armor->arm0->visible = layer == 1;
            armor->arm1->visible = layer == 1;
            armor->leg0->visible = layer == 2 || layer == 3;
            armor->leg1->visible = layer == 2 || layer == 3;

            setArmor(armor);
            if (armor != nullptr) armor->attackTime = model->attackTime;
            if (armor != nullptr) armor->riding = model->riding;
            if (armor != nullptr) armor->young = model->young;

            float brightness = SharedConstants::TEXTURE_LIGHTING
                                   ? 1
                                   : player->getBrightness(a);
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

            if (itemInstance->isEnchanted()) return 0xf;

            return 1;
        }
    }
    return -1;
}

void PlayerRenderer::prepareSecondPassArmor(
    std::shared_ptr<LivingEntity> _player, int layer, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(_player);
    std::shared_ptr<ItemInstance> itemInstance =
        player->inventory->getArmor(3 - layer);
    if (itemInstance != nullptr) {
        Item* item = itemInstance->getItem();
        if (dynamic_cast<ArmorItem*>(item)) {
            ArmorItem* armorItem = dynamic_cast<ArmorItem*>(item);
            bindTexture(HumanoidMobRenderer::getArmorLocation((ArmorItem*)item,
                                                              layer, true));

            float brightness = SharedConstants::TEXTURE_LIGHTING
                                   ? 1
                                   : player->getBrightness(a);
            glColor3f(brightness, brightness, brightness);
        }
    }
}

void PlayerRenderer::render(std::shared_ptr<Entity> _mob, double x, double y,
                            double z, float rot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Player> mob = std::dynamic_pointer_cast<Player>(_mob);

    if (mob->hasInvisiblePrivilege()) return;

    std::shared_ptr<ItemInstance> item = mob->inventory->getSelected();
    armorParts1->holdingRightHand = armorParts2->holdingRightHand =
        humanoidModel->holdingRightHand = item != nullptr ? 1 : 0;
    if (item != nullptr) {
        if (mob->getUseItemDuration() > 0) {
            UseAnim anim = item->getUseAnimation();
            if (anim == UseAnim_block) {
                armorParts1->holdingRightHand = armorParts2->holdingRightHand =
                    humanoidModel->holdingRightHand = 3;
            } else if (anim == UseAnim_bow) {
                armorParts1->bowAndArrow = armorParts2->bowAndArrow =
                    humanoidModel->bowAndArrow = true;
            }
        }
    }
    // 4J added, for 3rd person view of eating
    if (item != nullptr && mob->getUseItemDuration() > 0 &&
        item->getUseAnimation() == UseAnim_eat) {
        // These factors are largely lifted from ItemInHandRenderer to try and
        // keep the 3rd person eating animation as similar as possible
        float t = (mob->getUseItemDuration() - a + 1);
        float swing = 1 - (t / item->getUseDuration());
        armorParts1->eating = armorParts2->eating = humanoidModel->eating =
            true;
        armorParts1->eating_t = armorParts2->eating_t =
            humanoidModel->eating_t = t;
        armorParts1->eating_swing = armorParts2->eating_swing =
            humanoidModel->eating_swing = swing;
    } else {
        armorParts1->eating = armorParts2->eating = humanoidModel->eating =
            false;
    }

    armorParts1->sneaking = armorParts2->sneaking = humanoidModel->sneaking =
        mob->isSneaking();

    double yp = y - mob->heightOffset;
    if (mob->isSneaking() && !mob->instanceof(eTYPE_LOCALPLAYER)) {
        yp -= 2 / 16.0f;
    }

    // Check if an idle animation is needed
    if (mob->getAnimOverrideBitmask() & (1 << HumanoidModel::eAnim_HasIdle)) {
        if (mob->isIdle()) {
            humanoidModel->idle = true;
            armorParts1->idle = true;
            armorParts2->idle = true;
        } else {
            humanoidModel->idle = false;
            armorParts1->idle = false;
            armorParts2->idle = false;
        }
    } else {
        humanoidModel->idle = false;
        armorParts1->idle = false;
        armorParts2->idle = false;
    }

    // 4J-PB - any additional parts to turn on for this player (skin dependent)
    std::vector<ModelPart*>* pAdditionalModelParts =
        mob->GetAdditionalModelParts();
    // turn them on
    if (pAdditionalModelParts != nullptr) {
        for (auto it = pAdditionalModelParts->begin();
             it != pAdditionalModelParts->end(); ++it) {
            ModelPart* pModelPart = *it;

            pModelPart->visible = true;
        }
    }

    LivingEntityRenderer::render(mob, x, yp, z, rot, a);

    // turn them off again
    if (pAdditionalModelParts && pAdditionalModelParts->size() != 0) {
        for (auto it = pAdditionalModelParts->begin();
             it != pAdditionalModelParts->end(); ++it) {
            ModelPart* pModelPart = *it;

            pModelPart->visible = false;
        }
    }
    armorParts1->bowAndArrow = armorParts2->bowAndArrow =
        humanoidModel->bowAndArrow = false;
    armorParts1->sneaking = armorParts2->sneaking = humanoidModel->sneaking =
        false;
    armorParts1->holdingRightHand = armorParts2->holdingRightHand =
        humanoidModel->holdingRightHand = 0;
}

void PlayerRenderer::additionalRendering(std::shared_ptr<LivingEntity> _mob,
                                         float a) {
    float brightness =
        SharedConstants::TEXTURE_LIGHTING ? 1 : _mob->getBrightness(a);
    glColor3f(brightness, brightness, brightness);

    LivingEntityRenderer::additionalRendering(_mob, a);
    LivingEntityRenderer::renderArrows(_mob, a);

    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Player> mob = std::dynamic_pointer_cast<Player>(_mob);

    std::shared_ptr<ItemInstance> headGear = mob->inventory->getArmor(3);
    if (headGear != nullptr) {
        // don't render the pumpkin for the skins
        unsigned int uiAnimOverrideBitmask =
            mob->getSkinAnimOverrideBitmask(mob->getCustomSkin());

        if ((uiAnimOverrideBitmask &
             (1 << HumanoidModel::eAnim_DontRenderArmour)) == 0) {
            glPushMatrix();
            humanoidModel->head->translateTo(1 / 16.0f);

            if (headGear->getItem()->id < 256) {
                if (TileRenderer::canRender(
                        Tile::tiles[headGear->id]->getRenderShape())) {
                    float s = 10 / 16.0f;
                    glTranslatef(-0 / 16.0f, -4 / 16.0f, 0 / 16.0f);
                    glRotatef(90, 0, 1, 0);
                    glScalef(s, -s, s);
                }

                entityRenderDispatcher->itemInHandRenderer->renderItem(
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

    // need to add a custom texture for deadmau5
    if (mob != nullptr && gameServices().isXuidDeadmau5(mob->getXuid()) &&
        bindTexture(mob->customTextureUrl, "")) {
        for (int i = 0; i < 2; i++) {
            float yr = (mob->yRotO + (mob->yRot - mob->yRotO) * a) -
                       (mob->yBodyRotO + (mob->yBodyRot - mob->yBodyRotO) * a);
            float xr = mob->xRotO + (mob->xRot - mob->xRotO) * a;
            glPushMatrix();
            glRotatef(yr, 0, 1, 0);
            glRotatef(xr, 1, 0, 0);
            glTranslatef((6 / 16.0f) * (i * 2 - 1), 0, 0);
            glTranslatef(0, -6 / 16.0f, 0);
            glRotatef(-xr, 1, 0, 0);
            glRotatef(-yr, 0, 1, 0);

            float s = 8 / 6.0f;
            glScalef(s, s, s);
            humanoidModel->renderEars(1 / 16.0f, true);
            glPopMatrix();
        }
    }

    // 4J: removed
    /*bool loaded = mob->getCloakTexture()->isLoaded();
bool b1 = !mob->isInvisible();
bool b2 = !mob->isCapeHidden();*/
    if (bindTexture(mob->customTextureUrl2, "") && !mob->isInvisible()) {
        glPushMatrix();
        glTranslatef(0, 0, 2 / 16.0f);

        double xd = (mob->xCloakO + (mob->xCloak - mob->xCloakO) * a) -
                    (mob->xo + (mob->x - mob->xo) * a);
        double yd = (mob->yCloakO + (mob->yCloak - mob->yCloakO) * a) -
                    (mob->yo + (mob->y - mob->yo) * a);
        double zd = (mob->zCloakO + (mob->zCloak - mob->zCloakO) * a) -
                    (mob->zo + (mob->z - mob->zo) * a);

        float yr = mob->yBodyRotO + (mob->yBodyRot - mob->yBodyRotO) * a;

        double xa = sinf(yr * std::numbers::pi / 180);
        double za = -cosf(yr * std::numbers::pi / 180);

        float flap = (float)yd * 10;
        if (flap < -6) flap = -6;
        if (flap > 32) flap = 32;
        float lean = (float)(xd * xa + zd * za) * 100;
        float lean2 = (float)(xd * za - zd * xa) * 100;
        if (lean < 0) lean = 0;

        float pow = mob->oBob + (mob->bob - mob->oBob) * a;

        flap +=
            sin((mob->walkDistO + (mob->walkDist - mob->walkDistO) * a) * 6) *
            32 * pow;
        if (mob->isSneaking()) {
            flap += 25;
        }

        // 4J Stu - Fix for sprint-flying causing the cape to rotate up by 180
        // degrees or more
        float xRot = 6.0f + lean / 2 + flap;
        if (xRot > 64.0f) xRot = 64.0f;

        glRotatef(xRot, 1, 0, 0);
        glRotatef(lean2 / 2, 0, 0, 1);
        glRotatef(-lean2 / 2, 0, 1, 0);
        glRotatef(180, 0, 1, 0);
        humanoidModel->renderCloak(1 / 16.0f, true);
        glPopMatrix();
    }

    std::shared_ptr<ItemInstance> item = mob->inventory->getSelected();

    if (item != nullptr) {
        glPushMatrix();
        humanoidModel->arm0->translateTo(1 / 16.0f);
        glTranslatef(-1 / 16.0f, 7 / 16.0f, 1 / 16.0f);

        if (mob->fishing != nullptr) {
            item = std::make_shared<ItemInstance>(Item::stick);
        }

        UseAnim anim = UseAnim_none;  // null;
        if (mob->getUseItemDuration() > 0) {
            anim = item->getUseAnimation();
        }

        if (item->id < 256 &&
            TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
            float s = 8 / 16.0f;
            glTranslatef(-0 / 16.0f, 3 / 16.0f, -5 / 16.0f);
            s *= 0.75f;
            glRotatef(20, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
            glScalef(-s, -s, s);
        } else if (item->id == Item::bow->id) {
            float s = 10 / 16.0f;
            glTranslatef(0 / 16.0f, 2 / 16.0f, 5 / 16.0f);
            glRotatef(-20, 0, 1, 0);
            glScalef(s, -s, s);
            glRotatef(-100, 1, 0, 0);
            glRotatef(45, 0, 1, 0);
        } else if (Item::items[item->id]->isHandEquipped()) {
            float s = 10 / 16.0f;
            if (Item::items[item->id]->isMirroredArt()) {
                glRotatef(180, 0, 0, 1);
                glTranslatef(0, -2 / 16.0f, 0);
            }
            if (mob->getUseItemDuration() > 0) {
                if (anim == UseAnim_block) {
                    glTranslatef(0.05f, 0, -0.1f);
                    glRotatef(-50, 0, 1, 0);
                    glRotatef(-10, 1, 0, 0);
                    glRotatef(-60, 0, 0, 1);
                }
            }
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

        if (item->getItem()->hasMultipleSpriteLayers()) {
            for (int layer = 0; layer <= 1; layer++) {
                int col = item->getItem()->getColor(item, layer);
                float red = ((col >> 16) & 0xff) / 255.0f;
                float g = ((col >> 8) & 0xff) / 255.0f;
                float b = ((col) & 0xff) / 255.0f;

                glColor4f(red, g, b, 1);
                this->entityRenderDispatcher->itemInHandRenderer->renderItem(
                    mob, item, layer, false);
            }
        } else {
            int col = item->getItem()->getColor(item, 0);
            float red = ((col >> 16) & 0xff) / 255.0f;
            float g = ((col >> 8) & 0xff) / 255.0f;
            float b = ((col) & 0xff) / 255.0f;

            glColor4f(red, g, b, 1);
            this->entityRenderDispatcher->itemInHandRenderer->renderItem(
                mob, item, 0);
        }

        glPopMatrix();
    }
}

void PlayerRenderer::renderNameTags(std::shared_ptr<LivingEntity> player,
                                    double x, double y, double z,
                                    std::string msg, float scale,
                                    double dist) {
    LivingEntityRenderer::renderNameTags(player, x, y, z, msg, scale, dist);
}

void PlayerRenderer::scale(std::shared_ptr<LivingEntity> player, float a) {
    float s = 15 / 16.0f;
    glScalef(s, s, s);
}

void PlayerRenderer::renderHand() {
    float brightness = 1;
    glColor3f(brightness, brightness, brightness);

    humanoidModel->m_uiAnimOverrideBitmask =
        Minecraft::GetInstance()->player->getAnimOverrideBitmask();
    armorParts1->eating = armorParts2->eating = humanoidModel->eating =
        humanoidModel->idle = false;
    humanoidModel->attackTime = 0;
    humanoidModel->setupAnim(0, 0, 0, 0, 0, 1 / 16.0f,
                             Minecraft::GetInstance()->player);
    // 4J-PB - does this skin have its arm0 disabled? (Dalek, etc)
    if ((humanoidModel->m_uiAnimOverrideBitmask &
         (1 << HumanoidModel::eAnim_DisableRenderArm0)) == 0) {
        humanoidModel->arm0->render(1 / 16.0f, true);
    }
}

void PlayerRenderer::setupPosition(std::shared_ptr<LivingEntity> _mob, double x,
                                   double y, double z) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Player> mob = std::dynamic_pointer_cast<Player>(_mob);

    if (mob->isAlive() && mob->isSleeping()) {
        LivingEntityRenderer::setupPosition(
            mob, x + mob->bedOffsetX, y + mob->bedOffsetY, z + mob->bedOffsetZ);

    } else {
        if (mob->isRiding() && (mob->getAnimOverrideBitmask() &
                                (1 << HumanoidModel::eAnim_SmallModel)) != 0) {
            y += 0.5f;
        }
        LivingEntityRenderer::setupPosition(mob, x, y, z);
    }
}

void PlayerRenderer::setupRotations(std::shared_ptr<LivingEntity> _mob,
                                    float bob, float bodyRot, float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Player> mob = std::dynamic_pointer_cast<Player>(_mob);

    if (mob->isAlive() && mob->isSleeping()) {
        glRotatef(mob->getSleepRotation(), 0, 1, 0);
        glRotatef(getFlipDegrees(mob), 0, 0, 1);
        glRotatef(270, 0, 1, 0);
    } else {
        LivingEntityRenderer::setupRotations(mob, bob, bodyRot, a);
    }
}

// 4J Added override to stop rendering shadow if player is invisible
void PlayerRenderer::renderShadow(std::shared_ptr<Entity> e, double x, double y,
                                  double z, float pow, float a) {
    if (gameServices().getGameHostOption(eGameHostOption_HostCanBeInvisible) > 0) {
        std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(e);
        if (player != nullptr && player->hasInvisiblePrivilege()) return;
    }
    EntityRenderer::renderShadow(e, x, y, z, pow, a);
}

// 4J Added override
void PlayerRenderer::bindTexture(std::shared_ptr<Entity> entity) {
    std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(entity);
    bindTexture(player->customTextureUrl, player->getTexture());
}

ResourceLocation* PlayerRenderer::getTextureLocation(
    std::shared_ptr<Entity> entity) {
    std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(entity);
    return new ResourceLocation((_TEXTURE_NAME)player->getTexture());
}