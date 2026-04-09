
#include "UIScene_AnvilMenu.h"
#include "platform/game/game.h"

#include <assert.h>
#include <wchar.h>

#include <memory>
#include <utility>

#include "app/common/Tutorial/Tutorial.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_AbstractContainerMenu.h"
#include "app/linux/LinuxGame.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/inventory/AnvilMenu.h"
#include "minecraft/world/inventory/Slot.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "strings.h"
#include "util/StringHelpers.h"

class UILayer;

UIScene_AnvilMenu::UIScene_AnvilMenu(int iPad, void* _initData,
                                     UILayer* parentLayer)
    : UIScene_AbstractContainerMenu(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_showingCross = false;
    m_textInputAnvil.init(m_itemName, eControl_TextInput);

    m_labelAnvil.init(app.GetString(IDS_REPAIR_AND_NAME));

    AnvilScreenInput* initData = (AnvilScreenInput*)_initData;
    m_inventory = initData->inventory;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft->localgameModes[iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[iPad];
        m_previousTutorialState = gameMode->getTutorial()->getCurrentState();
        gameMode->getTutorial()->changeTutorialState(
            e_Tutorial_State_Anvil_Menu, this);
    }

    m_repairMenu =
        new AnvilMenu(initData->inventory, initData->level, initData->x,
                      initData->y, initData->z, pMinecraft->localplayers[iPad]);
    m_repairMenu->addSlotListener(this);

    Initialize(iPad, m_repairMenu, true, AnvilMenu::INV_SLOT_START,
               eSectionAnvilUsing, eSectionAnvilMax);

    m_slotListItem1.addSlots(AnvilMenu::INPUT_SLOT, 1);
    m_slotListItem2.addSlots(AnvilMenu::ADDITIONAL_SLOT, 1);
    m_slotListResult.addSlots(AnvilMenu::RESULT_SLOT, 1);

    bool expensive = false;
    std::string m_costString = "";

    if (m_repairMenu->cost > 0) {
        if (m_repairMenu->cost >= 40 &&
            !pMinecraft->localplayers[iPad]->abilities.instabuild) {
            m_costString = app.GetString(IDS_REPAIR_EXPENSIVE);
            expensive = true;
        } else if (!m_repairMenu->getSlot(AnvilMenu::RESULT_SLOT)->hasItem()) {
            // Do nothing
        } else {
            const char* costString = app.GetString(IDS_REPAIR_COST);
            char temp[256];
            snprintf(temp, 256, costString, m_repairMenu->cost);
            m_costString = temp;
            if (!m_repairMenu->getSlot(AnvilMenu::RESULT_SLOT)
                     ->mayPickup(std::dynamic_pointer_cast<Player>(
                         m_inventory->player->shared_from_this()))) {
                expensive = true;
            }
        }
    }
    setCostLabel(m_costString, expensive);

    if (initData) delete initData;

    setIgnoreInput(false);

    PlatformGame.SetRichPresenceContext(iPad, CONTEXT_GAME_STATE_ANVIL);
}

std::string UIScene_AnvilMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "AnvilMenuSplit";
    } else {
        return "AnvilMenu";
    }
}

void UIScene_AnvilMenu::handleReload() {
    Initialize(m_iPad, m_menu, true, AnvilMenu::INV_SLOT_START,
               eSectionAnvilUsing, eSectionAnvilMax);

    m_slotListItem1.addSlots(AnvilMenu::INPUT_SLOT, 1);
    m_slotListItem2.addSlots(AnvilMenu::ADDITIONAL_SLOT, 1);
    m_slotListResult.addSlots(AnvilMenu::RESULT_SLOT, 1);
}

void UIScene_AnvilMenu::tick() {
    UIScene_AbstractContainerMenu::tick();

    handleTick();
}

int UIScene_AnvilMenu::getSectionColumns(ESceneSection eSection) {
    int cols = 0;
    switch (eSection) {
        case eSectionAnvilItem1:
            cols = 1;
            break;
        case eSectionAnvilItem2:
            cols = 1;
            break;
        case eSectionAnvilResult:
            cols = 1;
            break;
        case eSectionAnvilInventory:
            cols = 9;
            break;
        case eSectionAnvilUsing:
            cols = 9;
            break;
        default:
            assert(false);
            break;
    }
    return cols;
}

int UIScene_AnvilMenu::getSectionRows(ESceneSection eSection) {
    int rows = 0;
    switch (eSection) {
        case eSectionAnvilItem1:
            rows = 1;
            break;
        case eSectionAnvilItem2:
            rows = 1;
            break;
        case eSectionAnvilResult:
            rows = 1;
            break;
        case eSectionAnvilInventory:
            rows = 3;
            break;
        case eSectionAnvilUsing:
            rows = 1;
            break;
        default:
            assert(false);
            break;
    }
    return rows;
}

void UIScene_AnvilMenu::GetPositionOfSection(ESceneSection eSection,
                                             UIVec2D* pPosition) {
    switch (eSection) {
        case eSectionAnvilItem1:
            pPosition->x = m_slotListItem1.getXPos();
            pPosition->y = m_slotListItem1.getYPos();
            break;
        case eSectionAnvilItem2:
            pPosition->x = m_slotListItem2.getXPos();
            pPosition->y = m_slotListItem2.getYPos();
            break;
        case eSectionAnvilResult:
            pPosition->x = m_slotListResult.getXPos();
            pPosition->y = m_slotListResult.getYPos();
            break;
        case eSectionAnvilName:
            pPosition->x = m_textInputAnvil.getXPos();
            pPosition->y = m_textInputAnvil.getYPos();
            break;
        case eSectionAnvilInventory:
            pPosition->x = m_slotListInventory.getXPos();
            pPosition->y = m_slotListInventory.getYPos();
            break;
        case eSectionAnvilUsing:
            pPosition->x = m_slotListHotbar.getXPos();
            pPosition->y = m_slotListHotbar.getYPos();
            break;
        default:
            assert(false);
            break;
    }
}

