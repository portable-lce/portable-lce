
#include "UIScene_LeaderboardsMenu.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <wchar.h>

#include <memory>

#include "platform/leaderboard/leaderboard.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_LeaderboardList.h"
#include "app/common/UI/UILayer.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/Console_Debug_enum.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/profile/profile.h"
#include "strings.h"

#define PLAYER_ONLINE_TIMER_ID 0
#define PLAYER_ONLINE_TIMER_TIME 100

// if the value is greater than 32000, it's an xzp icon that needs displayed,
// rather than the game icon
const int UIScene_LeaderboardsMenu::TitleIcons
    [UIScene_LeaderboardsMenu::NUM_LEADERBOARDS][7] = {
        {UIControl_LeaderboardList::e_ICON_TYPE_WALKED,
         UIControl_LeaderboardList::e_ICON_TYPE_FALLEN, Item::minecart_Id,
         Item::boat_Id, 0},
        {Tile::dirt_Id, Tile::cobblestone_Id, Tile::sand_Id, Tile::stone_Id,
         Tile::gravel_Id, Tile::clay_Id, Tile::obsidian_Id},
        {Item::egg_Id, Item::wheat_Id, Tile::mushroom_brown_Id, Tile::reeds_Id,
         Item::bucket_milk_Id, Tile::pumpkin_Id, 0},
        {UIControl_LeaderboardList::e_ICON_TYPE_ZOMBIE,
         UIControl_LeaderboardList::e_ICON_TYPE_SKELETON,
         UIControl_LeaderboardList::e_ICON_TYPE_CREEPER,
         UIControl_LeaderboardList::e_ICON_TYPE_SPIDER,
         UIControl_LeaderboardList::e_ICON_TYPE_SPIDERJOKEY,
         UIControl_LeaderboardList::e_ICON_TYPE_ZOMBIEPIGMAN,
         UIControl_LeaderboardList::e_ICON_TYPE_SLIME},
};
const UIScene_LeaderboardsMenu::LeaderboardDescriptor UIScene_LeaderboardsMenu::
    LEADERBOARD_DESCRIPTORS[UIScene_LeaderboardsMenu::NUM_LEADERBOARDS][4] = {
        {
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                4, true,
                IDS_LEADERBOARD_TRAVELLING_PEACEFUL),  // Travelling Peaceful
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                4, true, IDS_LEADERBOARD_TRAVELLING_EASY),  // Travelling Easy
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                4, true,
                IDS_LEADERBOARD_TRAVELLING_NORMAL),  // Travelling Normal
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                4, true, IDS_LEADERBOARD_TRAVELLING_HARD),  // Travelling Hard
        },
        {
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false,
                IDS_LEADERBOARD_MINING_BLOCKS_PEACEFUL),  // Mining Peaceful
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false, IDS_LEADERBOARD_MINING_BLOCKS_EASY),  // Mining Easy
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false,
                IDS_LEADERBOARD_MINING_BLOCKS_NORMAL),  // Mining Normal
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false, IDS_LEADERBOARD_MINING_BLOCKS_HARD),  // Mining Hard
        },
        {
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                6, false,
                IDS_LEADERBOARD_FARMING_PEACEFUL),  // Farming Peaceful
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                6, false, IDS_LEADERBOARD_FARMING_EASY),  // Farming Easy
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                6, false, IDS_LEADERBOARD_FARMING_NORMAL),  // Farming Normal
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                6, false, IDS_LEADERBOARD_FARMING_HARD),  // Farming Hard
        },
        {
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(0, false, -1),  //
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false, IDS_LEADERBOARD_KILLS_EASY),  // Kills Easy
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false, IDS_LEADERBOARD_KILLS_NORMAL),  // Kills Normal
            UIScene_LeaderboardsMenu::LeaderboardDescriptor(
                7, false, IDS_LEADERBOARD_KILLS_HARD),  // Kills Hard
        },
};

