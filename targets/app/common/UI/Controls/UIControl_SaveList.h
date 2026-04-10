#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_SaveList.h"
#include "app/common/UI/UIScene.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_ButtonList.h"

class UIControl_SaveList : public UIControl_ButtonList {
private:
    IggyName m_funcSetTextureName;

public:
    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    using UIControl_ButtonList::addItem;

    void addItem(const std::string& label);
    // void addItem(const std::wstring& label);

    void addItem(const std::string& label, int data);
    // void addItem(const std::wstring& label, int data);

    void addItem(const std::string& label, const std::string& iconName);
    // void addItem(const std::string& label, const std::wstring& iconName);
    void setTextureName(int iId, const std::string& iconName);

private:
    void addItem(const std::string& label, const std::string& iconName,
                 int data);
    // void addItem(const std::string& label, const std::string& iconName,
    //              int data);
};