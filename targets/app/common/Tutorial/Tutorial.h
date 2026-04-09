#pragma once
// using namespace std;
#include <chrono>
#include <cstdint>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "TutorialMessage.h"
#include "app/common/Tutorial/Constraints/TutorialConstraint.h"
#include "app/common/Tutorial/Hints/TutorialHint.h"
#include "app/common/Tutorial/Tasks/TutorialTask.h"
#include "minecraft/world/tutorial/ITutorial.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "util/Timer.h"

class Entity;
class ItemInstance;
class MobEffect;
class Tile;
class TutorialConstraint;
class TutorialHint;
class TutorialTask;

// #define TUTORIAL_HINT_DELAY_TIME 14000 // How long we should wait from
// displaying one hint to the next #define TUTORIAL_DISPLAY_MESSAGE_TIME 7000
// #define TUTORIAL_MINIMUM_DISPLAY_MESSAGE_TIME 2000
// #define TUTORIAL_REMINDER_TIME (TUTORIAL_DISPLAY_MESSAGE_TIME + 20000)
// #define TUTORIAL_CONSTRAINT_DELAY_REMOVE_TICKS 15
//
// // 0-24000
// #define TUTORIAL_FREEZE_TIME_VALUE 8000

class UIScene;
class Level;
class CXuiScene;
class Player;

class Tutorial : public ITutorial {
public:
    class PopupMessageDetails {
    public:
        int m_messageId;
        int m_promptId;
        int m_titleId;
        std::string m_messageString;
        std::string m_promptString;
        std::string m_titleString;
        int m_icon;
        int m_iAuxVal;
        bool m_allowFade;
        bool m_isReminder;
        bool m_replaceCurrent;
        bool m_forceDisplay;
        bool m_delay;

        PopupMessageDetails() {
            m_messageId = -1;
            m_promptId = -1;
            m_titleId = -1;
            m_messageString = "";
            m_promptString = "";
            m_titleString = "";
            m_icon = TUTORIAL_NO_ICON;
            m_iAuxVal = 0;
            m_allowFade = true;
            m_isReminder = false;
            m_replaceCurrent = false;
            m_forceDisplay = false;
            m_delay = false;
        }

        bool isSameContent(PopupMessageDetails* other);
    };

private:
    static int m_iTutorialHintDelayTime;
    static int m_iTutorialDisplayMessageTime;
    static int m_iTutorialMinimumDisplayMessageTime;
    static int m_iTutorialExtraReminderTime;
    static int m_iTutorialReminderTime;
    static int m_iTutorialConstraintDelayRemoveTicks;
    static int m_iTutorialFreezeTimeValue;
    eTutorial_State m_CurrentState;
    bool m_hasStateChanged;
    bool m_bSceneIsSplitscreen;

    bool m_bHasTickedOnce;
    time_util::time_point m_firstTickTime;

protected:
    std::unordered_map<int, TutorialMessage*> messages;
    std::vector<TutorialConstraint*> m_globalConstraints;
    std::vector<TutorialConstraint*> constraints[e_Tutorial_State_Max];
    std::vector<std::pair<TutorialConstraint*, unsigned char> >
        constraintsToRemove[e_Tutorial_State_Max];
    std::vector<TutorialTask*>
        tasks;  // We store a copy of the tasks for the main gameplay tutorial
                // so that we could display an overview menu
    std::vector<TutorialTask*> activeTasks[e_Tutorial_State_Max];
    std::vector<TutorialHint*> hints[e_Tutorial_State_Max];
    TutorialTask* currentTask[e_Tutorial_State_Max];
    TutorialConstraint* currentFailedConstraint[e_Tutorial_State_Max];

    bool m_freezeTime;
    bool m_timeFrozen;
    // D3DXVECTOR3 m_OriginalPosition;

public:
    time_util::time_point lastMessageTime;
    time_util::time_point m_lastHintDisplayedTime;

private:
    PopupMessageDetails* m_lastMessage;

    eTutorial_State m_lastMessageState;
    unsigned int m_iTaskReminders;

