#include "Painting.h"

#include <memory>
#include <vector>

#include "java/Random.h"
#include "minecraft/IGameServices.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/HangingEntity.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "nbt/CompoundTag.h"

typedef Painting::Motive _Motive;
const _Motive* Painting::Motive::values[] = {
    new _Motive("Kebab", 16, 16, 0 * 16, 0 * 16),
    new _Motive("Aztec", 16, 16, 1 * 16, 0 * 16),      //
    new _Motive("Alban", 16, 16, 2 * 16, 0 * 16),      //
    new _Motive("Aztec2", 16, 16, 3 * 16, 0 * 16),     //
    new _Motive("Bomb", 16, 16, 4 * 16, 0 * 16),       //
    new _Motive("Plant", 16, 16, 5 * 16, 0 * 16),      //
    new _Motive("Wasteland", 16, 16, 6 * 16, 0 * 16),  //

    new _Motive("Pool", 32, 16, 0 * 16, 2 * 16),     //
    new _Motive("Courbet", 32, 16, 2 * 16, 2 * 16),  //
    new _Motive("Sea", 32, 16, 4 * 16, 2 * 16),      //
    new _Motive("Sunset", 32, 16, 6 * 16, 2 * 16),   //
    new _Motive("Creebet", 32, 16, 8 * 16, 2 * 16),  //

    new _Motive("Wanderer", 16, 32, 0 * 16, 4 * 16),  //
    new _Motive("Graham", 16, 32, 1 * 16, 4 * 16),    //

    new _Motive("Match", 32, 32, 0 * 16, 8 * 16),          //
    new _Motive("Bust", 32, 32, 2 * 16, 8 * 16),           //
    new _Motive("Stage", 32, 32, 4 * 16, 8 * 16),          //
    new _Motive("Void", 32, 32, 6 * 16, 8 * 16),           //
    new _Motive("SkullAndRoses", 32, 32, 8 * 16, 8 * 16),  //
    new _Motive("Wither", 32, 32, 10 * 16, 8 * 16),
    new _Motive("Fighters", 64, 32, 0 * 16, 6 * 16),  //

    new _Motive("Pointer", 64, 64, 0 * 16, 12 * 16),       //
    new _Motive("Pigscene", 64, 64, 4 * 16, 12 * 16),      //
    new _Motive("BurningSkull", 64, 64, 8 * 16, 12 * 16),  //

    new _Motive("Skeleton", 64, 48, 12 * 16, 4 * 16),    //
    new _Motive("DonkeyKong", 64, 48, 12 * 16, 7 * 16),  //
};

// 4J Stu - Rather than creating a new string object here I am just using the
// actual number value of the characters in "SkullandRoses" which should be the
// longest name from the above
const int Painting::Motive::MAX_MOTIVE_NAME_LENGTH =
    13;  // JAVA: "SkullAndRoses".length();

// 4J - added for common ctor code
void Painting::_init(Level* level) { motive = nullptr; };

Painting::Painting(Level* level) : HangingEntity(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();

    _init(level);
}

Painting::Painting(Level* level, int xTile, int yTile, int zTile, int dir)
    : HangingEntity(level, xTile, yTile, zTile, dir) {
    _init(level);

    // 4J Stu - If you use this ctor, then you need to call the
    // PaintingPostConstructor
}

// 4J Stu - Added this so that we can use some shared_ptr functions that were
// needed in the ctor 4J Stu - Added motive param for debugging/artists only
void Painting::PaintingPostConstructor(int dir, int motive) {
#ifndef _CONTENT_PACKAGE
    if (gameServices().debugArtToolsOn() && motive >= 0) {
        this->motive = (Motive*)Motive::values[motive];
        setDir(dir);
    } else
#endif
    {
        std::vector<Motive*>* survivableMotives = new std::vector<Motive*>();
        for (int i = 0; i < LAST_VALUE; i++) {
            this->motive = (Motive*)Motive::values[i];
            setDir(dir);
            if (survives()) {
                survivableMotives->push_back(this->motive);
            }
        }
        if (!survivableMotives->empty()) {
            this->motive = survivableMotives->at(
                random->nextInt((int)survivableMotives->size()));
        }
        setDir(dir);
    }
}

Painting::Painting(Level* level, int x, int y, int z, int dir,
                   std::string motiveName)
    : HangingEntity(level, x, y, z, dir) {
    _init(level);

    for (int i = 0; i < LAST_VALUE; i++) {
        if ((Motive::values[i])->name.compare(motiveName) == 0) {
            this->motive = (Motive*)Motive::values[i];
            break;
        }
    }
    setDir(dir);
}

void Painting::addAdditonalSaveData(CompoundTag* tag) {
    /// TODO Safe to cast to non-const type?
    tag->putString("Motive", motive->name);

    HangingEntity::addAdditonalSaveData(tag);
}

void Painting::readAdditionalSaveData(CompoundTag* tag) {
    std::string motiveName = tag->getString("Motive");
    std::vector<Motive*>::iterator it;
    for (int i = 0; i < LAST_VALUE; i++) {
        if (Motive::values[i]->name.compare(motiveName) == 0) {
            this->motive = (Motive*)Motive::values[i];
        }
    }
    if (this->motive == nullptr) motive = (Motive*)Motive::values[Kebab];

    HangingEntity::readAdditionalSaveData(tag);
}

int Painting::getWidth() { return motive->w; }

int Painting::getHeight() { return motive->h; }

void Painting::dropItem(std::shared_ptr<Entity> causedBy) {
    if ((causedBy != nullptr) && causedBy->instanceof (eTYPE_PLAYER)) {
        std::shared_ptr<Player> player =
            std::dynamic_pointer_cast<Player>(causedBy);
        if (player->abilities.instabuild) {
            return;
        }
    }

    spawnAtLocation(std::make_shared<ItemInstance>(Item::painting), 0.0f);
}