UIScene_LeaderboardsMenu::UIScene_LeaderboardsMenu(int iPad, void* initData,
                                                   UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_bReady = false;

    m_bPopulatedOnce = false;

    m_newTop = m_newSel = -1;
    m_isProcessingStatsRead = false;
    // Ignore input until we're retrieved stats, or functions will be called in
    // here after we've backed out of the scene
    m_bIgnoreInput = true;

    // Alert the app the we want to be informed of ethernet connections
    app.SetLiveLinkRequired(true);

    // GetFriends();

    m_currentLeaderboard = 0;
    m_currentDifficulty = 2;
    SetLeaderboardHeader();
    m_currentFilter = IPlatformLeaderboard::eFM_Friends;

    char filterBuffer[40];
    snprintf(filterBuffer, 40, "%s%s", app.GetString(IDS_LEADERBOARD_FILTER),
             app.GetString(IDS_LEADERBOARD_FILTER_FRIENDS));
    m_labelFilter.init(filterBuffer);

    char entriesBuffer[40];
    snprintf(entriesBuffer, 40, "%s%i", app.GetString(IDS_LEADERBOARD_ENTRIES),
             0);
    m_labelEntries.init(entriesBuffer);

    ReadStats(-1);
}

UIScene_LeaderboardsMenu::~UIScene_LeaderboardsMenu() {
    // Alert the app the we no longer want to be informed of ethernet
    // connections
    app.SetLiveLinkRequired(false);
}

void UIScene_LeaderboardsMenu::updateTooltips() {
    int iTooltipFriendRequest = -1;
    int iTooltipGamerCardOrProfile = -1;

    ui.SetTooltips(m_iPad, iTooltipFriendRequest, IDS_TOOLTIPS_BACK,
                   IDS_TOOLTIPS_CHANGE_FILTER, iTooltipGamerCardOrProfile);
}

void UIScene_LeaderboardsMenu::updateComponents() {
    m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama,
                                 !app.GetGameStarted());
    m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
}

std::string UIScene_LeaderboardsMenu::getMoviePath() {
    return "LeaderboardMenu";
}

void UIScene_LeaderboardsMenu::tick() {
    UIScene::tick();
    PlatformLeaderboard.Tick();
}

void UIScene_LeaderboardsMenu::handleReload() {
    // We don't allow this in splitscreen, so just go back
    navigateBack();
}

