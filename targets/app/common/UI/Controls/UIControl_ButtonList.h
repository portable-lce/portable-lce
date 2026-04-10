#pragma once

#include <string>
#include <vector>

#include "app/common/Iggy/include/iggy.h"
#include "app/common/UI/Controls/UIControl_Base.h"
#include "app/common/UI/Controls/UIControl_ButtonList.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIControl_Base.h"

class UIControl_ButtonList : public UIControl_Base {
protected:
    IggyName m_addNewItemFunc, m_removeAllItemsFunc, m_funcHighlightItem,
        m_funcRemoveItem, m_funcSetButtonLabel, m_funcSetTouchFocus,
        m_funcCanTouchTrigger;

    int m_itemCount;
    int m_iCurrentSelection;

public:
    UIControl_ButtonList();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    void init(int id);
    virtual void ReInit();

    void clearList();

    void addItem(const std::string& label);
    // void addItem(const std::wstring& label);

    void addItem(const std::string& label, int data);
    // void addItem(const std::wstring& label, int data);

    void removeItem(int index);

    int getItemCount() { return m_itemCount; }

    void setCurrentSelection(int iSelection);
    int getCurrentSelection();

    void updateChildFocus(int iChild);

    void setButtonLabel(int iButtonId, const std::string& label);
};

class UIControl_DynamicButtonList : public UIControl_ButtonList {
protected:
    std::vector<UIString> m_labels;

public:
    virtual void tick();

    virtual void addItem(UIString label, int data = -1);

    virtual void removeItem(int index);
};