#pragma once

#include <cstdint>

// Type-safe handles for the platform sound subsystem. Each handle is a
// tagged 32-bit integer; the tag struct prevents accidental conversion
// between handle types at compile time.
//
// Handle 0 is reserved for "invalid". Backends should never return 0
// from a successful create call.

namespace platform::sound {

template <class Tag>
struct Handle {
    std::uint32_t id = 0;

    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    constexpr explicit operator bool() const noexcept { return valid(); }

    friend constexpr bool operator==(Handle, Handle) = default;
};

struct SoundTag {};
struct MusicTag {};

using SoundHandle = Handle<SoundTag>;
using MusicHandle = Handle<MusicTag>;

}  // namespace platform::sound