void UIScene_LeaderboardsMenu::handleInput(int iPad, int key, bool repeat,
                                           bool pressed, bool released,
                                           bool& handled) {
    if (m_bIgnoreInput && key != ACTION_MENU_CANCEL) return;

    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    // If this is not a press, do not action
    if (!pressed) return;

    /*app.DebugPrintf(
                    "     m_newSel = %i       [bottomId] = %i       [topId] =
       %i, [size] = %i\n", m_newSel, m_leaderboard.m_entries.size() == 0 ? 0 :
       m_leaderboard.m_entries[m_leaderboard.m_entries.size()-1].m_row,
                    GetEntryStartIndex(),
                    m_leaderboard.m_entries.size()
            );*/

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
                handled = true;
            }
            break;
        case ACTION_MENU_UP:
            --m_newSel;
            if (m_newSel < 0) m_newSel = 0;
            sendInputToMovie(key, repeat, pressed, released);
            break;
        case ACTION_MENU_DOWN:
            ++m_newSel;
            if (m_newSel >= m_leaderboard.m_totalEntryCount)
                m_newSel = m_leaderboard.m_totalEntryCount - 1;
            sendInputToMovie(key, repeat, pressed, released);
            break;
        case ACTION_MENU_LEFT_SCROLL:
        case ACTION_MENU_RIGHT_SCROLL: {
            // Do nothing if a stats read is currently in progress, otherwise
            // the system complains about to many read requests
            if (pressed && m_bPopulatedOnce &&
                PlatformLeaderboard.isIdle()) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Scroll);

                if (key == ACTION_MENU_RIGHT_SCROLL) {
                    ++m_currentDifficulty;
                    if (m_currentDifficulty == 4) m_currentDifficulty = 0;

                    if (m_currentLeaderboard == LEADERBOARD_KILLS_POSITION &&
                        m_currentDifficulty == 0)
                        m_currentDifficulty = 1;
                } else {
                    if (m_currentDifficulty == 0) m_currentDifficulty = 4;
                    --m_currentDifficulty;

                    if (m_currentLeaderboard == LEADERBOARD_KILLS_POSITION &&
                        m_currentDifficulty == 0)
                        m_currentDifficulty = 3;
                }

                SetLeaderboardHeader();

                ReadStats(-1);
                ui.PlayUISFX(eSFX_Press);
            }

            handled = true;
        } break;
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT: {
            // Do nothing if a stats read is currently in progress, otherwise
            // the system complains about to many read requests
            if (pressed && m_bPopulatedOnce &&
                PlatformLeaderboard.isIdle()) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Scroll);

                m_bReady = false;
                if (key == ACTION_MENU_RIGHT) {
                    ++m_currentLeaderboard;
                    if (m_currentLeaderboard == NUM_LEADERBOARDS)
                        m_currentLeaderboard = 0;
                } else {
                    if (m_currentLeaderboard == 0)
                        m_currentLeaderboard = NUM_LEADERBOARDS;
                    --m_currentLeaderboard;
                }

                if (m_currentLeaderboard == LEADERBOARD_KILLS_POSITION &&
                    m_currentDifficulty == 0)
                    m_currentDifficulty = 1;

                SetLeaderboardHeader();

                ReadStats(-1);
                ui.PlayUISFX(eSFX_Press);
            }
            handled = true;
        } break;
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN: {
            // Do nothing if a stats read is currently in progress, otherwise
            // the system complains about to many read requests
            if (pressed && m_bPopulatedOnce &&
                PlatformLeaderboard.isIdle()) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Scroll);

                if (m_leaderboard.m_totalEntryCount <= 10) break;

                sendInputToMovie(key, repeat, pressed, released);
            }
            handled = true;
        } break;
        case ACTION_MENU_X: {
            // Do nothing if a stats read is currently in progress, otherwise
            // the system complains about to many read requests
            if (pressed && m_bPopulatedOnce &&
                PlatformLeaderboard.isIdle()) {
                // CD - Added for audio
                ui.PlayUISFX(eSFX_Scroll);

                switch (m_currentFilter) {
                    case IPlatformLeaderboard::eFM_Friends: {
                        m_currentFilter = IPlatformLeaderboard::eFM_MyScore;
                        char filterBuffer[40];
                        snprintf(filterBuffer, 40, "%s%s",
                                 app.GetString(IDS_LEADERBOARD_FILTER),
                                 app.GetString(IDS_LEADERBOARD_FILTER_MYSCORE));
                        m_labelFilter.setLabel(filterBuffer);
                    } break;
                    case IPlatformLeaderboard::eFM_MyScore: {
                        m_currentFilter = IPlatformLeaderboard::eFM_TopRank;
                        char filterBuffer[40];
                        snprintf(filterBuffer, 40, "%s%s",
                                 app.GetString(IDS_LEADERBOARD_FILTER),
                                 app.GetString(IDS_LEADERBOARD_FILTER_OVERALL));
                        m_labelFilter.setLabel(filterBuffer);
                    } break;
                    case IPlatformLeaderboard::eFM_TopRank: {
                        m_currentFilter = IPlatformLeaderboard::eFM_Friends;
                        char filterBuffer[40];
                        snprintf(filterBuffer, 40, "%s%s",
                                 app.GetString(IDS_LEADERBOARD_FILTER),
                                 app.GetString(IDS_LEADERBOARD_FILTER_FRIENDS));
                        m_labelFilter.setLabel(filterBuffer);
                    } break;
                    default:
                        break;
                }

                ReadStats(-1);
                ui.PlayUISFX(eSFX_Press);
            }
            handled = true;
        } break;
        case ACTION_MENU_Y: {
            handled = true;
        } break;
        case ACTION_MENU_A: {
            handled = true;
        } break;
    }
}

