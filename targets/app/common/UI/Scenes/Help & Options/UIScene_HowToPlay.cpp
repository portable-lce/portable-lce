
#include "UIScene_HowToPlay.h"

#include <stdint.h>
#include <wchar.h>

#include <vector>

#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/sounds/SoundTypes.h"
#include "strings.h"
#include "util/StringHelpers.h"

class UILayer;

static UIScene_HowToPlay::SHowToPlayPageDef gs_aPageDefs[eHowToPlay_NumPages] =
    {
        {IDS_HOW_TO_PLAY_WHATSNEW, 0, 0},     // eHowToPlay_WhatsNew
        {IDS_HOW_TO_PLAY_BASICS, 0, 0},       // eHowToPlay_Basics
        {IDS_HOW_TO_PLAY_MULTIPLAYER, 0, 0},  // eHowToPlay_Multiplayer
        {IDS_HOW_TO_PLAY_HUD, 0, 0},          // eHowToPlay_HUD
        {IDS_HOW_TO_PLAY_CREATIVE,
         UIScene_HowToPlay::eHowToPlay_LabelCreativeInventory,
         1},  // eHowToPlay_Creative
        {IDS_HOW_TO_PLAY_INVENTORY,
         UIScene_HowToPlay::eHowToPlay_LabelIInventory,
         1},  // eHowToPlay_Inventory
        {IDS_HOW_TO_PLAY_CHEST, UIScene_HowToPlay::eHowToPlay_LabelSCInventory,
         2},  // eHowToPlay_Chest
        {IDS_HOW_TO_PLAY_LARGECHEST,
         UIScene_HowToPlay::eHowToPlay_LabelLCInventory,
         2},                                 // eHowToPlay_LargeChest
        {IDS_HOW_TO_PLAY_ENDERCHEST, 0, 0},  // eHowToPlay_EnderChest
        {IDS_HOW_TO_PLAY_CRAFTING, UIScene_HowToPlay::eHowToPlay_LabelCItem,
         3},  // eHowToPlay_InventoryCrafting
        {IDS_HOW_TO_PLAY_CRAFT_TABLE, UIScene_HowToPlay::eHowToPlay_LabelCTItem,
         3},  // eHowToPlay_CraftTable
        {IDS_HOW_TO_PLAY_FURNACE, UIScene_HowToPlay::eHowToPlay_LabelFFuel,
         4},  // eHowToPlay_Furnace
        {IDS_HOW_TO_PLAY_DISPENSER, UIScene_HowToPlay::eHowToPlay_LabelDText,
         2},  // eHowToPlay_Dispenser
        {IDS_HOW_TO_PLAY_BREWING, UIScene_HowToPlay::eHowToPlay_LabelBBrew,
         2},  // eHowToPlay_Brewing
        {IDS_HOW_TO_PLAY_ENCHANTMENT,
         UIScene_HowToPlay::eHowToPlay_LabelEEnchant,
         2},  // eHowToPlay_Enchantment
        {IDS_HOW_TO_PLAY_ANVIL,
         UIScene_HowToPlay::eHowToPlay_LabelAnvil_Inventory,
         3},                                   // eHowToPlay_Anvil
        {IDS_HOW_TO_PLAY_FARMANIMALS, 0, 0},   // eHowToPlay_Breeding
        {IDS_HOW_TO_PLAY_BREEDANIMALS, 0, 0},  // eHowToPlay_Breeding
        {IDS_HOW_TO_PLAY_TRADING,
         UIScene_HowToPlay::eHowToPlay_LabelTrading_Inventory,
         5},                                   // eHowToPlay_Trading
        {IDS_HOW_TO_PLAY_HORSES, 0, 0},        // eHowToPlay_Horses
        {IDS_HOW_TO_PLAY_BEACONS, 0, 0},       // eHowToPlay_Beacons
        {IDS_HOW_TO_PLAY_FIREWORKS, 0, 0},     // eHowToPlay_Fireworks
        {IDS_HOW_TO_PLAY_HOPPERS, 0, 0},       // eHowToPlay_Hoppers
        {IDS_HOW_TO_PLAY_DROPPERS, 0, 0},      // eHowToPlay_Droppers
        {IDS_HOW_TO_PLAY_NETHERPORTAL, 0, 0},  // eHowToPlay_NetherPortal
        {IDS_HOW_TO_PLAY_THEEND, 0, 0},        // eHowToPlay_NetherPortal
        {IDS_HOW_TO_PLAY_HOSTOPTIONS, 0, 0},   // eHowToPlay_HostOptions
};

