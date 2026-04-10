#pragma once
// using namespace std;

#include <memory>

#include "java/Class.h"
#include "minecraft/world/tutorial/TutorialEnum.h"

class Entity;
class ItemInstance;
class Tile;

#define TUTORIAL_HINT_MAX_MINE_REPEATS 20

class Level;
class Tutorial;

class TutorialHint {
public:
    enum eHintType {
        e_Hint_DiggerItem,
        e_Hint_HoldToMine,
        e_Hint_NoIngredients,
        e_Hint_ToolDamaged,
        e_Hint_TakeItem,
        e_Hint_Area,
        e_Hint_LookAtTile,
        e_Hint_LookAtEntity,
        e_Hint_SwimUp,
    };

protected:
    eHintType m_type;
    int m_descriptionId;
    Tutorial* m_tutorial;
    eTutorial_Hint m_id;

    int m_counter;
    Tile* m_lastTile;
    bool m_hintNeeded;
    bool m_allowFade;

public:
    TutorialHint(eTutorial_Hint id, Tutorial* tutorial, int descriptionId,
                 eHintType type, bool allowFade = true);
    virtual ~TutorialHint() {}

    eTutorial_Hint getId() { return m_id; }

    virtual int startDestroyBlock(std::shared_ptr<ItemInstance> item,
                                  Tile* tile);
    virtual int destroyBlock(Tile* tile);
    virtual int attack(std::shared_ptr<ItemInstance> item,
                       std::shared_ptr<Entity> entity);
    virtual int createItemSelected(std::shared_ptr<ItemInstance> item,
                                   bool canMake);
    virtual int itemDamaged(std::shared_ptr<ItemInstance> item);
    virtual bool onTake(std::shared_ptr<ItemInstance> item);
    virtual bool onLookAt(int id, int iData = 0);
    virtual bool onLookAtEntity(eINSTANCEOF type);
    virtual int tick();
    virtual bool allowFade() { return m_allowFade; }
};
