#pragma once

#include <stdint.h>

#include <format>
#include <memory>
#include <vector>

#include "minecraft/stats/Console_Awards_enum.h"
#include "Stat.h"
#include "Stats.h"
#include "java/Class.h"

class DamageSource;
class ItemInstance;
class Mob;
class Player;
class Stat;

// #include "minecraft/world/damageSource/DamageSource.h"

// #include "minecraft/stats/Console_Awards_enum.h"

/**
        4J-JEV:
                Java version exposed the static instance of each stat.

                This was inconvient for me as I needed to structure the
   stats/achievements differently on Durango.

                Using getters like this means we can use different Stats easilly
   on different platforms and still have a convenient identifier to use to award
   them.
*/
class GenericStats {
private:  // Static instance.
    static GenericStats* instance;

public:
    static void setInstance(GenericStats* newInstance) {
        instance = newInstance;
    }
    static GenericStats* getInstance() { return instance; }

    // For retrieving a stat from an id.
    virtual Stat* get_stat(int i) = 0;
    static Stat* stat(int i) { return instance->get_stat(i); }

    // STATS - STATIC //

    static Stat* walkOneM() { return instance->get_walkOneM(); }
    static Stat* swimOneM() { return instance->get_swimOneM(); }
    static Stat* fallOneM() { return instance->get_fallOneM(); }
    static Stat* climbOneM() { return instance->get_climbOneM(); }
    static Stat* minecartOneM() { return instance->get_minecartOneM(); }
    static Stat* boatOneM() { return instance->get_boatOneM(); }
    static Stat* pigOneM() { return instance->get_pigOneM(); }
    static Stat* portalsCreated() { return instance->get_portalsCreated(); }
    static Stat* cowsMilked() { return instance->get_cowsMilked(); }
    static Stat* netherLavaCollected() {
        return instance->get_netherLavaCollected();
    }

    static Stat* killMob() { return instance->get_killMob(); }

    static Stat* killsZombie() { return instance->get_killsZombie(); }
    static Stat* killsSkeleton() { return instance->get_killsSkeleton(); }
    static Stat* killsCreeper() { return instance->get_killsCreeper(); }
    static Stat* killsSpider() { return instance->get_killsSpider(); }
    static Stat* killsSpiderJockey() {
        return instance->get_killsSpiderJockey();
    }
    static Stat* killsZombiePigman() {
        return instance->get_killsZombiePigman();
    }
    static Stat* killsSlime() { return instance->get_killsSlime(); }
    static Stat* killsGhast() { return instance->get_killsGhast(); }
    static Stat* killsNetherZombiePigman() {
        return instance->get_killsNetherZombiePigman();
    }

    static Stat* breedEntity(eINSTANCEOF entityId) {
        return instance->get_breedEntity(entityId);
    }
    static Stat* tamedEntity(eINSTANCEOF entityId) {
        return instance->get_tamedEntity(entityId);
    }
    static Stat* curedEntity(eINSTANCEOF entityId) {
        return instance->get_curedEntity(entityId);
    }
    static Stat* craftedEntity(eINSTANCEOF entityId) {
        return instance->get_craftedEntity(entityId);
    }
    static Stat* shearedEntity(eINSTANCEOF entityId) {
        return instance->get_shearedEntity(entityId);
    }

    static Stat* totalBlocksMined() { return instance->get_totalBlocksMined(); }
    static Stat* timePlayed() { return instance->get_timePlayed(); }

    static Stat* blocksPlaced(int blockId) {
        return instance->get_blocksPlaced(blockId);
    }
    static Stat* blocksMined(int blockId) {
        return instance->get_blocksMined(blockId);
    }
    static Stat* itemsCollected(int itemId, int itemAux) {
        return instance->get_itemsCollected(itemId, itemAux);
    }
    static Stat* itemsCrafted(int itemId) {
        return instance->get_itemsCrafted(itemId);
    }
    static Stat* itemsSmelted(int itemId) {
        return instance->get_itemsSmelted(itemId);
    }  // 4J-JEV: Diffentiation needed, when only one type of event should be
       // sent (eg iron smelting).
    static Stat* itemsUsed(int itemId) {
        return instance->get_itemsUsed(itemId);
    }
    static Stat* itemsBought(int itemId) {
        return instance->get_itemsBought(itemId);
    }

    static Stat* killsEnderdragon() { return instance->get_killsEnderdragon(); }
    static Stat* completeTheEnd() { return instance->get_completeTheEnd(); }

