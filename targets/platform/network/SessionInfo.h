#pragma once

#include "platform/NetTypes.h"

// A struct that we store in the QoS data when we are hosting the session. Max
// size 1020 bytes.
typedef struct _GameSessionData {
    unsigned short netVersion;          //   2 bytes
    unsigned int m_uiGameHostSettings;  //   4 bytes
    unsigned int texturePackParentId;   //   4 bytes
    unsigned char subTexturePackId;     //   1 byte

    bool isReadyToJoin;  //   1 byte

    _GameSessionData() {
        netVersion = 0;
        m_uiGameHostSettings = 0;
        texturePackParentId = 0;
        subTexturePackId = 0;
    }
} GameSessionData;

class FriendSessionInfo {
public:
    SessionID sessionId;
    char* displayLabel;
    unsigned char displayLabelLength;
    unsigned char displayLabelViewableStartIndex;
    GameSessionData data;
    bool hasPartyMember;

    FriendSessionInfo() {
        displayLabel = nullptr;
        displayLabelLength = 0;
        displayLabelViewableStartIndex = 0;
        hasPartyMember = false;
    }

    ~FriendSessionInfo() {
        if (displayLabel != nullptr) delete displayLabel;
    }
};