int gs_pageToFlashMapping[eHowToPlay_NumPages] = {
    0,   // eHowToPlay_WhatsNew = 0,
    1,   // eHowToPlay_Basics,
    2,   // eHowToPlay_Multiplayer,
    3,   // eHowToPlay_HUD,
    4,   // eHowToPlay_Creative,
    5,   // eHowToPlay_Inventory,
    6,   // eHowToPlay_Chest,
    7,   // eHowToPlay_LargeChest,
    23,  // eHowToPlay_Enderchest,
    8,   // eHowToPlay_InventoryCrafting,
    9,   // eHowToPlay_CraftTable,
    10,  // eHowToPlay_Furnace,
    11,  // eHowToPlay_Dispenser,

    12,  // eHowToPlay_Brewing,
    13,  // eHowToPlay_Enchantment,
    21,  // eHowToPlay_Anvil,
    14,  // eHowToPlay_FarmingAnimals,
    15,  // eHowToPlay_Breeding,
    22,  // eHowToPlay_Trading,

    24,  // eHowToPlay_Horses
    25,  // eHowToPlay_Beacons
    26,  // eHowToPlay_Fireworks
    27,  // eHowToPlay_Hoppers
    28,  // eHowToPlay_Droppers

    16,  // eHowToPlay_NetherPortal,
    17,  // eHowToPlay_TheEnd,
    20,  // eHowToPlay_HostOptions,
};

