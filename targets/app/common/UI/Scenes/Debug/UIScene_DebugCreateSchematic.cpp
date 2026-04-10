
#include "app/common/UI/Scenes/Debug/UIScene_DebugCreateSchematic.h"

#include <wchar.h>

#include "app/common/UI/Controls/UIControl_Button.h"
#include "app/common/UI/Controls/UIControl_CheckBox.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TextInput.h"
#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "minecraft/GameEnums.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/ServerAction.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"

class UILayer;
#ifdef _DEBUG_MENUS_ENABLED
#include "util/StringHelpers.h"

UIScene_DebugCreateSchematic::UIScene_DebugCreateSchematic(int iPad,
                                                           void* initData,
                                                           UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    m_labelTitle.init("Name");
    m_labelStartX.init("StartX");
    m_labelStartY.init("StartY");
    m_labelStartZ.init("StartZ");
    m_labelEndX.init("EndX");
    m_labelEndY.init("EndY");
    m_labelEndZ.init("EndZ");

    m_textInputStartX.init("", eControl_StartX);
    m_textInputStartY.init("", eControl_StartY);
    m_textInputStartZ.init("", eControl_StartZ);
    m_textInputEndX.init("", eControl_EndX);
    m_textInputEndY.init("", eControl_EndY);
    m_textInputEndZ.init("", eControl_EndZ);
    m_textInputName.init("", eControl_Name);

    m_checkboxSaveMobs.init("Save Mobs", eControl_SaveMobs, false);
    m_checkboxUseCompression.init("Use Compression", eControl_UseCompression,
                                  false);

    m_buttonCreate.init("Create", eControl_Create);

    m_data = {};
    m_data.compressionType = Compression::eCompressionType_None;
}

std::string UIScene_DebugCreateSchematic::getMoviePath() {
    return "DebugCreateSchematic";
}

void UIScene_DebugCreateSchematic::handleInput(int iPad, int key, bool repeat,
                                               bool pressed, bool released,
                                               bool& handled) {
    ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
        case ACTION_MENU_UP:
        case ACTION_MENU_DOWN:
        case ACTION_MENU_PAGEUP:
        case ACTION_MENU_PAGEDOWN:
        case ACTION_MENU_LEFT:
        case ACTION_MENU_RIGHT:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_DebugCreateSchematic::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_Create: {
            // We want the start to be even
            if (m_data.startX > 0 && m_data.startX % 2 != 0)
                m_data.startX -= 1;
            else if (m_data.startX < 0 && m_data.startX % 2 != 0)
                m_data.startX -= 1;
            if (m_data.startY < 0)
                m_data.startY = 0;
            else if (m_data.startY > 0 && m_data.startY % 2 != 0)
                m_data.startY -= 1;
            if (m_data.startZ > 0 && m_data.startZ % 2 != 0)
                m_data.startZ -= 1;
            else if (m_data.startZ < 0 && m_data.startZ % 2 != 0)
                m_data.startZ -= 1;

            // We want the end to be odd to have a total size that is even
            if (m_data.endX > 0 && m_data.endX % 2 == 0)
                m_data.endX += 1;
            else if (m_data.endX < 0 && m_data.endX % 2 == 0)
                m_data.endX += 1;
            if (m_data.endY > Level::maxBuildHeight)
                m_data.endY = Level::maxBuildHeight;
            else if (m_data.endY > 0 && m_data.endY % 2 == 0)
                m_data.endY += 1;
            else if (m_data.endY < 0 && m_data.endY % 2 == 0)
                m_data.endY += 1;
            if (m_data.endZ > 0 && m_data.endZ % 2 == 0)
                m_data.endZ += 1;
            else if (m_data.endZ < 0 && m_data.endZ % 2 == 0)
                m_data.endZ += 1;

            MinecraftServer::getInstance()->queueServerAction(m_data);

            navigateBack();
        } break;
        case eControl_Name:
        case eControl_StartX:
        case eControl_StartY:
        case eControl_StartZ:
        case eControl_EndX:
        case eControl_EndY:
        case eControl_EndZ:
            m_keyboardCallbackControl = (eControls)((int)controlId);
            PlatformInput.RequestKeyboard(
                "Enter something", "", 0, 25,
                [this](bool bRes) -> int {
                    return handleKeyboardComplete(bRes);
                },
                IPlatformInput::EKeyboardMode_Default);
            break;
    };
}

void UIScene_DebugCreateSchematic::handleCheckboxToggled(F64 controlId,
                                                         bool selected) {
    switch ((int)controlId) {
        case eControl_SaveMobs:
            m_data.saveMobs = selected;
            break;
        case eControl_UseCompression:
            if (selected)
                m_data.compressionType = APPROPRIATE_COMPRESSION_TYPE;
            else
                m_data.compressionType = Compression::eCompressionType_RLE;
            break;
    }
}

int UIScene_DebugCreateSchematic::handleKeyboardComplete(bool bRes) {
    const char* text = PlatformInput.GetText();
    if (text[0] != '\0') {
        std::string value = text;
        int iVal = 0;
        if (!value.empty()) iVal = fromWString<int>(value);
        switch (m_keyboardCallbackControl) {
            case eControl_Name:
                m_textInputName.setLabel(value);
                if (!value.empty()) {
                    snprintf(m_data.name, 64, "%s", value.c_str());
                } else {
                    snprintf(m_data.name, 64, "schematic");
                }
                break;
            case eControl_StartX:
                m_textInputStartX.setLabel(value);

                if (iVal >= (LEVEL_MAX_WIDTH * -16) ||
                    iVal < (LEVEL_MAX_WIDTH * 16)) {
                    m_data.startX = iVal;
                }
                break;
            case eControl_StartY:
                m_textInputStartY.setLabel(value);

                if (iVal >= (LEVEL_MAX_WIDTH * -16) ||
                    iVal < (LEVEL_MAX_WIDTH * 16)) {
                    m_data.startY = iVal;
                }
                break;
            case eControl_StartZ:
                m_textInputStartZ.setLabel(value);

                if (iVal >= (LEVEL_MAX_WIDTH * -16) ||
                    iVal < (LEVEL_MAX_WIDTH * 16)) {
                    m_data.startZ = iVal;
                }
                break;
            case eControl_EndX:
                m_textInputEndX.setLabel(value);

                if (iVal >= (LEVEL_MAX_WIDTH * -16) ||
                    iVal < (LEVEL_MAX_WIDTH * 16)) {
                    m_data.endX = iVal;
                }
                break;
            case eControl_EndY:
                m_textInputEndY.setLabel(value);

                if (iVal >= (LEVEL_MAX_WIDTH * -16) ||
                    iVal < (LEVEL_MAX_WIDTH * 16)) {
                    m_data.endY = iVal;
                }
                break;
            case eControl_EndZ:
                m_textInputEndZ.setLabel(value);

                if (iVal >= (LEVEL_MAX_WIDTH * -16) ||
                    iVal < (LEVEL_MAX_WIDTH * 16)) {
                    m_data.endZ = iVal;
                }
                break;
            default:
                break;
        }
    }

    return 0;
}
#endif
