#pragma once

#include <cstdint>
#include <string>

#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_PlayerList.h"
#include "app/common/UI/UIScene.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "platform/network/NetTypes.h"

class INetworkPlayer;
class UILayer;

class UIScene_TeleportMenu : public UIScene {
private:
    enum EControls {
        eControl_GamePlayers,
    };

    bool m_teleportToPlayer;
    int m_playersCount;
    std::uint8_t
        m_players[MINECRAFT_NET_MAX_PLAYERS];  // An array of QNet small-id's
    char m_playersVoiceState[MINECRAFT_NET_MAX_PLAYERS];
    short m_playersColourState[MINECRAFT_NET_MAX_PLAYERS];
    std::string m_playerNames[MINECRAFT_NET_MAX_PLAYERS];

    UIControl_PlayerList m_playerList;
    UIControl_Label m_labelTitle;
    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
    UI_MAP_ELEMENT(m_playerList, "GamePlayers")
    UI_MAP_ELEMENT(m_labelTitle, "Title")
    UI_END_MAP_ELEMENTS_AND_NAMES()
public:
    UIScene_TeleportMenu(int iPad, void* initData, UILayer* parentLayer);

    virtual EUIScene getSceneType() { return eUIScene_TeleportMenu; }

    virtual void updateTooltips();
    virtual void handleReload();

    virtual void tick();

protected:
    // TODO: This should be pure virtual in this class
    virtual std::string getMoviePath();

public:
    // INPUT
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed,
                             bool released, bool& handled);

protected:
    virtual void handleGainFocus(bool navBack);
    void handlePress(F64 controlId, F64 childId);
    virtual void handleDestroy();

public:
    static void OnPlayerChanged(void* callbackParam, INetworkPlayer* pPlayer,
                                bool leaving);
};
