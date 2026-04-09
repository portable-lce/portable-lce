#pragma once

#include <cstdint>
#include <memory>
#include <variant>

// Type-safe payload carried by IGameServices::setAction /
// setXuiServerAction. Replaces the old `void* param` which mixed
// integers, booleans, and heap-allocated pointers without ownership
// tracking. Concrete heap-allocated payload types (e.g.
// _DebugSetCameraPosition, _XboxSchematicInitParam) inherit from
// XuiActionOwnedPayload so they can be destroyed polymorphically through
// the unique_ptr alternative inside the variant.

namespace minecraft {

struct XuiActionOwnedPayload {
    virtual ~XuiActionOwnedPayload() = default;
};

}  // namespace minecraft

using XuiActionPayload =
    std::variant<std::monostate, bool, std::int64_t,
                 std::unique_ptr<minecraft::XuiActionOwnedPayload>>;
