#pragma once

#include <cstdint>
#include <variant>

#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"

// Typed actions queued onto MinecraftServer from outside the server
// thread (UI, network, save manager). Each variant alternative is a
// plain data struct describing the requested operation; the server
// drains the queue in its tick loop and dispatches via std::visit.
//
// This replaces the old eXuiServerAction enum + per-pad polling +
// XuiActionPayload variant. The previous design forced the server to
// poll IGameServices every tick and pull a UI-shaped payload through a
// polymorphic base; now the server owns its own queue and the action
// types live alongside the consumer.

namespace minecraft::server {

struct SaveGame {
    bool autoSave = false;
};

struct DropDebugItem {
    int playerIndex = 0;
    int itemId = 0;
};

struct SpawnDebugMob {
    int playerIndex = 0;
    int mobFactoryId = 0;
};

struct PauseServer {
    bool paused = false;
};

struct ToggleRain {};
struct ToggleThunder {};

struct BroadcastSettingChanged {
    enum class Kind {
        Gamertags,
        BedrockFog,
        Difficulty,
    };
    Kind kind = Kind::Gamertags;
};

struct ExportSchematic {
    char name[64] = {};
    int startX = 0;
    int startY = 0;
    int startZ = 0;
    int endX = 0;
    int endY = 0;
    int endZ = 0;
    bool saveMobs = false;
    Compression::ECompressionTypes compressionType =
        Compression::eCompressionType_None;
};

struct SetCameraLocation {
    int playerIndex = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double yRot = 0.0;
    double elev = 0.0;
};

using ServerAction =
    std::variant<SaveGame, DropDebugItem, SpawnDebugMob, PauseServer,
                 ToggleRain, ToggleThunder, BroadcastSettingChanged,
                 ExportSchematic, SetCameraLocation>;

}  // namespace minecraft::server
