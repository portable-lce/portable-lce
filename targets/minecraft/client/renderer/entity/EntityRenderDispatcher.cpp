#include "minecraft/util/Log.h"
#include "platform/stubs.h"
#include "EntityRenderDispatcher.h"

#include <assert.h>

#include <cmath>
#include <utility>

#include "platform/renderer/renderer.h"
#include "ArrowRenderer.h"
#include "BatRenderer.h"
#include "BlazeRenderer.h"
#include "BoatRenderer.h"
#include "CaveSpiderRenderer.h"
#include "ChickenRenderer.h"
#include "CowRenderer.h"
#include "CreeperRenderer.h"
#include "DefaultRenderer.h"
#include "EnderCrystalRenderer.h"
#include "EnderDragonRenderer.h"
#include "EndermanRenderer.h"
#include "EntityRenderer.h"
#include "ExperienceOrbRenderer.h"
#include "FallingTileRenderer.h"
#include "FireballRenderer.h"
#include "FishingHookRenderer.h"
#include "GhastRenderer.h"
#include "GiantMobRenderer.h"
#include "HorseRenderer.h"
#include "ItemFrameRenderer.h"
#include "ItemRenderer.h"
#include "ItemSpriteRenderer.h"
#include "LavaSlimeRenderer.h"
#include "LeashKnotRenderer.h"
#include "LightningBoltRenderer.h"
#include "MinecartRenderer.h"
#include "MinecartSpawnerRenderer.h"
#include "MobRenderer.h"
#include "MushroomCowRenderer.h"
#include "OcelotRenderer.h"
#include "PaintingRenderer.h"
#include "PigRenderer.h"
#include "PlayerRenderer.h"
#include "SharedConstants.h"
#include "SheepRenderer.h"
#include "SilverfishRenderer.h"
#include "SkeletonRenderer.h"
#include "SlimeRenderer.h"
#include "SnowManRenderer.h"
#include "SpiderRenderer.h"
#include "SquidRenderer.h"
#include "TntMinecartRenderer.h"
#include "TntRenderer.h"
#include "VillagerGolemRenderer.h"
#include "VillagerRenderer.h"
#include "WitchRenderer.h"
#include "WitherBossRenderer.h"
#include "WitherSkullRenderer.h"
#include "WolfRenderer.h"
#include "ZombieRenderer.h"

#include "minecraft/client/model/ChickenModel.h"
#include "minecraft/client/model/CowModel.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/model/ModelHorse.h"
#include "minecraft/client/model/OcelotModel.h"
#include "minecraft/client/model/PigModel.h"
#include "minecraft/client/model/SheepFurModel.h"
#include "minecraft/client/model/SheepModel.h"
#include "minecraft/client/model/SlimeModel.h"
#include "minecraft/client/model/SquidModel.h"
#include "minecraft/client/model/WolfModel.h"
#include "minecraft/client/model/ZombieModel.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/PotionItem.h"
#include "minecraft/world/item/alchemy/PotionBrewing.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"

class Font;
class IconRegister;
class Options;
class Textures;

double EntityRenderDispatcher::xOff = 0.0;
double EntityRenderDispatcher::yOff = 0.0;
double EntityRenderDispatcher::zOff = 0.0;

EntityRenderDispatcher* EntityRenderDispatcher::instance = nullptr;

void EntityRenderDispatcher::staticCtor() {
    instance = new EntityRenderDispatcher();
}

