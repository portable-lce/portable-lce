#pragma once

#include <string>

#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class UIScene;

// This class for any name object in the flash scene
class UIControl {
public:
    enum eUIControlType {
        eNoControl,
        eButton,
        eButtonList,
        eCheckBox,
        eCursor,
        eDLCList,
        eDynamicLabel,
        eEnchantmentBook,
        eEnchantmentButton,
        eHTMLLabel,
        eLabel,
        eLeaderboardList,
        eMinecraftPlayer,
        eMinecraftHorse,
        ePlayerList,
        ePlayerSkinPreview,
        eProgress,
        eSaveList,
        eSlider,
        eSlotList,
        eTextInput,
        eTexturePackList,
        eBitmapIcon,
        eTouchControl,
    };

protected:
    eUIControlType m_eControlType;
    int m_id;
    bool m_bHidden;  // set by the Remove call
    bool m_isValid;

public:
    void setControlType(eUIControlType eType) { m_eControlType = eType; }
    eUIControlType getControlType() { return m_eControlType; }
    void setId(int iID) { m_id = iID; }
    int getId() { return m_id; }
    UIScene* getParentScene() { return m_parentScene; }

protected:
    IggyValuePath m_iggyPath;
    UIScene* m_parentScene;
    std::string m_controlName;

    IggyName m_nameXPos, m_nameYPos, m_nameWidth, m_nameHeight;
    IggyName m_funcSetAlpha, m_nameVisible;

    S32 m_x, m_y, m_width, m_height;
    float m_lastOpacity;
    bool m_isVisible;

public:
    UIControl();

    virtual bool setupControl(UIScene* scene, IggyValuePath* parent,
                              const std::string& controlName);

    IggyValuePath* getIggyValuePath();

    std::string getControlName() { return m_controlName; }

    virtual void tick() {}
    virtual void ReInit();

    virtual void setFocus(bool focus) {}

    S32 getXPos();
    S32 getYPos();
    S32 getWidth();
    S32 getHeight();

    void setOpacity(float percent);
    void setVisible(bool visible);
    bool getVisible();
    bool isVisible() { return m_isVisible; }
    bool isValid() { return m_isValid; }

    virtual bool hasFocus() { return false; }

protected:
    IggyName registerFastName(const std::string& name);
};
