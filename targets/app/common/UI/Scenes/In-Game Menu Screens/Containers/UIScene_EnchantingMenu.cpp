
#include "UIScene_EnchantingMenu.h"

#include <assert.h>

#include <memory>

#include "app/common/Game.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_EnchantmentBook.h"
#include "app/common/UI/Controls/UIControl_EnchantmentButton.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_AbstractContainerMenu.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/world/inventory/EnchantmentMenu.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "platform/game/game.h"
#include "platform/profile/profile.h"
#include "strings.h"

class UILayer;

UIScene_EnchantingMenu::UIScene_EnchantingMenu(int iPad, void* _initData,
                                               UILayer* parentLayer)
    : UIScene_AbstractContainerMenu(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_enchantButton[0].init(0);
    m_enchantButton[1].init(1);
    m_enchantButton[2].init(2);

    EnchantingScreenInput* initData = (EnchantingScreenInput*)_initData;

    m_labelEnchant.init(initData->name.empty() ? app.GetString(IDS_ENCHANT)
                                               : initData->name);

    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft->localgameModes[initData->iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[initData->iPad];
        m_previousTutorialState = gameMode->getTutorial()->getCurrentState();
        gameMode->getTutorial()->changeTutorialState(
            e_Tutorial_State_Enchanting_Menu, this);
    }

    EnchantmentMenu* menu =
        new EnchantmentMenu(initData->inventory, initData->level, initData->x,
                            initData->y, initData->z);

    Initialize(initData->iPad, menu, true, EnchantmentMenu::INV_SLOT_START,
               eSectionEnchantUsing, eSectionEnchantMax);

    m_slotListIngredient.addSlots(EnchantmentMenu::INGREDIENT_SLOT, 1);

    PlatformGame.SetRichPresenceContext(m_iPad, CONTEXT_GAME_STATE_ENCHANTING);

    delete initData;
}

std::string UIScene_EnchantingMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "EnchantingMenuSplit";
    } else {
        return "EnchantingMenu";
    }
}

void UIScene_EnchantingMenu::handleReload() {
    Initialize(m_iPad, m_menu, true, EnchantmentMenu::INV_SLOT_START,
               eSectionEnchantUsing, eSectionEnchantMax);

    m_slotListIngredient.addSlots(EnchantmentMenu::INGREDIENT_SLOT, 1);
}

int UIScene_EnchantingMenu::getSectionColumns(ESceneSection eSection) {
    int cols = 0;
    switch (eSection) {
        case eSectionEnchantSlot:
            cols = 1;
            break;
        case eSectionEnchantInventory:
            cols = 9;
            break;
        case eSectionEnchantUsing:
            cols = 9;
            break;
        default:
            assert(false);
            break;
    };
    return cols;
}

int UIScene_EnchantingMenu::getSectionRows(ESceneSection eSection) {
    int rows = 0;
    switch (eSection) {
        case eSectionEnchantSlot:
            rows = 1;
            break;
        case eSectionEnchantInventory:
            rows = 3;
            break;
        case eSectionEnchantUsing:
            rows = 1;
            break;
        default:
            assert(false);
            break;
    };
    return rows;
}

void UIScene_EnchantingMenu::GetPositionOfSection(ESceneSection eSection,
                                                  UIVec2D* pPosition) {
    switch (eSection) {
        case eSectionEnchantSlot:
            pPosition->x = m_slotListIngredient.getXPos();
            pPosition->y = m_slotListIngredient.getYPos();
            break;
        case eSectionEnchantInventory:
            pPosition->x = m_slotListInventory.getXPos();
            pPosition->y = m_slotListInventory.getYPos();
            break;
        case eSectionEnchantUsing:
            pPosition->x = m_slotListHotbar.getXPos();
            pPosition->y = m_slotListHotbar.getYPos();
            break;
        case eSectionEnchantButton1:
            pPosition->x = m_enchantButton[0].getXPos();
            pPosition->y = m_enchantButton[0].getYPos();
            break;
        case eSectionEnchantButton2:
            pPosition->x = m_enchantButton[1].getXPos();
            pPosition->y = m_enchantButton[1].getYPos();
            break;
        case eSectionEnchantButton3:
            pPosition->x = m_enchantButton[2].getXPos();
            pPosition->y = m_enchantButton[2].getYPos();
            break;
        default:
            assert(false);
            break;
    };
}