    static Stat* changedDimension(int from, int to) {
        return instance->get_changedDimension(from, to);
    }
    static Stat* enteredBiome(int biomeId) {
        return instance->get_enteredBiome(biomeId);
    }

    // ACHIEVEMENTS - STATIC //

    static Stat* achievement(eAward achievementId) {
        return instance->get_achievement(achievementId);
    }

    static Stat* openInventory();
    static Stat* mineWood();
    static Stat* buildWorkbench();
    static Stat* buildPickaxe();
    static Stat* buildFurnace();
    static Stat* acquireIron();
    static Stat* buildHoe();
    static Stat* makeBread();
    static Stat* bakeCake();
    static Stat* buildBetterPickaxe();
    static Stat* cookFish();
    static Stat* onARail();
    static Stat* buildSword();
    static Stat* killEnemy();
    static Stat* killCow();
    static Stat* flyPig();
    static Stat* snipeSkeleton();
    static Stat* diamonds();
    static Stat* ghast();
    static Stat* blazeRod();
    static Stat* potion();
    static Stat* theEnd();
    static Stat* winGame();
    static Stat* enchantments();
    static Stat* overkill();
    static Stat* bookcase();

    static Stat* leaderOfThePack();
    static Stat* MOARTools();
    static Stat* dispenseWithThis();
    static Stat* InToTheNether();

    static Stat* socialPost();
    static Stat* eatPorkChop();
    static Stat* play100Days();
    static Stat* arrowKillCreeper();
    static Stat* mine100Blocks();
    static Stat* kill10Creepers();

    static Stat* adventuringTime();  // Requires new Stat
    static Stat* repopulation();
    static Stat* diamondsToYou();   // +Durango
    static Stat* porkChop();        // Req Stat?
    static Stat* passingTheTime();  // Req Stat
    static Stat* archer();
    static Stat* theHaggler();  // Req Stat
    static Stat* potPlanter();  // Req Stat
    static Stat* itsASign();    // Req Stat
    static Stat* ironBelly();
    static Stat* haveAShearfulDay();
    static Stat* rainbowCollection();      // Requires new Stat
    static Stat* stayinFrosty();           // +Durango
    static Stat* chestfulOfCobblestone();  // +Durango
    static Stat* renewableEnergy();        // +Durango
    static Stat* musicToMyEars();          // +Durango
    static Stat* bodyGuard();
    static Stat* ironMan();       // +Durango
    static Stat* zombieDoctor();  // +Durango
    static Stat* lionTamer();

    // STAT PARAMS - STATIC //

    static std::vector<uint8_t> param_walk(int distance);
    static std::vector<uint8_t> param_swim(int distance);
    static std::vector<uint8_t> param_fall(int distance);
    static std::vector<uint8_t> param_climb(int distance);
    static std::vector<uint8_t> param_minecart(int distance);
    static std::vector<uint8_t> param_boat(int distance);
    static std::vector<uint8_t> param_pig(int distance);

    static std::vector<uint8_t> param_cowsMilked();

    static std::vector<uint8_t> param_blocksPlaced(int id, int data, int count);
    static std::vector<uint8_t> param_blocksMined(int id, int data, int count);
    static std::vector<uint8_t> param_itemsCollected(int id, int aux,
                                                     int count);
    static std::vector<uint8_t> param_itemsCrafted(int id, int aux, int count);
    static std::vector<uint8_t> param_itemsSmelted(int id, int aux, int cound);
    static std::vector<uint8_t> param_itemsUsed(
        std::shared_ptr<Player> plr, std::shared_ptr<ItemInstance> itm);
    static std::vector<uint8_t> param_itemsBought(int id, int aux, int count);

    static std::vector<uint8_t> param_mobKill(std::shared_ptr<Player> plr,
                                              std::shared_ptr<Mob> mob,
                                              DamageSource* dmgSrc);

    static std::vector<uint8_t> param_breedEntity(eINSTANCEOF mobType);
    static std::vector<uint8_t> param_tamedEntity(eINSTANCEOF mobType);
    static std::vector<uint8_t> param_curedEntity(eINSTANCEOF mobType);
    static std::vector<uint8_t> param_craftedEntity(eINSTANCEOF mobType);
    static std::vector<uint8_t> param_shearedEntity(eINSTANCEOF mobType);

    static std::vector<uint8_t> param_time(int timediff);

    static std::vector<uint8_t> param_changedDimension(int from, int to);
    static std::vector<uint8_t> param_enteredBiome(int biomeId);

