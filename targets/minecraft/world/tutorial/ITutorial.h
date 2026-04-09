#pragma once

#include <memory>
#include <string>

#include "minecraft/world/tutorial/TutorialEnum.h"

class Entity;
class ItemInstance;

// Domain interface for the player tutorial.
//
// minecraft/ consumers (Player, GameMode, ClientConnection, the player
// list) need to forward gameplay events into the tutorial system but
// they should not depend on the heavyweight Tutorial implementation
// in app/common/Tutorial/. The concrete Tutorial in app/ inherits
// from this interface; minecraft/ only sees ITutorial*.
//
// Tutorial state and hint enums (eTutorial_State, eTutorial_Hint)
// remain in minecraft/world/tutorial/TutorialEnum.h, which is itself a
// content-only header that minecraft/ can safely include.
class ITutorial {
public:
    virtual ~ITutorial() = default;

    [[nodiscard]] virtual bool isStateCompleted(eTutorial_State state) = 0;
    virtual void changeTutorialState(eTutorial_State newState) = 0;

    virtual bool setMessage(const std::string& message, int icon,
                            int auxValue) = 0;

    virtual void showTutorialPopup(bool show) = 0;

    virtual void onCrafted(std::shared_ptr<ItemInstance> item) = 0;
    virtual void onTake(std::shared_ptr<ItemInstance> item,
                        unsigned int invItemCountAnyAux,
                        unsigned int invItemCountThisAux) = 0;
    virtual void onSelectedItemChanged(std::shared_ptr<ItemInstance> item) = 0;
    virtual void onLookAt(int id, int iData = 0) = 0;
    virtual void onLookAtEntity(std::shared_ptr<Entity> entity) = 0;
    virtual void onRideEntity(std::shared_ptr<Entity> entity) = 0;

    [[nodiscard]] virtual bool canMoveToPosition(double xo, double yo,
                                                 double zo, double xt,
                                                 double yt, double zt) = 0;
};
