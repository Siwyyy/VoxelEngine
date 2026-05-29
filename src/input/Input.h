#pragma once
#include <glm/vec2.hpp>
#include <cstdint>

class Input
{
public:
    virtual ~Input() = default;

    inline static bool isKeyPressed(int32_t keycode_) { return s_instance->isKeyPressedImpl(keycode_); }
    inline static bool isMouseButtonPressed(int32_t button_) { return s_instance->isMouseButtonPressedImpl(button_); }
    inline static glm::vec2 getMousePosition() { return s_instance->getMousePositionImpl(); }

protected:
    virtual bool isKeyPressedImpl(int32_t keycode_) const = 0;
    virtual bool isMouseButtonPressedImpl(int32_t button_) const = 0;
    virtual glm::vec2 getMousePositionImpl() const = 0;

public:
    static Input* s_instance;
};