void UIScene_LeaderboardsMenu::ReadStats(int startIndex) {
    // If startIndex == -1, then use default values
    if (startIndex == -1) {
        m_newEntryIndex = 1;
        m_newReadSize = READ_SIZE;

        m_newEntriesCount = 0;

        m_leaderboard.m_totalEntryCount = 0;

        m_listEntries.clearList();
    } else {
        m_newEntryIndex = (unsigned int)startIndex;
        // m_newReadSize	= std::min((int)READ_SIZE,
        // (int)m_leaderboard.m_totalEntryCount-(startIndex-1));
    }   

    // app.DebugPrintf("Requesting stats read %d - %d - %d\n",
    // m_currentLeaderboard, startIndex == -1 ? m_currentFilter :
    // IPlatformLeaderboard::eFM_TopRank, m_currentDifficulty);

    IPlatformLeaderboard::EFilterMode filtermode;
    if (m_currentFilter == IPlatformLeaderboard::eFM_MyScore ||
        m_currentFilter == IPlatformLeaderboard::eFM_TopRank) {
        filtermode = (startIndex == -1 ? m_currentFilter
                                       : IPlatformLeaderboard::eFM_TopRank);
    } else {
        // 4J-JEV: Friends filter shouldn't switch to toprank.
        filtermode = m_currentFilter;
    }

    switch (filtermode) {
        case IPlatformLeaderboard::eFM_TopRank: {
            PlatformLeaderboard.ReadStats_TopRank(
                this, m_currentDifficulty,
                (IPlatformLeaderboard::EStatsType)m_currentLeaderboard,
                m_newEntryIndex, m_newReadSize);
        } break;
        case IPlatformLeaderboard::eFM_MyScore: {
            PlayerUID uid;
            PlatformProfile.GetXUID(PlatformProfile.GetPrimaryPad(), &uid,
                                    true);
            PlatformLeaderboard.ReadStats_MyScore(
                this, m_currentDifficulty,
                (IPlatformLeaderboard::EStatsType)m_currentLeaderboard,
                uid /*ignored on PS3*/, m_newReadSize);
        } break;
        case IPlatformLeaderboard::eFM_Friends: {
            PlayerUID uid;
            PlatformProfile.GetXUID(PlatformProfile.GetPrimaryPad(), &uid,
                                    true);
            PlatformLeaderboard.ReadStats_Friends(
                this, m_currentDifficulty,
                (IPlatformLeaderboard::EStatsType)m_currentLeaderboard,
                uid /*ignored on PS3*/, m_newEntryIndex, m_newReadSize);
        } break;
        default:
            break;
    }

    // Show the loading message
    m_labelInfo.setLabel(app.GetString(IDS_LEADERBOARD_LOADING));
    m_labelInfo.setVisible(true);
}

bool UIScene_LeaderboardsMenu::OnStatsReadComplete(
    IPlatformLeaderboard::eStatsReturn retIn, int numResults,
    IPlatformLeaderboard::ViewOut results) {
    // CScene_Leaderboards* scene =
    // reinterpret_cast<CScene_Leaderboards*>(userdata);

    m_isProcessingStatsRead = true;

    // bool noResults = PlatformLeaderboard.GetStatsState() !=
    // XboxIPlatformLeaderboard::eStatsState_Ready;
    bool ret;

    // app.DebugPrintf("Leaderboards read %d stats\n", numResults);

    m_numStats = numResults;
    m_stats = results;
    ret = RetrieveStats();

    // else PlatformLeaderboard.SetStatsRetrieved(false);

    PopulateLeaderboard(retIn);

    updateTooltips();

    m_isProcessingStatsRead = false;

    // allow user input now
    m_bIgnoreInput = false;

    return ret;
}