    // static std::vector<uint8_t> param_achievement(eAward id);

    // static std::vector<uint8_t> param_ach_onARail();
    // static std::vector<uint8_t> param_overkill(int damage); //TODO
    // static std::vector<uint8_t> param_openInventory(int menuId);
    // static std::vector<uint8_t> param_chestfulOfCobblestone();
    // static std::vector<uint8_t> param_musicToMyEars(int recordId);

    static std::vector<uint8_t> param_noArgs();

    // STATIC + VIRTUAL - ACHIEVEMENT - PARAMS //

    static std::vector<uint8_t> param_openInventory();
    static std::vector<uint8_t> param_mineWood();
    static std::vector<uint8_t> param_buildWorkbench();
    static std::vector<uint8_t> param_buildPickaxe();
    static std::vector<uint8_t> param_buildFurnace();
    static std::vector<uint8_t> param_acquireIron();
    static std::vector<uint8_t> param_buildHoe();
    static std::vector<uint8_t> param_makeBread();
    static std::vector<uint8_t> param_bakeCake();
    static std::vector<uint8_t> param_buildBetterPickaxe();
    static std::vector<uint8_t> param_cookFish();
    static std::vector<uint8_t> param_onARail(int distance);
    static std::vector<uint8_t> param_buildSword();
    static std::vector<uint8_t> param_killEnemy();
    static std::vector<uint8_t> param_killCow();
    static std::vector<uint8_t> param_flyPig();
    static std::vector<uint8_t> param_snipeSkeleton();
    static std::vector<uint8_t> param_diamonds();
    static std::vector<uint8_t> param_ghast();
    static std::vector<uint8_t> param_blazeRod();
    static std::vector<uint8_t> param_potion();
    static std::vector<uint8_t> param_theEnd();
    static std::vector<uint8_t> param_winGame();
    static std::vector<uint8_t> param_enchantments();
    static std::vector<uint8_t> param_overkill(int dmg);
    static std::vector<uint8_t> param_bookcase();

    static std::vector<uint8_t> param_leaderOfThePack();
    static std::vector<uint8_t> param_MOARTools();
    static std::vector<uint8_t> param_dispenseWithThis();
    static std::vector<uint8_t> param_InToTheNether();

    static std::vector<uint8_t> param_socialPost();
    static std::vector<uint8_t> param_eatPorkChop();
    static std::vector<uint8_t> param_play100Days();
    static std::vector<uint8_t> param_arrowKillCreeper();
    static std::vector<uint8_t> param_mine100Blocks();
    static std::vector<uint8_t> param_kill10Creepers();

    static std::vector<uint8_t> param_adventuringTime();
    static std::vector<uint8_t> param_repopulation();
    static std::vector<uint8_t> param_porkChop();
    static std::vector<uint8_t> param_diamondsToYou();
    static std::vector<uint8_t> param_passingTheTime();
    static std::vector<uint8_t> param_archer();
    static std::vector<uint8_t> param_theHaggler();
    static std::vector<uint8_t> param_potPlanter();
    static std::vector<uint8_t> param_itsASign();
    static std::vector<uint8_t> param_ironBelly();
    static std::vector<uint8_t> param_haveAShearfulDay();
    static std::vector<uint8_t> param_rainbowCollection();
    static std::vector<uint8_t> param_stayinFrosty();
    static std::vector<uint8_t> param_chestfulOfCobblestone(int cobbleStone);
    static std::vector<uint8_t> param_renewableEnergy();
    static std::vector<uint8_t> param_musicToMyEars(int recordId);
    static std::vector<uint8_t> param_bodyGuard();
    static std::vector<uint8_t> param_ironMan();
    static std::vector<uint8_t> param_zombieDoctor();
    static std::vector<uint8_t> param_lionTamer();

protected:
    // ACHIEVEMENTS - VIRTUAL //

    virtual Stat* get_achievement(eAward achievementId);

    // STATS - VIRTUAL //

    virtual Stat* get_walkOneM();
    virtual Stat* get_swimOneM();
    virtual Stat* get_fallOneM();
    virtual Stat* get_climbOneM();
    virtual Stat* get_minecartOneM();
    virtual Stat* get_boatOneM();
    virtual Stat* get_pigOneM();
    virtual Stat* get_portalsCreated();
    virtual Stat* get_cowsMilked();
    virtual Stat* get_netherLavaCollected();

    virtual Stat* get_killMob();