void UIScene_AnvilMenu::GetItemScreenData(ESceneSection eSection,
                                          int iItemIndex, UIVec2D* pPosition,
                                          UIVec2D* pSize) {
    UIVec2D sectionSize;

    switch (eSection) {
        case eSectionAnvilItem1:
            sectionSize.x = m_slotListItem1.getWidth();
            sectionSize.y = m_slotListItem1.getHeight();
            break;
        case eSectionAnvilItem2:
            sectionSize.x = m_slotListItem2.getWidth();
            sectionSize.y = m_slotListItem2.getHeight();
            break;
        case eSectionAnvilResult:
            sectionSize.x = m_slotListResult.getWidth();
            sectionSize.y = m_slotListResult.getHeight();
            break;
        case eSectionAnvilName:
            sectionSize.x = m_textInputAnvil.getWidth();
            sectionSize.y = m_textInputAnvil.getHeight();
            break;
        case eSectionAnvilInventory:
            sectionSize.x = m_slotListInventory.getWidth();
            sectionSize.y = m_slotListInventory.getHeight();
            break;
        case eSectionAnvilUsing:
            sectionSize.x = m_slotListHotbar.getWidth();
            sectionSize.y = m_slotListHotbar.getHeight();
            break;
        default:
            assert(false);
            break;
    }

    if (IsSectionSlotList(eSection)) {
        int rows = getSectionRows(eSection);
        int cols = getSectionColumns(eSection);

        pSize->x = sectionSize.x / cols;
        pSize->y = sectionSize.y / rows;

        int itemCol = iItemIndex % cols;
        int itemRow = iItemIndex / cols;

        pPosition->x = itemCol * pSize->x;
        pPosition->y = itemRow * pSize->y;
    } else {
        GetPositionOfSection(eSection, pPosition);
        pSize->x = sectionSize.x;
        pSize->y = sectionSize.y;
    }
}

void UIScene_AnvilMenu::setSectionSelectedSlot(ESceneSection eSection, int x,
                                               int y) {
    int cols = getSectionColumns(eSection);

    int index = (y * cols) + x;

    UIControl_SlotList* slotList = nullptr;
    switch (eSection) {
        case eSectionAnvilItem1:
            slotList = &m_slotListItem1;
            break;
        case eSectionAnvilItem2:
            slotList = &m_slotListItem2;
            break;
        case eSectionAnvilResult:
            slotList = &m_slotListResult;
            break;
        case eSectionAnvilInventory:
            slotList = &m_slotListInventory;
            break;
        case eSectionAnvilUsing:
            slotList = &m_slotListHotbar;
            break;
        default:
            assert(false);
            break;
    }

    slotList->setHighlightSlot(index);
}

UIControl* UIScene_AnvilMenu::getSection(ESceneSection eSection) {
    UIControl* control = nullptr;
    switch (eSection) {
        case eSectionAnvilItem1:
            control = &m_slotListItem1;
            break;
        case eSectionAnvilItem2:
            control = &m_slotListItem2;
            break;
        case eSectionAnvilResult:
            control = &m_slotListResult;
            break;
        case eSectionAnvilName:
            control = &m_textInputAnvil;
            break;
        case eSectionAnvilInventory:
            control = &m_slotListInventory;
            break;
        case eSectionAnvilUsing:
            control = &m_slotListHotbar;
            break;
        default:
            assert(false);
            break;
    }
    return control;
}

void UIScene_AnvilMenu::handleEditNamePressed() {
    setIgnoreInput(true);
    PlatformInput.RequestKeyboard(
        app.GetString(IDS_TITLE_RENAME), m_textInputAnvil.getLabel(), m_iPad,
        30,
        [this](bool bRes) -> int {
            // 4J HEG - No reason to set value if keyboard was cancelled
            setIgnoreInput(false);
            if (bRes) {
                std::string str = PlatformInput.GetText();
                setEditNameValue(str);
                m_itemName = std::move(str);
                updateItemName();
            }
            return 0;
        },
        IPlatformInput::EKeyboardMode_Default);
}

void UIScene_AnvilMenu::setEditNameValue(const std::string& name) {
    m_textInputAnvil.setLabel(name);
}

void UIScene_AnvilMenu::setEditNameEditable(bool enabled) {}

void UIScene_AnvilMenu::setCostLabel(const std::string& label, bool canAfford) {
    IggyDataValue result;
    IggyDataValue value[2];

    IggyStringUTF8 stringVal;
    stringVal.string = const_cast<char*>(label.c_str());
    stringVal.length = label.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal;

    value[1].type = IGGY_DATATYPE_boolean;
    value[1].boolval = canAfford;
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetCostLabel, 2, value);
}

void UIScene_AnvilMenu::showCross(bool show) {
    if (m_showingCross != show) {
        IggyDataValue result;
        IggyDataValue value[1];

        value[0].type = IGGY_DATATYPE_boolean;
        value[0].boolval = show;
        IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                                IggyPlayerRootPath(getMovie()),
                                                m_funcShowRedCross, 1, value);

        m_showingCross = show;
    }
}

void UIScene_AnvilMenu::handleDestroy() {
    // another player destroyed the anvil, so shut down the keyboard if it is
    // displayed
    UIScene_AbstractContainerMenu::handleDestroy();
}