bool UIScene_LeaderboardsMenu::RetrieveStats() {
    if (app.DebugSettingsOn() && (app.GetGameSettingsDebugMask() &
                                  (1L << eDebugSetting_DebugLeaderboards))) {
        m_leaderboard.m_totalEntryCount = NUM_ENTRIES;
        m_leaderboard.m_numColumns =
            LEADERBOARD_DESCRIPTORS[m_currentLeaderboard][m_currentDifficulty]
                .m_columnCount;

        // For each entry in the leaderboard
        for (unsigned int entryIndex = 0; entryIndex < NUM_ENTRIES;
             entryIndex++) {
            m_leaderboard.m_entries.push_back(LeaderboardEntry());
            m_leaderboard.m_entries[entryIndex].m_xuid = INVALID_XUID;

            m_leaderboard.m_entries[entryIndex].m_row = entryIndex;
            m_leaderboard.m_entries[entryIndex].m_rank = entryIndex + 1;
            snprintf(
                m_leaderboard.m_entries[entryIndex].m_wcRank, 12,
                "12345678");  //(int)m_leaderboard.m_entries[entryIndex].m_rank);

            snprintf(m_leaderboard.m_entries[entryIndex].m_gamerTag, 17,
                     "WWWWWWWWWWWWWWWW");

            // m_leaderboard.m_entries[entryIndex].m_locale = (entryIndex % 37)
            // + 1;

            bool isDistanceLeaderboard =
                LEADERBOARD_DESCRIPTORS[m_currentLeaderboard]
                                       [m_currentDifficulty]
                                           .m_isDistanceLeaderboard;

            for (unsigned int i = 0; i < m_leaderboard.m_numColumns; i++) {
                if (!isDistanceLeaderboard) {
                    m_leaderboard.m_entries[entryIndex].m_columns[i] =
                        USHRT_MAX;
                    snprintf(m_leaderboard.m_entries[entryIndex].m_wcColumns[i],
                             12, "%u",
                             m_leaderboard.m_entries[entryIndex].m_columns[i]);
                } else {
                    m_leaderboard.m_entries[entryIndex].m_columns[i] = UINT_MAX;
                    snprintf(m_leaderboard.m_entries[entryIndex].m_wcColumns[i],
                             12, "%.1fkm",
                             ((float)m_leaderboard.m_entries[entryIndex]
                                  .m_columns[i]) /
                                 100.f / 1000.f);
                }
            }

            m_leaderboard.m_entries[entryIndex].m_bPlayer = (entryIndex == 0);
            m_leaderboard.m_entries[entryIndex].m_bOnline = (entryIndex != 0);
            m_leaderboard.m_entries[entryIndex].m_bFriend = (entryIndex != 0);
            m_leaderboard.m_entries[entryIndex].m_bRequestedFriend = false;
        }

        // PlatformLeaderboard.SetStatsRetrieved(true);

        m_newEntryIndex = 0;
        m_newEntriesCount = NUM_ENTRIES;

        return true;
    }

    // assert( PlatformLeaderboard.GetStats() != nullptr );
    // PXUSER_STATS_READ_RESULTS stats =
    // PlatformLeaderboard.GetStats(); if( m_currentFilter ==
    // IPlatformLeaderboard::eFM_Friends  )
    // PlatformLeaderboard.SortFriendStats();

    bool isDistanceLeaderboard =
        LEADERBOARD_DESCRIPTORS[m_currentLeaderboard][m_currentDifficulty]
            .m_isDistanceLeaderboard;

    m_newEntriesCount = m_stats.m_numQueries;

    // First read
    if (m_leaderboard.m_totalEntryCount == 0) {
        m_leaderboard.m_entries.clear();

        m_leaderboard.m_totalEntryCount =
            (m_currentFilter == IPlatformLeaderboard::eFM_Friends)
                ? m_newEntriesCount
                : m_numStats;

        if (m_leaderboard.m_totalEntryCount == 0 || m_newEntriesCount == 0) {
            // PlatformLeaderboard.SetStatsRetrieved(false);
            return false;
        }

        m_leaderboard.m_numColumns = m_stats.m_queries[0].m_statsSize;

        for (unsigned int entryIndex = 0; entryIndex < m_newEntriesCount;
             ++entryIndex) {
            m_leaderboard.m_entries.push_back(LeaderboardEntry());
            CopyLeaderboardEntry(&(m_stats.m_queries[entryIndex]), entryIndex,
                                 isDistanceLeaderboard);
        }

        m_newEntryIndex = 0;

        // Clear these values so that we know whether or not they are set in the
        // next block
        m_newTop = -1;
        m_newSel = -1;

        // If the filter mode is "My Score" then centre the list around the
        // entries and select the player's score
        if (m_currentFilter == IPlatformLeaderboard::eFM_MyScore) {
            // Centre the leaderboard list on the entries
            m_newTop = GetEntryStartIndex();

            // Select the player entry
            for (unsigned int i = GetEntryStartIndex();
                 i < GetEntryStartIndex() + m_leaderboard.m_entries.size();
                 ++i) {
                if (m_leaderboard.m_entries[i - GetEntryStartIndex()]
                        .m_bPlayer) {
                    m_newSel = i;  // this might be off the screen!
                    // and reposition the top one
                    if (m_newSel - m_newTop > 9) {
                        m_newTop = m_newSel - 9;
                    }
                    break;
                }
            }
        }

        // If not set, default to start index
        if (m_newSel < 0) m_newTop = m_newSel = GetEntryStartIndex();
    }
    // Additional read
    else {
        if (m_newEntryIndex < GetEntryStartIndex() && m_newEntryIndex == 1) {
            // If we're at the top the new entries count is incorrect, so amend
            m_newEntriesCount = GetEntryStartIndex();
        }

        bool deleteFront = false;
        bool deleteBack = false;

        bool trim =
            m_leaderboard.m_entries.size() + m_newEntriesCount >= NUM_ENTRIES;

        unsigned int insertPosition = 0;

        // If the first new entry is at a smaller index than the current first
        // entry
        if (m_newEntryIndex < GetEntryStartIndex()) {
            insertPosition = 0;
            if (trim) deleteBack = true;
        } else {
            insertPosition = m_leaderboard.m_entries.size();
            if (trim) deleteFront = true;
        }

        m_newEntryIndex = insertPosition;

        // Copy results to entries list
        for (unsigned int i = 0; i < m_newEntriesCount; ++i) {
            m_leaderboard.m_entries.insert(
                m_leaderboard.m_entries.begin() + insertPosition,
                LeaderboardEntry());
            CopyLeaderboardEntry(&(m_stats.m_queries[i]), insertPosition,
                                 isDistanceLeaderboard);

            insertPosition++;
        }

        if (deleteFront) {
            // Delete front x entries
            m_leaderboard.m_entries.erase(
                m_leaderboard.m_entries.begin(),
                m_leaderboard.m_entries.begin() + READ_SIZE);
            m_newEntryIndex -= m_newReadSize;
        } else if (deleteBack) {
            // Delete back x entries
            m_leaderboard.m_entries.erase(
                m_leaderboard.m_entries.end() - READ_SIZE,
                m_leaderboard.m_entries.end());
        }
    }

    return true;
}

