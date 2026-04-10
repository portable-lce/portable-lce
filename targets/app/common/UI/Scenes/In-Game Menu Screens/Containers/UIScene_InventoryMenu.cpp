
#include "UIScene_InventoryMenu.h"

#include <assert.h>

#include <format>
#include <memory>
#include <vector>

#include "app/common/Game.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_MinecraftPlayer.h"
#include "app/common/UI/Controls/UIControl_SlotList.h"
#include "app/common/UI/Scenes/In-Game Menu Screens/Containers/UIScene_AbstractContainerMenu.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/player/LocalPlayer.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/inventory/InventoryMenu.h"
#include "minecraft/world/tutorial/TutorialEnum.h"
#include "strings.h"
#include "util/StringHelpers.h"

class UILayer;

#define INVENTORY_UPDATE_EFFECTS_TIMER_ID (10)
#define INVENTORY_UPDATE_EFFECTS_TIMER_TIME (1000)  // 1 second

UIScene_InventoryMenu::UIScene_InventoryMenu(int iPad, void* _initData,
                                             UILayer* parentLayer)
    : UIScene_AbstractContainerMenu(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    InventoryScreenInput* initData = (InventoryScreenInput*)_initData;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft->localgameModes[initData->iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)pMinecraft->localgameModes[initData->iPad];
        m_previousTutorialState = gameMode->getTutorial()->getCurrentState();
        gameMode->getTutorial()->changeTutorialState(
            e_Tutorial_State_Inventory_Menu, this);
    }

    InventoryMenu* menu = (InventoryMenu*)initData->player->inventoryMenu;

    initData->player->awardStat(GenericStats::openInventory(),
                                GenericStats::param_openInventory());

    Initialize(initData->iPad, menu, false, InventoryMenu::INV_SLOT_START,
               eSectionInventoryUsing, eSectionInventoryMax,
               initData->bNavigateBack);

    m_slotListArmor.addSlots(
        InventoryMenu::ARMOR_SLOT_START,
        InventoryMenu::ARMOR_SLOT_END - InventoryMenu::ARMOR_SLOT_START);

    if (initData) delete initData;

    for (unsigned int i = 0; i < MobEffect::NUM_EFFECTS; ++i) {
        m_bEffectTime[i] = 0;
    }

    updateEffectsDisplay();
    addTimer(INVENTORY_UPDATE_EFFECTS_TIMER_ID,
             INVENTORY_UPDATE_EFFECTS_TIMER_TIME);
}

std::string UIScene_InventoryMenu::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "InventoryMenuSplit";
    } else {
        return "InventoryMenu";
    }
}

void UIScene_InventoryMenu::handleReload() {
    Initialize(m_iPad, m_menu, false, InventoryMenu::INV_SLOT_START,
               eSectionInventoryUsing, eSectionInventoryMax, m_bNavigateBack);

    m_slotListArmor.addSlots(
        InventoryMenu::ARMOR_SLOT_START,
        InventoryMenu::ARMOR_SLOT_END - InventoryMenu::ARMOR_SLOT_START);

    for (unsigned int i = 0; i < MobEffect::NUM_EFFECTS; ++i) {
        m_bEffectTime[i] = 0;
    }
}

int UIScene_InventoryMenu::getSectionColumns(ESceneSection eSection) {
    int cols = 0;
    switch (eSection) {
        case eSectionInventoryArmor:
            cols = 1;
            break;
        case eSectionInventoryInventory:
            cols = 9;
            break;
        case eSectionInventoryUsing:
            cols = 9;
            break;
        default:
            assert(false);
            break;
    }
    return cols;
}

int UIScene_InventoryMenu::getSectionRows(ESceneSection eSection) {
    int rows = 0;
    switch (eSection) {
        case eSectionInventoryArmor:
            rows = 4;
            break;
        case eSectionInventoryInventory:
            rows = 3;
            break;
        case eSectionInventoryUsing:
            rows = 1;
            break;
        default:
            assert(false);
            break;
    }
    return rows;
}

void UIScene_InventoryMenu::GetPositionOfSection(ESceneSection eSection,
                                                 UIVec2D* pPosition) {
    switch (eSection) {
        case eSectionInventoryArmor:
            pPosition->x = m_slotListArmor.getXPos();
            pPosition->y = m_slotListArmor.getYPos();
            break;
        case eSectionInventoryInventory:
            pPosition->x = m_slotListInventory.getXPos();
            pPosition->y = m_slotListInventory.getYPos();
            break;
        case eSectionInventoryUsing:
            pPosition->x = m_slotListHotbar.getXPos();
            pPosition->y = m_slotListHotbar.getYPos();
            break;
        default:
            assert(false);
            break;
    }
}

void UIScene_InventoryMenu::GetItemScreenData(ESceneSection eSection,
                                              int iItemIndex,
                                              UIVec2D* pPosition,
                                              UIVec2D* pSize) {
    UIVec2D sectionSize;

    switch (eSection) {
        case eSectionInventoryArmor:
            sectionSize.x = m_slotListArmor.getWidth();
            sectionSize.y = m_slotListArmor.getHeight();
            break;
        case eSectionInventoryInventory:
            sectionSize.x = m_slotListInventory.getWidth();
            sectionSize.y = m_slotListInventory.getHeight();
            break;
        case eSectionInventoryUsing:
            sectionSize.x = m_slotListHotbar.getWidth();
            sectionSize.y = m_slotListHotbar.getHeight();
            break;
        default:
            assert(false);
            break;
    }

    int rows = getSectionRows(eSection);
    int cols = getSectionColumns(eSection);

    pSize->x = sectionSize.x / cols;
    pSize->y = sectionSize.y / rows;

    int itemCol = iItemIndex % cols;
    int itemRow = iItemIndex / cols;

    pPosition->x = itemCol * pSize->x;
    pPosition->y = itemRow * pSize->y;
}

