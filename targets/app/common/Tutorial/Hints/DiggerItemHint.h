#pragma once

#include "TutorialHint.h"
#include "app/common/Tutorial/TutorialEnum.h"

class DiggerItem;
class Level;
class Tutorial;

class DiggerItemHint : public TutorialHint {
private:
    int* m_iItems;
    unsigned int m_iItemsCount;

public:
    DiggerItemHint(eTutorial_Hint id, Tutorial* tutorial, int descriptionId,
                   int items[], unsigned int itemsLength);
    virtual int startDestroyBlock(std::shared_ptr<ItemInstance> item,
                                  Tile* tile);
    virtual int attack(std::shared_ptr<ItemInstance> item,
                       std::shared_ptr<Entity> entity);
};