EntityRenderDispatcher::EntityRenderDispatcher() {
    glEnable(GL_LIGHTING);
    renderers[eTYPE_SPIDER] = new SpiderRenderer();
    renderers[eTYPE_CAVESPIDER] = new CaveSpiderRenderer();
    renderers[eTYPE_PIG] =
        new PigRenderer(new PigModel(), new PigModel(0.5f), 0.7f);
    renderers[eTYPE_SHEEP] =
        new SheepRenderer(new SheepModel(), new SheepFurModel(), 0.7f);
    renderers[eTYPE_COW] = new CowRenderer(new CowModel(), 0.7f);
    renderers[eTYPE_MUSHROOMCOW] =
        new MushroomCowRenderer(new CowModel(), 0.7f);
    renderers[eTYPE_WOLF] =
        new WolfRenderer(new WolfModel(), new WolfModel(), 0.5f);
    renderers[eTYPE_CHICKEN] = new ChickenRenderer(new ChickenModel(), 0.3f);
    renderers[eTYPE_OCELOT] = new OcelotRenderer(new OcelotModel(), 0.4f);
    renderers[eTYPE_SILVERFISH] = new SilverfishRenderer();
    renderers[eTYPE_CREEPER] = new CreeperRenderer();
    renderers[eTYPE_ENDERMAN] = new EndermanRenderer();
    renderers[eTYPE_SNOWMAN] = new SnowManRenderer();
    renderers[eTYPE_SKELETON] = new SkeletonRenderer();
    renderers[eTYPE_WITCH] = new WitchRenderer();
    renderers[eTYPE_BLAZE] = new BlazeRenderer();
    renderers[eTYPE_ZOMBIE] = new ZombieRenderer();
    renderers[eTYPE_PIGZOMBIE] = new ZombieRenderer();
    renderers[eTYPE_SLIME] =
        new SlimeRenderer(new SlimeModel(16), new SlimeModel(0), 0.25f);
    renderers[eTYPE_LAVASLIME] = new LavaSlimeRenderer();
    renderers[eTYPE_PLAYER] = new PlayerRenderer();
    renderers[eTYPE_GIANT] = new GiantMobRenderer(new ZombieModel(), 0.5f, 6);
    renderers[eTYPE_GHAST] = new GhastRenderer();
    renderers[eTYPE_SQUID] = new SquidRenderer(new SquidModel(), 0.7f);
    renderers[eTYPE_VILLAGER] = new VillagerRenderer();
    renderers[eTYPE_VILLAGERGOLEM] = new VillagerGolemRenderer();
    renderers[eTYPE_BAT] = new BatRenderer();

    renderers[eTYPE_MOB] = new MobRenderer(new HumanoidModel(), 0.5f);

    renderers[eTYPE_ENDERDRAGON] = new EnderDragonRenderer();
    renderers[eTYPE_ENDER_CRYSTAL] = new EnderCrystalRenderer();

    renderers[eTYPE_WITHERBOSS] = new WitherBossRenderer();

    renderers[eTYPE_ENTITY] = new DefaultRenderer();
    renderers[eTYPE_PAINTING] = new PaintingRenderer();
    renderers[eTYPE_ITEM_FRAME] = new ItemFrameRenderer();
    renderers[eTYPE_LEASHFENCEKNOT] = new LeashKnotRenderer();
    renderers[eTYPE_ARROW] = new ArrowRenderer();
    renderers[eTYPE_SNOWBALL] = new ItemSpriteRenderer(Item::snowBall);
    renderers[eTYPE_THROWNENDERPEARL] =
        new ItemSpriteRenderer(Item::enderPearl);
    renderers[eTYPE_EYEOFENDERSIGNAL] =
        new ItemSpriteRenderer(Item::eyeOfEnder);
    renderers[eTYPE_THROWNEGG] = new ItemSpriteRenderer(Item::egg);
    renderers[eTYPE_THROWNPOTION] =
        new ItemSpriteRenderer(Item::potion, PotionBrewing::THROWABLE_MASK);
    renderers[eTYPE_THROWNEXPBOTTLE] = new ItemSpriteRenderer(Item::expBottle);
    renderers[eTYPE_FIREWORKS_ROCKET] = new ItemSpriteRenderer(Item::fireworks);
    renderers[eTYPE_LARGE_FIREBALL] = new FireballRenderer(2.0f);
    renderers[eTYPE_SMALL_FIREBALL] = new FireballRenderer(0.5f);
    renderers[eTYPE_DRAGON_FIREBALL] =
        new FireballRenderer(2.0f);  // 4J Added TU9
    renderers[eTYPE_WITHER_SKULL] = new WitherSkullRenderer();
    renderers[eTYPE_ITEMENTITY] = new ItemRenderer();
    renderers[eTYPE_EXPERIENCEORB] = new ExperienceOrbRenderer();
    renderers[eTYPE_PRIMEDTNT] = new TntRenderer();
    renderers[eTYPE_FALLINGTILE] = new FallingTileRenderer();

    renderers[eTYPE_MINECART_TNT] = new TntMinecartRenderer();
    renderers[eTYPE_MINECART_SPAWNER] = new MinecartSpawnerRenderer();
    renderers[eTYPE_MINECART_RIDEABLE] = new MinecartRenderer();

    renderers[eTYPE_MINECART_FURNACE] = new MinecartRenderer();
    renderers[eTYPE_MINECART_CHEST] = new MinecartRenderer();
    renderers[eTYPE_MINECART_HOPPER] = new MinecartRenderer();

    renderers[eTYPE_BOAT] = new BoatRenderer();
    renderers[eTYPE_FISHINGHOOK] = new FishingHookRenderer();

    renderers[eTYPE_HORSE] = new HorseRenderer(new ModelHorse(), .75f);

    renderers[eTYPE_LIGHTNINGBOLT] = new LightningBoltRenderer();
    glDisable(GL_LIGHTING);

    auto itEnd = renderers.end();
    for (classToRendererMap::iterator it = renderers.begin(); it != itEnd;
         it++) {
        it->second->init(this);
    }

    isGuiRender = false;  // 4J added
}

