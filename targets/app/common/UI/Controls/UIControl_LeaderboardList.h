#pragma once

#include <string>

#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_LeaderboardList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_LeaderboardList : public UIControl_Base {
private:
    IggyName m_funcInitLeaderboard, m_funcAddDataSet;
    IggyName m_funcResetLeaderboard;
    IggyName m_funcSetupTitles, m_funcSetColumnIcon;

public:
    enum ELeaderboardIcons {
        e_ICON_TYPE_IGGY = 0,
        e_ICON_TYPE_CLIMBED = 32001,
        e_ICON_TYPE_FALLEN = 32002,
        e_ICON_TYPE_WALKED = 32003,
        e_ICON_TYPE_SWAM = 32004,
        e_ICON_TYPE_ZOMBIE = 32005,
        e_ICON_TYPE_ZOMBIEPIGMAN = 32006,
        e_ICON_TYPE_GHAST = 32007,
        e_ICON_TYPE_CREEPER = 32008,
        e_ICON_TYPE_SKELETON = 32009,
        e_ICON_TYPE_SPIDER = 32010,
        e_ICON_TYPE_SPIDERJOKEY = 32011,
        e_ICON_TYPE_SLIME = 32012,
        e_ICON_TYPE_PORTAL = 32013,
    };
    UIControl_LeaderboardList();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    void init(int id);
    virtual void ReInit();

    void clearList();

    void setupTitles(const std::string& rank, const std::string& gamertag);
    void initLeaderboard(int iFirstFocus, int iTotalEntries, int iNumColumns);
    void setColumnIcon(int iColumn, int iType);
    void addDataSet(bool bLast, int iId, int iRank, const std::string& gamertag,
                    bool bDisplayMessage, const std::string& col0,
                    const std::string& col1, const std::string& col2,
                    const std::string& col3, const std::string& col4,
                    const std::string& col5, const std::string& col6);
};