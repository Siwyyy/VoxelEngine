#pragma once
#include "Input.h"
#include <GLFW/glfw3.h>

class GLFWInput final : public Input
{
public:
    GLFWInput(GLFWwindow* window);

private:
    [[nodiscard]] bool isKeyPressedImpl(int32_t keycode) const override;
    [[nodiscard]] bool isMouseButtonPressedImpl(int32_t button) const override;
    [[nodiscard]] glm::vec2 getMousePositionImpl() const override;

    GLFWwindow* m_window;
};