EntityRenderer* EntityRenderDispatcher::getRenderer(eINSTANCEOF e) {
    if ((e & eTYPE_PLAYER) == eTYPE_PLAYER) e = eTYPE_PLAYER;
    // EntityRenderer * r = renderers[e];
    auto it = renderers.find(e);  // 4J Stu - The .at and [] accessors
                                  // insert elements if they don't exist

    if (it == renderers.end()) {
        Log::info("Couldn't find renderer for entity of type %d\n", e);
        // New renderer mapping required in above table
        // assert(0);
        assert(0);
    }
    /* 4J - not doing this hierarchical search anymore. We need to explicitly
    add renderers for any eINSTANCEOF type that we want to be able to render if
    (it == renderers.end() && e != Entity::_class)
    {
    EntityRenderer *r = getRenderer(dynamic_cast<Entity::Class *>(
    e->getSuperclass() )); renderers.insert( classToRendererMap::value_type( e,
    r ) ); return r;
    //assert(false);
    }*/
    return it->second;
}

EntityRenderer* EntityRenderDispatcher::getRenderer(std::shared_ptr<Entity> e) {
    return getRenderer(e->GetType());
}

void EntityRenderDispatcher::prepare(
    Level* level, Textures* textures, Font* font,
    std::shared_ptr<LivingEntity> player,
    std::shared_ptr<LivingEntity> crosshairPickMob, Options* options, float a) {
    this->level = level;
    this->textures = textures;
    this->options = options;
    this->cameraEntity = player;
    this->font = font;
    this->crosshairPickMob = crosshairPickMob;

    if (player->isSleeping()) {
        int t = level->getTile(std::floor(player->x), std::floor(player->y),
                               std::floor(player->z));
        if (t == Tile::bed_Id) {
            int data =
                level->getData(std::floor(player->x), std::floor(player->y),
                               std::floor(player->z));

            int direction = data & 3;
            playerRotY = (float)(direction * 90 + 180);
            playerRotX = 0;
        }
    } else {
        playerRotY = player->yRotO + (player->yRot - player->yRotO) * a;
        playerRotX = player->xRotO + (player->xRot - player->xRotO) * a;
    }

    std::shared_ptr<Player> pl = std::dynamic_pointer_cast<Player>(player);
    if (pl->ThirdPersonView() == 2) {
        playerRotY += 180;
    }

    xPlayer = player->xOld + (player->x - player->xOld) * a;
    yPlayer = player->yOld + (player->y - player->yOld) * a;
    zPlayer = player->zOld + (player->z - player->zOld) * a;
}