void UIScene_LeaderboardsMenu::CopyLeaderboardEntry(
    IPlatformLeaderboard::ReadScore* statsRow, int leaderboardEntryIndex,
    bool isDistanceLeaderboard) {
    LeaderboardEntry* leaderboardEntry =
        &(m_leaderboard.m_entries[leaderboardEntryIndex]);

    memset(leaderboardEntry, 0, sizeof(LeaderboardEntry));
    leaderboardEntry->m_xuid = statsRow->m_uid;

    // Copy the rank
    leaderboardEntry->m_rank = statsRow->m_rank;
    unsigned int displayRank = leaderboardEntry->m_rank;
    if (displayRank > 9999999) displayRank = 9999999;
    snprintf(leaderboardEntry->m_wcRank, 12, "%u", displayRank);

    leaderboardEntry->m_idsErrorMessage = statsRow->m_idsErrorMessage;

    // Build a row ID
    if (m_currentFilter == IPlatformLeaderboard::eFM_Friends) {
        // If friends don't ID rows by rank
        leaderboardEntry->m_row = leaderboardEntryIndex;
    } else {
        leaderboardEntry->m_row = statsRow->m_rank - 1;
        if (leaderboardEntryIndex > 0) {
            // Check this row ID (/rank) against the last one, it might be the
            // same (this happens on PS3 when players have the same score, i.e.
            // if they share 76th position there'll be two rank 76 and the
            // following entry will be rank 78)
            LeaderboardEntry* prevEntry =
                &(m_leaderboard.m_entries[leaderboardEntryIndex - 1]);
            if (leaderboardEntry->m_row <= prevEntry->m_row) {
                leaderboardEntry->m_row = prevEntry->m_row + 1;
            }
        }
    }

    memcpy(leaderboardEntry->m_gamerTag, statsRow->m_name.data(),
           statsRow->m_name.size() * sizeof(char));

    // Copy the other columns
    for (unsigned int i = 0; i < statsRow->m_statsSize; i++) {
        leaderboardEntry->m_columns[i] = statsRow->m_statsData[i];
        memset(leaderboardEntry->m_wcColumns[i], 0, 12 * sizeof(char));
        if (!isDistanceLeaderboard) {
            unsigned int displayValue = leaderboardEntry->m_columns[i];
            if (displayValue > 99999) displayValue = 99999;
            snprintf(leaderboardEntry->m_wcColumns[i], 12, "%u", displayValue);
        } else {
            // check how many digits we have
            int iDigitC = 0;
            unsigned int uiVal = leaderboardEntry->m_columns[i];
            // 			uiVal=0xFFFFFFFF;
            // 			leaderboardEntry->m_columns[i-1]=uiVal;

            while (uiVal != 0) {
                uiVal /= 10;
                iDigitC++;
            }
            if (iDigitC < 4) {
                // m
                snprintf(leaderboardEntry->m_wcColumns[i], 12, "%um",
                         leaderboardEntry->m_columns[i]);
            } else if (iDigitC < 8) {
                // km with a .X
                snprintf(leaderboardEntry->m_wcColumns[i], 12, "%.1fkm",
                         ((float)leaderboardEntry->m_columns[i]) / 1000.f);
            } else {
                // bigger than that, so no decimal point
                snprintf(leaderboardEntry->m_wcColumns[i], 12, "%.0fkm",
                         ((float)leaderboardEntry->m_columns[i]) / 1000.f);
            }
        }
    }
}