    bool m_allowShow;

public:
    bool m_hintDisplayed;

private:
    bool hasRequestedUI;
    bool uiTempDisabled;

    UIScene* m_UIScene;

    int m_iPad;

public:
    bool m_allTutorialsComplete;
    bool m_fullTutorialComplete;
    bool m_isFullTutorial;

public:
    Tutorial(int iPad, bool isFullTutorial = false);
    virtual ~Tutorial();
    void tick();

    int getPad() { return m_iPad; }

    bool isStateCompleted(eTutorial_State state) override;
    virtual void setStateCompleted(eTutorial_State state);
    bool isHintCompleted(eTutorial_Hint hint);
    void setHintCompleted(eTutorial_Hint hint);
    void setHintCompleted(TutorialHint* hint);

    // completableId will be either a eTutorial_State value or eTutorial_Hint
    void setCompleted(int completableId);
    bool getCompleted(int completableId);

    void changeTutorialState(eTutorial_State newState, UIScene* scene);
    void changeTutorialState(eTutorial_State newState) override {
        changeTutorialState(newState, nullptr);
    }
    bool isSelectedItemState();

    bool setMessage(PopupMessageDetails* message);
    bool setMessage(TutorialHint* hint, PopupMessageDetails* message);
    bool setMessage(const std::string& message, int icon,
                    int auxValue) override;

    void showTutorialPopup(bool show) override;

    void useItemOn(Level* level, std::shared_ptr<ItemInstance> item, int x,
                   int y, int z, bool bTestUseOnly = false);
    void useItemOn(std::shared_ptr<ItemInstance> item,
                   bool bTestUseOnly = false);
    void completeUsingItem(std::shared_ptr<ItemInstance> item) override;
    void startDestroyBlock(std::shared_ptr<ItemInstance> item, Tile* tile);
    void destroyBlock(Tile* tile);
    void attack(std::shared_ptr<Player> player, std::shared_ptr<Entity> entity);
    void itemDamaged(std::shared_ptr<ItemInstance> item);

    void handleUIInput(int iAction);
    void createItemSelected(std::shared_ptr<ItemInstance> item, bool canMake);
    void onCrafted(std::shared_ptr<ItemInstance> item) override;
    void onTake(std::shared_ptr<ItemInstance> item,
                unsigned int invItemCountAnyAux,
                unsigned int invItemCountThisAux) override;
    void onSelectedItemChanged(std::shared_ptr<ItemInstance> item) override;
    void onLookAt(int id, int iData = 0) override;
    void onLookAtEntity(std::shared_ptr<Entity> entity) override;
    void onRideEntity(std::shared_ptr<Entity> entity) override;
    void onEffectChanged(MobEffect* effect, bool bRemoved = false) override;

    bool canMoveToPosition(double xo, double yo, double zo, double xt,
                           double yt, double zt) override;
    bool isInputAllowed(int mapping);

    void AddGlobalConstraint(TutorialConstraint* c);
    void AddConstraint(TutorialConstraint* c);
    void RemoveConstraint(TutorialConstraint* c, bool delayedRemove = false);
    void addTask(eTutorial_State state, TutorialTask* t);
    void addHint(eTutorial_State state, TutorialHint* h);
    void addMessage(int messageId, bool limitRepeats = false,
                    unsigned char numRepeats = TUTORIAL_MESSAGE_DEFAULT_SHOW);

    int GetTutorialDisplayMessageTime() {
        return m_iTutorialDisplayMessageTime;
    }

    // Only for the main gameplay tutorial
    std::vector<TutorialTask*>* getTasks();
    unsigned int getCurrentTaskIndex();

    UIScene* getScene() { return m_UIScene; }
    eTutorial_State getCurrentState() { return m_CurrentState; }

    // These are required so that we have a consistent mapping of the completion
    // bits stored in the profile data
    static void staticCtor();
    static std::vector<int> s_completableTasks;

    static void debugResetPlayerSavedProgress(int iPad);
};