    virtual Stat* get_killsZombie();
    virtual Stat* get_killsSkeleton();
    virtual Stat* get_killsCreeper();
    virtual Stat* get_killsSpider();
    virtual Stat* get_killsSpiderJockey();
    virtual Stat* get_killsZombiePigman();
    virtual Stat* get_killsSlime();
    virtual Stat* get_killsGhast();
    virtual Stat* get_killsNetherZombiePigman();

    virtual Stat* get_breedEntity(eINSTANCEOF entityId);
    virtual Stat* get_tamedEntity(eINSTANCEOF entityId);
    virtual Stat* get_curedEntity(eINSTANCEOF entityId);
    virtual Stat* get_craftedEntity(eINSTANCEOF entityId);
    virtual Stat* get_shearedEntity(eINSTANCEOF entityId);

    virtual Stat* get_totalBlocksMined();
    virtual Stat* get_timePlayed();

    virtual Stat* get_blocksPlaced(int blockId);
    virtual Stat* get_blocksMined(int blockId);
    virtual Stat* get_itemsCollected(int itemId, int itemAux);
    virtual Stat* get_itemsCrafted(int itemId);
    virtual Stat* get_itemsSmelted(int itemId);
    virtual Stat* get_itemsUsed(int itemId);
    virtual Stat* get_itemsBought(int itemId);

    virtual Stat* get_killsEnderdragon();
    virtual Stat* get_completeTheEnd();

    virtual Stat* get_changedDimension(int from, int to);
    virtual Stat* get_enteredBiome(int biomeId);

    // STAT PARAMS - VIRTUAL //

    virtual std::vector<uint8_t> getParam_walkOneM(int distance);
    virtual std::vector<uint8_t> getParam_swimOneM(int distance);
    virtual std::vector<uint8_t> getParam_fallOneM(int distance);
    virtual std::vector<uint8_t> getParam_climbOneM(int distance);
    virtual std::vector<uint8_t> getParam_minecartOneM(int distance);
    virtual std::vector<uint8_t> getParam_boatOneM(int distance);
    virtual std::vector<uint8_t> getParam_pigOneM(int distance);

    virtual std::vector<uint8_t> getParam_cowsMilked();

    virtual std::vector<uint8_t> getParam_blocksPlaced(int id, int data,
                                                       int count);
    virtual std::vector<uint8_t> getParam_blocksMined(int id, int data,
                                                      int count);
    virtual std::vector<uint8_t> getParam_itemsCollected(int id, int aux,
                                                         int count);
    virtual std::vector<uint8_t> getParam_itemsCrafted(int id, int aux,
                                                       int count);
    virtual std::vector<uint8_t> getParam_itemsSmelted(int id, int aux,
                                                       int count);
    virtual std::vector<uint8_t> getParam_itemsUsed(
        std::shared_ptr<Player> plr, std::shared_ptr<ItemInstance> itm);
    virtual std::vector<uint8_t> getParam_itemsBought(int id, int aux,
                                                      int count);

    virtual std::vector<uint8_t> getParam_mobKill(std::shared_ptr<Player> plr,
                                                  std::shared_ptr<Mob> mob,
                                                  DamageSource* dmgSrc);

    virtual std::vector<uint8_t> getParam_breedEntity(eINSTANCEOF entityId);
    virtual std::vector<uint8_t> getParam_tamedEntity(eINSTANCEOF entityId);
    virtual std::vector<uint8_t> getParam_curedEntity(eINSTANCEOF entityId);
    virtual std::vector<uint8_t> getParam_craftedEntity(eINSTANCEOF entityId);
    virtual std::vector<uint8_t> getParam_shearedEntity(eINSTANCEOF entityId);

    virtual std::vector<uint8_t> getParam_time(int timediff);

    virtual std::vector<uint8_t> getParam_changedDimension(int from, int to);
    virtual std::vector<uint8_t> getParam_enteredBiome(int biomeId);

    virtual std::vector<uint8_t> getParam_achievement(eAward id);

    virtual std::vector<uint8_t> getParam_onARail(int distance);
    virtual std::vector<uint8_t> getParam_overkill(int damage);
    virtual std::vector<uint8_t> getParam_openInventory(int menuId);
    virtual std::vector<uint8_t> getParam_chestfulOfCobblestone(
        int cobbleStone);
    virtual std::vector<uint8_t> getParam_musicToMyEars(int recordId);

    virtual std::vector<uint8_t> getParam_noArgs();
};

// Req Stats