void UIScene_LeaderboardsMenu::PopulateLeaderboard(
    IPlatformLeaderboard::eStatsReturn ret) {
    int iValidSlots = SetLeaderboardTitleIcons();
    if (ret == IPlatformLeaderboard::eStatsReturn_Success &&
        m_leaderboard.m_totalEntryCount > 0) {
        m_listEntries.setupTitles(app.GetString(IDS_LEADERBOARD_RANK),
                                  app.GetString(IDS_LEADERBOARD_GAMERTAG));

        // Update entries display
        char entriesBuffer[40];
        if (app.DebugSettingsOn() &&
            (app.GetGameSettingsDebugMask() &
             (1L << eDebugSetting_DebugLeaderboards))) {
            snprintf(entriesBuffer, 40, "%s12345678",
                     app.GetString(IDS_LEADERBOARD_ENTRIES));
        } else {
            snprintf(entriesBuffer, 40, "%s%i",
                     app.GetString(IDS_LEADERBOARD_ENTRIES),
                     m_leaderboard.m_totalEntryCount);
        }

        m_labelEntries.setLabel(entriesBuffer);
        m_labelInfo.setLabel("");
        m_labelInfo.setVisible(false);

        m_listEntries.initLeaderboard(
            m_newSel, m_leaderboard.m_totalEntryCount,
            LEADERBOARD_DESCRIPTORS[m_currentLeaderboard][m_currentDifficulty]
                .m_columnCount);

        int startIndex = m_newEntryIndex;
        int entryCount = m_newEntriesCount;

        for (unsigned int i = startIndex; i < (startIndex + entryCount); ++i) {
            bool isLast = i == ((startIndex + entryCount) - 1);

            int idsErrorMessage = m_leaderboard.m_entries[i].m_idsErrorMessage;

            if (idsErrorMessage > 0) {
                m_listEntries.addDataSet(
                    isLast, m_leaderboard.m_entries[i].m_row,
                    m_leaderboard.m_entries[i].m_rank,
                    m_leaderboard.m_entries[i].m_gamerTag,

                    true,  // 4J-JEV: Has error message to display.

                    app.GetString(idsErrorMessage), "", "", "", "", "", "");
            } else {
                m_listEntries.addDataSet(
                    isLast, m_leaderboard.m_entries[i].m_row,
                    m_leaderboard.m_entries[i].m_rank,
                    m_leaderboard.m_entries[i].m_gamerTag,

                    // 4J-TomK | The bDisplayMessage Flag defines if Leaderboard
                    // Data should be displayed (false) or if a specific message
                    // (true - when data is private for example) should be
                    // displayed. The message itself should be passed on in
                    // col0!
                    false,

                    m_leaderboard.m_entries[i].m_wcColumns[0],
                    m_leaderboard.m_entries[i].m_wcColumns[1],
                    m_leaderboard.m_entries[i].m_wcColumns[2],
                    m_leaderboard.m_entries[i].m_wcColumns[3],
                    m_leaderboard.m_entries[i].m_wcColumns[4],
                    m_leaderboard.m_entries[i].m_wcColumns[5],
                    m_leaderboard.m_entries[i].m_wcColumns[6]);
            }
        }
    } else {
        m_listEntries.setupTitles("", "");

        // Update entries display (to zero)
        char entriesBuffer[40];
        snprintf(entriesBuffer, 40, "%s0",
                 app.GetString(IDS_LEADERBOARD_ENTRIES));
        m_labelEntries.setLabel(entriesBuffer);

        // Show the no results message
#if !defined(_WINDOWS64)
        // so we check this for other platforms
        if (ret == IPlatformLeaderboard::eStatsReturn_NetworkError)
            m_labelInfo.setLabel(app.GetString(IDS_ERROR_NETWORK));
        else
#endif
            m_labelInfo.setLabel(app.GetString(IDS_LEADERBOARD_NORESULTS));
        m_labelInfo.setVisible(true);
    }
    m_bPopulatedOnce = true;
}