void UIScene_EnchantingMenu::GetItemScreenData(ESceneSection eSection,
                                               int iItemIndex,
                                               UIVec2D* pPosition,
                                               UIVec2D* pSize) {
    UIVec2D sectionSize;
    switch (eSection) {
        case eSectionEnchantSlot:
            sectionSize.x = m_slotListIngredient.getWidth();
            sectionSize.y = m_slotListIngredient.getHeight();
            break;
        case eSectionEnchantInventory:
            sectionSize.x = m_slotListInventory.getWidth();
            sectionSize.y = m_slotListInventory.getHeight();
            break;
        case eSectionEnchantUsing:
            sectionSize.x = m_slotListHotbar.getWidth();
            sectionSize.y = m_slotListHotbar.getHeight();
            break;
        case eSectionEnchantButton1:
            sectionSize.x = m_enchantButton[0].getWidth();
            sectionSize.y = m_enchantButton[0].getHeight();
            break;
        case eSectionEnchantButton2:
            sectionSize.x = m_enchantButton[1].getWidth();
            sectionSize.y = m_enchantButton[1].getHeight();
            break;
        case eSectionEnchantButton3:
            sectionSize.x = m_enchantButton[2].getWidth();
            sectionSize.y = m_enchantButton[2].getHeight();
            break;
        default:
            assert(false);
            break;
    };

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

void UIScene_EnchantingMenu::setSectionSelectedSlot(ESceneSection eSection,
                                                    int x, int y) {
    int cols = getSectionColumns(eSection);

    int index = (y * cols) + x;

    UIControl_SlotList* slotList = nullptr;
    switch (eSection) {
        case eSectionEnchantSlot:
            slotList = &m_slotListIngredient;
            break;
        case eSectionEnchantInventory:
            slotList = &m_slotListInventory;
            break;
        case eSectionEnchantUsing:
            slotList = &m_slotListHotbar;
            break;
        default:
            assert(false);
            break;
    };

    slotList->setHighlightSlot(index);
}

UIControl* UIScene_EnchantingMenu::getSection(ESceneSection eSection) {
    UIControl* control = nullptr;
    switch (eSection) {
        case eSectionEnchantSlot:
            control = &m_slotListIngredient;
            break;
        case eSectionEnchantInventory:
            control = &m_slotListInventory;
            break;
        case eSectionEnchantUsing:
            control = &m_slotListHotbar;
            break;
        case eSectionEnchantButton1:
            control = &m_enchantButton[0];
            break;
        case eSectionEnchantButton2:
            control = &m_enchantButton[1];
            break;
        case eSectionEnchantButton3:
            control = &m_enchantButton[2];
            break;
        default:
            assert(false);
            break;
    };
    return control;
}

void UIScene_EnchantingMenu::customDraw(IggyCustomDrawCallbackRegion* region) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft->localplayers[m_iPad] == nullptr ||
        pMinecraft->localgameModes[m_iPad] == nullptr)
        return;

    if (std::char_traits<char16_t>::compare(region->name, u"EnchantmentBook",
                                            15) == 0) {
        // Setup GDraw, normal game render states and matrices
        CustomDrawData* customDrawRegion = ui.setupCustomDraw(this, region);
        delete customDrawRegion;

        m_enchantBook.render(region);

        // Finish GDraw and anything else that needs to be finalised
        ui.endCustomDraw(region);
    } else {
        int slotId = -1;
        if (region->name != nullptr &&
            std::char_traits<char16_t>::length(region->name) > 11 &&
            std::char_traits<char16_t>::compare(region->name, u"slot_Button",
                                                11) == 0) {
            int i = 11;
            slotId = 0;

            while (region->name[i] >= u'0' && region->name[i] <= u'9') {
                slotId = slotId * 10 + (region->name[i] - u'0');
                i++;
            }
        }

        if (slotId >= 0) {
            // 4jcraft: sanity check because this code is utter trash garbage
            assert(slotId != 0 &&
                   "4J shitcode - attempted to access m_enchantButton with "
                   "slot_Button0. this shouldn't happen; if you're reading "
                   "this then go bug someone on GitHub or something");

            // Setup GDraw, normal game render states and matrices
            CustomDrawData* customDrawRegion = ui.setupCustomDraw(this, region);
            delete customDrawRegion;

            // 4jcraft: NOTE: if slotId == 0 this is UB, but it never is in
            // practice, plus added the assertion above as a sanity check
            m_enchantButton[slotId - 1].render(region);

            // Finish GDraw and anything else that needs to be finalised
            ui.endCustomDraw(region);
        } else {
            UIScene_AbstractContainerMenu::customDraw(region);
        }
    }
}
