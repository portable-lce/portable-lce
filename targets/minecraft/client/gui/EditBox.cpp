#include "EditBox.h"

#include "minecraft/SharedConstants.h"
#include "minecraft/client/gui/Screen.h"
#include "platform/stubs.h"

EditBox::EditBox(Screen* screen, Font* font, int x, int y, int width,
                 int height, const std::string& value) {
    // 4J - added initialisers
    maxLength = 0;
    frame = 0;
    enableBackgroundDrawing =
        true;  // 4jcraft: for toggling the background rendering (from 1.6.4,
               // mainly for RepairScreen)

    this->screen = screen;
    this->font = font;
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->setValue(value);
}

void EditBox::setValue(const std::string& value) { this->value = value; }

std::string EditBox::getValue() { return value; }

void EditBox::tick() { frame++; }

void EditBox::keyPressed(char ch, int eventKey) {
    if (!active || !inFocus) {
        return;
    }

    if (ch == 9) {
        screen->tabPressed();
    }
    /* 4J removed
        if (ch == 22)
            {
            String msg = Screen.getClipboard();
            if (msg == null) msg = "";
            int toAdd = 32 - value.length();
            if (toAdd > msg.length()) toAdd = msg.length();
            if (toAdd > 0) {
                value += msg.substring(0, toAdd);
            }
        }
            */

    if (eventKey == Keyboard::KEY_BACK && value.length() > 0) {
        value = value.substr(0, value.length() - 1);
    }
    if (SharedConstants::acceptableLetters.find(ch) != std::string::npos &&
        (value.length() < maxLength || maxLength == 0)) {
        value += ch;
    }
}

void EditBox::mouseClicked(int mouseX, int mouseY, int buttonNum) {
    bool newFocus = active && (mouseX >= x && mouseX < (x + width) &&
                               mouseY >= y && mouseY < (y + height));
    focus(newFocus);
}

void EditBox::focus(bool newFocus) {
    if (newFocus && !inFocus) {
        // reset the underscore counter to give quicker selection feedback
        frame = 0;
    }
    inFocus = newFocus;
}

void EditBox::render() {
    // 4jcraft: render the background conditionally
    if (enableBackgroundDrawing) {
        fill(x - 1, y - 1, x + width + 1, y + height + 1, 0xffa0a0a0);
        fill(x, y, x + width, y + height, 0xff000000);
    }

    // 4jcraft: offset conditionally
    int textX = x;
    int textY = y;
    if (enableBackgroundDrawing) {
        textX += 4;
        textY += (height - 8) / 2;
    }

    if (active) {
        bool renderUnderscore = inFocus && (frame / 6 % 2 == 0);
        drawString(font, value + (renderUnderscore ? "_" : ""), textX, textY,
                   (enableBackgroundDrawing ? 0xe0e0e0 : 0xffffff));
    } else {
        drawString(font, value, textX, textY,
                   (enableBackgroundDrawing ? 0xe0e0e0 : 0xffffff));
    }
}

void EditBox::setMaxLength(int maxLength) { this->maxLength = maxLength; }

int EditBox::getMaxLength() { return maxLength; }

// 4jcraft: for toggling the background rendering (from 1.6.4, mainly for
// RepairScreen)
void EditBox::setEnableBackgroundDrawing(bool enable) {
    enableBackgroundDrawing = enable;
}