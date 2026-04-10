#include "TutorialMessage.h"

#include "app/common/Game.h"

TutorialMessage::TutorialMessage(
    int messageId, bool limitRepeats /*= false*/,
    unsigned char numRepeats /*= TUTORIAL_MESSAGE_DEFAULT_SHOW*/)
    : messageId(messageId),
      limitRepeats(limitRepeats),
      numRepeats(numRepeats),
      timesShown(0) {}

bool TutorialMessage::canDisplay() {
    return !limitRepeats || (timesShown < numRepeats);
}

const char* TutorialMessage::getMessageForDisplay() {
    if (!canDisplay()) return "";

    if (limitRepeats) ++timesShown;

    return app.GetString(messageId);
}
