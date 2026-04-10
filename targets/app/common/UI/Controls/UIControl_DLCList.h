#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_DLCList.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_ButtonList.h"

class UIControl_DLCList : public UIControl_ButtonList {
private:
    IggyName m_funcShowTick;

public:
    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    using UIControl_ButtonList::addItem;
    void addItem(const std::string& label, bool showTick, int iId);
    // void addItem(const std::wstring& label, bool showTick, int iId);
    void showTick(int iId, bool showTick);
};