void UIScene_LeaderboardsMenu::SetLeaderboardHeader() {
    m_labelLeaderboard.setLabel(app.GetString(
        LEADERBOARD_DESCRIPTORS[m_currentLeaderboard][m_currentDifficulty]
            .m_title));
}

int UIScene_LeaderboardsMenu::SetLeaderboardTitleIcons() {
    int iValidIcons = 0;

    for (int i = 0; i < 7; i++) {
        if (TitleIcons[m_currentLeaderboard][i] == 0) {
            // m_pHTitleIconSlots[i]->SetShow(false);
        } else {
            iValidIcons++;
            m_listEntries.setColumnIcon(i, TitleIcons[m_currentLeaderboard][i]);
        }
    }

    return iValidIcons;
}

void UIScene_LeaderboardsMenu::customDraw(
    IggyCustomDrawCallbackRegion* region) {
    int slotId = parseSlotId(region->name);
    if (slotId == -1) {
        // assert(0);
        app.DebugPrintf("This is not the control we are looking for\n");
    } else {
        std::shared_ptr<ItemInstance> item = std::shared_ptr<ItemInstance>(
            new ItemInstance(TitleIcons[m_currentLeaderboard][slotId], 1, 0));
        customDrawSlotControl(region, m_iPad, item, 1.0f, false, false);
    }
}

void UIScene_LeaderboardsMenu::handleSelectionChanged(F64 selectedId) {
    ui.PlayUISFX(eSFX_Focus);
    m_newSel = (int)selectedId;
    updateTooltips();
}

// Handle a request from Iggy for more data
void UIScene_LeaderboardsMenu::handleRequestMoreData(F64 startIndex, bool up) {
    unsigned int item = (int)startIndex;

    if (m_leaderboard.m_totalEntryCount > 0 &&
        (item + 1) < GetEntryStartIndex()) {
        if (PlatformLeaderboard.isIdle()) {
            int readIndex = (GetEntryStartIndex() + 1) - READ_SIZE;
            if (readIndex <= 0) readIndex = 1;
            assert(readIndex >= 1 &&
                   readIndex <= (int)m_leaderboard.m_totalEntryCount);
            ReadStats(readIndex);
        }
    } else if (m_leaderboard.m_totalEntryCount > 0 &&
               (item + 1) >=
                   (GetEntryStartIndex() + m_leaderboard.m_entries.size())) {
        if (PlatformLeaderboard.isIdle()) {
            int readIndex =
                (GetEntryStartIndex() + 1) + m_leaderboard.m_entries.size();
            assert(readIndex >= 1 &&
                   readIndex <= (int)m_leaderboard.m_totalEntryCount);
            ReadStats(readIndex);
        }
    }
}

void UIScene_LeaderboardsMenu::handleTimerComplete(int id) {}

int UIScene_LeaderboardsMenu::ExitLeaderboards(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_LeaderboardsMenu* pClass = (UIScene_LeaderboardsMenu*)pParam;

    pClass->navigateBack();

    return 0;
}

// Get entry start size, if no entries returns 0
int UIScene_LeaderboardsMenu::GetEntryStartIndex() {
    return m_leaderboard.m_entries.size() == 0
               ? 0
               : m_leaderboard.m_entries[0].m_row;
}
