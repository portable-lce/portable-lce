#pragma once

#include "../IPlatformInput.h"

class SDL2Input : public IPlatformInput {
public:
    void Initialise(int iInputStateC, unsigned char ucMapC,
                    unsigned char ucActionC, unsigned char ucMenuActionC);
    void Tick(void);
    void SetDeadzoneAndMovementRange(unsigned int uiDeadzone,
                                     unsigned int uiMovementRangeMax);
    void SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction,
                           unsigned int uiActionVal);
    unsigned int GetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction);
    void SetJoypadMapVal(int iPad, unsigned char ucMap);
    unsigned char GetJoypadMapVal(int iPad);
    void SetJoypadSensitivity(int iPad, float fSensitivity);
    unsigned int GetValue(int iPad, unsigned char ucAction,
                          bool bRepeat = false);
    bool ButtonPressed(int iPad, unsigned char ucAction = 255);  // toggled
    bool ButtonReleased(int iPad, unsigned char ucAction);       // toggled
    bool ButtonDown(int iPad,
                    unsigned char ucAction = 255);  // button held down
    // Functions to remap the axis and triggers for in-game (not menus) -
    // SouthPaw, etc
    void SetJoypadStickAxisMap(int iPad, unsigned int uiFrom,
                               unsigned int uiTo);
    void SetJoypadStickTriggerMap(int iPad, unsigned int uiFrom,
                                  unsigned int uiTo);
    void SetKeyRepeatRate(float fRepeatDelaySecs, float fRepeatRateSecs);
    void SetDebugSequence(const char* chSequenceA,
                          std::function<int()> callback);
    float GetIdleSeconds(int iPad);
    bool IsPadConnected(int iPad);

    // In-Game values which may have been remapped due to Southpaw, swap
    // triggers, etc
    float GetJoypadStick_LX(int iPad, bool bCheckMenuDisplay = true);
    float GetJoypadStick_LY(int iPad, bool bCheckMenuDisplay = true);
    float GetJoypadStick_RX(int iPad, bool bCheckMenuDisplay = true);
    float GetJoypadStick_RY(int iPad, bool bCheckMenuDisplay = true);
    unsigned char GetJoypadLTrigger(int iPad, bool bCheckMenuDisplay = true);
    unsigned char GetJoypadRTrigger(int iPad, bool bCheckMenuDisplay = true);

    void SetMenuDisplayed(int iPad, bool bVal);
    int GetHotbarSlotPressed(int iPad);
    int GetScrollDelta();

    // Legacy keyboard request overloads with integer string-table ids used to
    // live here. The remaining public API keeps the direct text/callback form.
    EKeyboardResult RequestKeyboard(const char* Title, const char* Text,
                                    int iPad, unsigned int uiMaxChars,
                                    std::function<int(bool)> callback,
                                    SDL2Input::EKeyboardMode eMode);
    bool GetMenuDisplayed(int);
    const char* GetText();

    // Online check strings against offensive list - TCR 92
    // 	TCR # 092  CMTV Player Text String Verification
    // 		Requirement Any player-entered text visible to another player on
    // Xbox LIVE must be verified using the Xbox LIVE service before being
    // transmitted. Text that is rejected by the Xbox LIVE service must not be
    // displayed.
    //
    // 		Remarks
    // 		This requirement applies to any player-entered string that can
    // be exposed to other players on Xbox LIVE. It includes session names,
    // content descriptions, text messages, tags, team names, mottos, comments,
    // and so on.
    //
    // 		Games may decide to not send the text, blank it out, or use
    // generic text if the text was rejected by the Xbox LIVE service.
    //
    // 		Games verify the text by calling the XStringVerify function.
    //
    // 		Exemption It is not required to use the Xbox LIVE service to
    // verify real-time text communication. An example of real-time text
    // communication is in-game text chat.
    //
    // 		Intent Protect players from inappropriate language.
    bool VerifyStrings(char** pwStringA, int iStringC,
                       std::function<int(STRING_VERIFY_RESPONSE*)> callback);
    void CancelQueuedVerifyStrings(
        std::function<int(STRING_VERIFY_RESPONSE*)> callback);
    void CancelAllVerifyInProgress(void);

    int GetMouseX();
    int GetMouseY();

    // Primary pad (moved from Profile)
    int GetPrimaryPad();
    void SetPrimaryPad(int iPad);

    // bool InputDetected(int userIndex, char* inputText);
};
