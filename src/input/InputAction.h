#pragma once

#include <cstdint>

namespace voxl
{
    enum class InputAction : uint32_t
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
