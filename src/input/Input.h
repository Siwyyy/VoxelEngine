#pragma once
#include <cstdint>

#include <glm/vec2.hpp>

class Input
{
public:
    Input()                        = default;
    Input(const Input&)            = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&)                 = delete;
    Input& operator=(Input&&)      = delete;
    virtual ~Input()               = default;

    inline static bool isKeyPressed(int32_t keycode) { return s_instance->isKeyPressedImpl(keycode); }
    inline static bool isMouseButtonPressed(int32_t button) { return s_instance->isMouseButtonPressedImpl(button); }
    inline static glm::vec2 getMousePosition() { return s_instance->getMousePositionImpl(); }

protected:
    [[nodiscard]] virtual bool isKeyPressedImpl(int32_t keycode) const = 0;
    [[nodiscard]] virtual bool isMouseButtonPressedImpl(int32_t button) const = 0;
    [[nodiscard]] virtual glm::vec2 getMousePositionImpl() const = 0;

public:
    static Input* s_instance;
};
