#pragma once

#include <stdint.h>

#include <format>
#include <vector>

#include "GenericStats.h"
#include "minecraft/stats/Console_Awards_enum.h"
#include "java/Class.h"

class CommonStats : public GenericStats {
protected:
    using GenericStats::getParam_itemsUsed;

    virtual Stat* get_stat(int i);

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

    virtual Stat* get_killsZombie();
    virtual Stat* get_killsSkeleton();
    virtual Stat* get_killsCreeper();
    virtual Stat* get_killsSpider();
    virtual Stat* get_killsSpiderJockey();
    virtual Stat* get_killsZombiePigman();
    virtual Stat* get_killsSlime();
    virtual Stat* get_killsGhast();
    virtual Stat* get_killsNetherZombiePigman();

    virtual Stat* get_totalBlocksMined();
    virtual Stat* get_timePlayed();

    virtual Stat* get_breedEntity(eINSTANCEOF entityId);
    virtual Stat* get_tamedEntity(eINSTANCEOF entityId);
    virtual Stat* get_craftedEntity(eINSTANCEOF entityId);
    virtual Stat* get_shearedEntity(eINSTANCEOF entityId);

    virtual Stat* get_blocksPlaced(int blockId);
    virtual Stat* get_blocksMined(int blockId);
    virtual Stat* get_itemsCollected(int itemId, int itemAux);
    virtual Stat* get_itemsCrafted(int itemId);
    virtual Stat* get_itemsSmelted(int itemId);
    virtual Stat* get_itemsUsed(int itemId);
    virtual Stat* get_itemsBought(int itemId);

    virtual Stat* get_killsEnderdragon();
    virtual Stat* get_completeTheEnd();

    virtual Stat* get_enteredBiome(int biomeId);

    virtual Stat* get_achievement(eAward achievementId);

    virtual std::vector<uint8_t> getParam_walkOneM(int distance);
    virtual std::vector<uint8_t> getParam_swimOneM(int distance);
    virtual std::vector<uint8_t> getParam_fallOneM(int distance);
    virtual std::vector<uint8_t> getParam_climbOneM(int distance);
    virtual std::vector<uint8_t> getParam_minecartOneM(int distance);
    virtual std::vector<uint8_t> getParam_boatOneM(int distance);
    virtual std::vector<uint8_t> getParam_pigOneM(int distance);

    virtual std::vector<uint8_t> getParam_blocksMined(int id, int data,
                                                      int count);
    virtual std::vector<uint8_t> getParam_itemsCollected(int id, int aux,
                                                         int count);
    virtual std::vector<uint8_t> getParam_itemsCrafted(int id, int aux,
                                                       int count);
    virtual std::vector<uint8_t> getParam_itemsSmelted(int id, int aux,
                                                       int count);
    virtual std::vector<uint8_t> getParam_itemsUsed(int id, int aux, int count);
    virtual std::vector<uint8_t> getParam_itemsBought(int id, int aux,
                                                      int count);

    virtual std::vector<uint8_t> getParam_time(int timediff);

    virtual std::vector<uint8_t> getParam_noArgs();

protected:
    static std::vector<uint8_t> makeParam(int count = 1);

public:
    static int readParam(const std::vector<uint8_t>& paramBlob);
};