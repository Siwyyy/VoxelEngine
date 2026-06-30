#pragma once

#include <glm/vec2.hpp>

#include "InputCodes.h"

namespace voxl
{
    class Input
    {
    public:
        Input()                        = default;
        Input(const Input&)            = delete;
        Input& operator=(const Input&) = delete;
        Input(Input&&)                 = delete;
        Input& operator=(Input&&)      = delete;
        virtual ~Input()               = default;

        static bool isKeyPressed(KeyCode keyCode) { return s_instance->isKeyPressedImpl(keyCode); }
        static bool isMouseButtonPressed(MouseCode mouseCode) { return s_instance->isMouseButtonPressedImpl(mouseCode); }
        static glm::vec2 getMousePosition() { return s_instance->getMousePositionImpl(); }

    protected:
        [[nodiscard]] virtual bool isKeyPressedImpl(KeyCode keycode) const = 0;
        [[nodiscard]] virtual bool isMouseButtonPressedImpl(MouseCode button) const = 0;
        [[nodiscard]] virtual glm::vec2 getMousePositionImpl() const = 0;

    public:
        static Input* s_instance;
    };
} // namespace voxl