UIScene_HowToPlay::UIScene_HowToPlay(int iPad, void* initData,
                                     UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    std::string inventoryString = app.GetString(IDS_INVENTORY);
    m_labels[eHowToPlay_LabelCTItem].init(app.GetString(IDS_ITEM_HATCHET_WOOD));
    m_labels[eHowToPlay_LabelCTGroup].init(app.GetString(IDS_GROUPNAME_TOOLS));
    m_labels[eHowToPlay_LabelCTInventory3x3].init(inventoryString);
    m_labels[eHowToPlay_LabelCItem].init(app.GetString(IDS_TILE_WORKBENCH));
    m_labels[eHowToPlay_LabelCGroup].init(
        app.GetString(IDS_GROUPNAME_STRUCTURES));
    m_labels[eHowToPlay_LabelCInventory2x2].init(inventoryString);
    m_labels[eHowToPlay_LabelFFuel].init(app.GetString(IDS_FUEL));
    m_labels[eHowToPlay_LabelFInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelFIngredient].init(app.GetString(IDS_INGREDIENT));
    m_labels[eHowToPlay_LabelFChest].init(app.GetString(IDS_FURNACE));
    m_labels[eHowToPlay_LabelLCInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelCreativeInventory].init(
        app.GetString(IDS_GROUPNAME_BUILDING_BLOCKS));
    m_labels[eHowToPlay_LabelLCChest].init(app.GetString(IDS_CHEST_LARGE));
    m_labels[eHowToPlay_LabelSCInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelSCChest].init(app.GetString(IDS_CHEST));
    m_labels[eHowToPlay_LabelIInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelDInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelDText].init(app.GetString(IDS_DISPENSER));
    m_labels[eHowToPlay_LabelEEnchant].init(app.GetString(IDS_ENCHANT));
    m_labels[eHowToPlay_LabelEInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelBBrew].init(app.GetString(IDS_BREWING_STAND));
    m_labels[eHowToPlay_LabelBInventory].init(inventoryString);
    m_labels[eHowToPlay_LabelAnvil_Inventory].init(inventoryString.c_str());

    std::string wsTemp = app.GetString(IDS_REPAIR_COST);
    wsTemp.replace(wsTemp.find("%d"), 2, std::string("8"));

    m_labels[eHowToPlay_LabelAnvil_Cost].init(wsTemp.c_str());
    m_labels[eHowToPlay_LabelAnvil_ARepairAndName].init(
        app.GetString(IDS_REPAIR_AND_NAME));
    m_labels[eHowToPlay_LabelTrading_Inventory].init(inventoryString.c_str());
    m_labels[eHowToPlay_LabelTrading_Offer2].init(
        app.GetString(IDS_ITEM_EMERALD));
    m_labels[eHowToPlay_LabelTrading_Offer1].init(
        app.GetString(IDS_ITEM_EMERALD));
    m_labels[eHowToPlay_LabelTrading_NeededForTrade].init(
        app.GetString(IDS_REQUIRED_ITEMS_FOR_TRADE));

    m_labels[eHowToPlay_LabelBeacon_PrimaryPower].init(
        app.GetString(IDS_CONTAINER_BEACON_PRIMARY_POWER));
    m_labels[eHowToPlay_LabelBeacon_SecondaryPower].init(
        app.GetString(IDS_CONTAINER_BEACON_SECONDARY_POWER));

    m_labels[eHowToPlay_LabelFireworksText].init(
        app.GetString(IDS_HOW_TO_PLAY_MENU_FIREWORKS));
    m_labels[eHowToPlay_LabelFireworksInventory].init(inventoryString.c_str());

    m_labels[eHowToPlay_LabelHopperText].init(app.GetString(IDS_TILE_HOPPER));
    m_labels[eHowToPlay_LabelHopperInventory].init(inventoryString.c_str());

    m_labels[eHowToPlay_LabelDropperText].init(app.GetString(IDS_TILE_DROPPER));
    m_labels[eHowToPlay_LabelDropperInventory].init(inventoryString.c_str());

    wsTemp = app.GetString(IDS_VILLAGER_OFFERS_ITEM);
    wsTemp = replaceAll(wsTemp, "{*VILLAGER_TYPE*}",
                        app.GetString(IDS_VILLAGER_PRIEST));
    wsTemp.replace(wsTemp.find("%s"), 2, app.GetString(IDS_TILE_LIGHT_GEM));
    m_labels[eHowToPlay_LabelTrading_VillagerOffers].init(wsTemp.c_str());

    // Extract pad and required page from init data. We just put the data into
    // the pointer rather than using it as an address.
    uintptr_t uiInitData = reinterpret_cast<uintptr_t>(initData);

    EHowToPlayPage eStartPage =
        (EHowToPlayPage)((uiInitData >> 16) &
                         0xFFFu);  // Ignores MSB which is set to 1!

    StartPage(eStartPage);
}

std::string UIScene_HowToPlay::getMoviePath() {
    if (app.GetLocalPlayerCount() > 1) {
        return "HowToPlaySplit";
    } else {
        return "HowToPlay";
    }
}

void UIScene_HowToPlay::updateTooltips() {
    // Tool tips.
    int iPage = (int)(m_eCurrPage);

    int firstPage = eHowToPlay_WhatsNew;

    // 4J Stu - Add back for future platforms

    int iA = -1;
    int iX = -1;
    if (iPage == firstPage) {
        // No previous page.
        iA = IDS_HOW_TO_PLAY_NEXT;
    } else if ((iPage + 1) == eHowToPlay_NumPages) {
        // No next page.
        iX = IDS_HOW_TO_PLAY_PREV;
    } else {
        iA = IDS_HOW_TO_PLAY_NEXT;
        iX = IDS_HOW_TO_PLAY_PREV;
    }
    ui.SetTooltips(m_iPad, iA, IDS_TOOLTIPS_BACK, iX);
}

void UIScene_HowToPlay::handleReload() { StartPage(m_eCurrPage); }

