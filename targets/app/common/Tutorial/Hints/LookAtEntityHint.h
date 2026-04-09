#pragma once
// using namespace std;

#include "TutorialHint.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "java/Class.h"

class ItemInstance;
class Tutorial;

class LookAtEntityHint : public TutorialHint {
private:
    eINSTANCEOF m_type;
    int m_titleId;

public:
    LookAtEntityHint(eTutorial_Hint id, Tutorial* tutorial, int descriptionId,
                     int titleId, eINSTANCEOF type);
    // TODO: 4jcraft added, this was not implemented
    ~LookAtEntityHint(){};

    virtual bool onLookAtEntity(eINSTANCEOF type);
};
