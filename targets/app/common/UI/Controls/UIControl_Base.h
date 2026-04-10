#pragma once

#include <string>

#include "app/common/UI/Controls/UIControl.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif

// This class maps to the FJ_Base class in actionscript
class UIControl_Base : public UIControl {
protected:
    IggyName m_initFunc;
    IggyName m_setLabelFunc;
    IggyName m_funcGetLabel;
    IggyName m_funcCheckLabelWidths;

    bool m_bLabelChanged;
    UIString m_label;

public:
    UIControl_Base();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    virtual void tick();

    virtual void setLabel(UIString label, bool instant = false,
                          bool force = false);
    // virtual void setLabel(std::string label, bool instant = false,
    //                       bool force = false) {
    //     this->setLabel(UIString(label), instant, force);
    // }

    const char* getLabel();
    virtual void setAllPossibleLabels(int labelCount, char labels[][256]);
    int getId() { return m_id; }

    virtual bool hasFocus();
};