void UIScene_HowToPlay::handleInput(int iPad, int key, bool repeat,
                                    bool pressed, bool released,
                                    bool& handled) {
    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
                handled = true;
            }
            break;
        case ACTION_MENU_A:
            if (pressed) {
                // Next page
                int iNextPage = (int)(m_eCurrPage) + 1;
                if (iNextPage != eHowToPlay_NumPages) {
                    StartPage((EHowToPlayPage)(iNextPage));
                    ui.PlayUISFX(eSFX_Press);
                }
                handled = true;
            }
            break;
        case ACTION_MENU_X:
            if (pressed) {
                // Previous page
                int iPrevPage = (int)(m_eCurrPage)-1;

                // 4J Stu - Add back for future platforms
                {
                    if (iPrevPage >= 0) {
                        StartPage((EHowToPlayPage)(iPrevPage));
                        ui.PlayUISFX(eSFX_Press);
                    }
                }
                handled = true;
            }
            break;
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_HowToPlay::StartPage(EHowToPlayPage ePage) {
    m_eCurrPage = ePage;

    // Turn on just what we need for this screen.
    SHowToPlayPageDef* pDef = &(gs_aPageDefs[m_eCurrPage]);

    // Replace button identifiers in the text with actual button images.
    std::string replacedText =
        app.FormatHTMLString(m_iPad, app.GetString(pDef->m_iTextStringID));
    // 4J-PB - replace the title with the platform specific title, and the
    // platform name
    //	replacedText =
    // replaceAll(replacedText,"{*TITLE_UPDATE_NAME*}",app.GetString(IDS_TITLE_UPDATE_NAME));
    replacedText = replaceAll(replacedText, "{*KICK_PLAYER_DESCRIPTION*}",
                              app.GetString(IDS_KICK_PLAYER_DESCRIPTION));
    replacedText = replaceAll(replacedText, "{*BACK_BUTTON*}",
                              app.GetString(IDS_BACK_BUTTON));
    replacedText =
        replaceAll(replacedText, "{*DISABLES_ACHIEVEMENTS*}",
                   app.GetString(IDS_HOST_OPTION_DISABLES_ACHIEVEMENTS));

    // 4J-JEV: Temporary fix: LOC: Minecraft: XB1: KO: Font: Uncategorized:
    // Squares appear instead of hyphens in FIREWORKS description
    if (!ui.UsingBitmapFont()) {
        replacedText = replaceAll(replacedText, "\u00A9", "(C)");
        replacedText = replaceAll(replacedText, "\u00AE", "(R)");
        replacedText = replaceAll(replacedText, "\u2013", "-");
    }

    // strip out any tab characters and repeated spaces
    stripWhitespaceForHtml(replacedText, true);

    // Set the text colour
    std::string finalText(replacedText.c_str());
    char startTags[64];
    snprintf(startTags, 64, "<font color=\"#%08x\">",
             app.GetHTMLColour(eHTMLColor_White));
    finalText = startTags + finalText;

    std::vector<std::string> paragraphs;
    int lastIndex = 0;
    for (int index = finalText.find("\r\n", lastIndex, 2);
         index != std::string::npos;
         index = finalText.find("\r\n", lastIndex, 2)) {
        paragraphs.push_back(finalText.substr(lastIndex, index - lastIndex) +
                             " ");
        lastIndex = index + 2;
    }
    paragraphs.push_back(
        finalText.substr(lastIndex, finalText.length() - lastIndex));

    // Set the text in the scene
    IggyDataValue result;

    IggyDataValue* value = new IggyDataValue[paragraphs.size() + 1];
    IggyStringUTF8* stringVal = new IggyStringUTF8[paragraphs.size()];

    value[0].type = IGGY_DATATYPE_number;
    value[0].number = gs_pageToFlashMapping[(int)ePage];

    for (unsigned int i = 0; i < paragraphs.size(); ++i) {
        stringVal[i].string = const_cast<char*>(paragraphs[i].c_str());
        stringVal[i].length = paragraphs[i].length();
        value[i + 1].type = IGGY_DATATYPE_string_UTF8;
        value[i + 1].string8 = stringVal[i];
    }

    IggyResult out = IggyPlayerCallMethodRS(
        getMovie(), &result, IggyPlayerRootPath(getMovie()), m_funcLoadPage,
        1 + paragraphs.size(), value);

    delete[] value;
    delete[] stringVal;

    updateTooltips();
}