void EntityRenderDispatcher::render(std::shared_ptr<Entity> entity, float a) {
    double x = entity->xOld + (entity->x - entity->xOld) * a;
    double y = entity->yOld + (entity->y - entity->yOld) * a;
    double z = entity->zOld + (entity->z - entity->zOld) * a;

    // Fix for #61057 - TU7: Gameplay: Boat is glitching when player float
    // forward and turning. Fix to handle the case that yRot and yRotO wrap over
    // the 0/360 line
    float rotDiff = entity->yRot - entity->yRotO;
    if (rotDiff > 180 || rotDiff < -180) {
        if (entity->yRot > entity->yRotO) {
            rotDiff = (entity->yRot - 360) - entity->yRotO;
        } else {
            rotDiff = entity->yRot - (entity->yRotO - 360);
        }
    }
    float r = entity->yRotO + (rotDiff)*a;

    int col = entity->getLightColor(a);
    if (entity->isOnFire()) {
        col = SharedConstants::FULLBRIGHT_LIGHTVALUE;
    }
    int u = col % 65536;
    int v = col / 65536;
    glMultiTexCoord2f(GL_TEXTURE1, u / 1.0f, v / 1.0f);
    glColor4f(1, 1, 1, 1);

    render(entity, x - xOff, y - yOff, z - zOff, r, a);
}

void EntityRenderDispatcher::render(std::shared_ptr<Entity> entity, double x,
                                    double y, double z, float rot, float a,
                                    bool bItemFrame, bool bRenderPlayerShadow) {
    EntityRenderer* renderer = getRenderer(entity);
    if (renderer != nullptr) {
        renderer->SetItemFrame(bItemFrame);

        renderer->render(entity, x, y, z, rot, a);
        renderer->postRender(entity, x, y, z, rot, a, bRenderPlayerShadow);
    }
}

double EntityRenderDispatcher::distanceToSqr(double x, double y, double z) {
    double xd = x - xPlayer;
    double yd = y - yPlayer;
    double zd = z - zPlayer;
    return xd * xd + yd * yd + zd * zd;
}

Font* EntityRenderDispatcher::getFont() { return font; }

void EntityRenderDispatcher::registerTerrainTextures(
    IconRegister* iconRegister) {
    // for (EntityRenderer<? extends Entity> renderer : renderers.values())
    for (auto it = renderers.begin(); it != renderers.end(); ++it) {
        EntityRenderer* renderer = it->second;
        renderer->registerTerrainTextures(iconRegister);
    }
}

void EntityRenderDispatcher::renderHitbox(std::shared_ptr<Entity> entity,
                                          double x, double y, double z,
                                          float rot, float a) {
    glDepthMask(false);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glPushMatrix();
    Tesselator* t = Tesselator::getInstance();

    t->begin();
    t->color(255, 255, 255, 32);

    double wnx = -entity->bbWidth / 2;
    double wnz = -entity->bbWidth / 2;
    double enx = entity->bbWidth / 2;
    double enz = -entity->bbWidth / 2;

    double wsx = -entity->bbWidth / 2;
    double wsz = entity->bbWidth / 2;
    double esx = entity->bbWidth / 2;
    double esz = entity->bbWidth / 2;

    double top = entity->bbHeight;

    t->vertex(x + wnx, y + top, z + wnz);
    t->vertex(x + wnx, y, z + wnz);
    t->vertex(x + enx, y, z + enz);
    t->vertex(x + enx, y + top, z + enz);

    t->vertex(x + esx, y + top, z + esz);
    t->vertex(x + esx, y, z + esz);
    t->vertex(x + wsx, y, z + wsz);
    t->vertex(x + wsx, y + top, z + wsz);

    t->vertex(x + enx, y + top, z + enz);
    t->vertex(x + enx, y, z + enz);
    t->vertex(x + esx, y, z + esz);
    t->vertex(x + esx, y + top, z + esz);

    t->vertex(x + wsx, y + top, z + wsz);
    t->vertex(x + wsx, y, z + wsz);
    t->vertex(x + wnx, y, z + wnz);
    t->vertex(x + wnx, y + top, z + wnz);

    t->end();
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(true);
}