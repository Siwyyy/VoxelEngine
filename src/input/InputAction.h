#pragma once

#include <cstdint>

namespace voxl
{
    enum class InputAction : uint8_t
    {
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        Interact,
        SecondaryInteract,
        ToggleCursor,
        Exit
    };
} // namespace voxl
