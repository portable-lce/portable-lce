#pragma once

#include <string>

#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#include "platform/storage/storage.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

class UIScene_MessageBox : public UIScene {
private:
    enum EControls {
        eControl_Button0,
        eControl_Button1,
        eControl_Button2,
        eControl_Button3,

        eControl_COUNT
    };

    int (*m_Func)(void*, int, const IPlatformStorage::EMessageResult);
    void* m_lpParam;
    int m_buttonCount;

    UIControl_Button m_buttonButtons[eControl_COUNT];
    UIControl_Label m_labelTitle, m_labelContent;
    IggyName m_funcInit, m_funcAutoResize;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_buttonButtons[eControl_Button0], "Button0")
    UI_MAP_ELEMENT(m_buttonButtons[eControl_Button1], "Button1")
    UI_MAP_ELEMENT(m_buttonButtons[eControl_Button2], "Button2")
    UI_MAP_ELEMENT(m_buttonButtons[eControl_Button3], "Button3")

    UI_MAP_ELEMENT(m_labelTitle, "Title")
    UI_MAP_ELEMENT(m_labelContent, "Content")

    UI_MAP_NAME(m_funcInit, "Init")
    UI_MAP_NAME(m_funcAutoResize, "AutoResize")
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_MessageBox(int iPad, void* initData, UILayer* parentLayer);
    ~UIScene_MessageBox();

    virtual EUIScene getSceneType() { return eUIScene_MessageBox; }

    // Returns true if lower scenes in this scenes layer, or in any layer below
    // this scenes layers should be hidden
    virtual bool hidesLowerScenes() { return false; }
    virtual bool blocksInput() { return true; }

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

    virtual void updateTooltips();

public:
    virtual void handleReload();
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);
    virtual bool hasFocus(int iPad);

protected:
    void handlePress(F64 controlId, F64 childId);
};