void UIScene_InventoryMenu::setSectionSelectedSlot(ESceneSection eSection,
                                                   int x, int y) {
    int cols = getSectionColumns(eSection);

    int index = (y * cols) + x;

    UIControl_SlotList* slotList = nullptr;
    switch (eSection) {
        case eSectionInventoryArmor:
            slotList = &m_slotListArmor;
            break;
        case eSectionInventoryInventory:
            slotList = &m_slotListInventory;
            break;
        case eSectionInventoryUsing:
            slotList = &m_slotListHotbar;
            break;
        default:
            break;
    }

    if (slotList != nullptr) {
        slotList->setHighlightSlot(index);
    }
}

UIControl* UIScene_InventoryMenu::getSection(ESceneSection eSection) {
    UIControl* control = nullptr;
    switch (eSection) {
        case eSectionInventoryArmor:
            control = &m_slotListArmor;
            break;
        case eSectionInventoryInventory:
            control = &m_slotListInventory;
            break;
        case eSectionInventoryUsing:
            control = &m_slotListHotbar;
            break;
        default:
            break;
    }
    return control;
}

void UIScene_InventoryMenu::customDraw(IggyCustomDrawCallbackRegion* region) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft->localplayers[m_iPad] == nullptr ||
        pMinecraft->localgameModes[m_iPad] == nullptr)
        return;

    if (std::char_traits<char16_t>::compare(region->name, u"player", 6) == 0) {
        // Setup GDraw, normal game render states and matrices
        CustomDrawData* customDrawRegion = ui.setupCustomDraw(this, region);
        delete customDrawRegion;

        m_playerPreview.render(region);

        // Finish GDraw and anything else that needs to be finalised
        ui.endCustomDraw(region);
    } else {
        UIScene_AbstractContainerMenu::customDraw(region);
    }
}

void UIScene_InventoryMenu::handleTimerComplete(int id) {
    if (id == INVENTORY_UPDATE_EFFECTS_TIMER_ID) {
        updateEffectsDisplay();
    }
}

void UIScene_InventoryMenu::updateEffectsDisplay() {
    // Update with the current effects
    Minecraft* pMinecraft = Minecraft::GetInstance();
    std::shared_ptr<MultiplayerLocalPlayer> player =
        pMinecraft->localplayers[m_iPad];

    if (player == nullptr) return;

    std::vector<MobEffectInstance*>* activeEffects = player->getActiveEffects();

    // 4J - TomK setup time update value array size to update the active effects
    int iValue = 0;
    IggyDataValue* UpdateValue = new IggyDataValue[activeEffects->size() * 2];

    for (auto it = activeEffects->begin(); it != activeEffects->end(); ++it) {
        MobEffectInstance* effect = *it;

        if (effect->getDuration() >= m_bEffectTime[effect->getId()]) {
            std::string effectString = app.GetString(
                effect
                    ->getDescriptionId());  // I18n.get(effect.getDescriptionId()).trim();
            if (effect->getAmplifier() > 0) {
                std::string potencyString = "";
                switch (effect->getAmplifier()) {
                    case 1:
                        potencyString = " ";
                        potencyString += app.GetString(IDS_POTION_POTENCY_1);
                        break;
                    case 2:
                        potencyString = " ";
                        potencyString += app.GetString(IDS_POTION_POTENCY_2);
                        break;
                    case 3:
                        potencyString = " ";
                        potencyString += app.GetString(IDS_POTION_POTENCY_3);
                        break;
                    default:
                        potencyString = app.GetString(IDS_POTION_POTENCY_0);
                        break;
                }
                effectString += potencyString;
            }
            int icon = 0;
            MobEffect* mobEffect = MobEffect::effects[effect->getId()];
            if (mobEffect->hasIcon()) {
                icon = mobEffect->getIcon();
            }
            IggyDataValue result;
            IggyDataValue value[3];
            value[0].type = IGGY_DATATYPE_number;
            value[0].number = icon;

            IggyStringUTF8 stringVal;
            stringVal.string = const_cast<char*>(effectString.c_str());
            stringVal.length = effectString.length();
            value[1].type = IGGY_DATATYPE_string_UTF8;
            value[1].string8 = stringVal;

            int seconds =
                effect->getDuration() / SharedConstants::TICKS_PER_SECOND;
            value[2].type = IGGY_DATATYPE_number;
            value[2].number = seconds;
            IggyResult out = IggyPlayerCallMethodRS(
                getMovie(), &result, IggyPlayerRootPath(getMovie()),
                m_funcAddEffect, 3, value);
        }

        if (MobEffect::effects[effect->getId()]->hasIcon()) {
            // 4J - TomK set ids and remaining duration so we can update the
            // timers accurately in one call! (this prevents performance related
            // timer sync issues, especially on PSVita)
            UpdateValue[iValue].type = IGGY_DATATYPE_number;
            UpdateValue[iValue].number =
                MobEffect::effects[effect->getId()]->getIcon();
            UpdateValue[iValue + 1].type = IGGY_DATATYPE_number;
            UpdateValue[iValue + 1].number =
                (int)(effect->getDuration() /
                      SharedConstants::TICKS_PER_SECOND);
            iValue += 2;
        }

        m_bEffectTime[effect->getId()] = effect->getDuration();
    }

    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(
        getMovie(), &result, IggyPlayerRootPath(getMovie()),
        m_funcUpdateEffects, activeEffects->size() * 2, UpdateValue);

    delete activeEffects;
}
