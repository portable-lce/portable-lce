#include "UIControl_LeaderboardList.h"

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "util/StringHelpers.h"

UIControl_LeaderboardList::UIControl_LeaderboardList() {}

bool UIControl_LeaderboardList::setupControl(UIScene* scene,
                                             IggyValuePath* parent,
                                             const std::string& controlName) {
    UIControl::setControlType(UIControl::eLeaderboardList);
    bool success = UIControl_Base::setupControl(scene, parent, controlName);

    // UIControl_LeaderboardList specific initialisers
    m_funcInitLeaderboard = registerFastName("InitLeaderboard");
    m_funcAddDataSet = registerFastName("AddDataSet");
    m_funcResetLeaderboard = registerFastName("ResetLeaderboard");
    m_funcSetupTitles = registerFastName("SetupTitles");
    m_funcSetColumnIcon = registerFastName("SetColumnIcon");

    return success;
}

void UIControl_LeaderboardList::init(int id) {
    m_id = id;

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = id;
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_initFunc, 1, value);
}

void UIControl_LeaderboardList::ReInit() {
    UIControl_Base::ReInit();
    init(m_id);
}

void UIControl_LeaderboardList::clearList() {
    IggyDataValue result;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcResetLeaderboard, 0, nullptr);
}

void UIControl_LeaderboardList::setupTitles(const std::string& rank,
                                            const std::string& gamertag) {
    IggyDataValue result;
    IggyDataValue value[2];

    IggyStringUTF8 stringVal0;
    stringVal0.string = const_cast<char*>(rank.c_str());
    stringVal0.length = rank.length();
    value[0].type = IGGY_DATATYPE_string_UTF8;
    value[0].string8 = stringVal0;

    IggyStringUTF8 stringVal1;
    stringVal1.string = const_cast<char*>(gamertag.c_str());
    stringVal1.length = gamertag.length();
    value[1].type = IGGY_DATATYPE_string_UTF8;
    value[1].string8 = stringVal1;

    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcSetupTitles, 2, value);
}

void UIControl_LeaderboardList::initLeaderboard(int iFirstFocus,
                                                int iTotalEntries,
                                                int iNumColumns) {
    IggyDataValue result;
    IggyDataValue value[3];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iFirstFocus;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = iTotalEntries;

    value[2].type = IGGY_DATATYPE_number;
    value[2].number = iNumColumns;
    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcInitLeaderboard, 3, value);
}

void UIControl_LeaderboardList::setColumnIcon(int iColumn, int iType) {
    IggyDataValue result;
    IggyDataValue value[2];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = iColumn;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = (iType <= 32000) ? 0 : (iType - 32000);

    IggyResult out = IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                                            getIggyValuePath(),
                                            m_funcSetColumnIcon, 2, value);
}

void UIControl_LeaderboardList::addDataSet(
    bool bLast, int iId, int iRank, const std::string& gamertag,
    bool bDisplayMessage, const std::string& col0, const std::string& col1,
    const std::string& col2, const std::string& col3, const std::string& col4,
    const std::string& col5, const std::string& col6) {
    IggyDataValue result;
    IggyDataValue value[12];

    value[0].type = IGGY_DATATYPE_boolean;
    value[0].boolval = bLast;

    value[1].type = IGGY_DATATYPE_number;
    value[1].number = iId;

    value[2].type = IGGY_DATATYPE_number;
    value[2].number = iRank;

    IggyStringUTF8 stringVal0;
    stringVal0.string = const_cast<char*>(gamertag.c_str());
    stringVal0.length = gamertag.length();
    value[3].type = IGGY_DATATYPE_string_UTF8;
    value[3].string8 = stringVal0;

    value[4].type = IGGY_DATATYPE_boolean;
    value[4].boolval = bDisplayMessage;

    IggyStringUTF8 stringVal1;
    stringVal1.string = const_cast<char*>(col0.c_str());
    stringVal1.length = col0.length();
    value[5].type = IGGY_DATATYPE_string_UTF8;
    value[5].string8 = stringVal1;

    if (col1.empty()) {
        value[6].type = IGGY_DATATYPE_null;
    } else {
        IggyStringUTF8 stringVal2;
        stringVal2.string = const_cast<char*>(col1.c_str());
        stringVal2.length = col1.length();
        value[6].type = IGGY_DATATYPE_string_UTF8;
        value[6].string8 = stringVal2;
    }

    if (col2.empty()) {
        value[7].type = IGGY_DATATYPE_null;
    } else {
        IggyStringUTF8 stringVal3;
        stringVal3.string = const_cast<char*>(col2.c_str());
        stringVal3.length = col2.length();
        value[7].type = IGGY_DATATYPE_string_UTF8;
        value[7].string8 = stringVal3;
    }

    if (col3.empty()) {
        value[8].type = IGGY_DATATYPE_null;
    } else {
        IggyStringUTF8 stringVal4;
        stringVal4.string = const_cast<char*>(col3.c_str());
        stringVal4.length = col3.length();
        value[8].type = IGGY_DATATYPE_string_UTF8;
        value[8].string8 = stringVal4;
    }

    if (col4.empty()) {
        value[9].type = IGGY_DATATYPE_null;
    } else {
        IggyStringUTF8 stringVal5;
        stringVal5.string = const_cast<char*>(col4.c_str());
        stringVal5.length = col4.length();
        value[9].type = IGGY_DATATYPE_string_UTF8;
        value[9].string8 = stringVal5;
    }

    if (col5.empty()) {
        value[10].type = IGGY_DATATYPE_null;
    } else {
        IggyStringUTF8 stringVal6;
        stringVal6.string = const_cast<char*>(col5.c_str());
        stringVal6.length = col5.length();
        value[10].type = IGGY_DATATYPE_string_UTF8;
        value[10].string8 = stringVal6;
    }

    if (col6.empty()) {
        value[11].type = IGGY_DATATYPE_null;
    } else {
        IggyStringUTF8 stringVal7;
        stringVal7.string = const_cast<char*>(col6.c_str());
        stringVal7.length = col6.length();
        value[11].type = IGGY_DATATYPE_string_UTF8;
        value[11].string8 = stringVal7;
    }
    IggyResult out =
        IggyPlayerCallMethodRS(m_parentScene->getMovie(), &result,
                               getIggyValuePath(), m_funcAddDataSet, 12, value);
}
